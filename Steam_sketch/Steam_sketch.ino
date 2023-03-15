#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include "max6675.h"
#include <Servo.h>
#include <TimeLib.h>

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

//touch screen params
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

// touchs parms, NOT USED
//#define TS_MINX 150
//#define TS_MINY 120
//#define TS_MAXX 920
//#define TS_MAXY 940

// display parameters.
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4 // optional

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define PURPLE  0x20F0
#define GREY    0xC618
#define DARKGREY 0x6B0C

//Pressure range for the touch screen. 
# define MINPRESSURE 10
# define MAXPRESSURE 1000

int currentpage = 0; // - Home Screen, 1-settings, 2-calibration
float temp;
unsigned long timeLapsed;
float expected_temp;
int low;
int high;
int pos = 1; 

//Pin numbers for the temp sensor. It the uses SPI protocal.
int thermoDO = 11; // data out pin (Master In Slave Out -- MISO) 
int thermoCS = 12; // Chip select pin 
int thermoCLK = 13; // Serial clock pin 

class buttons
{
    /**
   * @brief Class to assist in creating buttons and detect if they are pressed.
   */
  private:
    int xcoor;
    int ycoor;
    int height;
    int width;
    String shape; // "Triangle" or "Rectangle"
    int radius=8;
    int TSPressurePt[2];
    Adafruit_TFTLCD* tft;

  public:
    int b_color = BLACK;
    int f_color = WHITE;
    String text = "Blank"; 
    int textSize = 2;
    int textColor = WHITE;

    // constructor for rectangle.
    buttons(Adafruit_TFTLCD* arg_tft, int xcoor, int ycoor int height, int width)
    {
      this->tft = arg_tft;
      this->xcoor = xcoor;
      this->ycoor = ycoor;
      this->shape = "RECTANGLE";
      this->height = height;
      this->width = width;
      buttons(Adafruit_TFTLCD* arg_tft, int xcoor, int ycoor int x0, int y0, )
    }

    void customize(int16_t b_color, uint16_t f_color, String _text, int size, int _textColor = WHITE)
    {
      b_color = b_color;
      f_color = f_color;
      text = _text;
      textColor = _textColor;
      textSize = size;
    }
    void Draw()
    {
      if (this->shape == "RECTANGLE")
      {
        tft->fillRoundRect(this->xcoor, this->xcoor, this->height, this->width, 8, this -> f_color);
        tft->drawRoundRect(this->ycoor, this->ycoor, this->height, this->width, 8, this -> b_color);
        this->setText(this->text);
      }
      else if (this->shape == "TRIANGLE")
      {
        tft->fillTriangle(260, 295, 320, 265, 260, 235, BLUE);
        tft->drawTriangle(220, 295, 160, 265, 220, 235, RED);
      }
    }
    void setText(String text)
    {
      tft->setCursor(this->xcoor+10, this->ycoor+10);
      tft->setTextSize(this->textSize);
      tft->setFont();
      tft->setTextColor(textColor);
      tft->print(text);
    }
    void Animate()
    {
      tft->fillRoundRect(this->xcoor, this->ycoor, this->height, this->width, this->radius, this->b_color);
      delay(70);
      this->Draw();
    }

    bool pressed(TSPoint* p)
    {
      if (p->z > MINPRESSURE && p->z < MAXPRESSURE)
      {
        return  p->x > this->TSPressurePt[0] && p->x < this->TSPressurePt[0] && p->y > this->TSPressurePt[1] && p->y < this->TSPressurePt[1];
      }
    }
};


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); //touch screen obj for detecting pressure points.
TSPoint p = ts.getPoint();
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); // tft obj for drawing on the screen.
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // temp sensor obj.
Servo myservo; //servo object for controlling the servo motor.

int dim[2] = {150, 40};
buttons TopLeftBtn(&tft, 20, 20,"RECTANGLE", 150, 40);
buttons TopRightBtn(&tft, 370, 20, "RECTANGLE", 40, 8);
buttons LeftArrowBtn(&tft, 260, 295, "TRIANGLE");
// buttons RightArrowBtn(&tft);
// buttons MidLeftBtn(&tft);
// buttons MidRightBtn(&tft);

// TopLeftBtn.setSize("Height", "width");
// TopRightBtn.setSize("Height", "width");
// LeftArrowBtn.setSize("Height", "Width")
// RightArrowBtn.setSize("Height", "Width")
// MidleftBtn.setSize("Height", "Width");
// MidRightBtn("Height", "Width")
//testing

void setup(){
  Serial.begin(9600);
  TopLeftBtn.customize(WHITE, GREY, "settings", 2);
  TopRightBtn.customize(GREY, GREY, "", 2);
}
void loop(){}