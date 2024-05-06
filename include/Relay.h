#include "PinMapping.h"
#include <TaskSchedulerDeclarations.h>
#include <TimeoutHelper.h>

class Relay
{
  public:
    static const uint8_t ON = 0;      //!< Relay state: on.
    static const uint8_t OFF = 1;     //!< Relay state: off.

    Relay(uint8_t pin);
    void on();                             //!< Turns the relay on.
    void off();                            //!< Turns the relay off.
    void toggle();                         //!< Toggles the relay state.
    boolean isStat();                        //!< @return True iff the relay is on.

    uint8_t getState();               //!< @return The current relay state (Relay::OFF or Relay::ON).
    void setState(uint8_t state       //!< The new state to switch to (Relay::OFF or Relay::ON).
                 );                        //!< Sets a relay state.

  protected:
    uint8_t _state;                   //!< state model
    uint8_t _pin;                     //!< pin model
};
