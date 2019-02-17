////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////
#define   DB_VERSION                            8         // Database version

#define   DEFAULT_TIMER_ON_ENABLED              true
#define   DEFAULT_TIMER_ON_HOUR                 9
#define   DEFAULT_TIMER_ON_MINUTE               0

#define   DEFAULT_TIMER_OFF_ENABLED             true
#define   DEFAULT_TIMER_OFF_HOUR                17
#define   DEFAULT_TIMER_OFF_MINUTE              0

#define   RELAY_OFF                             LOW
#define   RELAY_ON                              HIGH

#define   DEFAULT_POWERON_RELAY_STATE_ENABLED   true
#define   DEFAULT_POWERON_RELAY_STATE           RELAY_OFF

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _timers {

  int     enabled;                // Is this timer enabled

  int     on_enabled;
  int     on_hour;  
  int     on_minute;              // Relay will turn power on at 09:00

  int     off_enabled;
  int     off_hour;
  int     off_minute;             // Relay will turn power off at 16:00
    
}TIMERS;

static struct {
  int     updated;
  int     db_version;

  char    wifi_ssid[20];
  char    wifi_passwd[20];

  char    location[100];

  TIMERS  timer[TOTAL_TIMERS];

  bool    current_relay_state;          // RELAY_ON or RELAY_OFF
  bool    poweron_relay_state_enabled;  // does poweron relay state override current relay state ?
  bool    poweron_relay_state;          // RELAY_ON or RELAY_OFF

}conf;

////////////////////////////////////////////////////////////////////////////////////////////////////
void  printConfiguration() {

  Serial.printf("updated                     = %d\n", conf.updated );
  Serial.printf("db_version                  = %d\n", conf.db_version );
  Serial.printf("wifi_ssid                   = %s\n", conf.wifi_ssid );
  Serial.printf("location .                  = %s\n", conf.location );
  
  Serial.printf("timer0_enabled               = %d\n", conf.timer[0].enabled );
  Serial.printf("timer0_on_enabled            = %d\n", conf.timer[0].on_enabled );
  Serial.printf("timer0_on_hour               = %d\n", conf.timer[0].on_hour );
  Serial.printf("timer0_on_minute             = %d\n", conf.timer[0].on_minute );
  Serial.printf("timer0_off_enabled           = %d\n", conf.timer[0].off_enabled );
  Serial.printf("timer0_off_hour              = %d\n", conf.timer[0].off_hour );
  Serial.printf("timer0_off_minute            = %d\n", conf.timer[0].off_minute );
  
  Serial.printf("timer1_enabled               = %d\n", conf.timer[1].enabled );
  Serial.printf("timer1_on_enabled            = %d\n", conf.timer[1].on_enabled );
  Serial.printf("timer1_on_hour               = %d\n", conf.timer[1].on_hour );
  Serial.printf("timer1_on_minute             = %d\n", conf.timer[1].on_minute );
  Serial.printf("timer1_off_enabled           = %d\n", conf.timer[1].off_enabled );
  Serial.printf("timer1_off_hour              = %d\n", conf.timer[1].off_hour );
  Serial.printf("timer1_off_minute            = %d\n", conf.timer[1].off_minute );
  
  Serial.printf("current_relay_state         = %d\n", conf.current_relay_state );
  Serial.printf("poweron_relay_state_enabled = %d\n", conf.poweron_relay_state_enabled );
  Serial.printf("poweron_relay_state         = %d\n", conf.poweron_relay_state );

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  saveConfiguration() {

  Serial.printf("Saving configuration\n");

  conf.updated = now();
  printConfiguration();

  EEPROM.begin(512);
  for( unsigned int t=0; t < sizeof(conf); t++ ) {
    EEPROM.write(t, *((char *)&conf +t));
  }
  EEPROM.end();
  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  loadConfiguration() {

  int   t;

  Serial.printf("Loading configuration\n");

  EEPROM.begin(512);
  for( unsigned int t=0; t < sizeof(conf); t++ ) {
    *((char *)&conf + t) = EEPROM.read(t);
  }
  EEPROM.end();

  if(( conf.updated == 0) || (conf.db_version != DB_VERSION )) {
    Serial.printf("EEPROM Configuration Not found setting defaults\n");
    
    conf.db_version = DB_VERSION;
    strcpy(conf.wifi_ssid, DEFAULT_WIFI_SSID );
    strcpy(conf.wifi_passwd, DEFAULT_WIFI_PASSWD );
    conf.location[0] = '\0';

    for( t = 0; t < TOTAL_TIMERS; t++ ) {
      conf.timer[t].enabled = false;
      conf.timer[t].on_enabled = DEFAULT_TIMER_ON_ENABLED;
      conf.timer[t].on_hour = DEFAULT_TIMER_ON_HOUR;
      conf.timer[t].on_minute = DEFAULT_TIMER_ON_MINUTE;
      conf.timer[t].off_enabled = DEFAULT_TIMER_OFF_ENABLED;
      conf.timer[t].off_hour = DEFAULT_TIMER_OFF_HOUR;
      conf.timer[t].off_minute = DEFAULT_TIMER_OFF_MINUTE;
    }

    conf.current_relay_state = DEFAULT_POWERON_RELAY_STATE;
    conf.poweron_relay_state_enabled = DEFAULT_POWERON_RELAY_STATE_ENABLED;
    conf.poweron_relay_state = DEFAULT_POWERON_RELAY_STATE;

    saveConfiguration();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
const int gpio_13_led = 13;                             // LED GPIO - low = on, high = off
const int gpio_12_relay = 12;                           // Relay GPIO - low = off, high = on
const int gpio_0_button = 0;                           // Button GPIO
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
static bool http_requested_power_change = false;  // Request to change made by web
static bool http_requested_power_state = RELAY_OFF;

////////////////////////////////////////////////////////////////////////////////////////////////////
void  turnPowerOn() {

  if(digitalRead(gpio_12_relay)==LOW) {
    digitalWrite(gpio_13_led, LOW);
    digitalWrite(gpio_12_relay, HIGH);
    conf.current_relay_state = RELAY_ON;
    saveConfiguration();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  turnPowerOff() {

  if(digitalRead(gpio_12_relay)==HIGH) {
    digitalWrite(gpio_13_led, HIGH);
    digitalWrite(gpio_12_relay, LOW);
    conf.current_relay_state = RELAY_OFF;
    saveConfiguration();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  togglePower() {
  
  if(digitalRead(gpio_12_relay)==HIGH) {
    digitalWrite(gpio_13_led, HIGH);
    digitalWrite(gpio_12_relay, LOW);
    conf.current_relay_state = RELAY_OFF;
  } else {
    digitalWrite(gpio_13_led, LOW);
    digitalWrite(gpio_12_relay, HIGH);
    conf.current_relay_state = RELAY_ON;
  }
  saveConfiguration();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void  setupGPIO() {

  pinMode(gpio_13_led, OUTPUT);           // enable led port
  digitalWrite(gpio_13_led, HIGH);        // turn led off
  
  pinMode(gpio_12_relay, OUTPUT);         // enable relay port
  digitalWrite(gpio_12_relay, LOW);       // turn relay off

  pinMode(gpio_0_button, INPUT);          // enable button
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// WIFI
////////////////////////////////////////////////////////////////////////////////////////////////////
void  startWifi() {

  int   count = 0;

  WiFi.persistent( false );
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  WiFi.begin(conf.wifi_ssid, conf.wifi_passwd);

  Serial.println("Connecting to wifi");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    count ++;
    if( count > 10 ) {
      ESP.reset();
    }
  }
  delay(1000);
  Serial.println( "" );
  Serial.printf("Connected to %s\n", WiFi.SSID().c_str() );
  Serial.print( "  (" );
  Serial.print( WiFi.localIP() );
  Serial.println( ")" );
}
