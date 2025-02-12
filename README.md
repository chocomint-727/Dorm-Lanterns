# Dorm Lantern 

These files were used to create small lanterns for my college dorm room. Because I disliked the flourescent lighting installed in the ceiling, I worked to created low power warm lights that were able to communicate with my personal devices. This led me to a solution using an ESP32C3 and warm 12V LEDs.

The architecture of the lights involves one server and various clients. The server hosts a web server from which all lanterns are controlled. Using WebSockets, a user can connect to the server lantern and adjust the brightness of others in real time. The lanterns communicate via the ESP-NOW protocol, leading to near instant response times. Lampshades are laser cut from 3/16" plywood and covered with translucent tissue paper to diffuse the bulb.
