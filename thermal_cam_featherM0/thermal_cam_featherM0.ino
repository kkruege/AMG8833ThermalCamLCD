/***************************************************************************
  This is a library for the AMG88xx GridEYE 8x8 IR camera

  This sketch makes a 64 pixel thermal camera with the GridEYE sensor
  and a 128x128 tft screen https://www.adafruit.com/product/2088

  Designed specifically to work with the Adafruit AMG88 breakout
  ----> http://www.adafruit.com/products/3538

  These sensors use I2C to communicate. The device's I2C address is 0x69

  Adafruit invests time and resources providing this open source code,
  please support Adafruit andopen-source hardware by purchasing products
  from Adafruit!

  Written by Dean Miller for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ***************************************************************************/

/***************************************************************************
  MODIFIED for Adafruit Feather M0 (original was written for Arduino Uno)
  All changes are marked with "MODIFIED" comments.
  The modifications were generated with AI assistance (Claude by Anthropic).

  Feather M0 wiring:
    Screen  Vin -> 3V        Gnd -> GND
            SCK -> SCK       SI  -> MO      SO -> MI (optional)
            TCS -> 10        RST -> 5       D/C -> 6
    AMG8833 FeatherWing: stacks on top of the Feather, no wires needed.
    (If using the AMG8833 breakout instead: VIN -> 3V, GND -> GND,
     SDA -> SDA, SCL -> SCL)

  Note: hardware SPI on the Feather uses the pins labeled SCK/MO/MI,
  NOT pins 13/11/12 like on the Uno.

  Feather M0 WiFi: pins 2, 4, 7, 8 are used by the WiFi chip - don't use them.
 ***************************************************************************/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#include <Wire.h>
#include <Adafruit_AMG88xx.h>

#define TFT_CS     10 //chip select pin for the TFT screen
#define TFT_RST    5  // MODIFIED: was 9. On the Feather M0, pin 9 is tied to
                      // the battery-voltage divider (A7), so we use 5 instead.
                      // You can also connect this to the reset pin,
                      // in which case, set this #define pin to 0!
#define TFT_DC     6  // MODIFIED: was 8. Pin 8 isn't on the Feather M0 header
                      // (it's used internally, e.g. the LoRa radio's CS).

// MODIFIED: auto-scaling. When AUTO_SCALE is 1, every frame stretches the
// colors so the coldest pixel is blue and the hottest is red, so the image
// adapts to the room. Set it to 0 to use the fixed MINTEMP/MAXTEMP below.
#define AUTO_SCALE 1

// MODIFIED: smallest temperature spread (in C) auto-scale will stretch to.
// Stops sensor noise from flashing every color when nothing warm is in view.
// Smaller = more contrast but more flicker; bigger = calmer image.
#define MIN_SPAN 3.0

//low range of the sensor (this will be blue on the screen)
// (only used when AUTO_SCALE is 0)
#define MINTEMP 22

//high range of the sensor (this will be red on the screen)
#define MAXTEMP 34

//the colors we will be using
const uint16_t camColors[] = {0x480F,
0x400F,0x400F,0x400F,0x4010,0x3810,0x3810,0x3810,0x3810,0x3010,0x3010,
0x3010,0x2810,0x2810,0x2810,0x2810,0x2010,0x2010,0x2010,0x1810,0x1810,
0x1811,0x1811,0x1011,0x1011,0x1011,0x0811,0x0811,0x0811,0x0011,0x0011,
0x0011,0x0011,0x0011,0x0031,0x0031,0x0051,0x0072,0x0072,0x0092,0x00B2,
0x00B2,0x00D2,0x00F2,0x00F2,0x0112,0x0132,0x0152,0x0152,0x0172,0x0192,
0x0192,0x01B2,0x01D2,0x01F3,0x01F3,0x0213,0x0233,0x0253,0x0253,0x0273,
0x0293,0x02B3,0x02D3,0x02D3,0x02F3,0x0313,0x0333,0x0333,0x0353,0x0373,
0x0394,0x03B4,0x03D4,0x03D4,0x03F4,0x0414,0x0434,0x0454,0x0474,0x0474,
0x0494,0x04B4,0x04D4,0x04F4,0x0514,0x0534,0x0534,0x0554,0x0554,0x0574,
0x0574,0x0573,0x0573,0x0573,0x0572,0x0572,0x0572,0x0571,0x0591,0x0591,
0x0590,0x0590,0x058F,0x058F,0x058F,0x058E,0x05AE,0x05AE,0x05AD,0x05AD,
0x05AD,0x05AC,0x05AC,0x05AB,0x05CB,0x05CB,0x05CA,0x05CA,0x05CA,0x05C9,
0x05C9,0x05C8,0x05E8,0x05E8,0x05E7,0x05E7,0x05E6,0x05E6,0x05E6,0x05E5,
0x05E5,0x0604,0x0604,0x0604,0x0603,0x0603,0x0602,0x0602,0x0601,0x0621,
0x0621,0x0620,0x0620,0x0620,0x0620,0x0E20,0x0E20,0x0E40,0x1640,0x1640,
0x1E40,0x1E40,0x2640,0x2640,0x2E40,0x2E60,0x3660,0x3660,0x3E60,0x3E60,
0x3E60,0x4660,0x4660,0x4E60,0x4E80,0x5680,0x5680,0x5E80,0x5E80,0x6680,
0x6680,0x6E80,0x6EA0,0x76A0,0x76A0,0x7EA0,0x7EA0,0x86A0,0x86A0,0x8EA0,
0x8EC0,0x96C0,0x96C0,0x9EC0,0x9EC0,0xA6C0,0xAEC0,0xAEC0,0xB6E0,0xB6E0,
0xBEE0,0xBEE0,0xC6E0,0xC6E0,0xCEE0,0xCEE0,0xD6E0,0xD700,0xDF00,0xDEE0,
0xDEC0,0xDEA0,0xDE80,0xDE80,0xE660,0xE640,0xE620,0xE600,0xE5E0,0xE5C0,
0xE5A0,0xE580,0xE560,0xE540,0xE520,0xE500,0xE4E0,0xE4C0,0xE4A0,0xE480,
0xE460,0xEC40,0xEC20,0xEC00,0xEBE0,0xEBC0,0xEBA0,0xEB80,0xEB60,0xEB40,
0xEB20,0xEB00,0xEAE0,0xEAC0,0xEAA0,0xEA80,0xEA60,0xEA40,0xF220,0xF200,
0xF1E0,0xF1C0,0xF1A0,0xF180,0xF160,0xF140,0xF100,0xF0E0,0xF0C0,0xF0A0,
0xF080,0xF060,0xF040,0xF020,0xF800,};

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

Adafruit_AMG88xx amg;
unsigned long delayTime;
float pixels[AMG88xx_PIXEL_ARRAY_SIZE];
uint16_t displayPixelWidth, displayPixelHeight;

void setup() {
  // MODIFIED: Feather M0 WiFi only. The WiFi chip (ATWINC1500) shares the
  // SPI bus with the screen. Its chip select is pin 8; holding it HIGH
  // keeps the WiFi chip from talking over the screen's data.
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);

  Serial.begin(9600);
    Serial.println(F("AMG88xx thermal camera!"));

    tft.initR(INITR_144GREENTAB);   // initialize a ST7735S chip, black tab
    tft.fillScreen(ST7735_BLACK);

    displayPixelWidth = tft.width() / 8;
    displayPixelHeight = tft.height() / 8;

    //tft.setRotation(3);
    
    bool status;
    
    // default settings
    status = amg.begin();
    if (!status) {
        Serial.println("Could not find a valid AMG88xx sensor, check wiring!");
        while (1);
    }
    
    Serial.println("-- Thermal Camera Test --");
    delay(100); // let sensor boot up

}

void loop() {
  //read all the pixels
  amg.readPixels(pixels);

  // MODIFIED: pick the color range for this frame
  float lowT = MINTEMP;
  float highT = MAXTEMP;
  if (AUTO_SCALE) {
    lowT = 1000;
    highT = -1000;
    for (int i = 0; i < AMG88xx_PIXEL_ARRAY_SIZE; i++) {
      if (pixels[i] < lowT)  lowT = pixels[i];
      if (pixels[i] > highT) highT = pixels[i];
    }
    // widen the range around its middle if it's narrower than MIN_SPAN
    if (highT - lowT < MIN_SPAN) {
      float mid = (highT + lowT) / 2.0;
      lowT  = mid - MIN_SPAN / 2.0;
      highT = mid + MIN_SPAN / 2.0;
    }
  }

  for(int i=0; i<AMG88xx_PIXEL_ARRAY_SIZE; i++){
    // MODIFIED: was uint8_t + map(). uint8_t wrapped out-of-range values
    // before constrain() could clamp them, and map() only does whole numbers,
    // which wastes most of the colors over a small range. This uses decimals.
    int colorIndex = (int)((pixels[i] - lowT) * 255.0 / (highT - lowT));
    colorIndex = constrain(colorIndex, 0, 255);

    //draw the pixels!
    tft.fillRect(displayPixelHeight * floor(i / 8), displayPixelWidth * (i % 8),
        displayPixelHeight, displayPixelWidth, camColors[colorIndex]);
  }
}