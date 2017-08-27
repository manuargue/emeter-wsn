eMeter WSN
==========

Wireless Sensor Network (WSN) for monitoring the power consumption of home appliances and measuring the quality parameters of electrical energy, for integration with Smart Grid systems.

WSN is implemented over TI MSP430 uC running the main application and TI CC2530 running the TI ZigBee stack for wireless communication between nodes.

The data is colected by a central node (who starts the wireless network) and transmitted over UART to a computer running a logger application. The data is then displayed in a web report that can be accesible remotely.


Brief:

- ZigBee WSN using TI CC2530 SoC (C language)
- Main controller for nodes: TI MSP430 low power uC (C language)
- Logging app built on Java (MySQL data base/BIRT report designer)
- Utils scripts for testing FFT algorithms and ADC sampling (Matlab scripting)

:Author: Manuel Argüelles
:Version: 1.0