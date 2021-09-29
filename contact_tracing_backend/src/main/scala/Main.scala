import Main.system.dispatcher
import akka.{Done, NotUsed}
import akka.actor.{ActorSystem, Props, RootActorPath}
import akka.stream.RestartSettings
import akka.stream.alpakka.mqtt.{MqttConnectionSettings, MqttMessage, MqttQoS, MqttSubscriptions}
import akka.stream.alpakka.mqtt.scaladsl.{MqttMessageWithAck, MqttSink, MqttSource}
import akka.stream.scaladsl.{Flow, RestartSource, Sink, Source}
import akka.util.{ByteString, Timeout}
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence

import java.util.concurrent.TimeUnit
import scala.concurrent.duration.{DurationInt, FiniteDuration}
import scala.concurrent.{Future, Promise}
import scala.util.{Failure, Success}

object Main extends App {
  private def wrapWithAsRestartSource[M](
                                          source: => Source[M, Future[Done]]
                                        ): Source[M, Future[Done]] = {
    val fut = Promise[Done]
    RestartSource
      .withBackoff(
        RestartSettings(
          100.millis,
          3.seconds,
          randomFactor = 0.2d
        )
      ) { () =>
        source.mapMaterializedValue(mat => fut.completeWith(mat))
      }
      .mapMaterializedValue(_ => fut.future)
  }

    implicit val system: ActorSystem = ActorSystem("ContactTracingServer")
    implicit val timeout: Timeout = Timeout(20, TimeUnit.SECONDS)
    val connectionSettings = MqttConnectionSettings(
      "tcp://2.tcp.ngrok.io:10154",
      "backend",
      new MemoryPersistence
    )
      .withKeepAliveInterval(new FiniteDuration(20, TimeUnit.SECONDS))
      .withAutomaticReconnect(true)



    val mqttSource: Source[MqttMessageWithAck, Future[Done]] = {
      MqttSource.atLeastOnce(
        connectionSettings
          .withClientId(clientId = "backend-source")
          .withCleanSession(false)
          .withAuth(username = "backend", password = "AUTHZ"),
        MqttSubscriptions(
          Map(
            "/+/events" -> MqttQoS.atLeastOnce,
            "/+/contacts" -> MqttQoS.atLeastOnce
          )
        ),
        bufferSize = 8 // # messages before back-pressure is applied
      )
    }

    val mqttSink: Sink[MqttMessage, Future[Done]] =
      MqttSink(connectionSettings, MqttQoS.AtLeastOnce)

    wrapWithAsRestartSource(mqttSource).runForeach((msg: MqttMessageWithAck) => {
      msg.ack()
      val stripped_message: StrippedMqttMessage = new StrippedMqttMessage(msg)
      val name = stripped_message.topic.split("/")(1)
      system
        .actorSelection("user/" + name)
        .resolveOne()
        .onComplete {
          case Success(actor) =>
            actor ! stripped_message

          case Failure(ex) =>
            val actor =
              system.actorOf(Props(classOf[ContactTracingActor], mqttSink), name)
            actor ! stripped_message
        }
    })

}
