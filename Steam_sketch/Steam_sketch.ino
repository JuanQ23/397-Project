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

  public:
    Adafruit_TFTLCD* tft;
    buttons(Adafruit_TFTLCD* arg_tft)
    {
      tft = arg_tft;
    }

    /**
    * @brief draws a rectangular button on the screen. The button grows down-right.
    * 
    * @param x0 The x coordinate.
    * @param y0 the y coordinate.
    * @param w the width.
    * @param h the height. 
    * @param radius rounding the edges.
    * @param b_color the color of the border.
    * @param f_color the color of the fill.
    */
    void rect_button(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t b_color, uint16_t f_color)
    {

      tft->fillRoundRect(x0, y0, w, h, radius, f_color);
      tft->drawRoundRect(x0, y0, w, h, radius, b_color);
    }
    void tri_button(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
    {
      tft->fillTriangle(x0, y0, x1, y1, x2, y2, color);
      tft->drawTriangle(x0, y0, x1, y1, x2, y2, color);
    }
    void setText(int16_t x, int16_t y, uint8_t s, uint16_t c, const char p[])
    {
      tft->setCursor(x, y);
      tft->setTextSize(s);
      tft->setFont();
      tft->setTextColor(c);
      tft->print(p);
    }
    void setText(int16_t x, int16_t y, uint8_t s, uint16_t c, long n)
    {
      tft->setCursor(x, y);
      tft->setTextSize(s);
      tft->setFont();
      tft->setTextColor(c);
      tft->print(n);
    }
    void setText(int16_t x, int16_t y, uint8_t s, uint16_t c, char ch)
    { 
      tft->setCursor(x, y);
      tft->setTextSize(s);
      tft->setFont();
      tft->setTextColor(c);
      tft->print(ch);
    }
    void setText(int16_t x, int16_t y, uint8_t s, uint16_t c, int i)
    {
      tft->setCursor(x, y);
      tft->setTextSize(s);
      tft->setFont();
      tft->setTextColor(c);
      tft->print(i);
    }
    void buttonAnimation(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t b_color, uint16_t f_color)
    {
      tft->fillRoundRect(x0, y0, w, h, radius, b_color);
      delay(70);
      rect_button(x0, y0, w, h, radius, b_color, f_color);
    }

    bool pressed(char button[2], TSPoint* p)
    {
      if (p->z > MINPRESSURE && p->z < MAXPRESSURE)
      {

        if (button == "B1") // B1 = Button 1
        {
          return  p->x > 670 && p->x < 780 && p->y > 125 && p->y < 460;
        }
        else if (button == "B2") // B2 = Button 1
        {
          return p->x > 470 && p->x < 580 && p->y > 125 && p->y < 460;
        }
        else if (button == "B3") // B3 = Button 3
        {
          return p->x > 270 && p->x < 380 && p->y > 125 && p->y < 460;
        }
        else if (button == "TL") // Top left corner of screen (settings, go back etc)
        {
          return p->x > 770 && p->x < 830 && p->y > 120 && p->y < 350;
        }
        else if (button == "ML") // Mid left portion of screen (set minimum currently.)
        {
          return p->x > 470 && p->x < 580 && p->y > 125 && p->y < 460;
        }
        else if (button == "MR")  // Mid Right portion of screen (set max currently.)
        { 
          return p->x > 470 && p->x < 580 && p->y > 560 && p->y < 895;
        }
        else if (button == "LA") // left arrow
        {
          return p->y > 370 && p->y < 470 && p->x > 200 && p->x < 320;
        }
        else if (button == "RA") // left arrow
        {
          return p->y > 540 && p->y < 640 && p->x > 200 && p->x < 320;
        }
        else if (button == "TR")
        {
          return p->x > 770 && p->x < 830 && p->y > 760 && p->y < 890;
        }
      }
    }
};


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300); //touch screen obj for detecting pressure points.
TSPoint p = ts.getPoint();
Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); // tft obj for drawing on the screen.
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // temp sensor obj.
Servo myservo; //servo object for controlling the servo motor.
buttons b(&tft);

int temp_history[10] = {0};

void setup()
{

  myservo.attach(10);  // attaches the servo on pin 9 to the servo object
  myservo.write(0);
  
  //initialize TFT 
  tft.reset();
  tft.begin(tft.readID());

  //Initialize serial communication & print TFT Id
  Serial.begin(9600);
  Serial.println();
  Serial.print("reading id...0x");
  delay(500);
  Serial.println(tft.readID(), HEX);

  tft.fillScreen(BLACK);
  tft.setRotation(1); //set to horizontal display mode 
  // set text size and color 
  tft.setTextSize(3);
  tft.setTextColor(WHITE);

  setTime(12,0,0,1,1,20);

  // draw menu buttons
  drawHome();
}

void printDigits(int digits)
{
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);  
}

void digitalClockDisplay()
{
  b.setText(130, 140, 8, BLACK, hour());
  b.setText(210, 140, 8, BLACK, ":");
  if(minute() < 10)
  {
    b.setText(240, 140, 8, BLACK, "0");
    b.setText(290, 140, 8, BLACK, minute());
  }
  else 
  {
    b.setText(240, 140, 8, BLACK, minute());    
  }
}

void drawSettings()
{
  tft.fillScreen(BLACK);                 
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE); //Page border

  b.rect_button(20, 50, 200, 60, 8, WHITE, CYAN);
  b.rect_button(20, 125, 200, 60, 8, WHITE, CYAN);
  b.rect_button(20, 200, 200, 60, 8, WHITE, CYAN);
  b.rect_button(260, 50, 200, 60, 8, WHITE, CYAN);
  b.rect_button(260, 125, 200, 60, 8, WHITE, CYAN);
  b.rect_button(260, 200, 200, 60, 8, WHITE, CYAN);

  b.setText(175, 15, 3, WHITE, "Settings");

  b.setText(80, 70, 3, BLACK, "Home");
  b.setText(55, 150, 2, BLACK, "Calibration");
  b.setText(60, 225, 2, BLACK, "LED Lights");
  b.setText(310, 75, 2, BLACK, "Set Time");
}

void drawCalibration()
{
  tft.fillScreen(GREY);                 
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);

  b.rect_button(20, 20, 150, 40, 8, WHITE, BLACK);
  b.setText(60, 30, 3, WHITE, "Back");

  b.tri_button(260, 295, 320, 265, 260, 235, BLUE);
  b.tri_button(220, 295, 160, 265, 220, 235, RED);

  b.rect_button(20, 125, 200, 60, 8, WHITE, BLACK);
  b.rect_button(260, 125, 200, 60, 8, WHITE, BLACK);

  b.setText(35, 150, 2, WHITE, "Set to Min Pos.");
  b.setText(275, 150, 2, WHITE, "Set to Max Pos.");
}

void drawHome()
{
  tft.fillScreen(GREY);                 
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);

  b.rect_button(20, 20, 150, 40, 8, WHITE, GREY);

  b.setText(45, 35, 2, BLACK, "Settings");

  b.tri_button(260, 295, 320, 265, 260, 235, BLUE);  

  b.tri_button(220, 295, 160, 265, 220, 235, RED);

  digitalClockDisplay();

  temp = thermocouple.readFahrenheit();
  expected_temp = temp;

  b.setText(380, 30, 3, BLACK, round(expected_temp));
  tft.print(char(247));
  tft.print("F");
}

void update_pos(float temp, float expected_temp)
{
  if(expected_temp > temp)
  {
    pos = pos + 2;
    myservo.write(pos);
  }
  if(expected_temp < temp)
  {
    pos = pos - 2;
    myservo.write(pos);
  }
}

void store_change(int *arr, int n, int value)
{ 
    memmove(&arr[1], &arr[0], (n-1)*sizeof(int));
    arr[0] = value;
}
bool no_change(int* arr)
{
  //int size = sizeof(arr)/sizeof(int);

  for(int i=0; i<4; i++)
  {
    if (arr[i] != arr[i+1]) return false;
  }
  return true;
}
void loop()
{
  p = ts.getPoint(); //Read touchscreen 

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // Home
  if (currentpage == 0)
  {
    if (b.pressed("TL", &p)) // top left pressed,  then go to settings.
    {
      b.buttonAnimation(20, 20, 150, 40, 8, WHITE, GREY);
      b.setText(45, 35, 2, BLACK, "Settings");
      currentpage = 1;
      drawSettings();
      p.z = 0; // reset pressure so that next page does not get triggered.
    }
    // Raise target temp
    else if (b.pressed("LA", &p)) // elif left arrow pressed, Then increase temp target.
    {
      if(expected_temp <= 100) expected_temp += 1;

      // drawing the temperature. TODO: functionlize to update_display(__,__);
      tft.fillRoundRect(380, 20, 90, 40, 8, GREY);
      b.rect_button(370, 20, 100, 40, 8, BLACK, GREY);
      b.setText(380, 30, 3, BLACK, round(expected_temp));
      tft.print(char(247));
      tft.print("F");
      delay(200);
    }

    // lower target temp
    if (b.pressed("RA", &p));
    {
      if(expected_temp >= 50) expected_temp -= 1;

      // drawing the temperature. TODO: functionlize to update_display(__,__);
      tft.fillRoundRect(380, 20, 90, 40, 8, GREY);
      b.rect_button(370, 20, 100, 40, 8, BLACK, GREY);
      b.setText(380, 30, 3, BLACK, round(expected_temp));
      tft.print(char(247));
      tft.print("F");
      delay(200);
    }
    // setting the temperature.
    if(b.pressed("TR", &p))
    {
      temp_history[10] = {0};
      while((pos > low) && (pos < high))
      {
        //Animation block
        tft.fillRoundRect(370, 20, 100, 40, 8, GREY);
        b.setText(380, 30, 3, BLACK, round(expected_temp));
        tft.print(char(247));
        tft.print("F");
        //**


        temp = round(thermocouple.readFahrenheit()); // read temp
        update_pos(temp, expected_temp); //update position based oncurrent temp.
        delay(500);
        store_change(temp_history, 10, temp); // store the current temp. 

        //showing temperature history in Serial.out
        for (int j = 0; j < 10; j++)
        {
          Serial.print(temp_history[j]);
          Serial.print(", ");            
        }
        Serial.println();

        //break out once the taget temp is reached and is stable for 5 samples.
        if (temp == round(expected_temp) && no_change(temp_history))
        {
          break;
        }

        //update temperature on display.
        tft.fillRoundRect(130, 140, 200, 60, 8, GREY);          
        b.setText(150, 140, 8, BLACK, round(temp));
        tft.print(char(247));
        tft.print("F");
      }
    }
  }

  // Settings
  if (currentpage == 1)
  {
    if (b.pressed("B1", &p))
    {
      b.buttonAnimation(20, 50, 200, 60, 8, WHITE, CYAN);
      b.setText(80, 70, 3, BLACK, "Home");

      currentpage = 0;
      drawHome();
    }
    if (b.pressed("B2", &p))
    {
      b.buttonAnimation(20, 125, 200, 60, 8, WHITE, CYAN);
      b.setText(55, 150, 2, BLACK, "Calibration");

      currentpage = 2;
      drawCalibration();
      p.z = 0;
    }
    if (b.pressed("B3", &p))
    {
      b.buttonAnimation(20, 200, 200, 60, 8, WHITE, CYAN);
      b.setText(60, 225, 2, BLACK, "LED Lights");
    }
  }

  // Calibration
  if (currentpage == 2)
  {
    if (b.pressed("TL", &p))
    {
      b.buttonAnimation(20, 20, 150, 40, 8, WHITE, BLACK);
      b.setText(60, 30, 3, WHITE, "Back");

      pos = 1;
      myservo.write(0);
      currentpage = 1;
      drawSettings();
    }

    //Set Min
    if (b.pressed("ML", &p))
    {
      b.buttonAnimation(20, 125, 200, 60, 8, WHITE, BLACK);
      b.setText(35, 150, 2, WHITE, "Set to Min Pos.");
      low = pos;
    }

    // Set Max
    if (b.pressed("MR", &p))
    {
      b.buttonAnimation(260, 125, 200, 60, 8, WHITE, BLACK);
      b.setText(275, 150, 2, WHITE, "Set to Max Pos.");
      high = pos;
    }

    //Moves motor to high
    if (b.pressed("LA", &p))
    {
      if(pos != 180)
      {
        pos = pos + 1;
      }
      myservo.write(pos);
      delay(15); 
    }

    //Moves motor to low
    if (b.pressed("RA", &p))
    {
      if(pos != 0)
      {
        pos = pos - 1;
      }
      myservo.write(pos);
      delay(15); 
    }
  }
}

  //Manually changing temp.
  // if (currentpage == 3)
  // {

  // }
