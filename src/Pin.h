#pragma once

#include "Arduino.h"

class InputPullup {
   public:
	InputPullup(int pinNumber) : pinNumber{pinNumber} {
		pinMode(pinNumber, INPUT_PULLUP);
	}

	auto read() -> bool { return digitalRead(pinNumber) == LOW; }

   private:
	int pinNumber;
};
