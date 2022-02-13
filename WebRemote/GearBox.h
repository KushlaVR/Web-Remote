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

	bool debug = false;
	bool actuatorsChanged = false;

	int acceleration_to_100 = 5000;//Час розгону від 0 до 100% (ms)

	int gear1_min;//Мінімальні оберти першої передачі
	int gear1_max;//Максимальні оберти першої передачі

	int gear2_min;//Мінімальні оберти другої передачі
	int gear2_max;//Максимальні оберти другої передачі

	int gearR_min;//Мінімальні оберти задньої передачі
	int gearR_max;//Максимальні оберти задньої передачі

	int acceleratorPedalPosition = 0;//Позиція курка

	ulong time_stamp = 0;//Час останнього обчислення швидкості
	int speed = 0;//Фактична швидкість
	int regulatorSpeed = 0;
	int gear = 0;
	bool forwardDirection = true;

	void SetAcceleratorPedalPosition(int position);

	void loop();


	void calculateActuators();
	void printValues();

};

