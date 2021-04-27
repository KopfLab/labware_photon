/*
 * This code is for logging from a chemglass scale
 * Author: Sebastian Kopf <sebastian.kopf@colorado.edu>
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "ChemglassScaleLoggerComponent.h"

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
  /* data_reading_period */       5000  // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "scale 0.8.0",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  controller_state
);

// scale state
ScaleState* scale_state = new ScaleState(
  /* calc_rate */         CALC_RATE_MIN
);

// scale component
ChemglassScaleLoggerComponent* scale = new ChemglassScaleLoggerComponent(
  /* component name */        "scale", 
  /* pointer to controller */ controller,
  /* pointer to state */      scale_state
);

// data update callback function
void data_update_callback() {
  // latest data
  lcd->resetBuffer();
  if (scale->data[0].newest_value_valid)
    getDataDoubleText("Last", scale->data[0].newest_value, scale->data[0].units, 
      lcd->buffer, sizeof(lcd->buffer), PATTERN_KVU_SIMPLE, scale->data[0].decimals - 1);
  else
    strcpy(lcd->buffer, "Last: no data yet");
  lcd->printLineFromBuffer(2);

  // running data
  lcd->resetBuffer();
  if (scale->data[0].getN() > 0)
    getDataDoubleText("Avg", scale->data[0].getValue(), scale->data[0].units, scale->data[0].getN(), 
      lcd->buffer, sizeof(lcd->buffer), PATTERN_KVUN_SIMPLE, scale->data[0].getDecimals());
  else
    strcpy(lcd->buffer, "Avg: no data yet");
  lcd->printLineFromBuffer(3);

  // rate
  lcd->resetBuffer();
  if (scale->state->calc_rate == CALC_RATE_OFF) {
    if (scale->data[0].getN() > 1)
      getDataDoubleText("SD", scale->data[0].getStdDev(), scale->data[0].units, scale->data[0].getN(),
        lcd->buffer, sizeof(lcd->buffer), PATTERN_KVUN_SIMPLE, scale->data[0].getDecimals());
    else
      strcpy(lcd->buffer, "SD: not enough data");
    lcd->printLineFromBuffer(4);
  } else {
    if (scale->data[1].newest_value_valid)
      getDataDoubleText("Rate", scale->data[1].newest_value, scale->data[1].units, 
        lcd->buffer, sizeof(lcd->buffer), PATTERN_KVU_SIMPLE, scale->data[1].decimals);
    else
      strcpy(lcd->buffer, "Rate: not enough data");
    lcd->printLineFromBuffer(4);
  }
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
  //scale->debug();

  // callbacks
  controller->setDataUpdateCallback(data_update_callback);

  // add components
  controller->addComponent(scale);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
