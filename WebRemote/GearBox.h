#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif


class GearBox
{

public:
	GearBox();
	~GearBox();

	int acceleration_to_100 = 5000;//Час розгону від 0 до 100% (ms)
	int acceleratorPedalPosition = 0;//Позиція курка
	ulong time_stamp = 0;//Час останнього обчислення швидкості
	int speed = 0;//Фактична швидкість
	int regulatorSpeed = 0;
	int gear = 0;

	void SetAcceleratorPedalPosition(int position);

	void loop();

};

