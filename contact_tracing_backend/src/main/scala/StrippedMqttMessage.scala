import akka.stream.alpakka.mqtt.scaladsl.MqttMessageWithAck

class StrippedMqttMessage (msg: MqttMessageWithAck) {
  val topic: String = msg.message.topic

  val payload: String = msg.message.payload.decodeString("utf-8")


}
