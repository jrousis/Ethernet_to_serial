#include <ETH.h>
#include <EEPROM.h>
#include <WebServer.h>
#include <WiFi.h>

#define DEFAULT_PIN 02

#define EEPROM_DEFAULT 0 //default value for eeprom
#define EEP_IP_ADDRESS 1 //4 byte
#define EEP_IP_GATEWAY 5 //4 byte
#define EEP_SUBNET 9 //4 byte
#define EEP_MAC 13 //6 byte
#define EEP_PORT 19 //2 byte
#define EEP_BOUDRATE 21 //4 byte
#define EEP_USERNAME 25 //max 16 char
#define EEP_PASSWORD 41 //max 16 char

// Default values
IPAddress localIP(192, 168, 1, 19);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
uint8_t mac[6] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
uint16_t port = 23;
uint32_t baudrate = 9600;
String storedUsername = "admin";
String storedPassword = "12345";

WebServer server(80);
WiFiServer TCP_server;

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
        if (k == 0)
        {
            data[len] = '\0';
            return String(data);
            //escape loop            
        }
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

void setup() {
    Serial.begin(115200);    
    EEPROM.begin(64);
    //make default pin as input and enable pullup resistor
    pinMode(DEFAULT_PIN, INPUT_PULLUP);
    char user[17];
    char pass[17];
    //--------------------------------------------------------------------------------
    delay(100);

    Serial.println();
    Serial.println("Start initialize...");
    //print EEPROM_DEFAULT and DEFAULT_PIN
    Serial.println("EEPROM_DEFAULT: " + String(EEPROM.read(EEPROM_DEFAULT)));
    Serial.println("DEFAULT_PIN: " + String(digitalRead(DEFAULT_PIN)));


    // Set Default values for EEPROM
    // Check if default value in the EEPROM is empty
    if (EEPROM.read(EEPROM_DEFAULT) != 0x01 || digitalRead(DEFAULT_PIN) == LOW)
    {
        // Set default values
        Serial.println();
        Serial.println("Setting default values to EEPROM");

        // Write default values to EEPROM
        // Save IP to EEPROM
        
        saveIPAddress(localIP, EEP_IP_ADDRESS);
        saveIPAddress(gateway, EEP_IP_GATEWAY);
        saveIPAddress(subnet, EEP_SUBNET);
        EEPROM.write(EEP_PORT, port >> 8); EEPROM.write(EEP_PORT + 1, port & 0xFF);
        //Store the baudrate in 4 bytes in EEPROM
        EEPROM.write(EEP_BOUDRATE, baudrate >> 24); EEPROM.write(EEP_BOUDRATE + 1, (baudrate >> 16) & 0xFF);
        EEPROM.write(EEP_BOUDRATE + 2, (baudrate >> 8) & 0xFF); EEPROM.write(EEP_BOUDRATE + 3, baudrate & 0xFF);
        writeString(EEP_USERNAME, "admin\0", 6); // "admin"
        writeString(EEP_PASSWORD, "12345\0", 6); // "12345"

        // Write default value to EEPROM
        EEPROM.write(EEPROM_DEFAULT, 0x01);
        EEPROM.commit();
    }
	else
	{
		// Read values from EEPROM
		localIP = restoreIPAddress(EEP_IP_ADDRESS);
        gateway = restoreIPAddress(EEP_IP_GATEWAY);
		subnet = restoreIPAddress(EEP_SUBNET);
		port = (EEPROM.read(EEP_PORT) << 8) | EEPROM.read(EEP_PORT + 1);
        baudrate = (EEPROM.read(EEP_BOUDRATE) << 24) | (EEPROM.read(EEP_BOUDRATE + 1) << 16) | (EEPROM.read(EEP_BOUDRATE + 2) << 8) | EEPROM.read(EEP_BOUDRATE + 3);
        storedUsername = read_String(EEP_USERNAME, 16);
        storedPassword = read_String(EEP_PASSWORD, 16);

        Serial.println();
        Serial.println("Read from EEPROM"); 
        Serial.println("IP Address: " + localIP.toString());
        Serial.println("Subnet: " + subnet.toString());
        Serial.println("Port: " + String(port));
        Serial.println("Baudrate: " + String(baudrate));
        Serial.println("Username: " + storedUsername);
        Serial.println("Password: " + storedPassword);
	}

    Serial2.setPins(5, 17);
    Serial2.begin(baudrate, SERIAL_8N1, 5, 17);
    Serial2.setHwFlowCtrlMode(HW_FLOWCTRL_DISABLE);
    //--------------------------------------------------------------------------------
    
    // Set up Ethernet
    ETH.begin();
    ETH.config(localIP, gateway, subnet);
    //ETH.config(IPAddress(192, 168, 1, 177), IPAddress(192, 168, 1, 1), IPAddress(255, 255, 255, 0));
    //...............................................................................
    // Set up TCP server
    TCP_server.begin(port);
    Serial.println();
    Serial.println("TCP server started");

    //...............................................................................
    // Set up web server
    server.on("/", handleRoot);
    server.on("/login", handleLogin);
    server.on("/submit", handleSubmit);
    server.begin();
    Serial.println("HTTP server started");
    //WiFi mac address
    Serial.print("\nWiFi MAC Address: ");
    Serial.println(WiFi.macAddress());


}

void loop() {
    server.handleClient();

    // Check if there is a connected client waiting to be serviced by the server
    if (TCP_server.hasClient())
    {
        char c;

        WiFiClient client = TCP_server.available();
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

            server.handleClient();
        }
        Serial.println("");
        // Client disconnected
        Serial.println("Client disconnected");
    }
}

void handleRoot() {
    if (!server.authenticate(storedUsername.c_str(), storedPassword.c_str())) {
        return server.requestAuthentication();
    }

    // Build HTML form with initial values
    /*String html = "<html><body><form action='/submit' method='POST'>";
    html += "IP Address: <input type='text' name='ip' value='" + ip.toString() + "'><br>";
    html += "Subnet: <input type='text' name='subnet' value='" + subnet.toString() + "'><br>";
    html += "MAC Address: <input type='text' name='mac' value='" + String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX) + "'><br>";
    html += "Port: <input type='text' name='port' value='" + String(port) + "'><br>";
    html += "Username: <input type='text' name='username' value='" + String(user) + "'><br>";
    html += "Password: <input type='text' name='password' value='" + String(pass) + "'><br>";
    html += "<input type='submit' value='Submit'>";
    html += "</form></body></html>";
    server.send(200, "text/html", html);*/

    // Build HTML form with initial values

    String html = "<h2>Ethernet to Serial Link Settings</H2>";
    html += "<html><body><div style='display:flex; justify-content:center; align-items:top; height:100vh;'>";
    html += "<form action='/submit' method='POST'><table>";
    html += "<tr><td>IP Address:</td><td><input type='text' name='ip' value='" + localIP.toString() + "'></td></tr>";
    html += "<tr><td>Gate Way:</td><td><input type='text' name='gw' value='" + gateway.toString() + "'></td></tr>";
    html += "<tr><td>Subnet:</td><td><input type='text' name='subnet' value='" + subnet.toString() + "'></td></tr>";
    //html += "<tr><td>MAC Address:</td><td><input type='text' name='mac' value='" + String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX) + "'></td></tr>";
    html += "<tr><td>MAC Address:</td><td><input type='text' name='mac' value='" + WiFi.macAddress() + "'></td></tr>";
    html += "<tr><td>Port:</td><td><input type='text' name='port' value='" + String(port) + "'></td></tr>";
    html += "<tr><td>baudrate:</td><td><input type='text' name='baudrate' value='" + String(baudrate) + "'></td></tr>";
    html += "<tr><td>Username:</td><td><input type='text' name='username' value='" + String(storedUsername) + "'></td></tr>";
    html += "<tr><td>Password:</td><td><input type='text' name='password' value='" + String(storedPassword) + "'></td></tr>";
    html += "<tr><td colspan='2' style='text-align:center;'><input type='submit' value='Submit'></td></tr>";
    html += "</table><p></p><hr><p>Rousis Systems LTD</p></form></div></body></html>";
    server.send(200, "text/html", html);
}

void handleLogin() {
    if (!server.authenticate(storedUsername.c_str(), storedPassword.c_str())) {
        return server.requestAuthentication();
    }
    server.send(200, "text/plain", "Login Successful");
}

void handleSubmit() {
    if (!server.authenticate(storedUsername.c_str(), storedPassword.c_str())) {
        return server.requestAuthentication();
    }

    Serial.println();
    Serial.println("Web form save values:");

    if (server.hasArg("ip")) {
        Serial.println("IP Address: " + server.arg("ip"));

        IPAddress ip;
        ip.fromString(server.arg("ip"));
        saveIPAddress(ip, EEP_IP_ADDRESS);
        //EEPROM.put(EEP_IP_ADDRESS, ip);
    }

    if (server.hasArg("gw")) {
		IPAddress gw;
		gw.fromString(server.arg("gw"));
        saveIPAddress(gw, EEP_IP_GATEWAY);
		//EEPROM.put(EEP_IP_GATEWAY, gw);
	}

    if (server.hasArg("subnet")) {
        Serial.println("Subnet: " + server.arg("subnet"));

        IPAddress subnet;
        subnet.fromString(server.arg("subnet"));
        saveIPAddress(subnet, EEP_SUBNET);
        //EEPROM.put(EEP_SUBNET, subnet);
    }

    if (server.hasArg("mac")) {
        Serial.println("MAC Address: " + server.arg("mac"));

        String macStr = server.arg("mac");
        uint8_t mac[6];
        sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
        //EEPROM.put(EEP_MAC, mac);
    }

    if (server.hasArg("port")) {
        Serial.println("Port: " + server.arg("port"));

        port = server.arg("port").toInt();
        Serial.println("Port Int.: ");
        Serial.println(port);

        EEPROM.write(EEP_PORT, port >> 8); EEPROM.write(EEP_PORT + 1, port & 0xFF);
    }

    if (server.hasArg("baudrate")) {
		Serial.println("Baudrate: " + server.arg("baudrate"));

		baudrate = server.arg("baudrate").toInt();
		Serial.println("Baudrate Int.: ");
		Serial.println(baudrate);
        EEPROM.write(EEP_BOUDRATE, baudrate >> 24); EEPROM.write(EEP_BOUDRATE + 1, (baudrate >> 16) & 0xFF);
        EEPROM.write(EEP_BOUDRATE + 2, (baudrate >> 8) & 0xFF); EEPROM.write(EEP_BOUDRATE + 3, baudrate & 0xFF);
	}

    if (server.hasArg("username")) {
        Serial.println("Username: " + server.arg("username"));

        char user[17];
        strncpy(user, server.arg("username").c_str(), 16);
        user[16] = '\0';
        //EEPROM.put(EEP_USERNAME, user);
        storedUsername = String(user);
        writeString(EEP_USERNAME, storedUsername, 16);
    }

    if (server.hasArg("password")) {
        Serial.println("Password: " + server.arg("password"));

        char pass[17];
        strncpy(pass, server.arg("password").c_str(), 16);
        pass[16] = '\0';
        //EEPROM.put(EEP_PASSWORD, pass);
        storedPassword = String(pass);
        writeString(EEP_USERNAME, storedUsername, 16);
    }

    EEPROM.commit();
    server.send(200, "text/plain", "Settings Saved");
}
