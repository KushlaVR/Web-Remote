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

	int acceleration_to_100 = 5000;//��� ������� �� 0 �� 100% (ms)

	int gear1_min;//̳������ ������ ����� ��������
	int gear1_max;//���������� ������ ����� ��������

	int gear2_min;//̳������ ������ ����� ��������
	int gear2_max;//���������� ������ ����� ��������

	int gearR_min;//̳������ ������ ������ ��������
	int gearR_max;//���������� ������ ������ ��������

	int acceleratorPedalPosition = 0;//������� �����

	ulong time_stamp = 0;//��� ���������� ���������� ��������
	int speed = 0;//�������� ��������
	int regulatorSpeed = 0;
	int gear = 0;
	bool forwardDirection = true;

	void SetAcceleratorPedalPosition(int position);

	void loop();


	void calculateActuators();
	void printValues();

};

