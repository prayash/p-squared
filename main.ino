SYSTEM_MODE(SEMI_AUTOMATIC);

#include "dotstar.h"

#define PIXEL_COUNT 18
#define DATAPIN   D2
#define CLOCKPIN  D4
Adafruit_DotStar strip = Adafruit_DotStar(PIXEL_COUNT, DATAPIN, CLOCKPIN);

#define COLOR_LOOP_TIME 6000
int colorTracker = 170;
int lastColor = 0;

#define OFF 0
#define ON  1
int status = OFF;
int sensorPin = D7;
long pressStart;

bool startupPing = false;
long selfPingTimer;

void setup() {
  pinMode(sensorPin, INPUT);

  strip.begin();
  strip.setBrightness(180);
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 10, 10, 10);
  }
  strip.show();
  delay(50);

  Particle.connect();

  Particle.function("setWifi", setWifi);

  Particle.subscribe("fl3_ping", receivePing);
  Particle.subscribe("fl3_color", receiveColor);
}

void loop() {
  // TODO: if on for over 30 seconds, bail out

  determineState();
  display();
}

void determineState() {
  checkSensor();

  if (status == ON) {
    int timePassed = (millis() - pressStart) % COLOR_LOOP_TIME;
    int colorAdjustment = map(timePassed, 0, COLOR_LOOP_TIME, 0, 255);
    colorTracker = ((lastColor + colorAdjustment) % 254) + 1;
  } else if (status == OFF) {
    lastColor = colorTracker;
  }
}

void checkSensor() {
  int newStatus = digitalRead(sensorPin) ? ON : OFF;

  if (status == newStatus) {
    return;
  }

  if (newStatus == ON) {
    status = ON;
    pressStart = millis();
    Serial.println("Starting " + String(pressStart));
  } else if (newStatus == OFF) {
    status = OFF;

    Serial.println("Ending   " + String(millis()));
    Serial.println("Duration " + String((millis() - pressStart) / 1000.0));
    Particle.publish("fl3_color", String(colorTracker));
  }
}

void display() {
  if (!startupPing) {
    Particle.publish("fl3_ping");
    selfPingTimer = millis();
    startupPing = true;

    return;
  }

  uint32_t color = wheel(colorTracker);

  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void receiveColor(const char *event, const char *data) {
  int newColor = atoi(data);

  if (colorTracker == newColor) {
    return;
  }

  if (newColor != 0) {
    for (int i = 0; i < 100; i++) {
      int mapper = map(i, 0, 100, colorTracker, newColor);

      uint32_t color = wheel(mapper);
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
      }
      strip.show();
      delay(8);
    }

    colorTracker = newColor;
  }
}

void receivePing(const char *event, const char *data) {
  if (selfPingTimer + 5000 < millis()) {
    Particle.publish("fl3_color", String(colorTracker));
  }
}

uint32_t wheel(int position) {
  // Input a value 0 to 255 to get a color value.
  // The colors are a transition g - r - b.
  if(position < 85) {
   return strip.Color(position * 3, 255 - position * 3, 0);
 } else if(position < 170) {
   position -= 85;
   return strip.Color(255 - position * 3, 0, position * 3);
  } else {
   position -= 170;
   return strip.Color(0, position * 3, 255 - position * 3);
  }
}

int setWifi(String command) {
  int seperatorIndex = command.indexOf(":");

  if (seperatorIndex > 0) {
    String ssid     = command.substring(0, seperatorIndex);
    String password = command.substring(seperatorIndex + 1);

    WiFi.setCredentials(ssid, password);
    return 1;
  } else {
    return 0;
  }
}
