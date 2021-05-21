
/*
 * This code is for controlling and logging a JKem overhead stirrer
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "JKemStirrerLoggerComponent.h"

// display
LoggerDisplay* lcd = new LoggerDisplay(16, 2);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       3600, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   500, // in ms
  /* data_reading_period */       1000  // in ms
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
  /* status */            STIRRER_STATUS_MANUAL,
  /* speed [rpm] */       0
);

// stirrer component
JKemStirrerLoggerComponent* stirrer = new JKemStirrerLoggerComponent(
  /* component name */        "stirrer", 
  /* pointer to controller */ controller,
  /* pointer to state */      stirrer_state
);

// lcd update callback function
void lcd_update_callback() {
  lcd->resetBuffer();
  getStirrerStateStatusInfo(stirrer_state->status, lcd->buffer, sizeof(lcd->buffer), true);
  getStateDoubleText("speed", stirrer_state->rpm, "rpm", lcd->buffer + strlen(lcd->buffer), sizeof(lcd->buffer) - strlen(lcd->buffer), " %s %s", 0, false);
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
  //stirrer->debug();

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
