# GVG 110 Video Mixer Panel modification/to Arduino + Network conversion

This script, together with a few hardware modifications and extra hardware, lets you use you **old GVG 110 Panel** for use with another hardware video mixer like the **ATEM** or with software mixers like **Vmix** or even **obs-studio**

It requires a websocket server, that manages the panel and sends the appropriate commands to the mixing software/hardware

## How did you manage to do this?
The simple answer: A few weeks of reverse engineering and studying datasheets

## What extra hardware is required?

###### Important:
- Arduino Mega
- Ethernet Shield with the W5100 chip(I would prefer a seperate board, not the shield version)
- Some cabels

###### Optional:
- Dupont connector set
- Panel mount RJ45 and USB-B socket

## What modifications are required?

- Remove the old main processor (Labeled: mc68701s-1)(Todo: Picture)
- Build the adapter cable(Todo)
- Hide the Mega and the ethernet shield in the case(Todo)
- Some deeper modifications including cutting traces on the pcb(Todo)

##Address Mapping Table

Todo


## Arduino Software Setup
- Install the [ArduinoHttpClient](https://github.com/arduino-libraries/ArduinoHttpClient) libary
- In line 11, modify the MAC to match your shield
- In line 30, replace the value with the ip adress of your websocket server
- In line 31, replace the value with the port of your websocket server
- In lines 32-35, setup the ip, dns, gateway and subnetmask of the ethernet shield
- If you want to use DHCP, comment out line 87 and uncomment line 88

## Protocol

###### Events (Panel -> WS Server)

The general layout of a event is: `<id letter>:<value1>:<value2>....`

**Analog changed:**

`a:<index1>:<value1>:<index2>:<value2>...`

Example: T-Bar to 512 and Analog 6 to 1023:

`a:3:512:6:1023`

***

**Button(s) on:**

`b1:<index1>:<index2>...`

Example: Button 12 and 20 pressed:

`b1:12:20`

> In case that there are more then 40 buttons pressed at the same time(which is highly unlikely) it will change to "b2:.."

***

**Button(s) off:**

`c1:<index1>:<index2>...`

Example: Button 12 and 20 depressed:

`c1:12:20`

> In case that there are more then 40 buttons pressed at the same time(which is highly unlikely) it will change to "c2:.."

***

###### Requests (WS Server -> Panel)

The general layout of a request is: `<id letter>:<value1>:<value2>:....:`

Always end the command with a ":"

If you fail to do so, the software will hang and you have to reset the arduino.

**Switch on lamp(s):**

`a:<address1>:<address2>:...`

Example: Switch on lamp 3 and 35:

`a:3:35:`

***

**Switch off lamp(s):**

`b:<address1>:<address2>:...`

Example: Switch off lamp 3 and 35:

`b:3:35:`

***

**Switch off all lamps:**

`c:`

***

**Set Display and the 3 LEDs to the right of it:**

`d:<lcd1>:<lcd2>:<lcd3>:<lcd4>:<led>`

Example: Show "HELP" and switch off LEDs:

`d:12:11:13:14:15:`

Example: Show "9999" and switch on LEDs:

`d:9:9:9:9:0:`

Todo: More examples(leds)

**Charmap**

| Number | Displayed Char |
| --- | --- |
| 0 | 0 |
| 1 | 0 |
| 2 | 0 |
| 3 | 0 |
| 4 | 0 |
| 5 | 0 |
| 6 | 0 |
| 7 | 0 |
| 8 | 0 |
| 9 | 0 |
| 10 | - |
| 11 | E |
| 12 | H |
| 13 | L |
| 14 | P |
| 15 | <Blank> |

***