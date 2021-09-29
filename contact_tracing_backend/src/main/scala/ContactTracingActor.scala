import Main.system
import akka.{Done, NotUsed}
import akka.actor.Actor
import akka.stream.alpakka.mqtt.MqttMessage
import akka.stream.scaladsl.{Sink, Source}
import akka.util.ByteString
import scala.collection.mutable.ListBuffer
import scala.concurrent.Future

class ContactTracingActor(mqttSink: Sink[MqttMessage, Future[Done]])
  extends Actor {
  var contacts: Set[String] = Set[String]()
  override def receive: Receive = {
    case msg: StrippedMqttMessage =>
      val topic = msg.topic.split("/")(2)
      val new_contact =
        msg.payload
      if (topic == "contacts") {
        contacts += new_contact
      } else if (topic == "events") {
        var to_send: ListBuffer[MqttMessage] = ListBuffer[MqttMessage]()
        contacts.foreach(contact => {
          println(s"Sending on /${contact}/notifications message with payload \"Event of interest\"")
          to_send = to_send.addOne(
            MqttMessage(s"/$contact/notifications", ByteString("Event of interest"))
          )
        })
        val contacts_to_send: Source[MqttMessage, NotUsed] = Source(
          to_send.toList
        )
        contacts_to_send.runWith(mqttSink)
      } else {
        println("Unrecognized topic: $topic")
      }
  }

}
