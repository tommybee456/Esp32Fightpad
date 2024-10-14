# Esp32 Fightpad
Esp32-based wireless leverless controller

This project is an ESP32 based wireless controller that incorperates a trackpad. 

The device enumerates as both a mouse and generic gamepad[^1].

Feel free to omit the trackpad or adapt this project into a different controller.

Xnput firmware to be determined.

# Bill of Materials

  - For One Device

## Required

### Switches
-  12 x [Gateron Upgrade Low Profile Switch Hot-swap PCB 2.0 Socket](https://www.gateron.com/products/gateron-low-profile-switch-hot-swap-pcb-socket?VariantsId=10234)
-  12 x [Gateron KS-33 Low Profile 2.0 Switch](https://www.gateron.com/products/gateron-ks-33-low-profile-switch-set)
	- Use whichever you like, the NyPhy LP switches work too)

### Dev Board
-  1 x [LOLIN32 Lite Esp32 Dev Board with Type-C](https://www.aliexpress.us/item/2251832820263031.html)
	- (You can find these anywhere, I just linked what I got)
-  1 x [Battery](https://www.aliexpress.us/item/2251832821059874.html)
	- (User whatever you like, just make sure it has a JST-PH connector)
	- (DOUBLE CHECK THE POLARITY. You may also need to extend the wires a bit)
### Capacitors and Resistors
-  12 x [1206 100nF Caps](https://www.mouser.com/ProductDetail/80-C1206104K5RAC7867)
-  2 x [1206 4.7k Resistors](https://www.mouser.com/ProductDetail/279-CRG1206F4K7-10)
-  2 x [1206 1M Resistors](https://www.mouser.com/ProductDetail/279-CRGP1206F1M0)
-  2 x [1206 1k Resistors](https://www.mouser.com/ProductDetail/71-RCA12061K00FKEA)
### Buttons
-  6 x [6mm tactile buttons](https://www.mouser.com/ProductDetail/CUI-Devices/TS02-66-50-BK-160-SCR-D?qs=A6eO%252BMLsxmQ3H12AN4yOcw%3D%3D)
	- OR [Soft Tactile Buttons - I like these](https://www.digikey.com/en/products/detail/alps-alpine/SKPMAME010/19529201)
 
## Optional
- 12 x [NeoPixel Reverse Mount RGB LED](https://www.mouser.com/ProductDetail/485-4960)
- 1 x [Cirque 40mm Capacitive Trackpad](https://www.mouser.com/ProductDetail/355-TM0400402024-303)

# Build Instructions
(Work in progress)

[^1]: Xinput driver will be present in a later firmware iteration.
