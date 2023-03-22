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

//defining pages.
#define HOME 0
#define SETTINGS 1
#define CAL 2 //Calibration


int currentpage = 0; // - Home Screen, 1-settings, 2-calibration
float temp;
unsigned long timeLapsed;
float expected_temp;
int low;
int high;
int pos = 0; 

//Pin numbers for the temp sensor. It the uses SPI protocal.
int thermoDO = 11; // data out pin (Master In Slave Out -- MISO) 
int thermoCS = 12; // Chip select pin 
int thermoCLK = 13; // Serial clock pin 

class buttons
{
  private:
    int xcoor;
    int ycoor;
    int height;
    int width;
    String shape; // "Triangle" or "Rectangle"
    int radius=8;
    int TSPressurePt[4]; // [xlow xhigh ylow yhigh]

    int x1, x2;
    int y1, y2;

    int offx = 10; 
    int offy = 10;

    Adafruit_TFTLCD* tft;

  public:
    int b_color = BLACK;
    int f_color = WHITE;
    String text = "Blank"; 
    int textSize = 2;
    int textColor = WHITE;

    // constructor for rectangle.
    buttons(Adafruit_TFTLCD* arg_tft, int xcoor, int ycoor, int height, int width)
    {
      this->tft = arg_tft;
      this->xcoor = xcoor;
      this->ycoor = ycoor;
      this->shape = "RECTANGLE";
      this->height = height;
      this->width = width;
    }
    //Triangle constructor
    buttons(Adafruit_TFTLCD* arg_tft, int xcoor, int ycoor, int x1, int y1, int x2, int y2)
    {
      this->tft = arg_tft;
      this->xcoor = xcoor;
      this->ycoor = ycoor;
      this->shape = "TRIANGLE";
      this->x1 = x1;
      this->y1 = y1;
      this->x2 = x2;
      this->y2 = y2;
    }


    void customize(int16_t _b_color, uint16_t _f_color, String _text, int size, int _textColor = WHITE)
    {
      b_color = _b_color;
      f_color = _f_color;
      text = _text;
      textColor = _textColor;
      textSize = size;
    }
    void Draw()
    {
      if (this->shape == "RECTANGLE")
      {
        tft->fillRoundRect(this->xcoor, this->ycoor, this->height, this->width, 8, this -> f_color);
        tft->drawRoundRect(this->xcoor, this->ycoor, this->height, this->width, 8, this -> b_color);
        this->setText(this->text);
      }
      else if (this->shape == "TRIANGLE")
      {
        tft->fillTriangle(this->xcoor, this->ycoor, this->x1, this->y1, this->x2, this->y2 ,this->f_color);
        tft->drawTriangle(this->xcoor, this->ycoor, this->x1, this->y1, this->x2, this->y2 ,this->b_color);
      }
    }
    void setText(String text)
    {
      tft->setCursor(this->xcoor+this->offx, this->ycoor+this->offy);
      tft->setTextSize(this->textSize);
      tft->setFont();
      tft->setTextColor(textColor);
      tft->print(text);
    }

    void setTextPosition(int offx, int offy)
    {
      this->offx = offx;
      this->offy = offy;
    }
    void Animate()
    {
      tft->fillRoundRect(this->xcoor, this->ycoor, this->height, this->width, this->radius, this->b_color);
      delay(70);
      this->Draw();
    }

    void setPressurePts(int xlow, int xhigh, int ylow, int yhigh)
    {
      this->TSPressurePt[0] = xlow;
      this->TSPressurePt[1] = xhigh;
      this->TSPressurePt[2] = ylow;
      this->TSPressurePt[3] = yhigh;
    }
    bool pressed(TSPoint *p)
    {
        // Serial.println(String(p->x)+","+String(p->y)+","+String(p->z));
        // Serial.println(String(this->TSPressurePt[0])+","+String(this->TSPressurePt[1])+","+String(this->TSPressurePt[2])+","+String(this->TSPressurePt[3]));

      if (p->z > MINPRESSURE && p->z < MAXPRESSURE)
        return  p->x > this->TSPressurePt[0] && p->x < this->TSPressurePt[1] && p->y > this->TSPressurePt[2] && p->y < this->TSPressurePt[3];
      else 
        return false;
    }
};


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); //touch screen obj for detecting pressure points.
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); // tft obj for drawing on the screen.
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // temp sensor obj.
Servo myservo; //servo object for controlling the servo motor.

buttons TopLeftBtn(&tft, 20, 20, 150, 40);
buttons LeftArrowBtn(&tft, 260, 295, 320, 265, 260, 235);
buttons RightArrowBtn(&tft, 220, 295, 160, 265, 220, 235);

buttons homeBtn(&tft, 20, 50, 200, 60);
buttons calibrationBtn(&tft, 20, 125, 200, 60);
buttons LEDBtn(&tft, 20, 200, 200, 60);
buttons unknown1(&tft, 260, 50, 200, 60);
buttons unknown2(&tft, 260, 125, 200, 60);
buttons unknown3(&tft, 260, 200, 200, 60);

// buttons MidLeftBtn(&tft);
// buttons MidRightBtn(&tft);
// buttons TopRightBtn(&tft, 370, 20, "RECTANGLE", 40, 8);

// TopLeftBtn.setSize("Height", "width");
// TopRightBtn.setSize("Height", "width");
// LeftArrowBtn.setSize("Height", "Width")
// RightArrowBtn.setSize("Height", "Width")
// MidleftBtn.setSize("Height", "Width");
// MidRightBtn("Height", "Width")
//testing

void setup(){
  //servo init
  myservo.attach(10);  // attaches the servo on pin 9 to the servo object
  myservo.write(90);

  //TFT setup--------------------
  tft.reset();
  tft.begin(tft.readID());
  Serial.println();
  
  //console log for TFT
  Serial.begin(9600);
  Serial.print("reading id...0x");
  delay(500);
  Serial.println(tft.readID(), HEX);
  
  // more TFT stuff
  tft.fillScreen(BLACK);
  tft.setRotation(1); //set to horizontal display mode 
  
  // set text size and color 
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  //-----------------------

  // Draw home screen
  drawHome();
}

void drawHome()
{
  // Drawing home screen background
  tft.fillScreen(GREY);
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);

  // customizing Top left button and drawing it on to the screen. 
  TopLeftBtn.customize(WHITE, GREY, "Settings", 2, BLACK);
  TopLeftBtn.setTextPosition(25, 15);
  TopLeftBtn.setPressurePts(770, 830, 120, 350);
  TopLeftBtn.Draw();
  
  // customizing Left arrow button and drawing it onto the screen.
  LeftArrowBtn.customize(BLUE, BLUE, "", 0, BLUE);
  LeftArrowBtn.setPressurePts(200, 320, 370, 470);
  LeftArrowBtn.Draw();

  // customizing Right arrow button and drawing it on the screen.
  RightArrowBtn.customize(RED, RED, "", 0, RED);
  RightArrowBtn.setPressurePts(200, 320, 540, 640);
  RightArrowBtn.Draw();

  // TopRightBtn.customize(GREY, GREY, "", 2);
}

void drawSettings()
{
  //Drawing home screen background
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE); // Page border
  
  //creating and drawing home button
  homeBtn.customize(WHITE, CYAN, "Home", 3, BLACK);
  homeBtn.setTextPosition(60, 20);
  homeBtn.setPressurePts(670, 780, 125, 460);
  homeBtn.Draw();

  //Calibration button creation
  calibrationBtn.customize(WHITE, CYAN, "Calibration", 2, BLACK);
  calibrationBtn.setTextPosition(35, 25);
  calibrationBtn.setPressurePts(470, 580, 125, 460);
  calibrationBtn.Draw();

  //LED button creation.
  LEDBtn.customize(WHITE, CYAN, "LED Lights", 2, BLACK);
  LEDBtn.setTextPosition(40, 25);
  //calibrationBtn.setPressurePts(XXXXXXXXX);
  LEDBtn.Draw();
}

void drawCalibration()
{
  //drawing calibration page background.
  tft.fillScreen(GREY);                 
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);
  
  // setting top left button as Back button. 
  TopLeftBtn.customize(WHITE, GREY, "Back", 3, BLACK);
  TopLeftBtn.setTextPosition(40, 10);

  //drawing everything display.
  TopLeftBtn.Draw();
  LeftArrowBtn.Draw();
  RightArrowBtn.Draw();            

}
void loop()
{
  TSPoint p = ts.getPoint();
  // Serial.println(String(p.x)+","+String(p.y)+","+String(p.z));
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  //Serial.println(TopLeftBtn.pressed(&p));
  
  // Home Screen
  if (currentpage == HOME)
  {
      if (TopLeftBtn.pressed(&p))
      {
        TopLeftBtn.Animate();
        currentpage = SETTINGS;
        drawSettings();
      }

      else if (LeftArrowBtn.pressed(&p))
      {
        if (pos != 180)
        {
          pos = pos + 1;
        }
        myservo.write(pos);
        delay(15);
      }

      else if (RightArrowBtn.pressed(&p))
      {
        if (pos != 0)
        {
          pos = pos - 1;
        }
        myservo.write(pos);
        delay(15);
      }
  }
  // Settings Screen
  else if (currentpage == SETTINGS)
  {
    if (homeBtn.pressed(&p))
      {
        homeBtn.Animate();
        currentpage = HOME;
        drawHome();
      }
    else if (calibrationBtn.pressed(&p))
    {
      calibrationBtn.Animate();
      currentpage = CAL;
      drawCalibration();
    }
  }
  // Calibration Screen
  else if (currentpage == CAL)
  {
    if (TopLeftBtn.pressed(&p))
      {
        TopLeftBtn.Animate();
        currentpage = SETTINGS;
        drawSettings();
      }
  }
  
}