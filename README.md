# dpSniffino
An Arduino CDP/EDP/LLDP Sniffer

![Alt text](/photo/edp.jpg?raw=true "Trace EDP...")

![Alt text](/photo_pcb/back.jpg?raw=true "Back")

![Alt text](/photo_pcb/right.jpg?raw=true "Right")

##Schematic

![Alt text](/photo_pcb/component.jpg?raw=true "Component")

![Alt text](/photo_pcb/schematic.png?raw=true "Schematic")

##This project reference from:
- Chris van Marle's CDPSniffino http://qistoph.blogspot.com/2012/03/arduino-cdp-viewer.html
- Kristian version: http://www.modlog.net/?p=907
- Patrick Warichet: https://github.com/pwariche/ArduinoCDP

##Library:
1. Ethercard for ENC28J60: https://github.com/jcw/ethercard and I modified its ARP request times in Ethercard::packetLoop for PING.
2. Chris van Marle's DebounceButton: https://github.com/qistoph/ArduinoDebounceButton
