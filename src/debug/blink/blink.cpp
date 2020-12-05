#include "Particle.h"

// generic blink test

int led = D7; // This one is the little blue LED on your board. On the Photon it is next to D7, and on the Core it is next to the USB jack.
int interval = 500; // how many ms to wait between blinks


char device_name[20];
char device_public_ip[20];

void info_handler(const char *topic, const char *data) {
    if ( strcmp (topic, "spark/device/name") == 0) {
        Serial.println("Device name: " + String(data));
        strncpy ( device_name, data, sizeof(device_name) );
    } else if ( strcmp(topic, "spark/device/ip") == 0) {
        Serial.println("Device public IP: " + String(data));
        strncpy (device_public_ip, data, sizeof(device_public_ip));
    }
    // unsubscribe from events
    if ( strlen(device_name) > 0 && strlen(device_public_ip) > 0) {
        Particle.unsubscribe();
    }
}

void setup() {
    
  Serial.begin(9600);
  Time.zone(-6); // to get the time correctly
  Serial.println("Starting up...");
  pinMode(led, OUTPUT);

  // device info
  Serial.println("Requesting device info...");
  Particle.subscribe("spark/", info_handler, ALL_DEVICES);
  Particle.publish("spark/device/name", PRIVATE);
  Particle.publish("spark/device/ip", PRIVATE);
  Serial.println("Startup complete");

}

void loop() {
  // To blink the LED, first we'll turn it on...
  digitalWrite(led, HIGH);
  
  // We'll leave it on for x ms...
  delay(interval);

  // Then we'll turn it off...
  digitalWrite(led, LOW);
 
  // Wait x ms...
  delay(interval);

  // repeat
  Serial.println(Time.format(Time.now(), "%H:%M:%S %d.%m.%Y"));
}
