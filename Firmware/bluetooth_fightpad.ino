#define BOUNCE_WITH_PROMPT_DETECTION  // Make button state changes available immediately

#include <Bounce2.h>  // https://github.com/thomasfredericks/Bounce2
#include <GamepadDevice.h>
#include <BleCompositeHID.h>
#include <MouseDevice.h>
#include "Wire.h"
#include <FastLED.h>

FASTLED_USING_NAMESPACE

#include <CirquePinnacle.h>

#define DATA_PIN 32
//#define CLK_PIN   4
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS 12
CRGB leds[NUM_LEDS];
CRGB SOCD_MODE[4] = {CRGB(157, 0, 255), CRGB(255, 0, 0), CRGB(255, 255, 0), CRGB(0, 0, 255)};

#define BRIGHTNESS 96
#define FRAMES_PER_SECOND 120

uint8_t gHue = 0;

int batteryLevel = 100;

int left = 13;
int down = 15;
int right = 2;
int up = 12;
int A = 4;
int B = 16;
int R2 = 17;
int L2 = 5;
int X = 18;
int Y = 23;
int R1 = 33;
int L1 = 14;
int start = 26;
int sel = 27;
int home = 25;
int L3 = 0;
int R3 = 35;

int SOCD = 1;

bool light_mode = 0;

unsigned long socd_timer;
unsigned long socd_timer_old;
unsigned long RGB_timer;
unsigned long RGB_timer_old;
unsigned long power_timer;
unsigned long power_timer_old;

bool pressedpad[4] = { 0, 0, 0, 0 };

int power = 34;

BleCompositeHID compositeHID;
GamepadDevice* gamepad;
MouseDevice* mouse;
GamepadConfiguration bleGamepadConfig;

#define numOfButtons 13
#define numOfHatSwitches 1

#define DR_PIN 36

PinnacleTouchI2C trackpad(DR_PIN);

RelativeReport data;

Bounce debouncers[numOfButtons];
Bounce debouncers2[4];

byte dpadPins[4] = { left, down, right, up };
byte buttonPins[numOfButtons] = { A, B, R2, L2, X, Y, R1, L1, start, sel, home, L3, R3 };
byte physicalButtons[numOfButtons] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
bool ispressed[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void setup() {
  delay(1000);
  Wire.setPins(19, 22);
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEHostConfiguration hostConfig;
  hostConfig.setVid(0xe502);
  hostConfig.setPid(0xabcd);

  bleGamepadConfig.setAutoReport(false);
  bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD);  // CONTROLLER_TYPE_JOYSTICK, CONTROLLER_TYPE_GAMEPAD (DEFAULT), CONTROLLER_TYPE_MULTI_AXIS
  bleGamepadConfig.setButtonCount(numOfButtons);
  bleGamepadConfig.setHatSwitchCount(numOfHatSwitches);
  // Some non-Windows operating systems and web based gamepad testers don't like min axis set below 0, so 0 is set by default
  //bleGamepadConfig.setAxesMin(0x8001); // -32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  bleGamepadConfig.setAxesMin(0x0000);  // 0 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal
  bleGamepadConfig.setAxesMax(0x7FFF);  // 32767 --> int16_t - 16 bit signed integer - Can be in decimal or hexadecimal

  gamepad = new GamepadDevice(bleGamepadConfig);  // Simulation controls, special buttons and hats 2/3/4 are disabled by default

  mouse = new MouseDevice();

  compositeHID.addDevice(mouse);
  compositeHID.addDevice(gamepad);
  compositeHID.begin();

  if (!trackpad.begin()) {
    Serial.println("Trackpad Not Working");
  }

  trackpad.setDataMode(PINNACLE_RELATIVE);
  trackpad.relativeModeConfig(true, true);

  for (byte currentPinIndex = 0; currentPinIndex < numOfButtons; currentPinIndex++) {
    pinMode(buttonPins[currentPinIndex], INPUT_PULLUP);

    debouncers[currentPinIndex] = Bounce();
    debouncers[currentPinIndex].attach(buttonPins[currentPinIndex]);  // After setting up the button, setup the Bounce instance :
    debouncers[currentPinIndex].interval(5);
  }
  for (byte currentPinIndex = 0; currentPinIndex < 4; currentPinIndex++) {
    pinMode(dpadPins[currentPinIndex], INPUT_PULLUP);

    debouncers2[currentPinIndex] = Bounce();
    debouncers2[currentPinIndex].attach(dpadPins[currentPinIndex]);  // After setting up the button, setup the Bounce instance :
    debouncers2[currentPinIndex].interval(5);
  }

  pinMode(power, INPUT);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_34, 0);

  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  FastLED.setBrightness(BRIGHTNESS);

  batteryLevel = constrain(fastMap(analogRead(39), 1750, 2250, 0, 100), 0, 100);
  compositeHID.setBatteryLevel(batteryLevel);
}

void loop() {
  if (compositeHID.isConnected()) {
    if (trackpad.available()) {  // is there new data?

      // save buttons' previous state before getting updates
      uint8_t prevButtonStates = data.buttons;  // for edge detection

      trackpad.read(&data);  // get new data

      // edge detection for binary button data
      uint8_t buttonsChanged = prevButtonStates ^ data.buttons;
      if (buttonsChanged) {
        uint8_t toggledOff = buttonsChanged ^ (data.buttons & buttonsChanged);
        uint8_t toggledOn = buttonsChanged ^ toggledOff;
        if (toggledOn) {
          //gamepad->press();
          mouse->mousePress(toggledOn);
        }
        if (toggledOff) {
          //gamepad->release();
          mouse->mouseRelease(toggledOff);
        }
        mouse->mouseMove(0, 0);  //mouse liked to move when clicked, this fixes it idk why
        mouse->sendMouseReport();
      } else if (data.x || data.y || data.scroll) {
        // invert the y-axis, use the others as is
        mouse->mouseMove(data.x * -1.5, data.y * 1.5);
        mouse->sendMouseReport();
      }
    }

    bool sendReport = false;


    for (byte currentIndex = 0; currentIndex < numOfButtons; currentIndex++) {
      debouncers[currentIndex].update();

      if (debouncers[currentIndex].fell()) {
        gamepad->press(physicalButtons[currentIndex]);
        sendReport = true;
        if ((currentIndex + 4) == 8) {
          ispressed[currentIndex + 7] = 1;
        } else if (currentIndex + 4 == 9) {
          ispressed[currentIndex + 5] = 1;
        } else if (currentIndex + 4 == 10) {
          ispressed[currentIndex + 3] = 1;
        } else if (currentIndex + 4 == 11) {
          ispressed[currentIndex + 1] = 1;
        } else {
          ispressed[currentIndex + 4] = 1;
        }
        // for (int i = 0; i < 12; i++) {
        //   Serial.print(ispressed[i]);
        //   Serial.print('\t');
        // }
        // Serial.println();

        // Serial.print(physicalButtons[currentIndex]);
        // Serial.println(" pushed.");
      } else if (debouncers[currentIndex].rose()) {
        gamepad->release(physicalButtons[currentIndex]);
        sendReport = true;
        if ((currentIndex + 4) == 8) {
          ispressed[currentIndex + 7] = 0;
        } else if (currentIndex + 4 == 9) {
          ispressed[currentIndex + 5] = 0;
        } else if (currentIndex + 4 == 10) {
          ispressed[currentIndex + 3] = 0;
        } else if (currentIndex + 4 == 11) {
          ispressed[currentIndex + 1] = 0;
        } else {
          ispressed[currentIndex + 4] = 0;
        }
        // for (int i = 0; i < 12; i++) {
        //   Serial.print(ispressed[i]);
        //   Serial.print('\t');
        // }
        // Serial.println();
        // Serial.print("Button ");
        // Serial.print(physicalButtons[currentIndex]);
        // Serial.println(" released.");
      }
    }
    for (byte currentIndex = 0; currentIndex < 4; currentIndex++) {
      debouncers2[currentIndex].update();

      if (debouncers2[currentIndex].fell()) {
        pressedpad[currentIndex] = 1;
        sendReport = true;
        ispressed[currentIndex] = 1;
      } else if (debouncers2[currentIndex].rose()) {
        pressedpad[currentIndex] = 0;
        sendReport = true;
        ispressed[currentIndex] = 0;
      }
      gamepad->setHat1(filterpad(SOCD));
    }
    if (sendReport) {
      gamepad->sendGamepadReport();
    }
  }
  if (digitalRead(power) == 0) {
    while (digitalRead(power) == 0) {
      power_timer = millis();
      if (power_timer - power_timer_old >= 3000) {
        fill_solid(leds, NUM_LEDS, SOCD_MODE[1]);
        FastLED.show();
        delay(500);
        fill_solid(leds, NUM_LEDS, 0x000000);
        FastLED.show();
        delay(500);
        fill_solid(leds, NUM_LEDS, SOCD_MODE[1]);
        FastLED.show();
        delay(500);
        fill_solid(leds, NUM_LEDS, 0x000000);
        FastLED.show();
        delay(500);
        fill_solid(leds, NUM_LEDS, SOCD_MODE[1]);
        FastLED.show();
        delay(500);
        fill_solid(leds, NUM_LEDS, 0x000000);
        FastLED.show();
        esp_deep_sleep_start();
      } else if (power_timer - power_timer_old >= 50) {
        if (batteryLevel >= 60) {
          fill_solid(leds, NUM_LEDS, CRGB(0, 255, 0));
          FastLED.show();
        } else if (batteryLevel >= 30) {
          fill_solid(leds, NUM_LEDS, CRGB(255, 255, 0));
          FastLED.show();
        } else {
          fill_solid(leds, NUM_LEDS, CRGB(255, 0, 0));
          FastLED.show();
        }
      }
    }
  } else {
    power_timer_old = millis();
  }
  if (digitalRead(start) == 0 && digitalRead(sel) == 0) {
    socd_timer = millis();
    if (socd_timer - socd_timer_old >= 3000) {
      SOCD++;
      if (SOCD == 5) {
        SOCD = 1;
      }
      fill_solid(leds, NUM_LEDS, SOCD_MODE[SOCD]);
      FastLED.show();
      delay(500);
      socd_timer_old = socd_timer;
    }
  } else {
    socd_timer_old = millis();
  }

  if (digitalRead(R3) == 0 && digitalRead(L3) == 0) {
    RGB_timer = millis();
    if (RGB_timer - RGB_timer_old >= 3000) {
      fill_solid(leds, NUM_LEDS, SOCD_MODE[2]);
      FastLED.show();
      delay(500);
      fill_solid(leds, NUM_LEDS, 0x000000);
      FastLED.show();
      delay(500);
      fill_solid(leds, NUM_LEDS, SOCD_MODE[2]);
      FastLED.show();
      delay(500);
      for (int i = 0; i < 12; i++) {
        leds[i] = CRGB(0, 0, 0);
      }
      FastLED.show();
      RGB_timer_old = RGB_timer;
      light_mode = !light_mode;
    }
  } else {
    RGB_timer_old = millis();
  }
  if (light_mode == 0) {
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  }
  if (light_mode == 1) {
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
    for (int i = 0; i < 12; i++) {
      if (ispressed[i] == 0) {
        leds[i] = CRGB(0, 0, 0);
      }
    }
    FastLED.show();
    FastLED.delay(1000 / FRAMES_PER_SECOND);
  }
  EVERY_N_MILLISECONDS(20) {
    gHue++;
  }
  EVERY_N_MILLISECONDS(60000) {
    batteryLevel = constrain(fastMap(analogRead(39), 1750, 2250, 0, 100), 0, 100);
    compositeHID.setBatteryLevel(batteryLevel);
  }
}


bool left1_old;
bool down1_old;
bool right1_old;
bool up1_old;
bool bothLandR = 0;

int filterpad(int SOCD) {
  bool left1 = pressedpad[0];
  bool down1 = pressedpad[1];
  bool right1 = pressedpad[2];
  bool up1 = pressedpad[3];
  switch (SOCD) {
    case 1:  //neutral
      if (left1 == 1 && right1 == 1) {
        left1 = 0;
        right1 = 0;
      }
      if (up1 == 1 && down1 == 1) {
        up1 = 0;
        down1 = 0;
      }
      if (left1 == 1 && down1 == 1) {
        return 6;
      }
      if (left1 == 1 && up1 == 1) {
        return 8;
      }
      if (right1 == 1 && down1 == 1) {
        return 4;
      }
      if (right1 == 1 && up1 == 1) {
        return 2;
      }
      if (left1 == 1) {
        return 7;
      }
      if (down1 == 1) {
        return 5;
      }
      if (right1 == 1) {
        return 3;
      }
      if (up1 == 1) {
        return 1;
      }
      if (left1 == 0 && down1 == 0 && right1 == 0 && up1 == 0) {
        return 0;
      }
      break;

    case 2:  // up priority
      if (left1 == 1 && right1 == 1) {
        left1 = 0;
        right1 = 0;
      }
      if (up1 == 1 && down1 == 1) {
        up1 = 1;
        down1 = 0;
      }
      if (left1 == 1 && down1 == 1) {
        return 6;
      }
      if (left1 == 1 && up1 == 1) {
        return 8;
      }
      if (right1 == 1 && down1 == 1) {
        return 4;
      }
      if (right1 == 1 && up1 == 1) {
        return 2;
      }
      if (left1 == 1) {
        return 7;
      }
      if (down1 == 1) {
        return 5;
      }
      if (right1 == 1) {
        return 3;
      }
      if (up1 == 1) {
        return 1;
      }
      if (left1 == 0 && down1 == 0 && right1 == 0 && up1 == 0) {
        return 0;
      }
      break;

    case 3:  // first input
      if (left1_old == 1 && left1 == 1 && right1 == 1 && right1_old == 0) {
        left1 = 1;
        right1 = 0;
      }
      if (left1_old == 0 && left1 == 1 && right1 == 1 && right1_old == 1) {
        left1 = 0;
        right1 = 1;
      }
      if (up1_old == 1 && up1 == 1 && down1 == 1 && down1_old == 0) {
        up1 = 1;
        down1 = 0;
      }
      if (up1_old == 0 && up1 == 1 && down1 == 1 && down1_old == 1) {
        up1 = 0;
        down1 = 1;
      }
      if (left1 == 1 && down1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 6;
      }
      if (left1 == 1 && up1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 8;
      }
      if (right1 == 1 && down1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 4;
      }
      if (right1 == 1 && up1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 2;
      }
      if (left1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 7;
      }
      if (down1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 5;
      }
      if (right1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 3;
      }
      if (up1 == 1) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 1;
      }
      if (left1 == 0 && down1 == 0 && right1 == 0 && up1 == 0) {
        left1_old = left1;
        down1_old = down1;
        right1_old = right1;
        up1_old = up1;
        return 0;
      }
      break;

    case 4:  // last input
      if (left1 != right1) {
        left1_old = left1;
        right1_old = right1;
      }
      if (left1 == 1 && right1 == 1) {
        if (left1_old == 1) {
          right1 = 1;
          left1 = 0;
        }
        if (right1_old == 1) {
          right1 = 0;
          left1 = 1;
        }
      }
      if (up1 != down1) {
        up1_old = up1;
        down1_old = down1;
      }
      if (up1 == 1 && down1 == 1) {
        if (up1_old == 1) {
          down1 = 1;
          up1 = 0;
        }
        if (down1_old == 1) {
          down1 = 0;
          up1 = 1;
        }
      }
      // Serial.print(left1);
      // Serial.print('\t');
      // Serial.print(left1_old);
      // Serial.print('\t');
      // Serial.print(down1);
      // Serial.print('\t');
      // Serial.print(down1_old);
      // Serial.print('\t');
      // Serial.print(right1);
      // Serial.print('\t');
      // Serial.print(right1_old);
      // Serial.print('\t');
      // Serial.print(up1);
      // Serial.print('\t');
      // Serial.println(up1_old);
      if (left1 == 1 && down1 == 1) {
        return 6;
      }
      if (left1 == 1 && up1 == 1) {
        return 8;
      }
      if (right1 == 1 && down1 == 1) {
        return 4;
      }
      if (right1 == 1 && up1 == 1) {
        return 2;
      }
      if (left1 == 1) {
        return 7;
      }
      if (down1 == 1) {
        return 5;
      }
      if (right1 == 1) {
        return 3;
      }
      if (up1 == 1) {
        return 1;
      }
      if (left1 == 0 && down1 == 0 && right1 == 0 && up1 == 0) {
        return 0;
      }
      break;
  }
}

long fastMap(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
