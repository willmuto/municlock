#include <WiFi.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

/*

  Setting the 14-segment array
  -------------------------------------

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

   Stop id can be found with the following HTTP request:
   http://webservices.nextbus.com/service/publicXMLFeed?command=routeConfig&a=sf-muni&r=<ROUTE#>

   Once you have a stop id, you can query the prediction with the following:
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
//const char* SSID = "NETWORKNAME";
//const char* PASSWORD =  "PASSWORD";

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

int NUM_ROUTES = 6;
String routesAndStops[6][3] = {
  {"37", "4168", INBOUND}, 
  {"37", "4167", OUTBOUND},
  {"48", "4927", INBOUND}, 
  {"48", "4926", OUTBOUND}, 
  {"33", "4079", INBOUND},
  {"33", "4080", OUTBOUND}
};

void setup() {
  Serial.begin(SERIAL_BAUD);
  
  alpha4.begin(0x70);
  alpha4.clear();
  alpha4.writeDisplay();
 
  WiFi.begin(SSID, PASSWORD);
  String mac = WiFi.macAddress();
  if (DEBUG) Serial.println(mac);

  int waitingIndex = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(WIFI_RECONNECT_TIME);
    
    if (waitingIndex == 4)
    {
      waitingIndex = 0;
    }
    else 
    {
      waitingIndex++;
    }
    loading(waitingIndex);
    
  }

  debug("Connected to the WiFi network " + String(SSID)); 
}

void loading(int index) {
  uint16_t loadingChars[5][4] = {
    { 0x0000, 0x0000, 0x0000, 0x0000 },
    { 0x0900, 0x0000, 0x0000, 0x0000 }, 
    { 0x0900, 0x0900, 0x0000, 0x0000 }, 
    { 0x0900, 0x0900, 0x0900, 0x0000 }, 
    { 0x0900, 0x0900, 0x0900, 0x0900 }
  };

  alpha4.clear();

  for (int i = 0; i < 4; i++) {
    alpha4.writeDigitRaw(i, loadingChars[index][i]);
  }
                       
  alpha4.writeDisplay();
}

void debug(String msg) {
  if (DEBUG) Serial.println(msg);
}

void displayWrite(String msg, bool dot) {
  alpha4.clear();
  alpha4.writeDigitAscii(0, msg[0]);
  alpha4.writeDigitAscii(1, msg[1], dot);
  alpha4.writeDigitAscii(2, msg[2]);
  alpha4.writeDigitAscii(3, msg[3]);
  alpha4.writeDisplay();
}

void loop() {

    WiFiClient client;

    // Loop through routes
    for (int i = 0; i < NUM_ROUTES; i++) {

      debug("Connecting to " + String(HTTP_URL)); 
      
      if (!client.connect(HTTP_URL, HTTP_PORT)) {
        debug("Error connecting to " + String(HTTP_URL));
        
        delay(NEXTBUS_RECONNECT_TIME);
        return;
      }
      
      debug("Connected to " + String(HTTP_URL));

      String routeIndex = routesAndStops[i][0];
      String stopIndex = routesAndStops[i][1];
      String routeDir = routesAndStops[i][2];

      bool dot;
      if (routeDir == INBOUND) dot = false;
      if (routeDir == OUTBOUND) dot = true;

      String query = String("/service/publicXMLFeed?command=predictions&a=sf-muni&r=" + 
                            routeIndex + 
                            "&s=" + 
                            stopIndex);
                            
      String request = String("GET " + query + " HTTP/1.0");
                              
      debug("Making request");
      debug(request);
      debug(String(HTTP_URL) + query);
      
      client.println(request);
      client.println("Host: " + String(HTTP_URL));
      client.println("User-Agent: ESP32");
      client.println();

      String s;
      char c;
      if(client.find("<prediction ")){
        
        debug("Found prediction");
        s = client.readStringUntil('>');
  
        int minutesIndex = s.indexOf("minutes=");
        int startQuoteIndex = s.indexOf('"', minutesIndex) + 1;
        int endQuoteIndex = s.indexOf('"', startQuoteIndex);
        String minutes = s.substring(startQuoteIndex, endQuoteIndex);
        String output;

        if (minutes.length() == 1) {
          output = String(routeIndex + "0" + minutes);
        }

        else if (minutes.length() > 2) {
          output = String(routeIndex + "--" );
        }

        else 
        {
          output = String(routeIndex + minutes);
        }
        
        debug(output);
        displayWrite(output, dot);
        
      } else {
        debug("Didn't find prediction");
        String output = String(routeIndex + "--");
        
        debug(output);
        displayWrite(output, dot);
      }  
      
      delay(DELAY_TIME);
    }    
}
