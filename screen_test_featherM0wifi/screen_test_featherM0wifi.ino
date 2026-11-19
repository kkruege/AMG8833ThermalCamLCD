/***************************************************************************
  SCREEN + SENSOR WIRING TEST for Adafruit Feather M0 WiFi (ATWINC1500)
  Generated with AI assistance (Claude by Anthropic).

  What it does:
    1. Blinks the red LED (pin 13) so you know the sketch is running.
    2. Turns off the WiFi chip's SPI connection (pin 8 HIGH) so it can't
       interfere with the screen on the shared SPI bus.
    3. Starts the screen and shows full-screen colors, a border, and text.
    4. Scans the I2C bus to check the AMG8833 thermal sensor (0x69 or 0x68).
    5. Prints what you SHOULD be seeing to the Serial Monitor (115200 baud).

  Expected wiring:
    Screen  Vin -> 3V        Gnd -> GND
            SCK -> SCK       SI  -> MO      SO -> MI (optional)
            TCS -> 10        RST -> 5       D/C -> 6
    AMG8833 VIN -> 3V        GND -> GND
            SDA -> SDA       SCL -> SCL

  Feather M0 WiFi reserved pins (do NOT use): 2, 4, 7, 8
 ***************************************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>

#define TFT_CS   10
#define TFT_RST  5
#define TFT_DC   6

#define WINC_CS  8   // WiFi chip select - must be HIGH so WiFi ignores SPI
#define LED_PIN  13

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // Disable the WiFi chip on the SPI bus FIRST, before anything else uses SPI
  pinMode(WINC_CS, OUTPUT);
  digitalWrite(WINC_CS, HIGH);

  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);
  // The M0 uses native USB: wait up to 5 s for the Serial Monitor to open
  unsigned long start = millis();
  while (!Serial && millis() - start < 5000) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }

  Serial.println();
  Serial.println(F("=== Feather M0 WiFi screen + sensor test ==="));
  Serial.println(F("[OK] Sketch is running (red LED by USB should be blinking)"));
  Serial.println(F("[OK] WiFi chip deselected (pin 8 HIGH)"));

  // ---------- SCREEN TEST ----------
  Serial.println(F("\n--- Screen test ---"));
  Serial.println(F("Starting screen with INITR_144GREENTAB..."));
  tft.initR(INITR_144GREENTAB);
  Serial.println(F("[OK] Screen init sent (this can't confirm the screen heard it)"));

  showColor(ST77XX_RED,   "RED");
  showColor(ST77XX_GREEN, "GREEN");
  showColor(ST77XX_BLUE,  "BLUE");
  showColor(ST77XX_WHITE, "WHITE");

  drawTestPattern();
  Serial.println(F("You should now see: black background, white border on all"));
  Serial.println(F("4 edges, colored squares in each corner, and 'HELLO!'."));

  // ---------- SENSOR TEST ----------
  Serial.println(F("\n--- I2C scan (thermal sensor) ---"));
  Wire.begin();
  bool foundAMG = false;
  int deviceCount = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      deviceCount++;
      Serial.print(F("Found I2C device at 0x"));
      Serial.println(addr, HEX);
      if (addr == 0x69 || addr == 0x68) foundAMG = true;
    }
  }
  if (foundAMG) {
    Serial.println(F("[OK] AMG8833 thermal sensor found"));
  } else if (deviceCount == 0) {
    Serial.println(F("[FAIL] No I2C devices. Check sensor VIN, GND, SDA, SCL."));
  } else {
    Serial.println(F("[FAIL] I2C works but no AMG8833 at 0x69/0x68."));
  }

  // Show sensor result on screen too
  tft.setTextSize(1);
  tft.setCursor(8, 100);
  tft.setTextColor(foundAMG ? ST77XX_GREEN : ST77XX_RED);
  tft.print(foundAMG ? "Sensor: OK" : "Sensor: NOT FOUND");

  Serial.println(F("\n=== Test done. LED keeps blinking slowly. ==="));
}

void loop() {
  // Slow blink = sketch finished and still running
  digitalWrite(LED_PIN, HIGH);
  delay(500);
  digitalWrite(LED_PIN, LOW);
  delay(500);
}

void showColor(uint16_t color, const char *name) {
  Serial.print(F("Screen should be solid "));
  Serial.println(name);
  tft.fillScreen(color);
  delay(1000);
}

void drawTestPattern() {
  int w = tft.width();
  int h = tft.height();

  tft.fillScreen(ST77XX_BLACK);
  tft.drawRect(0, 0, w, h, ST77XX_WHITE);           // border on every edge

  tft.fillRect(2, 2, 12, 12, ST77XX_RED);           // top-left
  tft.fillRect(w - 14, 2, 12, 12, ST77XX_GREEN);    // top-right
  tft.fillRect(2, h - 14, 12, 12, ST77XX_BLUE);     // bottom-left
  tft.fillRect(w - 14, h - 14, 12, 12, ST77XX_YELLOW); // bottom-right

  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(25, 45);
  tft.print("HELLO!");

  Serial.print(F("Screen size reported: "));
  Serial.print(w);
  Serial.print(F(" x "));
  Serial.println(h);
}
