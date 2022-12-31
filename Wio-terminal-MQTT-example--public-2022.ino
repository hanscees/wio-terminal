
/*
 *  
Ino script for Wio terminal to send data via MQTT toward a Home Assistant server
Adjusted by hanscees (google it) 12-2022
Data send via Jason so lots of other sensors can also be integrated easily
feel free to copy-paste and improve

 WiFi and MQTT libs
    Libraries :
    - PubSubClient : https://github.com/knolleary/pubsubclient
    - ArduinoJson : https://github.com/bblanchon/ArduinoJson

    Configuration (HA) :
    sensor 1:
      platform: mqtt
      state_topic: 'office/sensor1'
      name: 'Temperature'
      unit_of_measurement: '°C'
      value_template: '{{ value_json.temperature }}'
    
    sensor 2:
      platform: mqtt
      state_topic: 'office/sensor1'
      name: 'Humidity'
      unit_of_measurement: '%'
      value_template: '{{ value_json.humidity }}'
 * 
 */
#include <rpcWiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>



/*
 * Constant & Variable Declarations
 */

// Network Settings
 // wifi credentials - please change username and password
 const char *ssid     = "hanscees_12G";  // boy that wifi is fast
 const char *password = "SecletBananOOOO0,droptables";   
 // MQTT settings
 #define MQTT_VERSION MQTT_VERSION_3_1_1
 const char *ID = "HCS-Wi-Term1";
 // MQTT: topic
 const char* MQTT_SENSOR_TOPIC = "livingroom/sensors/HCS-Wi-Term1";

 // MQTT broker - please change IP, username and password
 const char* mqttServer     = "mqqt.hansbees.com";   
 const int   mqttPort       =  1883;
 // const char* mqttUser       = "username"; // we dont use password, but ID 
 // const char* mqttPassword   = "password"; // since wifi is protected well enough

// make objects? 
 WiFiClient wclient;
 PubSubClient client(wclient); // Setup MQTT client  

/*-------------------------------------------------------------------------------*/
/* Function void publishData    (and callback as well)                            */
/*                                                                               */
/* TASK    : publish json data to MQTT server                                     */
/* UPDATE  : 30.12.2022                                                          */
/*-------------------------------------------------------------------------------*/

// function called to publish diverse measurements

void publishData(float massConcentrationPm2p5, float massConcentrationPm10p0, float ambientTemperature, 
float ambientHumidity, float vocIndex, float noxIndex) {
  // create a JSON object
  // doc : https://github.com/bblanchon/ArduinoJson/wiki/API%20Reference
  StaticJsonDocument<800> root;    ///  todo is this big enough?
  // old JsonObject root = <jsonDoc>();

  // old StaticJsonBuffer<800> jsonBuffer;    ///  todo is this big enough?
  // old JsonObject& root = jsonBuffer.createObject();
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  root["temp"] = (String)ambientTemperature;
  root["humid"] = (String)ambientHumidity;
  root["VOC"] = (String)vocIndex;
  root["NOx"] = (String)noxIndex;
  root["PM25"] = (String)massConcentrationPm2p5;;
  root["PM10"] = (String)massConcentrationPm10p0;
  // old root.prettyPrintTo(Serial);
  serializeJsonPretty(root, Serial);
  Serial.println("");
  /*
     {
        "temperature": "23.20" ,
        "humidity": "43.70"
     }
  */
  char data[800];  // todo is this big enough? made it 800 from 200 original
  // old root.printTo(data, root.measureLength() + 1);
  serializeJson(root, data, measureJson(root) + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
}




/*-------------------------------------------------------------------------------*/
/* Function void setup_wifi(void)                                                */
/*                                                                               */
/* TASK    : do not provide a wifi hotspot                                       */
/* UPDATE  : 30.08.2020                                                          */
/*-------------------------------------------------------------------------------*/
void setup_wifi() 
{
  
  Serial.println(" Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    millisDelay(2000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println(" [x] WiFi connected");
  Serial.print(" [?] IP address: ");
  Serial.println(WiFi.localIP());
  millisDelay(10000);
}

/*-------------------------------------------------------------------------------*/
/* Function void reconnect()                                                     */
/*                                                                               */
/* TASK    : mqtt reconnect to server                                            */
/* UPDATE  : 06.03.2021                      
//* boolean connect (clientID, [username, password], [willTopic, willQoS, 
// willRetain, willMessage], [cleanSession])                                    */
/*-------------------------------------------------------------------------------*/
void reconnect() 
{
  while (!client.connected()) 
  {
  //  if (client.connect(ID,mqttUser, mqttPassword)) 
    if (client.connect(ID))
    {
      Serial.println("MTTQ IS ONline"); 
    } else 
    {
      Serial.println("MTTQ IS Offline, waiting 5 secs"); 
      Serial.println("ERROR: failed, rc=");
      Serial.print(client.state());
      millisDelay(5000);
      // todo make it non blocking
    }
  }
}



/*-------------------------------------------------------------------------------*/
/* Function void millisDelay(int duration)                                       */
/*                                                                               */
/* TASK    : timer                                                               */
/* UPDATE  : 06.03.2021                                                          */
/*-------------------------------------------------------------------------------*/
void millisDelay(int duration) 
{
    long timestart = millis();
    while ((millis()-timestart) < duration) {};
}


/*-------------------------------------------------------------------------------*/
/* Function void setup()                                                         */
/*                                                                               */
/* TASK    : setup all needed requirements                                       */
/* UPDATE  : 06.03.2021                                                          */
/*           09.03.2021 - ntp time server                                        */
/*-------------------------------------------------------------------------------*/
void setup() 
{
  int serial_timeout = 0;
  Serial.begin(115200);
  while (!Serial) {
        delay(100);
     serial_timeout++;
    if(serial_timeout > 100) {
    //  tft.setCursor(0, 35);
    //  Serial.println("Failed to connect to serial");
      break;
    }
   }

  // Connect to WiFi & Establish Server
  setup_wifi();

  
  // Connect to MQTT and subscribe topic messages
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);  // hmmmm todo?

}

/*-------------------------------------------------------------------------------*/
/* Function void loop()                                                          */
/*                                                                               */
/* TASK    : this runs forever                                                   */
/* UPDATE  : 06.03.2021                                                          */
/*           07.03.2021 - backlight on/off no only works via brightness change   */
/*-------------------------------------------------------------------------------*/
void loop() 
{
  // check if all connections are stable
  if (!client.loop()) reconnect();   // make this non-blocking first please

  // MQTT Communication
  // see https://pubsubclient.knolleary.net/api
  // boolean publish (topic, payload, [length], [retained])


// get measurements done please
//float h = dht.readHumidity();
// Read temperature as Celsius (the default)
//  float t = dht.readTemperature();

//test run
float Pm2p5 = 12.4;
float Pm10 = 13.2;
float Temp = 19.01;
float Humid = 45;
float voc = 3.35;
float nox = 0.1;

publishData(Pm2p5, Pm10, Temp, Humid, voc, nox);  /////  publish !

millisDelay(30000); // send data every 30 seconds or so

}
