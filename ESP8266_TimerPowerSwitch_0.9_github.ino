/*
  Simple Wifi Switch
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <Ticker.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#define   PG_VERSION                            0.9       // Program version
#define   DEBUG                                 1

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#define   TOTAL_TIMERS                          2

#define   DEFAULT_WIFI_SSID          "yourssid"
#define   DEFAULT_WIFI_PASSWD        "yourwifipasswd"

#define   WWW_USERNAME              "mywebusername"                   // Web UI Username
#define   WWW_PASSWORD              "mywebpassword"                   // Web UI Password


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "SharedLibraries.h"              // Must appear before other included classes
#include "NTPTimeClass.h"
#include "HTTPLibs.h"


NTPTime   Ntp1( 3600000, "pool.ntp.org" );


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

Ticker                UpdateTheBootLEDs;


////////////////////////////////////////////////////////////////////////////////////////////////////
// Callback Function to flash the Led during boot up (and perhaps when network is lost)
////////////////////////////////////////////////////////////////////////////////////////////////////

static  bool    led_state = HIGH;

void  updateTheBootLEDsCb() {

  if( led_state == HIGH ) {
    digitalWrite(gpio_13_led, LOW);       // LED on
    led_state = LOW;   
  } else {
    digitalWrite(gpio_13_led, HIGH);      // LED off
    led_state = HIGH;    
  }
}



////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(void){

  Serial.begin(115200);
  delay(1000);
  Serial.printf("Starting ESP8266 Timer Power Switch - v%5.2f\n", (float)PG_VERSION );
  delay(1000);

  UpdateTheBootLEDs.attach_ms( 200, updateTheBootLEDsCb );

  loadConfiguration();
#ifdef DEBUG
  Serial.println("Loaded configuration");
#endif
  delay(500);
  
  setupGPIO(); 
  startWifi();
  Ntp1.update();
  startWebServer();

  UpdateTheBootLEDs.detach();
  digitalWrite(gpio_13_led, HIGH);      // LED off

  if( conf.poweron_relay_state_enabled ) {
    if( conf.poweron_relay_state == RELAY_ON ) {
      Serial.println("PowerOnState Enabled and turning power on");
      turnPowerOn();
    } else {
      Serial.println("PowerOnState Enabled and turning power off");
      turnPowerOff();
    }
  } else if( conf.current_relay_state == RELAY_ON ) {
    Serial.println("Current relay state on - turning power on");
    turnPowerOn();
  } else {
    Serial.println("Current relay state off - turning power off");
    turnPowerOff();
  }

  Serial.println("Startup complete");
  Serial.println();


}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
int  now_minute;
int  now_hour;
int  gpio0_button_pressed = false;

int gpio_0_button_state = HIGH;       // the current reading from the input pin
int last_gpio_0_button_state = HIGH;   // the previous reading from the input pin

long last_gpio_0_debounce_time = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output     flickers

void loop(void){

  int     reading = 0;
  int     t;

  if( WiFi.status() != WL_CONNECTED ) {
    Serial.println("Lost WiFi Connection");
    delay(5000);
    startWifi();    
  } else {
    Ntp1.update();

    if( Ntp1.NTP_time_syncronised == true ) {
      now_minute = minute();
      now_hour = hour();
      
      for( t = 0; t < 3; t++ ) {
        if( conf.timer[t].enabled == true ) {
          if( conf.timer[t].on_enabled ) {
            if ((now_hour == conf.timer[t].on_hour) &&
                (now_minute == conf.timer[t].on_minute) && 
                (conf.current_relay_state == RELAY_OFF )) {
              turnPowerOn();
            }
          }
          if( conf.timer[t].off_enabled ) {
            if ((now_hour == conf.timer[t].off_hour) && 
                (now_minute == conf.timer[t].off_minute) &&
                (conf.current_relay_state == RELAY_ON )) {
              turnPowerOff();
            }
          }
        }
      }
    }
    if( http_requested_power_change == true ) {  // Request to change made by web
      http_requested_power_change = false;
      if( http_requested_power_state == RELAY_OFF ) {
          turnPowerOff();        
      } else {
          turnPowerOn();        
      }
    }

    reading = digitalRead(gpio_0_button);
    if( reading != last_gpio_0_button_state ) {
      Serial.printf("reading (%s) != last_gpio_0_button_state (%s)\n", reading == LOW ? "LOW" : "HIGH", last_gpio_0_button_state == LOW ? "LOW" : "HIGH" );
      last_gpio_0_debounce_time = millis();
    
    }
    if(( millis() - last_gpio_0_debounce_time ) > debounceDelay ) {
      if( reading != gpio_0_button_state ) {
        Serial.printf("reading (%s) != gpio_0_button_state (%s)\n", reading == LOW ? "LOW" : "HIGH", gpio_0_button_state == LOW ? "LOW" : "HIGH" );
        gpio_0_button_state = reading;      
        if( gpio_0_button_state == HIGH ) {
          Serial.println("gpio_0_button_state = HIGH");
          togglePower();
        }
      }    
    }
    last_gpio_0_button_state = reading;
    server.handleClient();                // Service webserver requests
  }
} 
