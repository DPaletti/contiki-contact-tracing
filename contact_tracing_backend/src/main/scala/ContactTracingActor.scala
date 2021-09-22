import Main.system
import akka.{Done, NotUsed}
import akka.actor.Actor
import akka.stream.alpakka.mqtt.MqttMessage
import akka.stream.alpakka.mqtt.scaladsl.MqttMessageWithAck
import akka.stream.scaladsl.{Sink, Source}
import akka.util.ByteString
import java.nio.charset.StandardCharsets
import scala.collection.mutable.ListBuffer
import scala.concurrent.Future

class ContactTracingActor(mqttSink: Sink[MqttMessage, Future[Done]])
  extends Actor {
  var contacts: Set[String] = Set[String]()
  override def receive: Receive = {
    case msg: MqttMessageWithAck =>
      val topic = msg.message.topic.split("/")(2)
      val contact_list =
        msg.message.payload.decodeString("US-ASCII")
      println(
        s"ByteArray with decoding ${msg.message.payload.decodeString("US-ASCII")}"
      )
      println(s"ByteArray without decoding ${msg.message.payload}")
      println(s"Contact list is: $contact_list")
      if (topic == "contacts") {
        contacts = contacts union parseContacts(
          contact_list
        )
      } else if (topic == "events") {
        var to_send: ListBuffer[MqttMessage] = ListBuffer[MqttMessage]()
        contacts.foreach(contact => {
          to_send = to_send.addOne(
            MqttMessage(s"/$contact/notifications", ByteString(""))
          )
        })
        val contacts_to_send: Source[MqttMessage, NotUsed] = Source(
          to_send.toList
        )
        println(to_send)
        contacts_to_send.runWith(mqttSink)
      } else {
        println("Unrecognized topic: $topic")
      }
  }

  def parseContacts(c: String): Set[String] = {
    var result: Set[String] = Set[String]()
    c.drop(1)
      .dropRight(2)
      .split("-")
      .foreach(contact => {
        result += contact
      })
    result
  }
}
