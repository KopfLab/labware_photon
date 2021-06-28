#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "StepperLoggerComponent.h"
#include "OpticalDensityLoggerComponent.h"

// display
LoggerDisplay* lcd = new LoggerDisplay(
  /* columns */    16, 
  /* rows */       2,
  /* pages */      2);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       300, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   5000, // in ms
  /* data_reading_period */       30000  // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "ms 0.6",
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
  /* ms3 */         WKP, // FIXME: can I use this or have to stick with WKP? might have to
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
  /* rpm */                       600 // start speed [rpm]
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

// optical density state
OpticalDensityState* od_state = new OpticalDensityState(
  /* beam */                  BEAM_AUTO,
  /* read length */           500, // ms
  /* warmup */                200   // ms
);

// optical density logger
OpticalDensityLoggerComponent* od_logger = new OpticalDensityLoggerComponent(
  /* component name */        "OD",
  /* pointer to controller */ controller,
  /* pointer to state */      od_state,
  /* led pin */               D4,
  /* ref pin */               A1,
  /* sig pin */               A0,
  /* zero read # */           10,
  /* led offset */            11.0, // tested manually, FIXME: move to state (variable already there so no updates necessary later) and implement an automatic led offset read?
  /* stirrer */               stirrer
);

// state update callback function
char state_info[50];
void lcd_update_callback() {
  lcd->resetBuffer();
  if (lcd->getCurrentPage() == 1) {
    float sig_percent = 0;
    float ref_percent = 0;
    if (od_logger->state->beam == BEAM_OFF) {
      sig_percent = od_logger->sig_dark.getMean() / 4095.0 * 100.0;
      ref_percent = od_logger->ref_dark.getMean() / 4095.0 * 100.0;
      snprintf(lcd->buffer, sizeof(lcd->buffer), "OFF:%4.1f/%4.1f", sig_percent, ref_percent);
    } else if (od_logger->isMaxing()) {
      sig_percent = od_logger->sig_beam.getMean() / 4095.0 * 100.0;
      // in zeroing max mode
      if (sig_percent >= 99.95)
        lcd->addToBuffer("SATURATED!");
      else
        snprintf(lcd->buffer, sizeof(lcd->buffer), "max %4.1f&next", sig_percent);
    } else if (od_logger->state->beam == BEAM_ON || od_logger->isMaxing()) {
      sig_percent = od_logger->sig_beam.getMean() / 4095.0 * 100.0;
      ref_percent = od_logger->ref_beam.getMean() / 4095.0 * 100.0;
      // regular beam on
      if (sig_percent >= 99.95 && ref_percent >= 99.95)
        lcd->addToBuffer("ON: SAT!/SAT!");
      else if (sig_percent >= 99.95)
        snprintf(lcd->buffer, sizeof(lcd->buffer), "ON: SAT!/%4.1f", ref_percent);
      else if (ref_percent >= 99.95)
        snprintf(lcd->buffer, sizeof(lcd->buffer), "ON: %4.1f/SAT!", sig_percent);
      else
        snprintf(lcd->buffer, sizeof(lcd->buffer), "ON: %4.1f/%4.1f", sig_percent, ref_percent);
    } else if (od_logger->state->beam == BEAM_PAUSE) {
      lcd->addToBuffer("OD: paused");
    } else if (od_logger->state->is_zeroed && od_logger->data[0].newest_value_valid) {
      getDataDoubleText("OD", od_logger->data[0].newest_value, 
          lcd->buffer, sizeof(lcd->buffer), PATTERN_KV_SIMPLE, 3);
    } else if (od_logger->state->is_zeroed) {
      lcd->addToBuffer("OD: reading");
    } else {
      lcd->addToBuffer("OD: zero me");
    }
  } else if (lcd->getCurrentPage() == 2) {
    getStepperStateStatusInfo(stepper_state->status, state_info, sizeof(state_info), true); 
    lcd->addToBuffer(state_info);
    lcd->addToBuffer(" ");
    getStepperStateSpeedInfo(stepper_state->rpm, state_info, sizeof(state_info), true);
    lcd->addToBuffer(state_info);
    lcd->addToBuffer(" ");
    getStepperStateDirectionInfo(stepper_state->direction, state_info, sizeof(state_info), true);
    lcd->addToBuffer(state_info);
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
  //stirrer->debug();
  od_logger->debug();
  
  // callbacks
  controller->setStateUpdateCallback(lcd_update_callback);
  controller->setDataUpdateCallback(lcd_update_callback);

  // add components
  controller->addComponent(stirrer);
  controller->addComponent(od_logger);

  // controller
  controller->init();
}

void loop() {
  controller->update();
}
