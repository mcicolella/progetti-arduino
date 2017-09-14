/*
  Arduino WeatherShield MQTT version
  by Mauro Cicolella  - www.emmecilab.net

  Hardware:
  * Arduino UNO
  * Ethernet shield
  * WeatherShield by www.ethermania.com
   
  Software:
  * WeatherShield lib https://github.com/mcicolella/progetti-arduino/blob/master/weathershield/lib/WeatherShieldLib.zip
  * PubSubClient lib https://github.com/knolleary/pubsubclient/releases

  MQTT original code by www.lucadentella.it

*/

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <WeatherShield1.h>

// MQTT configuration
#define CLIENT_ID "WeatherShieldMQTT"
#define TEMPERATURE_TOPIC "/weathershield/temperature"
#define HUMIDITY_TOPIC "/weathershield/humidity"
#define PRESSURE_TOPIC "/weathershield/pressure"
#define PUBLISH_DELAY 5000

#define RXBUFFERLENGTH 4
WeatherShield1 weatherShield;

// Set ethernet shield MAC and IP address
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 1, 150 }; 

// MQTT broker address
IPAddress mqttBroker(192,168,1,50);

// variables
EthernetClient ethClient;
PubSubClient mqttClient;
long previousMillis;

float temp;
boolean currentLineIsBlank = true;
String readString;

/* Class for sensors reading */
class WeatherData {

  private:
    float fTemperature;
    unsigned short shTemperature;
    float fPressure;
    unsigned short shPressure;
    float fHumidity;
    unsigned short shHumidity;

  public:
    WeatherData() {
      bReady = false;  // constructor
    }

    // get methods
    float getAvgTemperature() {
      return fTemperature;
    }
    float getInstTemperature() {
      return ((float)shTemperature / 16);
    }
    float getAvgPressure() {
      return fPressure;
    }
    float getInstPressure() {
      float value ;
      value = (((float)shPressure / 1024) + 0.095) / 0.009;
      return value;
    }
    float getAvgHumidity() {
      return fHumidity;
    }
    float getInstHumidity() {
      float value;
      value = (((float)shHumidity / 1024) - 0.1515) / 0.00636; // without compensation
      // compensation relative humidity with read temperature
      value = value / (1.0546 - 0.00216 * getInstTemperature());
      return value;

    }

    // set methods
    void setAvgTemperature(float Temperature) {
      fTemperature = Temperature;
    }
    void setInstTemperature(unsigned short Temperature) {
      shTemperature = Temperature;
    }
    void setAvgPressure(float Pressure) {
      fPressure = Pressure;
    }
    void setInstPressure(unsigned short Pressure) {
      shPressure = Pressure;
    }
    void setAvgHumidity(float Humidity) {
      fHumidity = Humidity;
    }
    void setInstHumidity(unsigned short Humidity) {
      shHumidity = Humidity;
    }

  public:
    boolean bReady;
};

WeatherData weatherData;

void setup()
{
  Serial.begin(9600);

  // start connection
  Ethernet.begin(mac, ip);

  // setup mqtt client
  mqttClient.setClient(ethClient);
  mqttClient.setServer(mqttBroker, 1883);
  Serial.println(F("MQTT client configured correctly"));
}

void loop()
{

  // it's time to send new data?
  if(millis() - previousMillis > PUBLISH_DELAY) {
    if(readValues(weatherShield)){
       sendData(TEMPERATURE_TOPIC, weatherData.getInstTemperature());
       sendData(HUMIDITY_TOPIC, weatherData.getInstHumidity());
       sendData(PRESSURE_TOPIC, weatherData.getInstPressure());
     }
    previousMillis = millis();
  }
  mqttClient.loop();
}


void sendData(char* topic, float value) {

  char msgBuffer[20];
  
  Serial.print("Sending to topic '");
  Serial.println(topic);
  Serial.print("' value '");
  Serial.println(value);
  Serial.println("'");
    
  if(mqttClient.connect(CLIENT_ID)) {
    mqttClient.publish(topic, dtostrf(value, 6, 2, msgBuffer));
  }
}


/* this function reads sensors values and stores them in a WeatherData object
   return value = false if there are some problems with WeatherShield.
*/

boolean readValues(WeatherShield1 &weatherShield) {

  /* buffer */
  unsigned char ucBuffer[RXBUFFERLENGTH];

  /* connection check */
  if (weatherShield.sendCommand(CMD_ECHO_PAR, 100, ucBuffer)) {

    /* temperature */
    if (weatherShield.sendCommand(CMD_GETTEMP_C_AVG, 0, ucBuffer))
      weatherData.setAvgTemperature(weatherShield.decodeFloatValue(ucBuffer));
    if (weatherShield.sendCommand(CMD_GETTEMP_C_RAW, PAR_GET_LAST_SAMPLE, ucBuffer))
      weatherData.setInstTemperature(weatherShield.decodeShortValue(ucBuffer));

    /* pressure */
    if (weatherShield.sendCommand(CMD_GETPRESS_AVG, 0, ucBuffer))
      weatherData.setAvgPressure(weatherShield.decodeFloatValue(ucBuffer));
    if (weatherShield.sendCommand(CMD_GETPRESS_RAW, PAR_GET_LAST_SAMPLE, ucBuffer))
      weatherData.setInstPressure(weatherShield.decodeShortValue(ucBuffer));

    /* humidity  */
    if (weatherShield.sendCommand(CMD_GETHUM_AVG, 0, ucBuffer))
      weatherData.setAvgHumidity(weatherShield.decodeFloatValue(ucBuffer));
    if (weatherShield.sendCommand(CMD_GETHUM_RAW, PAR_GET_LAST_SAMPLE, ucBuffer))
      weatherData.setInstHumidity(weatherShield.decodeShortValue(ucBuffer));

    return true;
  }
  return false;
}



