# Contact tracing with body-worn IoT devices

This project simulates a contact tracing application for body-worn IoT devices.
These devices move and use the radio to detect if any other is close.

This project was created as part of a set of deliveries for Middleware Technologies for Distributed Systems course.

# How to run
## Host #1
1. Start Contiki VM
2. Start Mosquitto on your machine
3. Start ngrok with tcp and port 1883
4. Open on the VM /etc/mosquitto/mosquitto.conf and set ngrok address
5. Restart Mosquitto on the VM
6. Start COOJA, open simulation and start serial socket server on the border router
7. Start tunnel in contiki/examples/rpl-border-router/
8. Start simulation

## Host #2
1. Set Mosquitto address on the backend
2. Start backend

## Documentation
The documentation is hosted in the [GitHub Docs] folder.


## Authors
This project was developed by [Chiara Marzano](mailto:chiara.marzano@mail.polimi.it), [Massimiliano Nigro](mailto:massimiliano.nigro@mail.polimi.it), [Daniele Paletti](mailto:daniele.paletti@mail.polimi.it).

[GitHub Docs]: https://www.github.com/DPaletti/contiki-contact-tracing/report.pdf
