# Bitpixel Poi
## What it is
This is a Tutorial for Building LED-Pixel Poi

## What it does
Each poi consists of a LED Stri and a microcontroller. When you switch on a poi, it searches for a wifi hotspot it knows. If it doesn't find one, it becomes a hotspot itself. In this way, the first poi you switch on becomes the master and all others connect to it as Slaves.  
The master also starts a web server that provides the user interface.

You can now connect to this hotspot with a smartphone, tablet or computer and open the user interface (UI) in the web browser.

The UI is connected to the web server via websocket and so are all the slaves.

TODO: Was kann die UI, wie kommen Bilder drauf


# Hardware

for one Poi:
- 1 Microcontroller: ESP8266 Wemos D1 mini
- 1 Battery: 18650 battery with integrated protection circuit
- 1 Batteryholder
- 1 Led Strip: SK9822 LED Strip - 144 LED/m - One Meter - IP67
- yes, exactly, that is indeed already the whole technology

## The Microcontroller

The ESP8622 is cheap, fast and has built in Wifi. I used a Wemos D1 board at first, as I had a few of them anyway. The only limitation was the somewhat small memory of 4MB, so I tried the Wemos D1 mini Pro with 16MB. Both work equally well.

To be able to flash the Wemos D1 mini Pro I had to install CP210x Windows Drivers:
https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads

## The LED Strip

I use a SK9822 led strip with separate clock and data line (4 wires in total).
In comparison to the cheaper ws2812b and others with 3 wires the refresh rate ist much higher and this is what we need in this case.
Adafruit Dotstar, APA102 are other with separate clock line.

144 Led/m was the most dense I found.

I recommend using the IP65 version (waterproof) beyause they have a soft silicone coating which makes them very impact resistant.



More Infos about Led Strips:
- https://www.derunledlights.com/de/the-difference-between-addressable-rgb-led-strip-ws2811-ws2812b-ws2813-ws2815-sk6812-sk9822/
- https://hackaday.io/project/162435-board-for-modular-led-wearables/log/158183-apa102-sk9822-and-dotstar-led-strips
- http://www.technoblogy.com/show?3SO4
- https://quinled.info/2019/01/21/ip20-vs-ip65-vs-ip67-waterproof/

## Wiring

- Battery to 5V an GND
- LedStrip to 5V and GND
- CLOCK_PIN D5
- DATA_PIN D7


## Assemble playable Poi

The core consists of a wooden stick (beech, 8mm x 54cm) to which the LED strips are glued. At the upper end, a battery holder with a perforated metal band is attached as an extension of the rod. The hand straps are attached to this with key rings. The controller is glued to the battery holder.
For better reflection, the wooden stick is wrapped with silver adhesive tape and the LED strips are glued to it. Everything is held together by transparent heat-shrink tubing.

Material list:
(in addition to the hardware from above and a soldering iron to connect the harware)
- Wooden stick
- metal perforated tape
- transparent heat shrink tubing
- Hot glue
- Screws Key rings
- Hand loops
- neoprene velcro cable conduit

### On and Off and unwanted reset

Batteryholders with springs, plastic wedges to disconnect thee battery or to fixate ist.
On impact the controlle sometimes is reset. If it was the Master all connections are lost. To avoid this I sometimes use a third Cip as a non-poi master.

Batteries with soldered plugs may be more robust, just unplug to turn off.

# Programming

Aruino IDE, PlatformIO


In the Sketch you can switch from 4 wire ones to 3 wire ones by changing the comments:
```
// #define USE_DOTSTAR 1
#define USE_NEOPIXEL 1
```
https://github.com/FastLED/FastLED/wiki/Chipset-reference

## The Sketch

src/main.cpp ist the entrypoint, also to read the code.

The included file ```mypixels.h``` contains the images as numbers and the HTML UI as String.  
This file is the output of the image preprocessing.

Needed Libraries:


## Image Preprocessing

nodejs


in-Folder
