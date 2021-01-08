/**
 * This class is the basis for logger components that provide control over some peripheral but don't read data.
 */

#pragma once
#include "LoggerComponent.h"
#include "LoggerController.h"

/* component */
class ControllerLoggerComponent : public LoggerComponent
{

  public:

    /*** constructors ***/
    // these types of logger components don't usually have global time offsets --> set to false
    ControllerLoggerComponent (const char *id, LoggerController *ctrl) : LoggerComponent(id, ctrl, false) {}

    virtual void clearData(bool clear_persistent = false) {
      // by default these types of controllers don't clear data, if they use data for logging they manage clearing internally
    };

};
