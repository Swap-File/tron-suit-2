/* ESP8266 RetroBrowser for Teensy3.1 (direct connection , only separate 3,3V for ESP)
originated from http://hackaday.io/project/3072/instructions
	
*/
	
#define SSID        "Black Mesa B&G" // WLAN SSID
#define PASS        "gordonfreeman" // WLAN PW

#define DEST_HOST   "retro.hackaday.com"
#define DEST_IP     "192.254.235.21"

#define TIMEOUT     10000 // ms connection to AP maybe slow
#define  CONTINUE    false
#define  HALT        true


#define CPU_RESTART_ADDR (uint32_t *)0xE000ED0C
#define CPU_RESTART_VAL 0x5FA0004
#define CPU_RESTART (*CPU_RESTART_ADDR = CPU_RESTART_VAL); //may restart Teensy with macro CPU_RESTART but loose USB connection

#define ECHO_COMMANDS // Un-comment to echo AT+ commands to serial monitor

// Print error message and loop stop.
void errorHalt(String msg)
{
  Serial.println(msg);
  Serial.println("*HALT*");
  //CPU_RESTART //xxx
  while(true){};
}

// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean echoFind(String keyword)
{
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  
  // Fail if the target string has not been sent by deadline.
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline)
  {
    if (Serial1.available())
    {
      char ch = Serial1.read();
      Serial.write(ch);
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println();
          return true;
        }
    }
  }
  return false;  // Timed out
}

// Read and echo all available module output.
// (Used when we're indifferent to "OK" vs. "no change" responses or to get around firmware bugs.)
void echoFlush()
  {while(Serial1.available()) Serial.write(Serial1.read());}

  
// Echo module output until 3 newlines encountered.
// (Used when we're indifferent to "OK" vs. "no change" responses.)
void echoSkip()
{
  echoFind("\n");        // Search for nl at end of command echo
  echoFind("\n");        // Search for 2nd nl at end of response.
  echoFind("\n");        // Search for 3rd nl at end of blank line.
}

// Send a command to the module and wait for acknowledgement string
// (or flush module output if no ack specified).
// Echoes all data received to the serial monitor.
boolean echoCommand(String cmd, String ack, boolean halt_on_fail)
{
  Serial1.println(cmd);
  #ifdef ECHO_COMMANDS
    Serial.print("-->"); Serial.println(cmd);
  #endif
  
  // If no ack response specified, skip all available module output.
  if (ack == "")
    echoSkip();
  else
    // Otherwise wait for ack.
    if (!echoFind(ack))          // timed out waiting for ack string 
      if (halt_on_fail)			 // CONTINUE=false - skip critical halt
        errorHalt(cmd+" *failed");// Critical failure halt. ( ack not empty and not found and Halt true ) 
      else
        return false;            // Let the caller handle it.
  Serial.println("*==================*"); // request and answer OK
  return true;                   // ack blank or ack found
}

// Connect to the specified wireless network.
boolean connectWiFi()
{
  String cmd = "AT+CWJAP=\""; cmd += SSID; cmd += "\",\""; cmd += PASS; cmd += "\"";
			 //AT+CWJAP="SSID","12345678"
    if (echoCommand(cmd, "OK", CONTINUE)) // Join Access Point
  {
    Serial.println("*connected to WiFi*");
	
    return true;
  }
  else
  {
    Serial.println("*Connection to WiFi failed.");
    return false;
  }
}

// ******** SETUP ********
void setup()  
{
  Serial.begin(115200);         // Arduino serial Monitor via USB Teensy3.1
  Serial1.begin(115200);        // Communication with ESP8266 
  
  Serial1.setTimeout(TIMEOUT);
  
  delay(5000); // wait for serial init
  // echoCommand("AT+CIOBAUD=115200", "BAUD->115200", CONTINUE); // or 9600 if you like it - slow need hard reset
  while(!Serial); // wait for open USBser
  Serial.println("*ESP8266 Demo*\n\r*0018000922 !*");
  Serial.println("*on Teensy 3.1*");
  echoCommand("AT+CHELLO", "OK", CONTINUE);
  echoCommand("AT+RST", "[System Ready, Vendor:www.ai-thinker.com]", HALT);    // Reset & test if the module is ready  
  Serial.println("*Module is ready.");
  
  echoCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
  // Get connection status 
  echoCommand("AT+CIPSTATUS", "OK", CONTINUE); // Status:TCP/IP
  echoCommand("AT+CWMODE=1", "", HALT);    // Station mode - maybe " no change " only
  echoCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
  echoCommand("AT+CWLAP", "OK", CONTINUE); // List available access points
  
  
  echoCommand("AT+CIPMUX=1", "OK", HALT);    // Allow multiple connections (we'll only use the first).
    echoCommand("AT+CIPMODE?", "OK", HALT);  // default=0 
  //connect to the wifi
  boolean connection_established = false;
  for(int i=0;i<5;i++)
  {
			
	if(connectWiFi())
    {
      
	  connection_established = true;
	  echoCommand("AT+CIFSR", "OK", HALT);         // Echo IP address. also 0.0.0.0 is possible but bad)
      break;
    }
  }
  if (!connection_established) errorHalt("*Connection failed");
  
  delay(5000);
  //echoCommand("AT+CWSAP?", "OK", CONTINUE); // set the parameters of AP and Test - maybe only first time useful
    
}

// ******** LOOP ********
void loop() 
{
  // Establish TCP connection
  // AT+CIPSTART=0,"TCP","192.254.235.21",80
  String cmd = "AT+CIPSTART=0,\"TCP\",\""; cmd += DEST_IP; cmd += "\",80";
  
    if (!echoCommand(cmd, "Linked", HALT)) return; //no ip//Link typ ERROR
  // Get connection status 
  echoCommand("AT+CIPSTATUS", "OK", CONTINUE); // Status:3 ist ok
  
  // Build HTTP request. GET / HTTP/1.1
                       //Host: retro.hackaday.com:80
  cmd = "GET / HTTP/1.1\r\nHost: "; cmd += DEST_HOST; cmd += ":80\r\n\r\n";
  
  // Ready the module to receive raw data
  if (!echoCommand("AT+CIPSEND=0,"+String(cmd.length()), ">", HALT))
  {
    echoCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("*Connection timeout.");
    return;
  }
  
  // Send the raw HTTP request
  echoCommand(cmd, "OK", CONTINUE);  // GET
  
  // Loop forever echoing data received from destination server.
  while(true)
    while (Serial1.available())
      Serial.write(Serial1.read());
      
  errorHalt("ONCE ONLY"); // never happens to me
}
