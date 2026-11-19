/***************************************************************************
  AMG8833 THERMAL SENSOR TEST for Adafruit Feather M0 WiFi
  Generated with AI assistance (Claude by Anthropic).

  Prints the sensor's raw 8x8 temperature grid to the Serial Monitor
  (115200 baud) once per second, plus the min / max / average, and shows
  min / max / average on the screen.

  How to use:
    1. Open the Serial Monitor at 115200 baud.
    2. Watch the numbers with nothing in front of the sensor.
    3. Hold your hand 5-10 cm in front of the sensor.
       -> MAX should jump up to roughly 28-34 C.
       -> If the numbers never change, the sensor isn't being read.

  Wiring (same as the camera sketch):
    Screen  Vin->3V  Gnd->GND  SCK->SCK  SI->MO  TCS->10  RST->5  D/C->6
    AMG8833 VIN->3V  GND->GND  SDA->SDA  SCL->SCL
 ***************************************************************************/

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_AMG88xx.h>

#define TFT_CS   10
#define TFT_RST  5
#define TFT_DC   6
#define WINC_CS  8   // WiFi chip select - keep HIGH so WiFi stays off the SPI bus

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_AMG88xx amg;
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];

void setup() {
  pinMode(WINC_CS, OUTPUT);
  digitalWrite(WINC_CS, HIGH);

  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && millis() - start < 5000);   // wait for Serial Monitor

  tft.initR(INITR_144GREENTAB);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);

  Serial.println(F("\n=== AMG8833 sensor test ==="));

  // Try the default address (0x69) first, then the alternate (0x68)
  bool ok = amg.begin(0x69);
  if (ok) {
    Serial.println(F("[OK] Sensor found at 0x69"));
  } else {
    ok = amg.begin(0x68);
    if (ok) {
      Serial.println(F("[OK] Sensor found at 0x68 (alternate address!)"));
      Serial.println(F("     -> In the camera sketch, change amg.begin() to amg.begin(0x68)"));
    }
  }

  if (!ok) {
    Serial.println(F("[FAIL] No sensor at 0x69 or 0x68. Check SDA, SCL, VIN, GND."));
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(4, 4);
    tft.print("SENSOR NOT FOUND");
    while (1);
  }

  tft.setCursor(4, 4);
  tft.print("Sensor OK");
  delay(100);   // let the sensor boot up
}

void loop() {
  amg.readPixels(pixels);

  float minT = 1000, maxT = -1000, sum = 0;
  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    if (pixels[i] < minT) minT = pixels[i];
    if (pixels[i] > maxT) maxT = pixels[i];
    sum += pixels[i];
  }
  float avg = sum / AMG88xx_PIXEL_ARRAY_SIZE;

  // ---- Serial: full 8x8 grid ----
  Serial.println(F("\n8x8 grid (C):"));
  for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
    Serial.print(pixels[i], 1);
    Serial.print((i % 8 == 7) ? "\n" : "\t");
  }
  Serial.print(F("MIN ")); Serial.print(minT, 1);
  Serial.print(F("  MAX ")); Serial.print(maxT, 1);
  Serial.print(F("  AVG ")); Serial.print(avg, 1);
  Serial.print(F("  | board temp ")); Serial.println(amg.readThermistor(), 1);

  if (minT == maxT) {
    Serial.println(F("[WARN] Every pixel is identical - sensor data probably not updating"));
  }

  // ---- Screen: summary ----
  tft.setTextSize(2);
  tft.setCursor(4, 30);  tft.print("MIN "); tft.print(minT, 1); tft.print(" ");
  tft.setCursor(4, 55);  tft.print("MAX "); tft.print(maxT, 1); tft.print(" ");
  tft.setCursor(4, 80);  tft.print("AVG "); tft.print(avg, 1);  tft.print(" ");
  tft.setTextSize(1);
  tft.setCursor(4, 110); tft.print("Wave your hand!");

  delay(1000);
}
