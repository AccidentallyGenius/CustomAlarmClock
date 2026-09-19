#include <Adafruit_GFX_h>
#include <Adafruit_ST7789_h>
#include <SPI.h>

#define TFT_SCLK 9
#define TFT_MOSI 10
#define TFT_DC 5
#define TFT_CS 6

class MyST7789: public Adafruit_ST7789 {
  public:
    MyST7789(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk)
      : Adafruit_ST7789(cs, dc, mosi, sclk) {}
    
    void setOffsets(uint8_t col, uint8_t row) {
      _colstart = _colstart2 = col;
      _rowstart = _rowstart2 = row;
    }
};

MyST7789 screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK)

void setup() {
  Serial.begin(115200)

  screen.init(76, 284);
  screen.setOffsets(82, 18);
  screen.invertDisplay(false);
  screen.setRotation(1);

  Serial.println("TFT Initialized!");

  screen.fillScreen(ST77XX_BALCK);
  screen.setTextSize(6);
  screen.SetCursor(0, 0);
  screen.print(10)
}

void loop() {
  

}
