////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
ESP8266WebServer server(80);
static  int     html_entry_error = false;
static  String  html_entry_error_msg = "";

const   char  *www_username = WWW_USERNAME;
const   char  *www_password = WWW_PASSWORD;
const   char  *www_realm = "Timer Power Switch Authentication";

String pg_version = String(PG_VERSION);

String  authFailResponse = "Authentication Failed";

String html_page_header = "<html><head><title>" + WiFi.hostname() + "</title></head><body>"
                          "<script>\n"
                          "var h,m,s,animate;\n"
                          "function init(){h=$HOURS$;m=$MINUTES$;s=$SECONDS$;clock();};\n"
                          "function clock() {s++;if(s==60) {s=0;m++;if(m==60){m=0;h++;if(h==24){h=0;}}}$('sec',s);$('min',m);$('hr',h);animate=setTimeout(clock,1000);};\n "
                          "function $(id,val){if(val<10){val='0'+val;} document.getElementById(id).innerHTML=val;};window.onload=init;\n "
                          "</script>\n"
                          "<p><center><h1>Timer Power Switch " + WiFi.hostname() + " (" + pg_version + ")</h1></center></p>\n"
                          "<p><center><h2><div class=\"location\"><span id=\"location\">$LOCATION$</span></div></h2></center></p>\n"
                          "<p><center><h2><a href=\"/\">Home</a> - <a href=\"/settings\">Settings</a></h2></center></p>\n"
                          "<p><center><h2><div class=\"clock\"><span>Time </span><span id=\"hr\">00</span><span> : </span><span id=\"min\">00</span><span> : </span><span id=\"sec\">00</span></div></h2></center></p>\n";

String html_page_footer = "<p>&nbsp;</p><p><center><h4>&copy; Mick Vaites 2018</h4></center></p>"
                          "</body></html>";
String ajax_homepage  = "<script>\n"
    "var y=setInterval(function() {loadData(\"/json/status\", parse_Json )}, 10000);\n"
    "function loadData(url, cFunction, value) {var xhttp;xhttp=new XMLHttpRequest();xhttp.onreadystatechange = function() {if (this.readyState == 4 && this.status == 200) {cFunction(this);}};xhttp.open(\"GET\", url, true);xhttp.send();}\n"
    "function parse_Json(xhttp) {var obj = JSON.parse(xhttp.responseText );\n"
    " if( obj.location ) {\n"
    "   document.getElementById(\"location\").innerHTML = \"Location: \" + obj.location;\n"
    " }\n"
    " document.getElementById(\"timer0_enabled\").innerHTML = obj.timer0_enabled;\n"
    " document.getElementById(\"timer0_on_enabled\").innerHTML = obj.timer0_on_enabled;\n"
    " document.getElementById(\"timer0_on_time\").innerHTML = obj.timer0_on_time;\n"    
    " document.getElementById(\"timer0_off_enabled\").innerHTML = obj.timer0_off_enabled;\n"
    " document.getElementById(\"timer0_off_time\").innerHTML = obj.timer0_off_time;\n"    
    " document.getElementById(\"timer1_enabled\").innerHTML = obj.timer1_enabled;\n"
    " document.getElementById(\"timer1_on_enabled\").innerHTML = obj.timer1_on_enabled;\n"
    " document.getElementById(\"timer1_on_time\").innerHTML = obj.timer1_on_time;\n"    
    " document.getElementById(\"timer1_off_enabled\").innerHTML = obj.timer1_off_enabled;\n"
    " document.getElementById(\"timer1_off_time\").innerHTML = obj.timer1_off_time;\n"
    " document.getElementById(\"poweron_relay_state_enabled\").innerHTML = obj.poweron_relay_state_enabled;\n"
    " document.getElementById(\"poweron_relay_state\").innerHTML = obj.poweron_relay_state;\n"
    " if ( obj.current_relay_state === 0 ) {\n"
    "  document.getElementById(\"relay_state_str1\").innerHTML = \"OFF\";\n"
    "  document.getElementById(\"relay_state_button\").innerHTML = \"\\x3Ca\\x20href\\x3D\\x22on\\x22\\x3E\\x3Cbutton\\x3ESwitch\\x20ON\\x3C/button\\x3E\\x3C/a\\x3E\";\n"
    " } else {\n"
    "  document.getElementById(\"relay_state_str1\").innerHTML = \"ON\";\n"
    "  document.getElementById(\"relay_state_button\").innerHTML = \"\\x3Ca\\x20href\\x3D\\x22off\\x22\\x3E\\x3Cbutton\\x3ESwitch\\x20OFF\\x3C/button\\x3E\\x3C/a\\x3E\";\n"
    " }}\n"
    "</script>\n";

/*
"  document.getElementById(\"relay_state_button\").innerHTML = \"<a href=\"off\"><button>Switch OFF</button></a>\";\n"
 * \x3C = <
 * \x20 = space
 * \x3D = =
 * \x22 = "
 * \x3E = >
 * 3
 */


void  showHomePage() {

  String  message = html_page_header;
  String  state_html;
  String  button_html;
  String  on_enabled_html;
  String  off_enabled_html;
  String  poweron_state_enabled_html;
  String  poweron_state_html;
  char    fmtbuf1[100];
  char    fmtbuf2[60];
  int     t;

  sprintf( fmtbuf1, "h=%d;m=%d;s=%d", hour(), minute(), second());
  message.replace( "h=$HOURS$;m=$MINUTES$;s=$SECONDS$", String(fmtbuf1) );
  if( conf.location[0] == '\0' ) {
    message.replace( "$LOCATION$", "" );    
  } else {
    sprintf( fmtbuf1, "Location: %s", conf.location );
    message.replace( "$LOCATION$", fmtbuf1 );        
  }

  if( html_entry_error == true ) {
    html_entry_error = false;
    message += "<p><center><h3><font color=\"red\">" + html_entry_error_msg + "</font></h3></center></p>";
  }

  if( http_requested_power_change == true ) {
    if( http_requested_power_state == RELAY_ON ) {
      state_html = "ON";
      button_html = "<a href=\"off\"><button>Switch OFF</button></a>";  
    } else {
      state_html = "OFF";
      button_html += "<a href=\"on\"><button>Switch ON</button></a>";      
    }
  } else if(digitalRead(gpio_12_relay)==HIGH) {
    state_html = "ON";
    button_html = "<a href=\"off\"><button>Switch OFF</button></a>";
  } else {
    state_html = "OFF";
    button_html += "<a href=\"on\"><button>Switch ON</button></a>";
  }
  message += "<center>"
             "<p><h3><div><span>Power switch is </span><span id=\"relay_state_str1\">" + state_html + "</span></div></h3></p>"
             "<p><div><span id=\"relay_state_button\">" + button_html + "</span></div></p>"
             "</center>";
  message += "<center><table>";
  for( t = 0; t < TOTAL_TIMERS; t ++ ) {
    message += "<tr><td colspan=\"3\" align=\"center\"><hr width=\"100%\"></td</tr>\n";
    
    sprintf( fmtbuf1, "%s", conf.timer[t].enabled ? "Enabled" : "Disabled" );
    message += "<tr><td colspan=\"2\" align=\"center\"><h3>Timer " + String( t ) + "</h3></td><td id=\"timer" + String(t) + "_enabled\">" + String( fmtbuf1 ) + "</b></td</tr>\n";

    sprintf( fmtbuf1, "%02d:%02d", conf.timer[t].on_hour, conf.timer[t].on_minute );
    sprintf( fmtbuf2, "%s", conf.timer[t].on_enabled ? "Enabled" : "Disabled" );
    
    message += "<tr><td align=\"right\">Power on time </td><td id=\"timer" + String(t) + "_on_time\">" + String( fmtbuf1 ) + "</td><td id=\"timer" + String(t) + "_on_enabled\">" + String( fmtbuf2 ) + "</td</tr>\n";

    sprintf( fmtbuf1, "%02d:%02d", conf.timer[t].off_hour, conf.timer[t].off_minute );
    sprintf( fmtbuf2, "%s", conf.timer[t].off_enabled ? "Enabled" : "Disabled" );
    
    message += "<tr><td align=\"right\">Power off time </td><td id=\"timer" + String(t) + "_off_time\">" + String( fmtbuf1 ) + "</td><td id=\"timer" + String(t) + "_off_enabled\">" + String( fmtbuf2 ) + "</td</tr>\n";

  }
  message += "<tr><td colspan=\"3\" align=\"center\"><hr width=\"100%\"></td</tr>\n";

  if( conf.poweron_relay_state_enabled ) {
    poweron_state_enabled_html = "Yes";
  } else {
    poweron_state_enabled_html = "No";
  }
  if( conf.poweron_relay_state ) {
    poweron_state_html = "On";
  } else {
    poweron_state_html = "Off";
  }
  message += "<tr><td colspan=\"3\" align=\"center\"><div class=\"override_power\"><span>Override current power on boot </span><span id=\"poweron_relay_state_enabled\">" + poweron_state_enabled_html + "</span>"
             "<span>, set state to </span><span id=\"poweron_relay_state\">" + poweron_state_html + "</span></div></td</tr>\n";
  message += "</table></center>\n";
  message += ajax_homepage + html_page_footer;
  server.send(200, "text/html", message );  

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void handleRoot() {

  if( !server.authenticate(www_username, www_password)) {
    return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
  }

  showHomePage();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void handleRelayOn() {
  
  if( !server.authenticate(www_username, www_password)) {
    return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
  }
  
  http_requested_power_state = RELAY_ON;
  http_requested_power_change = true;
  showHomePage();
  
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void handleRelayOff() {

  if( !server.authenticate(www_username, www_password)) {
    return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
  }
    
  http_requested_power_state = RELAY_OFF;
  http_requested_power_change = true;
  showHomePage();

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
#ifdef DEBUG
  Serial.println(message);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void handleEditSettings() {
  int   a, t;
  String message = html_page_header;
  String selected = "selected";
  String selected_0, selected_1;
  char  fmtbuf1[100];

  if( !server.authenticate(www_username, www_password)) {
    return server.requestAuthentication(BASIC_AUTH, www_realm, authFailResponse);
  }

  sprintf( fmtbuf1, "h=%d;m=%d;s=%d", hour(), minute(), second());
  message.replace( "h=$HOURS$;m=$MINUTES$;s=$SECONDS$", String(fmtbuf1) );

  if( conf.location[0] == '\0' ) {
    message.replace( "$LOCATION$", "" );    
  } else {
    sprintf( fmtbuf1, "Location: %s", conf.location );
    message.replace( "$LOCATION$", fmtbuf1 );        
  }


  if( html_entry_error == true ) {
    html_entry_error = false;
    message += "<p><center><h3><font color=\"red\">" + html_entry_error_msg + "</font></h3></center></p>\n";
  }

  message += "<form action=\"/save_settings\">\n"
             "<center>\n"
             "<table cellspacing=\"10\" cellpadding=\"2\">\n"
             "<tr><td colspan=\"3\" align=\"center\"><h2>Change Settings</h2></td></tr>\n";

  for( t = 0; t < TOTAL_TIMERS; t++ ) {
    sprintf( fmtbuf1, "%s", conf.timer[t].enabled ? "Enabled" : "Disabled" );
    message += "<tr><td colspan=\"3\" align=\"center\"><hr width=\"100%\"></td></tr>\n"
               "<tr><td colspan=\"2\" align=\"center\"><h3>Timer " + String( t ) + "</h3></td><td>Enabled <input type=\"checkbox\" name=\"timer" + String(t) + "_enabled\" value=\"enabled\" ";
    if( conf.timer[t].enabled == true) {
      message += "checked";
    }
    message += "></td></tr>\n"
               "<tr><td align=\"right\">Power on time :</td><td align=\"Left\"><select name=\"timer" + String(t) + "_on_hour\">";
    for(a = 0; a < 24; a++ ) {
      message += "<option value=\"" + String(a) + "\"";
      if( a == conf.timer[t].on_hour ) {
        message += " " + selected;
      }
      message += ">" + String(a) + "</option>";
    }
    message += "</select>:<select name=\"timer" + String(t) + "_on_minute\">";
    for(a = 0; a < 60; a += 5  ) {
      message += "<option value=\"" + String(a) + "\"";
      if( a == conf.timer[t].on_minute ) {
        message += " " + selected;
      }
      message += ">" + String(a) + "</option>";
    }  
    message += "</select></td><td>Enabled <input type=\"checkbox\" name=\"timer" + String(t) + "_on_enabled\" value=\"enabled\" ";
    if( conf.timer[t].on_enabled == true) {
      message += "checked";
    }
    message += "></td></tr>\n"
               "<tr><td align=\"right\">Power off time :</td><td align=\"Left\"><select name=\"timer" + String(t) + "_off_hour\">";
    for(a = 0; a < 24; a++ ) {
      message += "<option value=\"" + String(a) + "\"";
      if( a == conf.timer[t].off_hour ) {
        message += " " + selected;
      }
      message += ">" + String(a) + "</option>";
    }
    message += "</select>:<select name=\"timer" + String(t) + "_off_minute\">";
    for(a = 0; a < 60; a += 5  ) {
      message += "<option value=\"" + String(a) + "\"";
      if( a == conf.timer[t].off_minute ) {
        message += " " + selected;
      }
      message += ">" + String(a) + "</option>";
    }
    message += "</select></td><td>Enabled <input type=\"checkbox\" name=\"timer" + String(t) + "_off_enabled\" value=\"enabled\" ";
    if( conf.timer[t].off_enabled == true) {
      message += "checked";
    }
    message += "></td></tr>\n";
  }
  message += "<tr><td colspan=\"3\" align=\"center\"><hr width=\"100%\"></td></tr>\n";

  if( conf.poweron_relay_state == 0 ) {
    selected_0 = "Selected";
    selected_1 = "";
  } else {
    selected_0 = "";
    selected_1 = "Selected";
  }
  message += "<tr>"
                "<td colspan=\"2\" align=\"center\">Overide poweron status to <select name=\"poweron_relay_state\"><option value=\"0\" " + selected_0 + ">Off</option><option value=\"1\" " + selected_1 + ">On</option></select></td>"
                "<td>Enabled <input type=\"checkbox\" name=\"poweron_relay_state_enabled\" value=\"enabled\" ";
  if( conf.poweron_relay_state_enabled == true ) {
    message += "checked";
  }
  message += " ></td></tr>\n"
             "<tr><td>Location : </td><td colspan=\"2\"><input type=\"text\" name=\"location\" size=\"20\" value=\"" + String(conf.location) + "\"></td></tr>\n"
             "<tr><td colspan=\"3\" align=\"center\"><input type=\"submit\" value=\"Submit Change\"><input type=\"reset\" value=\"Reset\"></td></tr>\n"
             "</table>"
             "</form>" + html_page_footer;

  Serial.printf( "Length of html = %d\n", message.length() );
  server.send(200, "text/html", message);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void handleCommitSettings() {

  int   t;
  char  tmpbuf[30];
  int   req_timer_on_hour = -1;
  int   req_timer_on_minute = -1;
  int   req_timer_off_hour = -1;
  int   req_timer_off_minute = -1;
  int   req_on_hour_min = -1;
  int   req_off_hour_min = -1;
  bool  bad_hostname = false;
  int   valid_args;
  
  for( t = 0; t < TOTAL_TIMERS; t++ ) {
    valid_args = 0;
    sprintf( tmpbuf, "timer%d_on_hour", t ); 
    if( server.arg(tmpbuf)  != "" ) {
      req_timer_on_hour = server.arg(tmpbuf).toInt();
      req_on_hour_min = req_timer_on_hour * 60;
      valid_args ++;
    }
    sprintf( tmpbuf, "timer%d_on_minute", t ); 
    if( server.arg(tmpbuf)  != "" ) {
      req_timer_on_minute = server.arg(tmpbuf).toInt();
      req_on_hour_min += req_timer_on_minute;
      valid_args ++;
    }

    sprintf( tmpbuf, "timer%d_off_hour", t ); 
    if( server.arg(tmpbuf)  != "" ) {
      req_timer_off_hour = server.arg(tmpbuf).toInt();
      req_off_hour_min = req_timer_off_hour * 60;
      valid_args ++;
    }
    sprintf( tmpbuf, "timer%d_off_minute", t ); 
    if( server.arg(tmpbuf)  != "" ) {
      req_timer_off_minute = server.arg(tmpbuf).toInt();
      req_off_hour_min += req_timer_off_minute;
      valid_args ++;
    }
    
    // Accomodate on/off times that span midnight
    if( req_timer_on_hour == 0 && req_timer_off_hour == 23 ) {
      req_on_hour_min += (24 * 60);
    } else if ( req_timer_on_hour == 23 && req_timer_off_hour == 0 ) {
      req_off_hour_min += (24 * 60);      
    }
    
    Serial.printf("req_timer%d_on_hour = %d\n", t, req_timer_on_hour );
    Serial.printf("req_timer%d_on_minute = %d\n", t, req_timer_on_minute );

    Serial.printf("req_timer%d_off_hour = %d\n", t, req_timer_off_hour );
    Serial.printf("req_timer%d_off_minute = %d\n", t, req_timer_off_minute );

    if( valid_args < 4 ) {
      html_entry_error = true;
      html_entry_error_msg = "Missing entries for timer" + String(t);
      server.sendHeader("Location", String("/settings"), true);
      server.send ( 302, "text/plain", "");  
      return;
    } else if( abs(req_on_hour_min - req_off_hour_min) < 20 ) {
      html_entry_error = true;
      html_entry_error_msg = "On/Off times for timer" + String(t) + " needs to be at least 20 minutes apart";
      server.sendHeader("Location", String("/settings"), true);
      server.send ( 302, "text/plain", "");  
      return;
    }
    
    conf.timer[t].on_hour = req_timer_on_hour;
    conf.timer[t].on_minute = req_timer_on_minute;
    conf.timer[t].off_hour = req_timer_off_hour;
    conf.timer[t].off_minute = req_timer_off_minute;
    
    sprintf( tmpbuf, "timer%d_enabled", t ); 
    if( server.arg(tmpbuf)  == "enabled" ) {
      conf.timer[t].enabled = true;
    } else {
      conf.timer[t].enabled = false;    
    }

    sprintf( tmpbuf, "timer%d_on_enabled", t ); 
    if( server.arg(tmpbuf)  == "enabled" ) {
      conf.timer[t].on_enabled = true;
    } else {
      conf.timer[t].on_enabled = false;    
    }
    sprintf( tmpbuf, "timer%d_off_enabled", t ); 
    if( server.arg(tmpbuf)  == "enabled" ) {
      conf.timer[t].off_enabled = true;
    } else {
      conf.timer[t].off_enabled = false;    
    }
  }

  if( server.arg("location").length() > 20 ) {
      html_entry_error = true;
      html_entry_error_msg = "Hostname too long";
      server.sendHeader("Location", String("/settings"), true);
      server.send ( 302, "text/plain", "");  
      return;
  } else {
    bad_hostname = false;
    for( int a=0; a < server.arg("location").length() && bad_hostname == false; a++ ) {
      if(( not isalpha(server.arg("location")[a] )) && ( not isdigit(server.arg("location")[a])) && (server.arg("location")[a] != ' ')) {
        bad_hostname = true;
      }
    }
    if( bad_hostname == true ) {
      html_entry_error_msg = "Hostname contains invalid charaters";
      server.sendHeader("Location", String("/settings"), true);
      server.send ( 302, "text/plain", "");  
      return;
    } else {
      if( server.arg("location").length() == 0 ) {
        conf.location[0] = '\0';
      } else {
        strcpy( conf.location, server.arg("location").c_str());
      }
    }
  }
  if( server.arg("poweron_relay_state") == "0" ) {
    conf.poweron_relay_state = 0;
  } else if( server.arg("poweron_relay_state") == "1" ) {
    conf.poweron_relay_state = 1;    
  }

  if( server.arg("poweron_relay_state_enabled") == "enabled" ) {
    conf.poweron_relay_state_enabled = true;
  } else {
    conf.poweron_relay_state_enabled = false;    
  }

  saveConfiguration();

  server.sendHeader("Location", String("/"), true);
  server.send ( 302, "text/plain", "");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// returns a JSON with current status
void  ajaxJSON_ReturnStatus() {
  int     t;
  char    tmpbuf[20];

  String  json_msg = "{ \"updated\":\"" + String(conf.updated) + "\","
                     "  \"db_version\":\"" + String(conf.db_version) + "\",";
  for( t = 0; t < TOTAL_TIMERS; t++ ) {
    json_msg += "  \"timer" + String(t) + "_enabled\":";
    if( conf.timer[t].enabled) {
      json_msg += "\"Enabled\",";
    } else {
      json_msg += "\"Disabled\",";
    }
    sprintf( tmpbuf, "%02d:%02d", conf.timer[t].on_hour, conf.timer[t].on_minute );
    json_msg += "  \"timer" + String(t) + "_on_time\":\"" + String(tmpbuf) + "\","
                "  \"timer" + String(t) + "_on_enabled\":";
    if( conf.timer[t].on_enabled) {
      json_msg += "\"Enabled\",";
    } else {
      json_msg += "\"Disabled\",";
    }
    sprintf( tmpbuf, "%02d:%02d", conf.timer[t].off_hour, conf.timer[t].off_minute );
    json_msg += "  \"timer" + String(t) + "_off_time\":\"" + String(tmpbuf) + "\","
                "  \"timer" + String(t) + "_off_enabled\":";
    if( conf.timer[t].off_enabled) {
      json_msg += "\"Enabled\",";
    } else {
      json_msg += "\"Disabled\",";
    }    
  }
  json_msg += "  \"current_relay_state\":" + String(conf.current_relay_state) + ","
              "  \"poweron_relay_state_enabled\":";
  if( conf.poweron_relay_state_enabled ) {
    json_msg += "\"Yes\",";
  } else {
    json_msg += "\"No\",";
  }
  json_msg += "  \"poweron_relay_state\":";
  if( conf.poweron_relay_state ) {
    json_msg += "\"On\",";
  } else {
    json_msg += "\"Off\",";
  }
  json_msg += " \"location\":\"" + String(conf.location) + "\"";

  json_msg += " }";

  server.send(200, "text/html", json_msg );
#ifdef DEBUG
  Serial.println("ajaxJSON_ReturnMonitorStatus");
  Serial.println(json_msg);
#endif

}


////////////////////////////////////////////////////////////////////////////////////////////////////
void startWebServer() { // Start a HTTP server with a file read handler and an upload handler

  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  server.on("/", HTTP_GET, handleRoot );  
  server.on("/on", handleRelayOn );
  server.on("/off", handleRelayOff );
  server.on("/settings", handleEditSettings );
  server.on("/save_settings", handleCommitSettings );
  server.on("/json/status", ajaxJSON_ReturnStatus );
  server.begin();                             // start the HTTP server
#ifdef DEBUG
  Serial.println("HTTP server started.");
#endif
}
