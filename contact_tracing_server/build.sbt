name := "contact_tracing_server"

version := "0.1"

scalaVersion := "2.13.6"

val AkkaVersion = "2.6.14"

libraryDependencies ++= Seq(
  "com.lightbend.akka" %% "akka-stream-alpakka-mqtt" % "3.0.3",
  "com.typesafe.akka" %% "akka-stream" % AkkaVersion
)
