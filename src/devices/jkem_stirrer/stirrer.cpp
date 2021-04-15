
/*
 * This code is for controlling and logging a JKem overhead stirrer
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "JKemStirrerLoggerComponent.h"

// display
LoggerDisplay* lcd = new LoggerDisplay(20, 4);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       600, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   2000, // in ms
  /* data_reading_period */       10000  // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "jkem 0.1.0",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  controller_state
);

// scale state
StirrerState* stirrer_state = new StirrerState(
  /* speed [rpm] */       0,
  /* status */            STIRRER_STATUS_MANUAL
);

// scale component
JKemStirrerLoggerComponent* stirrer = new JKemStirrerLoggerComponent(
  /* component name */        "stirrer", 
  /* pointer to controller */ controller,
  /* pointer to state */      stirrer_state
);

// data update callback function
void lcd_update_callback() {
  Serial.println("implement lcd update");
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
  controller->debugData();
  //controller->debugState();
  //controller->debugCloud();
  //controller->debugWebhooks();
  stirrer->debug();

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // callbacks
  controller->setDataUpdateCallback(lcd_update_callback);
  controller->setStateUpdateCallback(lcd_update_callback);

  // add components
  controller->addComponent(stirrer);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
