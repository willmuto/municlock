#include <WiFi.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

// Contains SSID and PASSWORD for wifi network,
// both const char* variables.
#include "mywifi.h"

/*

  Setting the 14-segment array
  -------------------------------------
   https://learn.adafruit.com/14-segment-alpha-numeric-led-featherwing/overview

              A
          ----------
        F| \H |J /K |B
         |  \ | /   |
          --G1  --G2
         |  / | \   |
        E| /L |M \N |C
          ---------- . 
               D     DP

    DP    N     M     L    K    J    H    G2   G1   F   E   D  C  B  A
    4000  2000  1000  800  400  200  100  80   40   20  10  8  4  2  1


  Getting the route data for Muni
  -------------------------------------
   https://gist.github.com/grantland/7cf4097dd9cdf0dfed14#file-readme-md

   Stop number can be found with the following HTTP request:
   http://webservices.nextbus.com/service/publicXMLFeed?command=routeConfig&a=sf-muni&r=<ROUTE#>

   Once you have a stop number, you can query the prediction with the following:
   http://webservices.nextbus.com/service/publicXMLFeed?command=predictions&a=sf-muni&r=<ROUTE#>&s=<STOP#>
  
 */

bool DEBUG = 1;
const String INBOUND = "in";
const String OUTBOUND = "out";

unsigned int SERIAL_BAUD = 115200;
unsigned int DELAY_TIME = 10000;              // in milliseconds
unsigned int NEXTBUS_RECONNECT_TIME = 30000;  // in milliseconds
unsigned int WIFI_RECONNECT_TIME = 100;       // in milliseconds

const int HTTP_PORT = 80;
const char* HTTP_URL = "webservices.nextbus.com";

int NUM_ROUTES = 6;

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
WiFiClient client;

// Multi-dimensional array for loading animation.
// Each row is a frame in the animation, each column
// maps to a digit on the display.
unsigned int NUM_LOADING_FRAMES = 5;
uint16_t loadingAnim[5][4] = {
  { 0x0000, 0x0000, 0x0000, 0x0000 },  // "    "
  { 0x0900, 0x0000, 0x0000, 0x0000 },  // ">   "
  { 0x0900, 0x0900, 0x0000, 0x0000 },  // ">>  "
  { 0x0900, 0x0900, 0x0900, 0x0000 },  // ">>> "
  { 0x0900, 0x0900, 0x0900, 0x0900 }   // ">>>>"
};

// Multi-dimensional array which is ordered:
//  Route#, Stop#, Direction
String routesAndStops[6][3] = {
  {"37", "4168", INBOUND}, 
  {"37", "4167", OUTBOUND},
  {"48", "4927", INBOUND}, 
  {"48", "4926", OUTBOUND}, 
  {"33", "4079", INBOUND},
  {"33", "4080", OUTBOUND}
};

// Counter used for loading animation on display
int waitingIndex;

void setup() 
{
  // First time initialization of display 
  // and wifi client.
  
  Serial.begin(SERIAL_BAUD);
  
  alpha4.begin(0x70);
  alpha4.clear();
  alpha4.writeDisplay();
 
  WiFi.begin(SSID, PASSWORD);
  String mac = WiFi.macAddress();
  if (DEBUG) Serial.println(mac);

  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFI_RECONNECT_TIME);
    updateLoadingAnimation();
  }

  debug("Connected to the WiFi network " + String(SSID)); 
}

void updateLoadingAnimation() 
{
  // Increment the animation counter and write the
  // characters for the that frame to the display.
  
  if (waitingIndex < NUM_LOADING_FRAMES - 1)
  {
    waitingIndex++;
  }
  else 
  {
    waitingIndex = 0;
  }

  alpha4.clear();

  for (int i = 0; i < 4; i++) 
  {
    alpha4.writeDigitRaw(i, loadingAnim[waitingIndex][i]);
  }
                       
  alpha4.writeDisplay();
}

void debug(String msg) 
{
  // Print debug output.
  
  if (DEBUG) Serial.println(msg);
}

void displayWrite(String msg, bool dot) 
{
  // Send a message to the display. Characters after
  // the fourth will be discarded. Optionally light
  // up the dot on the second character; used for
  // specifying bus route direction (inbound vs outbound).
  
  alpha4.clear();
  alpha4.writeDigitAscii(0, msg[0]);
  alpha4.writeDigitAscii(1, msg[1], dot);
  alpha4.writeDigitAscii(2, msg[2]);
  alpha4.writeDigitAscii(3, msg[3]);
  alpha4.writeDisplay();
}

String createRequest(String routeNum, String stopNum) 
{
  // Create the query string that is sent to the HTTP client
  // in the form of "GET /path/to/endpoint HTTP/1.0"
  
  String query = String("/service/publicXMLFeed?command=predictions&a=sf-muni&r=" + 
                            routeNum + 
                            "&s=" + 
                            stopNum);

  debug("Created request for URL: " + String(HTTP_URL) + query);
  
  return String("GET " + query + " HTTP/1.0");
}

void sendRequest(String routeNum, String stopNum) 
{
  // Send the HTTP request
  
  String request = createRequest(routeNum, stopNum);

  debug("Making request");
  debug(request);
  
  client.println(request);
  client.println("Host: " + String(HTTP_URL));
  client.println("User-Agent: ESP32");
  client.println();
}

String getPrediction() {
  // After a request is sent, try to find a prediction
  // in the response. If it exists, the estimate will be
  // in an XML tag which looks like this:
  //    <prediction epochTime="1551081565747" seconds="1673" minutes="27" />
  
  String s;
  
  if(client.find("<prediction "))
  {
    
    debug("Found prediction");
    s = client.readStringUntil('>');

    int minutesIndex = s.indexOf("minutes=");
    int startQuoteIndex = s.indexOf('"', minutesIndex) + 1;
    int endQuoteIndex = s.indexOf('"', startQuoteIndex);
    String minutes = s.substring(startQuoteIndex, endQuoteIndex);
    
    if (minutes.length() == 1) {
      // Pad with zeros if under 10
      return String("0" + minutes);
    }
    else if (minutes.length() > 2) {
      // if over 99, don't bother
      return String("--");
    }
    else 
    {
      return minutes;
    }
    
  } 
  else 
  {
    debug("Couldn't find prediction");
    return String("--");
  }  
}

void displayPrediction(String routeNum, String prediction, bool dot)
{
  // Display the route number and prediction on the display. 
  
  String output = String(routeNum + prediction);
  debug(output);
  displayWrite(output, dot);
}

void loop() 
{
  // Main execution loop

  // Loop through routes
  for (int i = 0; i < NUM_ROUTES; i++) 
  {
    debug("Connecting to " + String(HTTP_URL)); 
    
    if (!client.connect(HTTP_URL, HTTP_PORT)) 
    {
      debug("Error connecting to " + String(HTTP_URL));
      
      delay(NEXTBUS_RECONNECT_TIME);
      return;
    }
    
    debug("Connected to " + String(HTTP_URL));

    String routeIndex = routesAndStops[i][0];
    String stopIndex = routesAndStops[i][1];
    String routeDir = routesAndStops[i][2];

    sendRequest(routeIndex, stopIndex);
    String prediction = getPrediction();
    displayPrediction(routeIndex, prediction, routeDir == OUTBOUND);
    
    delay(DELAY_TIME);
  }    
}
