#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// debugging options
#define CLOUD_DEBUG_ON
//#define WEBHOOKS_DEBUG_ON
#define STATE_DEBUG_ON
//#define DATA_DEBUG_ON
//#define SERIAL_DEBUG_ON
//#define LCD_DEBUG_ON
//#define STATE_RESET // FIXME auto state reset

#include "LoggerController.h"
#include "LoggerComponent.h"
#include "DataReaderLoggerComponent.h"
#include "SerialReaderLoggerComponent.h"
#include "ExampleLoggerComponent.h"

// default instances
LoggerDisplay LCD_16x2 (16, 2);
LoggerDisplay LCD_20x4 (20, 4);

// lcd
//LoggerDisplay* lcd = &LCD_20x4;
LoggerDisplay* lcd = &LCD_16x2;

// initial state
LoggerControllerState* state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       1800, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   500, // in ms
  /* data_reading_period */       1000 // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "debug 0.2",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  state
);

// components
LoggerComponent* cp1 = new LoggerComponent(
  "cp1 test", controller, false, false
);

ExampleState* cp2_state = new ExampleState();

ExampleLoggerComponent* cp2 = new ExampleLoggerComponent(
  "example component", controller, cp2_state
);

// manual wifi management
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

// setup
void setup() {

  // turn wifi module on
  WiFi.on();

  // serial
  Serial.begin(9600);
  delay(1000);

  // debug modes
  //controller->debugDisplay();
  //controller->debugData();
  //controller->debugState();
  //controller->debugCloud();
  //controller->debugWebhooks();

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // add components
  controller->addComponent(cp1);
  controller->addComponent(cp2);

  // controller
  controller->init();

}

// loop
void loop() {
  controller->update();
}

