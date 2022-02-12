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

	int acceleration_to_100 = 5000;//��� ������� �� 0 �� 100% (ms)
	int acceleratorPedalPosition = 0;//������� �����
	ulong time_stamp = 0;//��� ���������� ���������� ��������
	int speed = 0;//�������� ��������
	int regulatorSpeed = 0;
	int gear = 0;

	void SetAcceleratorPedalPosition(int position);

	void loop();

};

