#include <Arduino.h>
#include <BleMouse.h>
#include <Adafruit_NeoPixel.h>

#define TOGGLE_BUTTON_PIN 14
#define WS2812_PIN 48
#define MINUTE 60000


Adafruit_NeoPixel pixel(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);
BleMouse bleMouse("Logitech M705", "Logitech", 85);

bool is_enabled = true;
bool button_was_pressed = false;
unsigned long button_press_time = 0;
unsigned long last_battery_update = 0;
int battery_level = 85;
int move_interval = 12;
uint16_t min_distance = 30;
uint16_t max_distance = 600;
uint8_t led_brightness = 50;

/**
 * @brief sets the NeoPixel LED color with brightness scaling
 * @param r
 * @param g
 * @param b
 * @param brightness optional brightness override (0-255), uses global if 0
 */
void setLedColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness = 0) {
  if (!brightness) brightness = led_brightness;
  pixel.setPixelColor(0, pixel.Color((r*brightness)/255, (g*brightness)/255, (b*brightness)/255));
  pixel.show();
}

/**
 * @brief updates battery level every 15 minutes
 * simulates battery drain (1-2%) and instant recharge when below 18%
 */
void updateBatteryLevel() {
  if (millis() - last_battery_update > MINUTE * 15) {
    battery_level = (battery_level < 18) ? random(85, 100) : battery_level - random(1, 3);
    bleMouse.setBatteryLevel(battery_level);
    last_battery_update = millis();
  }
}

/**
 * @brief handles button press detection with debouncing
 * toggles enabled state on valid press (50ms-2s duration)
 */
void checkButton() {
  bool pressed = (digitalRead(TOGGLE_BUTTON_PIN) == LOW);

  if (pressed && !button_was_pressed) {
    button_press_time = millis();
    button_was_pressed = true;
  } else if (!pressed && button_was_pressed) {
    unsigned long duration = millis() - button_press_time;
    button_was_pressed = false;

    if (duration >= 50 && duration < 2000) {
      is_enabled = !is_enabled;
      setLedColor(is_enabled ? 255 : 0, is_enabled ? 255 : 0, 0);
    }
  }
}

/**
 * @brief performs random mouse movement with variable patterns
 * mouse moves in random direction for random distance with occasional wobble
 */
void performMovement() {
  int distance = random(min_distance, max_distance);
  int x = random(3) - 1, y = random(3) - 1;

  for (int i = 0; i < distance && is_enabled; i++) {
    checkButton();
    bleMouse.move(x, y, 0);

    int current_delay = (random(100) < 10) ? random(8, 20) : move_interval;
    for (int d = 0; d < current_delay && is_enabled; d += 2) {
      checkButton();
      delay(2);
    }

    if (random(100) < 5) bleMouse.move(random(-1, 2), random(-1, 2), 0);
  }
}

/**
 * @brief randomly performs mouse clicks
 * 10% chance to click, with 33% right-click and 67% left-click probability
 */
void performClick() {
  if (random(10) == 3 && is_enabled) {
    bleMouse.click(random(3) == 0 ? MOUSE_RIGHT : MOUSE_LEFT);
    for (int d = 0; d < 200 && is_enabled; d += 10) {
      checkButton();
      delay(10);
    }
  }
}

/**
 * @brief generates random delay duration between movements
 * @return delay in milliseconds (85% normal, 10% long, 5% short)
 */
int getRandomDelay() {
  int chance = random(100);
  return chance < 85 ? random(5000, 9000) :
         chance < 95 ? random(9000, 14000) :
         random(1000, 5000);
}

void setup() {
  Serial.begin(115200);
  pinMode(TOGGLE_BUTTON_PIN, INPUT_PULLUP);

  pixel.begin();
  pixel.show();

  randomSeed(analogRead(0) + millis());
  battery_level = random(45, 95);

  bleMouse.begin();
  delay(random(2000, 5000));

  setLedColor(is_enabled ? 255 : 0, is_enabled ? 255 : 0, 0);
}

void loop() {
  checkButton();

  if (!is_enabled) {
    setLedColor(0, 0, 0);
    delay(50);
    return;
  }

  if (!bleMouse.isConnected()) {
    delay(10);
    return;
  }

  updateBatteryLevel();
  performMovement();
  performClick();

  unsigned long pause_start = millis();
  int pause_duration = getRandomDelay();

  while (millis() - pause_start < pause_duration && is_enabled) {
    checkButton();
    delay(50);
  }
}
