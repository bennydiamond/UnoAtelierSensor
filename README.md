# UNO MQTT DSC Keybus interface #

Arduino sketch that interface HC-SR04 ping sensor and AM2301 temperature.humidity sensor.

This code is to be used with an ATMEGA328p Uno board and a ENC28J60 Ethernet PHY device.
A couple of standard libraries are included in project as they were modified for this project.



### Specifics ###

Due to memory constraints of the ATMEGA328p, UIPEthernet lib have slightly modified compilation properties.
For UIPEthernet:

* UDP is disabled
* ARP table hold max 4 entries
* Reduced max open ports to 3
* Reduced max TCP connections to 3
* Reduced TCP packets buffer size to 4
	

Note: ENC28J60 was used because it was the only part I had on hand.
      RunningMedian library has been templated and simplified (for my needs) to hold uint8_t values rather than float.


### Build setup ###

To be compiled using PlatformIO.

### Hardware ###

ENC28J60 and UIPEthernet library requires usage of the ATMEAG328p SPI bus.
Ping and DHT sensors only require digital IOs.