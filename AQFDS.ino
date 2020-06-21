// Library used to connect to Wifi using the ESP8266 chip
#include <ESP8266WiFi.h>

// Libraries used for connecting to a WiFi network and for uploading code with OTA
#include <WiFiManager.h>
#include <ArduinoOTA.h>

// Libraries used for OLED display communication
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Libraries for sensors communication
#include <SparkFunCCS811.h>
#include "DFRobot_SHT20.h"

// ########################################### Part 1 ###############################

// CCS811 I2C Address
// #define CCS811_ADDR 0x5B // default I2C Address
#define CCS811_ADDR 0x5A // alternate I2C Address, used because it works

// For 0.91'' displays, change height to 32
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Instantiate an OLED object with the preprocessor variables declared and the reference to the Wire library
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
int x = display.width(); // variable storing the width of the OLED display

CCS811 ccs(CCS811_ADDR); // Instantiate a CCS811 object with the specified I2C address
DFRobot_SHT20 sht20; // Instantiate a SHT20 object

// Values to store the measurements of CCS811 sensor
uint16_t CCS811_eCO2;
uint16_t CCS811_TVOC;

// Values to store the measurements of SHT20 sensor
float SHT20_relativeHumidity;
float SHT20_temperature;

// Buzzer pin
const int buzzer = 16; // GPIO16 of ESP8266

String API_KEY = "TVDNE4DFQYN9667U"; // The Write API key for the ThingSpeak channel
WiFiServer server(80); // Open a server on the specified port

char ThingSpeakAddress[] = "api.thingspeak.com"; // ThingSpeak address

// ########################################### Part 2 ###############################

void setup() {

  delay(1000);

  ArduinoOTA.setPassword("admin"); // Sets a password for uploading the data
  ArduinoOTA.begin(); // Begin the OTA process

  //Display
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    for (;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();

  // Clear the buffer of the display
  display.clearDisplay();

  // Set the size of the text displayed
  display.setTextSize(1);

  // Sets the color of the text
  display.setTextColor(WHITE);

  // Sets the cursor to position x and y
  display.setCursor(0, 0);

  // Prints the text
  display.print("Welcome using the");

  display.setCursor(0, 8);
  display.print("AQFD system!");

  // Displays the text
  display.display();

  delay(3000);

  // Function stating the the OTA update is starting
  ArduinoOTA.onStart([]() {
    display.setCursor(0, 0);
    display.print("Starting OTA");
    display.display();
    delay(2000);
  });

  // Function stating that the OTA update is finishing
  ArduinoOTA.onEnd([]() {
    display.setCursor(0, 0);
    display.print("Finishing OTA");
    display.display();
    delay(2000);
  });

  // Function printing the progress of the OTA process
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.setCursor(0, 0);
    display.print("Progress: ");
    display.setCursor(0, 8);
    display.print(progress / (total / 100));
    display.print("%");
    display.display();
    delay(1000);
  });

  // Function used to print the errors raised during OTA process
  ArduinoOTA.onError([](ota_error_t error) {
    display.setCursor(0, 0);
    display.print("OTA Error[");
    display.print(error);
    display.print("]: ");
    display.display();

    switch (error) {
      case OTA_AUTH_ERROR:
        display.setCursor(0, 8);
        display.print("Auth failed!");
        display.display();
        delay(3000);
        break;
      case OTA_BEGIN_ERROR:
        display.setCursor(0, 8);
        display.print("Begin failed!");
        display.display();
        delay(3000);
        break;
      case OTA_CONNECT_ERROR:
        display.setCursor(0, 8);
        display.print("Connect failed!");
        display.display();
        delay(3000);
        break;
      case OTA_RECEIVE_ERROR:
        display.setCursor(0, 8);
        display.print("Receive failed!");
        display.display();
        delay(3000);
        break;
      case OTA_END_ERROR:
        display.setCursor(0, 8);
        display.print("End failed!");
        display.display();
        delay(3000);
        break;
    }
  });

  // Audio alarm
  pinMode(buzzer, OUTPUT);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("The system will now");
  display.setCursor(0, 8);
  display.print("start to initialize");
  display.setCursor(0, 16);
  display.print("the internal sensors.");
  display.display();
  delay(2000);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("SHT20: ");
  display.display();
  delay(2000);

  sht20.initSHT20();
  delay(500);
  sht20.checkSHT20();
  delay(500);

  display.print("OK");
  display.display();
  delay(1000);

  display.setCursor(0, 8);
  display.print("CCS811: ");
  display.display();
  delay(2000);

  CCS811Core::CCS811_Status_e returnCode = ccs.beginWithStatus();

  if (returnCode == CCS811Core::CCS811_Status_e::CCS811_Stat_SUCCESS) {
    display.print("OK");
    display.display();
    delay(2000);
  } else {
    display.print("ERROR!!!");
    display.display();
    delay(2000);

    display.setCursor(0, 16);
    display.print("REBOOTING!");
    display.display();
    delay(3000);

    ESP.restart();
  }

  // ########################################### Part 3 ###############################

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("The system will now");
  display.setCursor(0, 8);
  display.print("attempt to connect to");
  display.setCursor(0, 16);
  display.print("the local");
  display.setCursor(0, 24);
  display.print("WiFi network.");
  display.display();

  delay(3000);

  // Connecting to the local network text
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Connecting");
  display.setCursor(0, 8);
  display.print("to network...");
  display.display();

  delay(2000);

  connectToNetwork();

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("The system needs 20");
  display.setCursor(0, 8);
  display.print("minutes to calibrate.");
  display.setCursor(0, 16);
  display.print("After this, the data");
  display.setCursor(0, 24);
  display.print("becomes more accurate");
  display.display();

  delay(4000);

  // System started alarm test
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);

  // System started message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Setup is completed.");
  display.setCursor(0, 8);
  display.print("The system will now");
  display.setCursor(0, 16);
  display.print("begin to display");
  display.setCursor(0, 24);
  display.print("the data gathered.");
  display.display();

  delay(4000);

  // ########################################### Part 4 ###############################
}

void printSensorData() {

  display.clearDisplay();// Clean Display
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print("T:");
  display.setCursor(15, 0);
  display.print(SHT20_temperature);
  display.print(" ");
  display.print((char)247); // degree symbol
  display.print("C");

  display.setCursor(68, 0);
  display.print("H:");
  display.setCursor(80, 0);
  display.print(SHT20_relativeHumidity);
  display.print(" %");

  display.setCursor(0, 8);
  display.print("TVOC:");
  display.setCursor(35, 8);
  display.print(CCS811_TVOC);
  display.print(" ppb");

  display.setCursor(0, 16);
  display.print("eCO2:");
  display.setCursor(35, 16);
  display.print(CCS811_eCO2);
  display.print(" ppm");

  display.display();
}

void connectToNetwork() {
  // Instantiate a WiFiManager object
  WiFiManager wifiMan;

  // If your project needs to be more secured you can set a strong password
  //  wifiMan.autoConnect("AirQualityAndFireDetector_AP", "_LiCenTa20&&20_");

  wifiMan.autoConnect("AirQualityAndFireDetector_AP");

  // Set the ESP to station mode
  WiFi.mode(WIFI_STA);

  // If the ESP couldn't connect to the network
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {

    display.clearDisplay();
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Connection");
    display.setCursor(0, 8);
    display.print("failed!");
    display.setCursor(0, 16);
    display.print("REBOOTING!");
    display.display();

    delay(500);
    // Restart the chip
    ESP.restart();
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("WiFi connected");
  display.setCursor(0, 9);
  display.print("IP Address: ");
  display.setCursor(0, 17);
  display.print(WiFi.localIP());
  display.display();

  delay(3000);
}

void doUploadToTS() {

  WiFiClient client = server.available();

  // If the system couldn't connect to ThingSpeak platform on port 80
  if (!client.connect(ThingSpeakAddress, 80)) {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Connection to");
    display.setCursor(0, 9);
    display.print("ThingSpeak failed!");
    display.setCursor(0, 17);
    display.print(WiFi.localIP());
    display.display();

    delay(500);
    ESP.restart();

  } else {
    delay(500);

    // Check if CCS811 has the data ready
    if (ccs.dataAvailable()) {

      // Check the results of the CCS algorithm
      ccs.readAlgorithmResults();

      SHT20_relativeHumidity = sht20.readHumidity();
      SHT20_temperature = sht20.readTemperature();

      // Check for NAN values
      if (isnan(SHT20_relativeHumidity) || isnan(SHT20_temperature)) {

        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.setCursor(0, 0);
        display.print("NAN values");
        display.setCursor(0, 8);
        display.print("detected!");
        display.setCursor(0, 16);
        display.print("REBOOTING!");
        display.display();

        delay(500);
        ESP.restart();
      }

      // Calibrate the CCS811 sensor
      ccs.setEnvironmentalData(SHT20_relativeHumidity, SHT20_temperature);

      delay(500);
      CCS811_eCO2 = ccs.getCO2();
      CCS811_TVOC = ccs.getTVOC();
      delay(500);

//      // Wait for more accurate data from CCS811
//      int i = 0;
//      int t1 = millis();
//            
//      while ((CCS811_eCO2 == 400 || CCS811_eCO2 == 0) && CCS811_TVOC == 0) {
//
//        CCS811_eCO2 = ccs.getCO2();
//        CCS811_TVOC = ccs.getTVOC();
//        delay(500);
//
//        if (i++ == 35) break;
//      }

      // This area is used to detect the sources of pollution
      // MOLD pollution
      if (CCS811_TVOC >= 110 && CCS811_TVOC < 200 && SHT20_relativeHumidity >= 65) {
        printSensorData();
        display.setCursor(0, 24);
        display.print("MOLD detected!");
        display.display();
      }

      // CIGARETTE smoke pollution
      if (CCS811_TVOC > 200 && CCS811_TVOC < 500) {

        printSensorData();
        char alert[] = "Possible BENZENE sources pollution!";
        int minX = -6 * strlen(alert);
        int counter = 0;

        while (counter < 300) {
          printSensorData();
          display.setCursor(x, 24);
          display.print(alert);
          display.display();

          x -= 2; // scroll speed

          if (x < minX) {
            x = display.width();
          }
          counter += 1;
        }

        printSensorData();
        display.setCursor(0, 24);
        display.print("Possible source is: ");
        display.display();

        delay(2000);

        printSensorData();
        display.setCursor(0, 24);
        display.print("CIGARETTE smoke!");
        display.display();

        delay(4000);
      }

      // PERFUME or ADHESIVE in atmosphere
      if (CCS811_TVOC >= 500 && CCS811_TVOC < 620) {

        char alert[] = "Perfume or adhesive detected";
        int minX = -6 * strlen(alert);
        int counter = 0;

        while (counter < 300) {
          printSensorData();
          display.setCursor(x, 24);
          display.print(alert);
          display.display();

          x -= 2; // scroll speed

          if (x < minX) {
            x = display.width();
          }
          counter += 1;
        }

        delay(1500);
      }

      // Construction materials pollution detected
      else if (CCS811_TVOC >= 800 && CCS811_TVOC < 1150) {

        char alert[] = "Construction materials pollution detected!";
        int minX = -6 * strlen(alert);
        int counter = 0;

        while (counter < 400) {
          printSensorData();
          display.setCursor(x, 24);
          display.print(alert);
          display.display();

          x -= 2; // scroll speed

          if (x < minX) {
            x = display.width();
          }
          counter += 1;
        }
      }

      if (CCS811_TVOC > 1150 || (CCS811_eCO2 > 2500 && CCS811_eCO2 < 4000)) {
        // Possible contamination with benzene sources or very high CO2 level
        // conversion from ug/m3 to ppb for TVOC:
        // ug/m^3_Value = ppb_Value x (12.187) * molecularWeight(benzene will be used, with a molecular weight
        // of 78.11 g / mol) / (273.15 + temperatureInCelsius)
        // An atmospheric pressure value of 1 atmosphere was considered
        // A value of TVOC of 3000 ug/m3 - 10000 ug/mg is considered DANGEROUS
        // So, if the value of TVOC in ug/m3 is > 3671 (after some trial and error) an alarm will be used
        // So the value that is considered dangerous will be:
        // Value of temperature in Celsius will be set to 25 C as default
        // Will give us the value of 1150 ppb, which is the value searched for
        printSensorData();

        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(200);

        char alert[] = "Dangerously high BENZENE sources pollution! Refresh the indoor air immediately!";
        int minX = -6 * strlen(alert);
        int counter = 0;

        while (counter < 1000) {
          printSensorData();
          display.setCursor(x, 24);
          display.print(alert);
          display.display();

          x -= 2; // scroll speed

          if (x < minX) {
            x = display.width();
          }
          counter += 1;
        }

        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(200);
        digitalWrite(buzzer, LOW);
        delay(200);
      }

      if (SHT20_temperature >= 50) {
        printSensorData();
        display.setCursor(0, 24);
        display.print("Very HIGH Temperature!");
        display.display();
      }

      else if (SHT20_relativeHumidity <= 10) {
        printSensorData();
        display.setCursor(0, 24);
        display.print("Very LOW Humidity!");
        display.display();
      }

      if (SHT20_temperature >= 50 && SHT20_relativeHumidity <= 10 && CCS811_eCO2 >= 4000) {
        // possible fire detected or high temperature detected in a room! Take action!
        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        delay(200);

        display.setCursor(0, 0);
        display.print("Possible danger:");
        display.setCursor(0, 8);
        display.print("FIRE is detected!");
        display.setCursor(0, 16);
        display.print("Remain calm and");
        display.setCursor(0, 24);
        display.print("search an exit!");
        display.display();

        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        delay(200);
        digitalWrite(buzzer, HIGH);
        delay(1000);
        digitalWrite(buzzer, LOW);
        delay(200);

        delay(30000);
      }

      else {
        printSensorData();
        display.setCursor(0, 24);

        display.print("Air quality is good");
        display.display();
        delay(1000);
      }
      // ########################################### Part 5 ###############################

      // The data is packed and sent to the ThingSpeak platform
      // This is the order of the fields in the ThingSpeak channel
      String data = "field1=" + String(CCS811_eCO2, DEC); // ppm
      data += "&field2=" + String(CCS811_TVOC, DEC); // ppb
      data += "&field3=" + String(SHT20_relativeHumidity, DEC); // %
      data += "&field4=" + String(SHT20_temperature, DEC); // degrees Celsius

      client.print("POST /update HTTP/1.1\n");
      client.print("Host: api.thingspeak.com\n");
      client.print("Connection: close\n");
      client.print("X-THINGSPEAKAPIKEY: " + API_KEY + "\n");
      client.print("Content-Type: application/x-www-form-urlencoded\n");
      client.print("Content-Length: ");
      client.print(data.length());
      client.print("\n\n");
      client.print(data);
      delay(500);

    } else if (ccs.checkForStatusError()) {
      //If the CCS811 found an internal error, print it.
      printSensorError();
    }

    printSensorData();
    display.setCursor(0, 24);
    display.print("UPLOADING data...");
    display.display();
    delay(4000);

    printSensorData();
    display.setCursor(0, 24);
    display.print("UPLOAD successfull!");
    display.display();
    delay(4000);

    printSensorData();
    display.setCursor(0, 24);
    display.print("CHECKING air quality");
    display.display();
    delay(4000);

    // ########################################### Part 6 ###############################
  }

  client.stop();
  delay(500);
}

// The program will never get here, because of the deepSleep command
void loop() {

  // Loop solution works with these 3 functions:
  ArduinoOTA.handle();

  doUploadToTS();

  delay(65000);
}

// ########################################### Part 8 ###############################

// Check for errors of CCS811 and print them on screen
void printSensorError()
{
  uint8_t error = ccs.getErrorRegister();

  if (error == 0xFF) //comm error
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Error was not");
    display.setCursor(0, 8);
    display.print("recongnized!");
    display.setCursor(0, 16);
    display.print("REBOOTING!");
    display.display();

    delay(500);
    ESP.restart();
  }
  else {

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.print("Sensor error: ");

    if (error & 1 << 5) {

      display.setCursor(0, 8);
      display.print("Heater Supply!");
      display.setCursor(0, 16);
      display.print("REBOOTING!");
      display.display();

      delay(500);
      ESP.restart();
    }
    if (error & 1 << 4) {

      display.setCursor(0, 8);
      display.print("Heater Fault!");
      display.setCursor(0, 16);
      display.print("REBOOTING!");
      display.display();

      delay(500);
      ESP.restart();
    }
    if (error & 1 << 3) {

      display.setCursor(0, 8);
      display.print("Max Resistance!");
      display.setCursor(0, 16);
      display.print("REBOOTING!");
      display.display();

      delay(500);
      ESP.restart();
    }
    if (error & 1 << 2) {

      display.setCursor(0, 8);
      display.print("Invalid meas mode!");
      display.setCursor(0, 16);
      display.print("REBOOTING!");
      display.display();

      delay(500);
      ESP.restart();
    }
    if (error & 1 << 1) {

      display.setCursor(0, 8);
      display.print("Invalid read reg!");
      display.setCursor(0, 16);
      display.print("REBOOTING!");
      display.display();

      delay(500);
      ESP.restart();
    }
    if (error & 1 << 0) {

      display.setCursor(0, 8);
      display.print("Invalid msg!");
      display.setCursor(0, 16);
      display.print("REBOOTING!");
      display.display();

      delay(500);
      ESP.restart();
    }
  }
}

// ########################################### Part 7 ###############################
