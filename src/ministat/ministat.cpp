#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks
#include "application.h"

// debugging options
#define CLOUD_DEBUG_ON
//#define WEBHOOKS_DEBUG_ON
#define STATE_DEBUG_ON
//#define DATA_DEBUG_ON
//#define SERIAL_DEBUG_ON
//#define LCD_DEBUG_ON
#define STEPPER_DEBUG_ON
//#define STATE_RESET // FIXME auto state reset

// keep track of installed version
#define STATE_VERSION    4 // change whenver StepperState structure changes
#define DEVICE_VERSION  "ms 0.1.0" // update with every code update

// M800 controller
#include "StepperController.h"

// lcd
//DeviceDisplay* lcd = &LCD_20x4;
DeviceDisplay LCD_16x2_27 (0x27, 16, 2);
DeviceDisplay* lcd = &LCD_16x2_27;

// board
StepperBoard* board = &PHOTON_STEPPER_BOARD;

// driver
//StepperDriver* driver = &DRV8825;
StepperDriver* driver = &DRV8834;

// motor
//StepperMotor* motor = &WM114ST;
StepperMotor* motor = &STPM35;

// initial state
#ifdef NOPE // fixme: this is for pump stepper
StepperState* state = new StepperState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       3600, // in seconds
  /* data_logging_type */         LOG_BY_TIME, // log by time
  /* direction */                 DIR_CW, // start clockwise
  /* status */                    STATUS_OFF, // start off
  /* rpm */                       1 // start speed [rpm]
  // no specification of microstepping mode = automatic mode
);
#endif

// initial state
StepperState* state = new StepperState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       3600, // in seconds
  /* data_logging_type */         LOG_BY_TIME, // log by time
  /* direction */                 DIR_CW, // start clockwise
  /* status */                    STATUS_OFF, // start off
  /* rpm */                       100 // start speed [rpm]
  // no specification of microstepping mode = automatic mode
);

// controller
StepperController* stirrer = new StepperController(
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to board */  board,
  /* pointer to driver */ driver,
  /* pointer to motor */  motor,
  /* pointer to state */  state
);

// using system threading to improve timely stepper stepping
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void setup() {

  // serial
  Serial.begin(9600);

  // lcd temporary messages
  lcd->setTempTextShowTime(3); // how many seconds temp time

  // controller
  stirrer->init();

}

void loop() {
  stirrer->update();
}


/*
// maybe test this more
// might need a system where every line of LCD screen gets stored in a String
// and then only updated in the separate threat whenever necessary
led1Thread = new Thread("sample", test_lcd);
Thread* led1Thread;

os_thread_return_t test_lcd(){
    for(;;) {
        lcd.update();
    }
}*/
