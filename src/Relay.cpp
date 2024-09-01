#include "Relay.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include "MessageOutput.h"

Relay::Relay(uint8_t pin) {
	this->_pin = pin;
	pinMode(pin, OUTPUT);
	//digitalWrite(pin, LOW);
}

void Relay::on(void)
{
    this->setState(ON);                                              //!< explicitly set to ON
}

void Relay::off(void)
{
    this->setState(OFF);                                             //!< explicitly set to OFF
}

void Relay::toggle(void)
{
    this->setState(HIGH ^ this->_state);                             //!< invert the state
}

uint8_t Relay::getState(void)
{
    return this->_state;                                             //!< get stored state
}

void Relay::setState(uint8_t state)
{
  if (this->_state != state) {                                     //!< check for actual state change
    this->_state = (OFF == state ? OFF : ON);                      //!< store new state, explicitly 'casting' to ON or OFF
    MessageOutput.printf("Relay Pin: ");
    MessageOutput.print(_pin);
    MessageOutput.print(" St:");
    MessageOutput.println(_state);
    digitalWrite(this->_pin, this->_state); //!< apply logic mode and set pin state
  }
}

boolean Relay::isStat(void)
{
    MessageOutput.print("Relay Status?");
    MessageOutput.print(_pin);
    MessageOutput.print(" State:");
    //MessageOutput.println(!getState());
    //return (ON == this->getState());
    bool status = digitalRead(_pin);
    MessageOutput.println(status);
    return (status);
}

