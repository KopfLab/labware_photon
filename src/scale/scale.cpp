#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "ScaleLoggerComponent.h"

// display
LoggerDisplay* lcd = new LoggerDisplay(20, 4);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       3600, // in seconds
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

// stepper component
ScaleLoggerComponent* scale = new ScaleLoggerComponent(
  /* component name */        "scale", 
  /* pointer to controller */ controller,
  /* pointer to state */      scale_state,
  /* baud rate */             4800,
  /* serial config */         SERIAL_8N1
);

// state update callback function
char state_info[50];
void state_update_callback() {
  /*
  lcd->resetBuffer();
  getStepperStateStatusInfo(stepper_state->status, state_info, sizeof(state_info), true); 
  lcd->addToBuffer(state_info);
  lcd->addToBuffer(" ");
  getStepperStateSpeedInfo(stepper_state->rpm, state_info, sizeof(state_info), true);
  lcd->addToBuffer(state_info);
  lcd->addToBuffer(" ");
  getStepperStateDirectionInfo(stepper_state->direction, state_info, sizeof(state_info), true);
  lcd->addToBuffer(state_info);
  lcd->printLineFromBuffer(2);
  */
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
  controller->forceReset();
  //controller->debugDisplay();
  controller->debugData();
  //controller->debugState();
  //controller->debugCloud();
  //controller->debugWebhooks();
  scale->debug();

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // callbacks
  controller->setStateUpdateCallback(state_update_callback);

  // add components
  controller->addComponent(scale);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
