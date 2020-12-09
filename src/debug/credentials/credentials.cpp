/*
 * program to clear existing and set new wfi credentials programmatically
 * adjust set_credenetials function below to set one or multiple networks
 * NOTE: this does not claim a device, make sure to claim through particle setup first
 */
#include "Particle.h"

// overwrite here
void set_credentials() {

}

// set wifi credentials
byte mac[6];
bool connect = false;
bool connected = false;

// device info
char device_name[20];
char device_public_ip[20];

// manual wifi setup
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

// info handler
void info_handler(const char *topic, const char *data) {
    if ( strcmp (topic, "spark/device/name") == 0) {
        Serial.println("Found device name: " + String(data));
        strncpy ( device_name, data, sizeof(device_name) );
    } else if ( strcmp(topic, "spark/device/ip") == 0) {
        Serial.println("Found device public IP: " + String(data));
        strncpy (device_public_ip, data, sizeof(device_public_ip));
    }
    // unsubscribe from events
    if ( strlen(device_name) > 0 && strlen(device_public_ip) > 0) {
        Particle.unsubscribe();
    }
}

// setup
void setup() {
  
  // turn wifi module on
  WiFi.on();

  // start serial
  Serial.begin(9600);
  waitFor(Serial.isConnected, 5000); // give monitor 5 seconds to connect
  delay(500);
  Serial.println("Starting up...");

  // timezone
  Time.zone(-6); // to get the time correctly

  // particle subscriptions
  Particle.subscribe("spark/", info_handler, ALL_DEVICES);
  
  // mac address
  WiFi.macAddress(mac);
  Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // wifi credentials
  if (WiFi.clearCredentials())
    Serial.println("Credentials cleared");
  set_credentials();

  // finished
  Serial.println("Setup complete");

}

void loop() {

  // cloud connection
  if (Particle.connected()) {
    if (!connected) {
      // connection just made
      WiFi.macAddress(mac);
      Serial.printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      Serial.println(Time.format(Time.now(), "Cloud connection established at %H:%M:%S %d.%m.%Y"));
      Particle.publish("spark/device/name", PRIVATE);
      Particle.publish("spark/device/ip", PRIVATE);
      connected = true;
    }
    Particle.process();
  } else if (connected) {
    // should be connected but isn't
    Serial.println(Time.format(Time.now(), "Lost cloud connected at %H:%M:%S %d.%m.%Y"));
    connect = false;
    connected = false;
  } else if (!connect) {
    // start cloud connection
    Serial.println(Time.format(Time.now(), "Initiate connection at %H:%M:%S %d.%m.%Y"));
    Particle.connect();
    connect = true;
  }



}