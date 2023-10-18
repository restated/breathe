#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>
#include <SunRise.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Neopixels! Change to match your own use case.
#define LED_PIN    14  // Digital IO pin connected to the NeoPixels.

#define LED_COUNT 24  // Number of NeoPixels

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//Global variables, defined and redefined in later functions.

int aqiValue; // function getWeather
uint32_t col = strip.Color(0, 255, 0);
bool Sun = false;

String jsonBuffer;

//Creating a state machine. We can pass smoothly from one state to another based on our variables, if statements, and a switch case in function rgbBreathe.
enum
{
  INIT = 0,
  FADEIN,
  HOLD,
  FADEOUT,
  NIGHT
};

//Constants for function rgbBreathe.

const uint8_t MinBrightness = 20;       //value 0-255
const uint8_t MaxBrightness = 80;      //value 0-255

const uint32_t fadeInWait = 50;          //lighting up speed, steps.
const uint32_t fadeOutWait = 50;         //dimming speed, steps.

//SECRET INFORMATION

// Your WiFi credentials
const char* ssid     = SSID;
const char* password = PASSWORD;

// Your AirVisual location and API key
String CITY = "CITY";
String STATE = "PROVINCE";
String COUNTRY = "COUNTRY";
String AirVisualKey = KEY;


// SunRise library
double latitude = LAT;
double longitude = LON;




// Setup will run once, on initialization.

void setup() {
  Serial.begin(115200);   // Talk to us!

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  timeClient.begin();


  
  checkWeather();
  checkTime();
}

// Run forever, on a loop
void loop()
{
  static uint32_t
  tAPI = 0ul;
  static uint32_t
  tTime = 0ul;

  uint32_t
  tNow = millis();

  if ( tNow - tAPI >= 3600000ul ) //every hour
    {
      tAPI = tNow;
      checkWeather();
    }
    
  if ( tNow - tTime >= 900000ul ) // every 15 minutes
  {
      tTime = tNow;
      checkTime();
  }

  rgbBreathe();

}

void checkWeather() {
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {
    String serverPath = "http://api.airvisual.com/v2/city?city=" + CITY + "&state=" + STATE + "&country=" + COUNTRY + "&key=" + AirVisualKey;

    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer);
    JSONVar myObject = JSON.parse(jsonBuffer);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    if (JSON.typeof(myObject) == "undefined") {
      Serial.println("Parsing input failed!");
      return;
    }

    int aqiValue = myObject["data"]["current"]["pollution"]["aqius"];

    Serial.print("AQI: ");
    Serial.println(myObject["data"]["current"]["pollution"]["aqius"]);
   

     if (aqiValue <= 50) {
      col = strip.Color(0, 255, 128); //teal
      }
    else if (aqiValue >= 51 && aqiValue < 101) {
      col = strip.Color(179, 89, 0); //cozy yellow
      }
    else if (aqiValue >= 101 && aqiValue < 151) {
      col = strip.Color(255, 85, 0); //orange
      }
    else if (aqiValue >= 151 && aqiValue <201) {
      col = strip.Color(255, 0, 0); //red
      }
    else if (aqiValue >= 201 && aqiValue < 301) {
      col = strip.Color(255, 0, 128); //pink
      }
    else if (aqiValue >= 301) {
      col = strip.Color(148, 0, 211); //purple
      }
    else {
        col = strip.Color(0, 255, 0); //default green
    }

    }

  else
  {
    Serial.println("WiFi Disconnected");
  }
}

void checkTime() {
    timeClient.update();
    unsigned long t = timeClient.getEpochTime();
    Serial.println(t);
    
  
     // Calling the SunRise library and asking it to calculate whether the sun is visible at this date/time.
    SunRise sr;
    sr.calculate(latitude, longitude, t);
    if (sr.isVisible)
    {
      Sun = true;
      Serial.println("The Sun is visible!");
    }
    else {
      Sun = false;
      Serial.println("The Sun is not visible.");
    }
  }


// STATE MACHINE!!!!!!!!!!!!!!!
void rgbBreathe( void )
{
  static uint16_t
  i;
  static uint8_t
  b,
  loops,
  dly,
  state = INIT;
  static uint32_t
  //col,
  tFade;
  uint32_t
  tNow = millis();

  switch ( state )
  {
    case    INIT:
      b = MinBrightness;
      dly = 20;
      //col = strip.Color(255, 0, 0);
      tFade = tNow;

      if (Sun)
      {
        state = FADEIN;
      }
      else
      {
        state = NIGHT;
      }

      break;

    case    FADEIN:
      if ( tNow - tFade >= fadeInWait )
      {
        tFade = tNow;
        strip.setBrightness(b * 255 / 255);
        for ( i = 0; i < strip.numPixels(); i++ )
          strip.setPixelColor(i, col);
        strip.show();

        b++;
        if ( b == MaxBrightness )
        {
          i = 0;
          strip.setBrightness(MaxBrightness * 255 / 255);
          state = HOLD;

        }//if

      }//if

      break;

    case    HOLD:
      if ( tNow - tFade >= dly )
      {
        tFade = tNow;
        strip.setPixelColor(i, col);
        strip.show();

        i++;
        if ( i == strip.numPixels() )
        {
          b = MaxBrightness;
          state = FADEOUT;

        }//if

      }//if

      break;

    case    FADEOUT:
      if ( tNow - tFade >= fadeOutWait )
      {
        tFade = tNow;
        strip.setBrightness(b * 255 / 255);
        for (i = 0; i < strip.numPixels(); i++)
        {
          strip.setPixelColor(i, col);
        }
        strip.show();

        b--;
        if ( b == MinBrightness )
        {
          if (Sun) {
            state = FADEIN;
          }
          else
          {
            state = NIGHT;
          }

        }//if

      }//if

      break;

    case    NIGHT:
      strip.setBrightness(3);
      strip.fill(col);
      strip.show();
      if (Sun)
      {
        state = INIT;
      }
      else
      {
        state = NIGHT;
      }
      break;

  }//switch

}//rgbBreathe


String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your IP address with path or Domain name with URL path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
} //String
