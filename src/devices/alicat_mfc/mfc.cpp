/*
 * This code is for controlling an Alicat MFC with a 4-line LCD logger
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "AlicatMFCLoggerComponent.h"

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
  /* version */           "mfc 1.0.0",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  controller_state
);

// MFC state
MFCState* mfc_state = new MFCState(
  /* mfc_id */                "A"
);

// MFC component
AlicatMFCLoggerComponent* mfc = new AlicatMFCLoggerComponent(
  /* component name */        "mfc", 
  /* pointer to controller */ controller,
  /* pointer to state */      mfc_state
);

// data update callback function
void data_update_callback() {
    // gas info and setpoint
    char sp[20];
    int i = 4;
    getDataDoubleText("SP", mfc->data[i].getValue(), mfc->data[i].units, sp, sizeof(sp), PATTERN_KVU_SIMPLE, mfc->data[i].getDecimals()-1);
    if (mfc->data[i].getN() > 0)
      snprintf(lcd->buffer, sizeof(lcd->buffer), "%s=%s %s", mfc_state->mfc_id, mfc->gas, sp);
    else
      snprintf(lcd->buffer, sizeof(lcd->buffer), "%s=%s", mfc_state->mfc_id, mfc->gas);
    lcd->printLineFromBuffer(2);

    // pressure
    i = 0;
    if (mfc->data[i].getN() > 0)
      getDataDoubleText(mfc->data[i].variable, mfc->data[i].getValue(), mfc->data[i].units, mfc->data[i].getN(), lcd->buffer, sizeof(lcd->buffer), PATTERN_KVUN_SIMPLE, mfc->data[i].getDecimals());
    else
      getInfoKeyValue(lcd->buffer, sizeof(lcd->buffer), mfc->data[i].variable, "no data yet", PATTERN_KV_SIMPLE);
    lcd->printLineFromBuffer(3);
    
    // mass flow
    i = 3;
    if (mfc->data[i].getN() > 0)
      getDataDoubleText("F", mfc->data[i].getValue(), mfc->data[i].units, mfc->data[i].getN(), lcd->buffer, sizeof(lcd->buffer), PATTERN_KVUN_SIMPLE, mfc->data[i].getDecimals());
    else
      getInfoKeyValue(lcd->buffer, sizeof(lcd->buffer), "F", "no data yet", PATTERN_KV_SIMPLE);
    lcd->printLineFromBuffer(4);

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
  //mfc->debug();

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // callbacks
  controller->setDataUpdateCallback(data_update_callback);

  // add components
  controller->addComponent(mfc);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
