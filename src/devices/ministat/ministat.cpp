#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "StepperLoggerComponent.h"

// display
LoggerDisplay* lcd = new LoggerDisplay(16, 2);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       3600, // in seconds
  /* data_logging_type */         LOG_BY_TIME
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "ms 0.2",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  controller_state
);

// board
StepperBoard* board = new StepperBoard(
  /* dir */         D2,
  /* step */        D3,
  /* enable */      D7,
  /* ms1 */         D6,
  /* ms2 */         D5,
  /* ms3 */         D4,
  /* max steps/s */ 500 // very conservative to make sure motor doesn't lock
);

// microstep modes (DRV8825 chip)
const int DRV8834_MICROSTEP_MODES_N = 2;
MicrostepMode DRV8834_MICROSTEP_MODES[DRV8834_MICROSTEP_MODES_N] =
  {
    /* n_steps, M0, M1, (CONFIG) */
    {1,  LOW,  LOW,  LOW},  // full step
    {2,  HIGH, LOW,  LOW}   // half step
    // can technically also do 4, 8, 16, 32 but since using it to drive
    // relatively fast rotations for stirring and want the torque
  };

// driver (DRV8825 chip)
StepperDriver* driver = new StepperDriver(
  /* dir cw */      HIGH,
  /* step on */     HIGH,
  /* enable on */   HIGH,
  /* modes */       DRV8834_MICROSTEP_MODES,
  /* modes_n */     DRV8834_MICROSTEP_MODES_N
);

// motor (STPM35)
StepperMotor* motor = new StepperMotor(
  /* steps */       48,  // 48 steps/rotation, 7.5 degree step angle
  /* gearing */      1
);

// stepper state
StepperState* stepper_state = new StepperState(
  /* direction */                 DIR_CW, // start clockwise
  /* status */                    STATUS_OFF, // start off
  /* rpm */                       100 // start speed [rpm]
  // no specification of microstepping mode = automatic mode
);

// stepper component
StepperLoggerComponent* stirrer = new StepperLoggerComponent(
  /* component name */        "stirrer", 
  /* pointer to controller */ controller,
  /* pointer to state */      stepper_state,
  /* pointer to board */      board,
  /* pointer to driver */     driver,
  /* pointer to motor */      motor
);

// state update callback function
char state_info[50];
void state_update_callback() {
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
  controller->debugState();
  controller->debugCloud();
  //controller->debugWebhooks();
  stirrer->debug();

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // callbacks
  controller->setStateUpdateCallback(state_update_callback);

  // add components
  controller->addComponent(stirrer);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
