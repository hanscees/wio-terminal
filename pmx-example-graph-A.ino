// seeed wio-terminal combined with grove sen55 to monitor air-quality on the lcd screen of the wio-terminal
// adapted from SDS011 Air Quality Monitor
// --------------------------
//
// Adapted from SDS011 Sensor libray by R. Zschiegner (rz@madavi.de).
// adjusted by hanscees@hanscees.com for wio-terminal with seeed sens55 all-in-one air quality monitor

// changes, many mainly because old code paused device 5 minutes between readings, sen55 not
// also got rid of fancy fonts

// added coordinates to code R01 - R14 to understand where what texts are 

// 0   |--------------------------------|--------------------------------320
//     |
//     |   R04 ->--------------------------------------------------
//     |   | R7        R10             R05                     R12
//     |   | R8        R11   R02->
// 120 |   -                  |
//     |
//     |
//     |
//     |
//     |  R13                                                        R01
// 240 ------------------------------------------------------------------320


#include <Arduino.h>
#include <SensirionI2CSen5x.h>  // seeed sens55 library (sensorion )
#include <Wire.h>   // for i2c sensors
#include <TFT_eSPI.h>  // tft library
#include "rpcWiFi.h" //Wi-Fi library
// #include"Free_Fonts.h" // fonts
// #include"seeed_line_chart.h" //include the library


SensirionI2CSen5x sen5x; 
TFT_eSPI tft; //initialize TFT LCD

const char* ssid = "abcdefg";             // Your WiFi SSID.
const char* password = "*****";           // Your WiFi Password.

String serverIP = "192.168.1.3:8802";          // The server IP address and Port number set up in the PC Server/plotter Application.
String deviceId = "air_quality";               // The device ID that the PC server Application will recognize.



int loopCount = 0;

String quality;                                           // Define PM2.5 value as LOW, MEDIUM etc (UK Defra scale).
int colour;                                               // Define PM2.5 value as colour (UK Defra scale)

// ####################  in here goes the bargraph data!! 
short pm25Array[320];                                     // Array to hold sensor values for the histogram.

float p10, p25;                                           // Variabled for PM10 and MP2.5 data from sensor.
int error;                                                // Confirms valid data from sensor. 0 = error.
short arrayPointer = 15;                                  // Array element currently being written to (16-bit integer)
int yPos;                                                 // Vertical marker for bar chart.
int sleepSeconds;

float volts = 0.0;
const float Vmax = 5.75;                                 // Max voltage can be set here. Depends on resistor tolerances.



//  ##############################  draw histogram bargraph  ########################
// todo dont understand constrain line and sqrt: value is stored *10 ????
// this function was original to get an array from storage after pausing the device
// but we dont pause this sens55 device since it can run for 10 years sais the specs

void plotHistogram() {                                    // Function to re-draw the histogram.
   tft.fillRect(15,120, 319, 90, ILI9341_BLACK);          // Clear plotting area

   byte line;
   for (int i = 15; i <= 319;  i++){
      getTextData25(pm25Array[i] / 10);                   // Get colour corrsponding to each air quality level. Value is stored
                                                          // multiplied by 10 so divide by 10 here to get true value.                        
      line = constrain(sqrt(pm25Array[i]*40), 0, 105);    // Calculate length of line to plot. sqrt compresses higher values.
     tft.drawFastVLine(i, 225 - line, line,  colour);     // Draw vertical line in chosen colour.
   }
}


#define LIGHT_GREEN  0x9FF3                               // Define colours used by UK Defra to specify pollutant bands.
#define MID_GREEN    0x37E0
#define DARK_GREEN   0x3660
#define LIGHT_YELLOW 0xFFE0
#define MID_YELLOW   0xFE60
#define ORANGE       0xFCC0
#define LIGHT_RED    0xFB2C
#define MID_RED      0xF800
#define DARK_RED     0x9800
#define PURPLE       0xC99F


// UK air pollution bands for PM2.5 and PM10 Particles.  
// https://uk-air.defra.gov.uk/air-pollution/daqi?view=more-info&pollutant=pm25#pollutant

int getTextData25(int value) {                                                         // Function sets three global variables: 'Ypos'
  switch (value) {                                                                     // (vertical cursor position), 'colour' &
    case 0 ... 11 : yPos = 100; colour = LIGHT_GREEN; quality = "1 LOW"; break;        // 'quality' and returns half the length of the
    case 12 ... 23 : yPos = 90; colour = MID_GREEN; quality = "2 LOW"; break;          // text string 'quality' whose value is used to
    case 24 ... 35 : yPos = 80; colour = DARK_GREEN; quality = "3 LOW"; break;         // centre justify the text on the display.
    case 36 ... 41 : yPos = 70; colour = LIGHT_YELLOW; quality = "4 MODERATE"; break;
    case 42 ... 47 : yPos = 60; colour = MID_YELLOW; quality = "5 MODERATE"; break;
    case 48 ... 53 : yPos = 50; colour = ORANGE; quality = "6 MODERATE";  break;
    case 54 ... 58 : yPos = 40; colour = LIGHT_RED;  quality = "7 HIGH"; break;
    case 59 ... 64 : yPos = 30; colour = MID_RED;  quality = "8 HIGH"; break;
    case 65 ... 70 : yPos = 20; colour = DARK_RED;  quality = "9 HIGH"; break;  
    case 71 ... 9999: yPos = 10; colour = PURPLE;  quality = "10 VERY HIGH"; break;   
    default: yPos = 10; colour = ILI9341_MAGENTA;  quality = "HAZARDOUS"; break;
  }  
  return (quality.length() / 2) * 6;
}                                         


int getTextDataPM10(int value) {
  switch (value) {
    case 0 ... 16 : colour = LIGHT_GREEN; quality = "1 LOW"; break;
    case 17 ... 33 : colour = MID_GREEN; quality = "2 LOW"; break;
    case 34 ... 50 :  colour = DARK_GREEN; quality = "3 LOW"; break;
    case 51 ... 58 : colour = LIGHT_YELLOW; quality = "4 MODERATE"; break;
    case 59 ... 66 : colour = MID_YELLOW; quality = "5 MODERATE"; break;
    case 67 ... 75 : colour = ORANGE; quality = "6 MODERATE";  break;
    case 76 ... 83 : colour = LIGHT_RED;  quality = "7 HIGH"; break;
    case 84 ... 91 : colour = MID_RED;  quality = "8 HIGH"; break;
    case 92 ... 100 : colour = DARK_RED;  quality = "9 HIGH"; break;  
    case 101 ... 9999: colour = PURPLE;  quality = "10 VERY HIGH"; break;   
    default: colour = ILI9341_MAGENTA;  quality = "HAZARDOUS"; break;
  }
   return (quality.length() / 2) * 6;
}


void printSerialNumber() {
    uint16_t error;
    char errorMessage[256];
    unsigned char serialNumber[32];
    uint8_t serialNumberSize = 32;

    error = sen5x.getSerialNumber(serialNumber, serialNumberSize);
    if (error) {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("SerialNumber:");
        Serial.println((char*)serialNumber);
    }
}


void setup() {

  int serial_timeout = 0;
  Serial.begin(115200);
  while (!Serial) {
        delay(100);
     serial_timeout++;
    if(serial_timeout > 100) {
    //  tft.setCursor(0, 35);
    //  tft.println("Failed to connect to serial");
      break;
    }



    }

Serial.println("hi, just started serial device");
delay(5000);

  tft.begin(); 

  tft.setRotation(1); 
  tft.setTextWrap(true);

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(1);
 

  // sleepSeconds = (SAMPLE_INTERVAL * 60) - SAMPLE_SECS;          // Calculate sleep time in seconds.

Serial.println("beginning wifi sequence");

  WiFi.begin(ssid, password);                                   // Connect WiFi to server running on PC to
                                                                // plot PM10 and PM2.5 values.
 
  int wifi_timeout = 0;
  tft.setCursor(0, 10);
  tft.print("Connecting to WiFi...");
  Serial.println("connecting to wifi ");
  while (WiFi.status() != WL_CONNECTED)  {
    delay(10);
    tft.setCursor(0, 20);
    tft.fillRect(0, 20, 100, 10, ILI9341_BLACK);
    tft.print(wifi_timeout);
    Serial.println("waiting for wifi, toot ");
    wifi_timeout++;
    if(wifi_timeout > 100) {
      tft.setCursor(0, 35);
      tft.println("Failed to connect to Wifi");
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.setCursor(0, 20);
    tft.print("Connected to ");
    tft.println(WiFi.SSID());
    tft.print("IP address: ");
    tft.println(WiFi.localIP());
    delay(500);
  }
 
 Serial.println("wifi done, lets print static labels");
  tft.fillScreen(ILI9341_BLACK);
 
  tft.setCursor(15, tft.height() -20);
 
// tft.setFreeFont(FF10); //select Free, Mono, Oblique, 12pt.
//  tft.drawString("Mono 12pt",70,110);//prints string at (70,110)
 
//   tft.drawString("Hello world!",0,0); //draw text string 


  tft.setTextSize(1);                                         // Print static labels and headers on display.
  tft.setCursor(150, 10);
  tft.println("PM 2.5");
  tft.setCursor(150, 17);
  tft.print("ug/m3");

  tft.setCursor(5, 10);
  tft.print("PM 10"); 
  tft.setCursor(5, 17);
  tft.print("ug/m3"); 

  tft.fillRect(312, 10, 6, 10, PURPLE);                        // Print a colour key for the Defra pollutant bands.
  tft.fillRect(312, 20, 6, 10, DARK_RED);
  tft.fillRect(312, 30, 6, 10, MID_RED);
  tft.fillRect(312, 40, 6, 10, LIGHT_RED);
  tft.fillRect(312, 50, 6, 10, ORANGE);
  tft.fillRect(312, 60, 6, 10, MID_YELLOW);
  tft.fillRect(312, 70, 6, 10, LIGHT_YELLOW);
  tft.fillRect(312, 80, 6, 10, DARK_GREEN);
  tft.fillRect(312, 90, 6, 10, MID_GREEN);
  tft.fillRect(312, 100, 6, 10, LIGHT_GREEN);
  
  Serial.println("polluted bands are on screen now, see? ");
//  delay(5000);


  tft.drawFastVLine(13, 120, 108, ILI9341_BLUE);               // Draw histogram vertical axis
  tft.setTextColor(ILI9341_BLUE);                        // why only till 50, pm2.5 can be 350 ?
 
  tft.setCursor(0, 120);
  tft.print(" ^");
  tft.setCursor(0, 130);
  tft.print("50"); 
  tft.setCursor(0, 170);
  tft.print("10"); 
  tft.setCursor(0, 200);
  tft.print(" 1");


  // it actually uses tft.height and tft.width 
  tft.drawFastHLine(12, tft.height() - 14, tft.width()-1, ILI9341_BLUE);     // Draw histogram horizontal axis

  for (int x = 319; x > 15; x-=12) {
     tft.drawFastVLine(x, 227, 3, ILI9341_BLUE);                             // Draw 1-hour ticks on horizontal axis.
  }
  tft.setTextColor(ILI9341_BLUE);
  tft.setCursor(105, 232); 
  tft.print("Air Quality Monitor");  // quality scale goes from 1 LOW to hazardous


    Serial.println("histogram axis have been drawn, see? ");
//   delay(5000);

   int rssi = WiFi.RSSI();

   tft.setCursor(0,  232); 
   tft.fillRect(0, 232, 50, 8, ILI9341_BLACK);
   tft.setTextSize(1);

   if (WiFi.status() == WL_CONNECTED) {
     tft.setTextColor(ILI9341_BLUE);
     tft.print("RSSI " + String(rssi) + " dB");
   } else {
     tft.setTextColor(ILI9341_RED);
     tft.print("No WiFi");
     Serial.println("NO WIFI ");    
   }
   

   // lets see if the sensor works , function to use

  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);  // watch out todo
  
  // now start sensor sens55

  Serial.println("starting sensor, blieb blieb ? ");
//  delay(5000);


    Wire.begin();

    sen5x.begin(Wire);

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();
    if (error) {
        Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }

  Serial.println("if sens55 errors, you should see tht now, plotting begins for the first time ");
//  delay(5000);


  // first time we plot histogram, since we dont save it after a current outage it will be empty
  plotHistogram();
                                    

    // uint16_t error;
    // Adjust tempOffset to account for additional temperature offsets
    // exceeding the SEN module's self heating.
    float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {
        Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("Temperature Offset set to ");
        Serial.print(tempOffset);
        Serial.println(" deg. Celsius (SEN54/SEN55 only");
    }

    // Start Measurement
    error = sen5x.startMeasurement();
    if (error) {
        Serial.println("Error trying to execute startMeasurement(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    }
}

void loop() {

  Serial.print("starting void loopt now ");
  delay(5000);                                           // delay in loop. 5 seconds


  tft.setTextSize(1);                                     //
  tft.setCursor(248, 232);   
  tft.setTextColor(ILI9341_GREEN, ILI9341_BLACK);
  tft.print("SAMPLING ");  

  //  unclear if this will work with WIO terminal : yes it does!! 
  int raw = 0;                                             // Get supply voltage. Useful when battery operated.
  for (byte i=0;i<10;i++) {
    raw += analogRead(A0);
    delay(10);
  }
  raw = raw / 10;
  float volts = (raw / 1023.0) * Vmax;

  
  tft.fillRect(248, 232, 70, 10, ILI9341_BLACK); //  R01 right below: where sampling was printed: 
 
  tft.fillRect(85, 105, 65, 8, ILI9341_BLACK);   // R02 ?? central text probably we dont use
  

  //  original code replaced by code for sens55  
  
  uint16_t error;
    char errorMessage[256];

//    delay(1000);

    // Read Measurement
    float massConcentrationPm1p0;
    float massConcentrationPm2p5;
    float massConcentrationPm4p0;
    float massConcentrationPm10p0;
    float ambientHumidity;
    float ambientTemperature;
    float vocIndex;
    float noxIndex;

    error = sen5x.readMeasuredValues(
        massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
        massConcentrationPm10p0, ambientHumidity, ambientTemperature, vocIndex,
        noxIndex);

    if (error) {
        Serial.println("Error trying to execute readMeasuredValues(): ");
        errorToString(error, errorMessage, 256);
        Serial.println(errorMessage);
    } else {
        Serial.print("MassConcentrationPm1p0:");
        Serial.print(massConcentrationPm1p0);
        Serial.print("\t");
        Serial.print("MassConcentrationPm2p5:");
        Serial.print(massConcentrationPm2p5);
        Serial.print("\t");
        Serial.print("MassConcentrationPm4p0:");
        Serial.print(massConcentrationPm4p0);
        Serial.print("\t");
        Serial.print("MassConcentrationPm10p0:");
        Serial.print(massConcentrationPm10p0);
        Serial.print("\t");
        Serial.print("AmbientHumidity:");
        if (isnan(ambientHumidity)) {
            Serial.print("n/a");
        } else {
            Serial.print(ambientHumidity);
        }
        Serial.print("\t");
        Serial.print("AmbientTemperature:");
        if (isnan(ambientTemperature)) {
            Serial.print("n/a");
        } else {
            Serial.print(ambientTemperature);
        }
        Serial.print("\t");
        Serial.print("VocIndex:");
        if (isnan(vocIndex)) {
            Serial.print("n/a");
        } else {
            Serial.print(vocIndex);
        }
        Serial.print("\t");
        Serial.print("NoxIndex:");
        if (isnan(noxIndex)) {
            Serial.println("n/a");
        } else {
            Serial.println(noxIndex);
        }
    }

    // sens55 variables we have just measured are
//    float massConcentrationPm1p0;
//    float massConcentrationPm2p5;
//    float massConcentrationPm4p0;
//    float massConcentrationPm10p0;
//    float ambientHumidity;
//    float ambientTemperature;
//    float vocIndex;
//    float noxIndex;


//	error = sdsSensor.read(&p25,&p10);                   // Read PM2.5 and PM10 values from sensor.
//	if (! error) {
//   Serial.print("P2.5: ");
//   Serial.println(p25);
//	 Serial.print("P10:  ");
//   Serial.println(p10);

// so old variables were called p25 and p10 

    
   int x = getTextData25(massConcentrationPm2p5); // yields quality (from 1 low to hazardous) and colour and Ypos
//     case 34 ... 50 :  colour = DARK_GREEN; quality = "3 LOW"; break;

// and apparantly the width of the quality text, prob in pixels?? 
// so x is 1/2 length of text in pixels


   Serial.println("what is x now?");  // yields 12 mostly when low, 36 when high 
   Serial.println(x);
 //  int x = getTextData25(p25);                                          // Function retuns (width of text)/2 so we can
                                                                        // centre-justify it on the display. It also sets
   tft.setTextColor(colour);                                            // the text colour appropriate to the PM2.5 value as
                                                                        // defined by the UK Defra documentation.
    // triangles point right-above where we are in the pmX scaling (green or red or.. )                                    
   tft.fillRect(305, 10, 5, 105, ILI9341_BLACK);                        // Clear old triangle
   tft.fillTriangle(305, yPos, 308, yPos+5, 305, yPos+10, colour);      // Plot new position of triangle on colour scale
   

// #############################################################
// print middle top screen pm2.5 values ########################

   tft.fillRect(100, 40, 110, 8, ILI9341_BLACK);   // R03 volt?         // Clear display areas where new text will                
                                                                        // be drawn. (Graphical fonts don't overwrite
   tft.fillRect(0, 36, 285, 77, ILI9341_BLACK);   // R04 all above histogram?   // previous text.
   
// rectange is drawn like coordinate 1, (width, height), length horizontal line, lenhth vertical line )

   // todo ######################### why does this go wrong? 
   // ok here we use the quality text pixel length 
    tft.setCursor(165 - x, 40);     // pm2.5 above         // Set cursor to centre of display area.
   
   tft.print(quality);  // R05 air quality scale goes from 1 LOW to hazardous: shows pm 2.5 in center above

   tft.setTextSize(2);
 // tft.setFreeFont(&FreeSansBold12pt7b);  // yes seeed code
   // tft.setFreeFont(FF10); //select Free, Mono, Oblique, 18pt.     // Change to new font.

   String sp25 = String(massConcentrationPm2p5);
 //  String sp25 = String(p25);                                           // Convert PM2.5 value to text because the
                                                                        // 'getTextBounds' function needs text.
 
// doesnt work, forget 
//   int16_t x1, y1;
//   uint16_t w, h;
//   tft.getTextBounds(sp25, 0,0, &x1, &y1, &w, &h);   // doesnt work                   // We mainly want the width of the text that
//                                                                        // we're about to print so we can centre-justify it.
// see https://forums.adafruit.com/viewtopic.php?f=25&t=96909#p487371
//     w = 15;
//     h = 15;
//     Serial.println("wondering what w is here, now in loop  ");
//       Serial.print(w);
//  delay(5000);
//    tft.setCursor(183-(w/2), 110);

// write pm 2.5 in the middle above, bit bigger
  tft.setCursor(165 - x, 50);  // R06 = pm2.5 conc txt
  tft.print(massConcentrationPm2p5, 1);
  
//  Serial.println("just printed pm2.5 on screen now, see? ");
//  delay(5000);
// tft.setFreeFont(&FreeSansBoldOblique12pt7b); //select Free, Sans, Bold, Oblique, 12pt.
   
//   tft.setFont();                                                       // Revert to standard font.

   tft.setTextSize(1);
   
   // print on screen pm10 left upper corner
   tft.setTextColor(colour, ILI9341_BLACK);                             // PM10 data is less-used so just print it
   tft.fillRect(0, 30, 100, 10, ILI9341_BLACK);  // R07                // in the top left corner of the display.
   tft.setCursor(0, 40);
   tft.print(quality);                 // R08 quality pm10 left corner
   tft.setTextSize(2);
   tft.fillRect(0, 55, 60, 20, ILI9341_BLACK); // first erase old text
 
   tft.setCursor(2, 55);  // R09 coordinate
   tft.print(massConcentrationPm10p0, 1);   /// R09 conc pm10 left corner, bigger 

  Serial.println("just printed pm10 see ? ");
//  delay(5000);

// print volt 
   tft.setTextSize(1); 
   tft.setTextColor(ILI9341_GREEN);
   tft.fillRect(80, 10, 50, 10, ILI9341_BLACK); // r10 rectangle volt
   tft.setCursor(80, 10);  //r11  coordinate
   tft.print(volts);  // R11 volts 
   tft.print("v");    // r11

 // Serial.println("just printed volts now see? ");
 // delay(5000);


   tft.fillRect(245, 10, 30, 10, ILI9341_BLACK);  // r12  right corner till 275, so 35 left unknown why 

// lets write temperature and humidity for fun and profit to R12
   tft.setTextColor(TFT_ORANGE);
   tft.setCursor(245, 10);  //r12  coordinate
   tft.print(ambientTemperature);  // R12 temperature
   tft.print(" C");    // r12 

   tft.setTextColor(TFT_BLUE);
   tft.fillRect(80, 30, 30, 10, ILI9341_BLACK); // r10 rectangle volt
   tft.setCursor(80, 30);  //r14  coordinate
   tft.print(ambientHumidity);  // R14 humidity 
   tft.print(" Hum");    // r14 

// lets write VOC and NOX  for fun and profit to R12 
   tft.setTextColor(TFT_YELLOW);
   tft.fillRect(80, 50, 30, 10, ILI9341_BLACK); // r10 rectangle volt
   tft.setCursor(80, 50);  //r15  coordinate

// some dark compiler stuff, I want to get rid of float and show an integer (ie 1, and not 1.00 )
// hope Im doing ok ? 
   int voc_int = static_cast<int>(vocIndex);  // getting rid of float messing up my screen

   tft.print(voc_int);  // R15 VOx 
   tft.print(" VOC");    // r15 

   tft.setTextColor(TFT_RED);
   tft.fillRect(245, 25, 30, 10, ILI9341_BLACK); // r10 rectangle volt
   tft.setCursor(245, 25);  //r14  coordinate
// float to integer 
   int nox_int = static_cast<int>(noxIndex);
   tft.print(nox_int);  // R14 NOx 
   tft.print(" NO");    // r14
//  ambientHumidity, ambientTemperature, vocIndex,  noxIndex);


// ====== plot histogram (bar graph) ==============

   if (arrayPointer >= 319) {                              // If array has been filled, move all values down one.
     for (int i = 15; i <= 319; i++) {
       pm25Array[i] = pm25Array[i+1];
     }
   }
 
 // stuff pm25 into array, but times 10 to save memory probably 
   pm25Array[arrayPointer] = (short) (massConcentrationPm2p5 * 10);           // Multiply float value by 10 to make short integer.

   plotHistogram();  // this is probably where screen flickers , hmm 
   
   if (arrayPointer < 319) arrayPointer++;                 // Increment the pointer to store the next value.



   delay(100);

 
 //======= end plot ====================


   int rssi = WiFi.RSSI();                                   // Get the WiFi signal strength and print on the display.

   tft.setCursor(0,  232); 
   tft.fillRect(0, 232, 50, 8, ILI9341_BLACK);    // R13  left below corner
   tft.setTextSize(1);
   
   if (WiFi.status() == WL_CONNECTED) {
     tft.setTextColor(ILI9341_BLUE);
     tft.print(" RSSI " + String(rssi) + " dB");
   } else {
     tft.setTextColor(ILI9341_RED);
     tft.print(" No WiFi");    
   }
 

   // do some mttq stuff here later
   // if (WiFi.status() == WL_CONNECTED)  { 
   //       } 
	
  }
 

 // Serial.print("Heap size at end of loop ");
 // Serial.println(system_get_free_heap_size() );         // Free  memory check!

  
