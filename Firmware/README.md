This folder has the FW for the Wireless Fightpad. There is currently only 1 option for firmware which is the bluetooth firmware. Another firmware is still being worked on which would use another esp32 as a dongle. That version will hopefully support x-input.

You will need to install [ESP32-BLE-CompositeHID](https://github.com/Mystfit/ESP32-BLE-CompositeHID)

IMPORTANT NOTE: ESP32 Arduino Core V3.x.x does not work as they changed how a lot of BLE(and other stuff) works. Please use v2.0.17 and before.
