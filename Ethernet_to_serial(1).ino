
#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <HardwareSerial.h>
//set epprom for save data
#include <EEPROM.h>

#define EEPROM_SIZE 256
#define EEPROM_DEFAULT 0 //default value for eeprom
#define EEP_IP_ADDRESS 1 //4 byte
#define EEP_SUBNET 5 //4 byte
#define EEP_MAC 9 //6 byte
#define EEP_PORT 15 //2 byte
#define EEP_USERNAME 17 //max 16 char
#define EEP_PASSWORD 33 //max 16 char

char http_username[16];
char http_password[16];

/*
   * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
   * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
   * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
#define ETH_CLK_MODE    ETH_CLOCK_GPIO16_OUT

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN   16

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE        ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR        1

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18

// MAC address of Device 1
uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEE, 0xFE, 0xEE }; // use any MAC

// Select the IP address for Device 1
IPAddress localIP(192, 168, 1, 61); // IP address of Device 1

// Select the subnet mask for Device 1
IPAddress subnet(255, 255, 255, 0); // Subnet mask for Device 1

// Select the port number for communication
uint16_t port = 23; //telnet use port 23

const size_t ETH_bufferSize = 64;
uint8_t ETH_buffer[ETH_bufferSize] = { 0 };
uint8_t totalBufSize = 0;

WiFiServer server;

//-------------------------------------------------------------------
//Set a web server at port 80
//const char* http_username = "admin";
//const char* http_password = "12345";

const char* PARAM_INPUT_1 = "state";

const char* PARAM_IP_ADDRESS = "ip";

const int output = 2;

AsyncWebServer webserver(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.6rem;}
    body {max-width: 600px; margin:0px auto; padding-bottom: 10px;}
    .switch {position: relative; display: inline-block; width: 120px; height: 68px} 
    .switch input {display: none}
    .slider {position: absolute; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; border-radius: 34px}
    .slider:before {position: absolute; content: ""; height: 52px; width: 52px; left: 8px; bottom: 8px; background-color: #fff; -webkit-transition: .4s; transition: .4s; border-radius: 68px}
    input:checked+.slider {background-color: #2196F3}
    input:checked+.slider:before {-webkit-transform: translateX(52px); -ms-transform: translateX(52px); transform: translateX(52px)}
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <button onclick="logoutButton()">Logout</button>

  
    
  <form action="/get" target="hidden-form">
    inputString (current value %inputString%): <input type="text" name="inputString">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputInt (current value %inputInt%): <input type="number " name="inputInt">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  <form action="/get" target="hidden-form">
    inputFloat (current value %inputFloat%): <input type="number " name="inputFloat">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  <iframe style="display:none" name="hidden-form"></iframe>  

  <p>Ouput - GPIO 2 - State <span id="state">%STATE%</span></p>
  %BUTTONPLACEHOLDER%
<script>function toggleCheckbox(element) {
  var xhr = new XMLHttpRequest();
  if(element.checked){ 
    xhr.open("GET", "/update?state=1", true); 
    document.getElementById("state").innerHTML = "ON";  
  }
  else { 
    xhr.open("GET", "/update?state=0", true); 
    document.getElementById("state").innerHTML = "OFF";      
  }
  xhr.send();
}
function logoutButton() {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/logout", true);
  xhr.send();
  setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
}
</script>
</body>
</html>
)rawliteral";

const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <p>Logged out or <a href="/">return to homepage</a>.</p>
  <p><strong>Note:</strong> close all web browser tabs to complete the logout process.</p>
</body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var) {
    //Serial.println(var);
    if (var == "BUTTONPLACEHOLDER") {
        String buttons = "";
        String outputStateValue = outputState();
        buttons += "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
        return buttons;
    }
    if (var == "STATE") {
        if (digitalRead(output)) {
            return "ON";
        }
        else {
            return "OFF";
        }
    }
    return String();
}

String outputState() {
    if (digitalRead(output)) {
        return "checked";
    }
    else {
        return "";
    }
    return "";
}

void notFound(AsyncWebServerRequest* request) {
    request->send(404, "text/plain", "Not found");
}
//-------------------------------------------------------------------
void writeString(char add, String data, uint8_t length)
{
    int _size = length; // data.length();
    int i;
    for (i = 0; i < _size; i++)
    {
        if (data[i] == 0) { break; }
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + i, '\0');   //Add termination null character for String Data
    EEPROM.commit();
}

String read_String(char add, uint8_t length)
{
    int i;
    char data[100]; //Max 100 Bytes
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < length)   //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

// Function to save IP address to EEPROM
void saveIPAddress(IPAddress ip, char add) {
    for (int i = 0; i < 4; i++) {
        EEPROM.write(add + i, ip[i]);
    }
    EEPROM.commit();
}

// Function to restore IP address from EEPROM
IPAddress restoreIPAddress(char add) {
    IPAddress ip;
    for (int i = 0; i < 4; i++) {
        ip[i] = EEPROM.read(add + i);
    }
    return ip;
}

void setup()
{
    EEPROM.begin(EEPROM_SIZE);
    Serial.begin(115200);
    Serial2.setPins(5, 17);
    Serial2.begin(9600, SERIAL_8N1, 5, 17);
    Serial2.setHwFlowCtrlMode(HW_FLOWCTRL_DISABLE);
    delay(100);

    // Set Default values for EEPROM
    // Check if default value in the EEPROM is empty
    if (EEPROM.read(EEPROM_DEFAULT) != 0x02)
	{
		// Set default values
        Serial.println("Setting default values to EEPROM");

		// Write default values to EEPROM
        // Save IP to EEPROM
        saveIPAddress(localIP, EEP_IP_ADDRESS);
        saveIPAddress(subnet, EEP_SUBNET);
        EEPROM.put(EEP_PORT, port >> 8); EEPROM.put(EEP_PORT, port & 0xFF);
        writeString(EEP_USERNAME, "admin\0",6); // "admin"
        writeString(EEP_PASSWORD, "12345\0",6); // "12345"

		// Write default value to EEPROM
		EEPROM.write(EEPROM_DEFAULT, 0x01);
		EEPROM.commit();
	}
    
	// Read IP_ADDRESS from EEPROM to a local array
    Serial.println("Reading IP address from EEPROM");
    localIP = restoreIPAddress(EEP_IP_ADDRESS);
    subnet = restoreIPAddress(EEP_SUBNET);

    port = EEPROM.get(EEP_PORT, port);
    for (size_t i = 0; i < 16; i++)
    {
        http_username[i] = EEPROM.read(EEP_USERNAME + i);
    }
    for (size_t i = 0; i < 16; i++)
    {
        http_password[i] = EEPROM.read(EEP_PASSWORD + i);
    }

    EEPROM.get(EEP_PASSWORD, http_password);

    //get device MAC address
    ETH.macAddress(mac);


    // Initialize Ethernet and set the MAC address
    ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
    ETH.macAddress(mac);

    // Set the local IP address, subnet mask, and start Ethernet connection
    ETH.config(localIP, IPAddress(), subnet);

    // Start the TCP server
    server.begin(port);
    Serial.print("TCP server started at ");
    Serial.print(localIP);
    Serial.print(":");
    Serial.println(port);

    //-------------------------------------------------------------------
    // Route for root / web page
    webserver.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->authenticate(http_username, http_password))
            return request->requestAuthentication();
        request->send_P(200, "text/html", index_html, processor);
    });

    webserver.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(401);
    });

    webserver.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send_P(200, "text/html", logout_html, processor);
    });

    // Send a GET request to <ESP_IP>/update?state=<inputMessage>
    webserver.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
        if (!request->authenticate(http_username, http_password))
            return request->requestAuthentication();
        String inputMessage;
        String inputParam;
        // GET input1 value on <ESP_IP>/update?state=<inputMessage>
        if (request->hasParam(PARAM_INPUT_1)) {
            inputMessage = request->getParam(PARAM_INPUT_1)->value();
            inputParam = PARAM_INPUT_1;
            digitalWrite(output, inputMessage.toInt());
        }
        else {
            inputMessage = "No message sent";
            inputParam = "none";
        }
        Serial.println(inputMessage);
        request->send(200, "text/plain", "OK");
    });

    webserver.onNotFound(notFound);
    webserver.begin();

    Serial.println("Web server started at port 80");

    //Serial1.println("Serial 2 is enable!");
}

void loop()
{
    // Check if there is a connected client waiting to be serviced by the server
    if (server.hasClient())
    {
        char c;

        WiFiClient client = server.available();
        // New client connected
        Serial.println("Client available");

        while (client.connected())
        {
            // Check if there is data available from the client
            if (client.available())
            {
                // Read the data from the client
                c = client.read();
                Serial.print(c);
                Serial2.print(c);

                /*if (c == '\n')
                {
                    Serial.println();
                    client.print("Ok!\n");
                }*/
            }
            //delay(10);
            if (Serial2.available())
            {
                c = Serial2.read();
				client.print(c);
            }
        }
        Serial.println("");
        // Client disconnected
        Serial.println("Client disconnected");
    }
}

/*
 The code is simple, it initializes the Ethernet connection and starts a TCP server on port 23. When a client connects to the server, the server reads the data sent by the client and prints it on the serial monitor.
 The code is uploaded to the ESP32 board and the serial monitor is opened. The IP address of the ESP32 board is printed on the serial monitor.
 The ESP32 board is connected to the computer using a USB cable. The IP address of the ESP32 board is
*/
