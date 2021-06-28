
/*
 * This code is for logging a DS18-type temperature sensor
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "DataReaderLoggerComponent.h"
#include "DS18B20TemperatureLoggerComponent.h"

// display
LoggerDisplay* lcd = new LoggerDisplay(16, 2);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       600, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   100, // in ms
  /* data_reading_period */       1000  // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "temp 0.2",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  controller_state
);

// temperature sensor component
DS18B20TemperatureLoggerComponent* temperature = new DS18B20TemperatureLoggerComponent(
  /* component name */        "temperature", 
  /* pointer to controller */ controller,
  /* 1-wire pin */            A0
);

// lcd update callback function
void lcd_update_callback() {
  lcd->resetBuffer();
  if (temperature->foundSensor()) {
    // got a sensor -> show data if there is any yet
    if (temperature->data[0].newest_value_valid)
      getDataDoubleText("T", temperature->data[0].newest_value, temperature->data[0].units, 
        lcd->buffer, sizeof(lcd->buffer), PATTERN_KVU_SIMPLE, temperature->data[0].decimals);
    else
      strcpy(lcd->buffer, "T: no data yet");
  } else {
    // no sensor yet (or got disconnected)
    strcpy(lcd->buffer, "searching sensor");
  }
  lcd->printLineFromBuffer(2);
}

// manual wifi management
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

void setup() {

  // turn wifi module on
  WiFi.on();

  // serial
  Serial.begin(9600);
  delay(1000);

  // debugging
  //controller->forceReset();
  //controller->debugDisplay();
  //controller->debugData();
  //controller->debugState();
  //controller->debugCloud();
  //controller->debugWebhooks();

  // callbacks
  controller->setDataUpdateCallback(lcd_update_callback);

  // add components
  controller->addComponent(temperature);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
