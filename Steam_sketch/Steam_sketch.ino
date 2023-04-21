#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h> //resistive touch screen library
#include "max6675.h" //temperature amplifier library
#include <Servo.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h> // LED lights library


//touch screen paramsas
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 23   // can be a digital pinffff
#define XP 22   // can be a digital pin

// display parameters.
#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0

//LED paramaters
#define LED_PIN 10
#define LED_COUNT 30

// Assign human-readable names to some common 16-bit color values:
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x03E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define PURPLE  0x780F 
#define GREY    0xC618
#define DARKGREY 0x6B0C
#define ORANGE  0xFD20
#define PINK    0xF81F

//Pressure range for the touch screen. 
#define MINPRESSURE 10
#define MAXPRESSURE 1000

//defining pages.
#define HOME 0
#define SETTINGS 1
#define CAL 2 //Calibration
#define LED 3

// EEPROM addresses variable addresses.
#define lowAddress 0
#define highAddress 4

//Pin numbers for the temp sensor. It the uses SPI protocal.
int thermoDO = 50; // data out pin (Master In Slave Out -- MISO) 
int thermoCS = 53; // Chip select pin 
int thermoCLK = 52; // Serial clock pin 

// Global variables
int currentpage = 0;
float curr_temp;
unsigned long timeLapsed;
int expected_temp;
int low;
int high;
int motorPos = 90; 
int k;


//flags
bool stopFlag = false;

//Class that facilitates button creation, custimization, and animation.
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

    // customize button fill, board color, text, and text size.
    void customize(int16_t _b_color, uint16_t _f_color, String _text, int size, int _textColor = WHITE)
    {
      b_color = _b_color;
      f_color = _f_color;
      text = _text;
      textColor = _textColor;
      textSize = size;
    }

    // method that draws button on the screen.
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

    // method that writes text inside of the button
    void setText(String text)
    {
      tft->setCursor(this->xcoor+this->offx, this->ycoor+this->offy);
      tft->setTextSize(this->textSize);
      tft->setFont();
      tft->setTextColor(textColor);
      tft->print(text);
    }

    //offsets the position of the sets with respect to the button
    void setTextPosition(int offx, int offy)
    {
      this->offx = offx;
      this->offy = offy;
    }

    // button animation. 
    void Animate()
    {
      tft->fillRoundRect(this->xcoor, this->ycoor, this->height, this->width, this->radius, this->b_color);
      delay(70);
      this->Draw();
    }

    // setting the pressure window for the button.
    void setPressurePts(int xlow, int xhigh, int ylow, int yhigh)
    {
      this->TSPressurePt[0] = xlow;
      this->TSPressurePt[1] = xhigh;
      this->TSPressurePt[2] = ylow;
      this->TSPressurePt[3] = yhigh;
    }

    // check whether a button is pressed or not.
    bool isPressed(TSPoint *p)
    {
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
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); 

// creating all of the button objects.
buttons TopLeftBtn(&tft, 20, 20, 150, 40);
buttons LeftArrowBtn(&tft, 220, 295, 160, 265, 220, 235);
buttons RightArrowBtn(&tft, 260, 295, 320, 265, 260, 235);
buttons homeBtn(&tft, 20, 50, 200, 60);
buttons calibrationBtn(&tft, 20, 125, 200, 60);
buttons LEDBtn(&tft, 20, 200, 200, 60);
buttons unknown1(&tft, 260, 50, 200, 60);
buttons unknown2(&tft, 260, 125, 200, 60);
buttons unknown3(&tft, 260, 200, 200, 60);
buttons setMinBtn(&tft, 20, 125, 200, 60);
buttons setMaxBtn(&tft, 260, 125, 200, 60);
buttons color1(&tft, 210, 80, 80, 60);
buttons color2(&tft, 290, 80, 80, 60);
buttons color3(&tft, 370, 80, 80, 60);
buttons color4(&tft, 210, 140, 80, 60);
buttons color5(&tft, 290, 140, 80, 60);
buttons color6(&tft, 370, 140, 80, 60);
buttons color7(&tft, 210, 200, 80, 60);
buttons color8(&tft, 290, 200, 80, 60);
buttons color9(&tft, 370, 200, 80, 60);
buttons targetTempBtn(&tft, 130, 120, 220, 60);
buttons current_temp(&tft, 370, 20, 90, 40);

void setup(){
  // ISR set up.
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), emergencyStop, FALLING);


  //loading in settings from EEPROM
  low = EEPROM[lowAddress];
  high = EEPROM[highAddress];
  myservo.write(low);


  //servo init
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object

  //led strip init.
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(20); // Set BRIGHTNESS to about 1/5 (max = 255)

  //TFT init--------------------
  tft.reset();
  tft.begin(tft.readID());
  Serial.println();

  //console log for TFT.
  Serial.begin(9600);

  Serial.print("reading id...0x");
  delay(500);
  Serial.println(tft.readID(), HEX);
  
  tft.fillScreen(BLACK);
  tft.setRotation(1); //set to horizontal display mode 

  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  //----------------------------------

  //calibration screen buttons set up.
  setMinBtn.customize(WHITE, BLACK, "Set to Min Pos.", 2, WHITE);
  setMinBtn.setTextPosition(10, 10);
  setMinBtn.setPressurePts(470, 580, 125, 460);
  setMaxBtn.customize(WHITE, BLACK, "Set to Max Pos.", 2, WHITE);
  setMaxBtn.setTextPosition(10, 10);
  setMaxBtn.setPressurePts(470, 580, 560, 895);
  
  //settings screen buttons set up
  homeBtn.customize(WHITE, CYAN, "Home", 3, BLACK);
  homeBtn.setTextPosition(60, 20);
  homeBtn.setPressurePts(670, 780, 125, 460);
  
  //calibration button setup.
  calibrationBtn.customize(WHITE, CYAN, "Calibration", 2, BLACK);
  calibrationBtn.setTextPosition(35, 25);
  calibrationBtn.setPressurePts(470, 580, 125, 460);
  LEDBtn.customize(WHITE, CYAN, "LED Lights", 2, BLACK);
  LEDBtn.setTextPosition(40, 25);
  LEDBtn.setPressurePts(270, 380, 125, 460);

  //home screen buttons set up.
  TopLeftBtn.setPressurePts(770, 830, 120, 350);
  LeftArrowBtn.customize(BLUE, BLUE, "", 0, BLUE);
  LeftArrowBtn.setPressurePts(200, 320, 370, 470);
  RightArrowBtn.customize(RED, RED, "", 0, RED);
  RightArrowBtn.setPressurePts(200, 320, 540, 640);

  //led screen buttons set up.
  color1.customize(RED, RED, "", 2, BLACK);
  color1.setPressurePts(570, 670, 450, 560);
  color2.customize(BLUE, BLUE, "", 2, BLACK);
  color2.setPressurePts(570, 670, 600, 710);
  color3.customize(GREEN, GREEN, "", 2, BLACK);
  color3.setPressurePts(570, 670, 750, 860);
  color4.customize(YELLOW, YELLOW, "", 2, BLACK);
  color4.setPressurePts(430, 530, 450, 560);
  color5.customize(ORANGE, ORANGE, "", 2, BLACK);
  color5.setPressurePts(430, 530, 600, 710);
  color6.customize(PINK, PINK, "", 2, BLACK);
  color6.setPressurePts(430, 530, 750, 860);
  color7.customize(PURPLE, PURPLE, "", 2, BLACK);
  color7.setPressurePts(280, 370, 450, 560);
  color8.customize(WHITE, WHITE, "", 2, BLACK);
  color8.setPressurePts(280, 370, 600, 710);
  color9.customize(GREY, GREY, "", 2, BLACK);
  color9.setPressurePts(280, 370, 750, 860);

  //initial temperature reading.
  curr_temp = thermocouple.readFahrenheit();
  expected_temp = curr_temp;

  //Target button set up.
  targetTempBtn.customize(BLACK, GREY, String(expected_temp), 6, BLACK);
  targetTempBtn.setPressurePts(500, 610, 325, 690);

  //current temp text on screen. 
  current_temp.customize(GREY, GREY, String(curr_temp), 3, BLACK);

  drawHome();
}

// function that plots on serial.plotter.
void monitor(float temp, float angle, int exp_temp)
{
  Serial.print("Currtemp:");
  Serial.print(temp);
  Serial.print(",");
  Serial.print("angle:");
  Serial.print(angle);
  Serial.print(",");
  Serial.print("expected:");
  Serial.println(exp_temp);

}

// drawing the home screen.
void drawHome()
{
  // Drawing home screen background
  tft.fillScreen(GREY);
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);

  // customizing Top left button and drawing it on to the screen. 
  TopLeftBtn.customize(WHITE, GREY, "Settings", 2, BLACK);
  TopLeftBtn.setTextPosition(25, 15);
  
  TopLeftBtn.Draw();
  LeftArrowBtn.Draw();
  RightArrowBtn.Draw();

  //draw expexted temperature in the middle
  targetTempBtn.Draw();
  current_temp.Draw();
  //draw current temperature on the top right of the screen.

}
// Draws the setting menu
void drawSettings()
{
  //Drawing home screen background
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);
  
  //draw setting buttons (Home, Calibration, LED Lights)
  homeBtn.Draw();
  calibrationBtn.Draw();
  LEDBtn.Draw();
}

// Draws the calibration menu
void drawCalibration()
{
  //drawing calibration page background.
  tft.fillScreen(GREY);                 
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);
  
  // setting top left button as Back button. 
  TopLeftBtn.customize(WHITE, GREY, "Back", 3, BLACK);
  TopLeftBtn.setTextPosition(40, 10);

  //drawing everything on the display
  TopLeftBtn.Draw(); // Back button
  LeftArrowBtn.Draw(); // Increases motor position
  RightArrowBtn.Draw(); // Decreases motor position
  setMinBtn.Draw(); // Sets the motors min position
  setMaxBtn.Draw(); // Sets the motors max position
}


// Draws the LED lights menu
void drawLEDMenu()
{
  // Drawing the page background
  tft.fillScreen(BLACK);                 
  tft.drawRoundRect(0, 0, 479, 319, 8, WHITE);

  // setting top left button as Back button.   
  TopLeftBtn.customize(WHITE, BLACK, "Back", 3, WHITE);
  TopLeftBtn.setTextPosition(40, 10);
  TopLeftBtn.Draw();

  //drawing buttons for different colors.
  color1.Draw();
  color2.Draw();
  color3.Draw();
  color4.Draw();
  color5.Draw();
  color6.Draw();
  color7.Draw();
  color8.Draw();
  color9.Draw();
  
}

// Updates the position of the motor based on certain parameters
void update_pos(float curr_temp, int expected_temp)
{
  // Taking the difference in current temp and expected temp and
  // moving the position of the motor
  int mult = 0;
  if (abs(curr_temp-expected_temp) <= 3) mult = 0;
  else if (abs(curr_temp-expected_temp) <= 10) mult = 1;
  else if (abs(curr_temp-expected_temp) <= 20) mult = 5;
  else if (abs(curr_temp-expected_temp) <= 30) mult = 8;

  if (expected_temp > curr_temp && motorPos >= high + 2)
  {
    motorPos -= 1*mult; // turns the motor counter clockwise, to hot.
    myservo.write(motorPos);
  }
  else if (expected_temp < curr_temp && motorPos <= low - 2)
  {
    motorPos += 1*mult; // turns the motor counter clockwise, to .
    myservo.write(motorPos);
  }
}

// Storing temperature data into an array
void store_change(float *arr, int n, float value)
{
  memmove(&arr[1], &arr[0], (n - 1) * sizeof(float));
  arr[0] = value;
}

// Checks to see if the current temp is below or above
// 3 degrees
bool withinThreshold(float curr, int exp)
{
  return abs(exp - curr) <= 3;
}

// Checks the array for any change or difference of 0.5
bool no_change(float *arr)
{
  for (int i = 0; i < 7; i++)
  {
    for (int j =0; j < 7; j++)
    {
      if (abs(arr[i] - arr[j]) > .5)
        return false;
    }
  }
  return true;
}

// Rainbow LED
void rainbow(int wait) {
  strip.rainbow(0);
  strip.show(); // Update strip with new contents
  delay(wait);  // Pause for a moment
}

void loop()
{
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  
  // Home Screen
  if (currentpage == HOME)
  {
    if (millis() - timeLapsed > 2000)
    {
      current_temp.text = thermocouple.readFahrenheit();
      current_temp.Draw();
      timeLapsed = millis();
    }

    // Directs you to the setting page
    else if (TopLeftBtn.isPressed(&p))
    {
      TopLeftBtn.Animate();
      currentpage = SETTINGS;
      drawSettings();
    }

    // Begins to find the target temperature
    else if (targetTempBtn.isPressed(&p))
    {
      if (expected_temp <= 79.2) motorPos = 172;
      else if (expected_temp <= 87) motorPos = 130;
      else if (expected_temp <= 102) motorPos = 105;
      else if (expected_temp <= 123) motorPos = 94;
      else if (expected_temp <= 130) motorPos = 91;
      myservo.write(motorPos);
      targetTempBtn.Animate();
      float temp_history[8] = {0};
      stopFlag = false;

      // while the ISR is not triggered.
      while(!stopFlag)
      {
        
        //store  temperature samples in a size 8 array.
        curr_temp = thermocouple.readFahrenheit(); 
        store_change(temp_history, 8, curr_temp);
        monitor(curr_temp, motorPos, expected_temp);

        // after 2 seconds, i.e we have 8 temperature samples, IF the water temp stablized
        // THEN update the position.
        if(no_change(temp_history) && !withinThreshold(curr_temp, expected_temp))
        {
          
          //updating the position waiting 3 seconds and storing the temperature change.
          update_pos(curr_temp, expected_temp); 
          for(int i = 0; i < 12; i++)
          {
            curr_temp = thermocouple.readFahrenheit(); 
            store_change(temp_history, 8, curr_temp);
            monitor(curr_temp, motorPos, expected_temp);
            current_temp.text = curr_temp;
            current_temp.Draw();
            delay(250);
          }

          // while the temperature changes, continue to listen.
          while(!no_change(temp_history) && !stopFlag){
            monitor(curr_temp, motorPos, expected_temp);
            curr_temp = thermocouple.readFahrenheit();
            current_temp.text = curr_temp;
            current_temp.Draw();

            store_change(temp_history, 8, curr_temp);
            delay(250);
          }
          timeLapsed = millis();
        }
        else if(withinThreshold(curr_temp, expected_temp) && no_change(temp_history))
          break;

        current_temp.text = curr_temp;
        current_temp.Draw();
        delay(250); // wait for the temp sensor.
      }
    }
    // else if left isPressed, then decrease expected temp.
    else if (LeftArrowBtn.isPressed(&p))
    {
      expected_temp = expected_temp - 1;
      targetTempBtn.text = expected_temp;
      targetTempBtn.Draw();
    }
    // else if right isPressed, then increase expected temp.
    else if (RightArrowBtn.isPressed(&p))
    {
      expected_temp = expected_temp + 1;
      targetTempBtn.text = expected_temp;
      targetTempBtn.Draw();
    }
  }
  // Settings Screen
  else if (currentpage == SETTINGS)
  {
    // If home isPressed, Then draw home screen
    if (homeBtn.isPressed(&p)) 
    {
      homeBtn.Animate();
      currentpage = HOME;
      drawHome();
    }
    // if calibration button isPressed, Then draw CALibration screen.
    else if (calibrationBtn.isPressed(&p))
    {
      calibrationBtn.Animate();
      currentpage = CAL;
      // motorPos = 90; // resets motor position to half-way point
      drawCalibration();
    }
    else if (LEDBtn.isPressed(&p))
    {
      LEDBtn.Animate();
      currentpage = LED;
      drawLEDMenu();
    }
  }
  // Calibration screen.
  else if (currentpage == CAL)
  {
    curr_temp = thermocouple.readFahrenheit();

    //if BACK is isPressed, then go back to SETTINGS. 
    if (TopLeftBtn.isPressed(&p))
    {
      TopLeftBtn.Animate();
      motorPos = low;
      myservo.write(motorPos);
      currentpage = SETTINGS;
      drawSettings();
    }
    //if left isPressed, then rotate motor left.
    else if (LeftArrowBtn.isPressed(&p) && motorPos != 180)
    {
      motorPos = motorPos + 1;
      myservo.write(motorPos);
      delay(15);
    }
    // if right isPressed, then rotate motor right.
    else if (RightArrowBtn.isPressed(&p) && motorPos != 0)
    {
      motorPos = motorPos - 1;
      myservo.write(motorPos);
      delay(15);
    }
    // if min btn is pressed, then set the min variable.
    else if(setMinBtn.isPressed(&p))
    {
      setMinBtn.Animate();
      low = motorPos;
      EEPROM[lowAddress] = low;
    }
    // if max btn is pressed, then set the max vairable.
    else if (setMaxBtn.isPressed(&p))
    {
      setMaxBtn.Animate();
      high = motorPos;
      EEPROM[highAddress] = high;
    }
  }
  // LED Page. this page is used to set the LED lights color.
  else if (currentpage == LED)
  {
    if (TopLeftBtn.isPressed(&p))
    {
      TopLeftBtn.Animate();
      currentpage = SETTINGS;
      drawSettings();
    }
    else if(color1.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 255, 0, 0);
        strip.show();
      }      
    }
    else if(color2.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 0, 0, 255);
        strip.show();
      }
    }
    else if(color3.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 0, 255, 0);
        strip.show();
      }
    }
    else if(color4.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 255, 255, 0);
        strip.show();
      }
    }
    else if(color5.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 139, 64, 0);
        strip.show();
      }
    }
    else if(color6.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 255, 0, 255);
        strip.show();
      }
    }
    else if(color7.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 75, 0, 130);
        strip.show();
      }
    }
    else if(color8.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        strip.setPixelColor(k, 255, 255, 255);
        strip.show();
      }
    }
    else if(color9.isPressed(&p))
    {
      for(k = 0; k < LED_COUNT; k++)
      {
        rainbow(10);
        strip.show();
      }
    }
  }
}

// ISR that stops the shower and sets the respective flag.
void emergencyStop(){
    stopFlag = true;
    myservo.write(low);
  }