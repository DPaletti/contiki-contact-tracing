import akka.Done
import akka.actor.ActorSystem
import akka.stream.alpakka.mqtt.{
  MqttConnectionSettings,
  MqttQoS,
  MqttSubscriptions
}
import akka.stream.alpakka.mqtt.scaladsl.{MqttMessageWithAck, MqttSource}
import akka.stream.scaladsl.Source
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence

import scala.concurrent.Future

object Main extends App {
  implicit val system: ActorSystem = ActorSystem("ContactTracingServer")
  val connectionSettings = MqttConnectionSettings(
    "tcp://localhost:1883",
    "backend",
    new MemoryPersistence
  )

  val mqttFlow: Source[MqttMessageWithAck, Future[Done]] =
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

  val result = mqttSource
    .via()
    .mapAsync(1)(messageWithAck =>
      messageWithAck.ack().map(_ => messageWithAck.message)
    )
    .take(input.size)
    .runWith(Sink.seq)
}
