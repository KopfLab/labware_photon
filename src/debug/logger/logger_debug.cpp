#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// debugging options
#define CLOUD_DEBUG_ON
//#define WEBHOOKS_DEBUG_ON
#define STATE_DEBUG_ON
//#define DATA_DEBUG_ON
//#define SERIAL_DEBUG_ON
//#define LCD_DEBUG_ON
#define STATE_RESET // FIXME auto state reset

// keep track of installed version
#define STATE_VERSION    4 // change whenver StepperState structure changes
#define LOGGER_VERSION  "debug 0.1" // update with every code update

#include "LoggerController.h"

// lcd
//LoggerDisplay* lcd = &LCD_20x4;
LoggerDisplay* lcd = &LCD_16x2;

// initial state
LoggerState* state = new LoggerState(
  /* locked */                    false,
  /* state_logging */             false,
  /* data_logging */              false,
  /* data_reading_period_min */   //500, // in ms
  /* data_reading_period */       //1000, // in ms
  /* data_logging_period */       1800, // in seconds
  /* data_logging_type */         LOG_BY_TIME
);

// controller
LoggerController* controller = new LoggerController(
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  state
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

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // controller
  controller->init();

}

// loop
void loop() {
  controller->update();
}

