#include "GearBox.h"

GearBox::GearBox()
{
}

GearBox::~GearBox()
{
}

void GearBox::SetAcceleratorPedalPosition(int position)
{
	if (acceleratorPedalPosition != position) {
		acceleratorPedalPosition = position;
		Serial.print("Accelerator=>");
		Serial.println(position);
	}
}

void GearBox::loop()
{
	actuatorsChanged = false;
	ulong m = millis();
	ulong delta_m = m - time_stamp;
	if (delta_m > 10) {
		if (acceleratorPedalPosition > speed) {
			double targetAcceleration = ((double)delta_m * 100.0) / (double)acceleration_to_100;
			int newSpeed = speed + targetAcceleration;
			if (newSpeed > 100) newSpeed = 100;
			if (newSpeed > acceleratorPedalPosition) newSpeed = acceleratorPedalPosition;
			if (newSpeed != speed) {
				speed = newSpeed;
				time_stamp = m;
				actuatorsChanged = true;
			}
		}
		else if (acceleratorPedalPosition < speed)
		{
			double targetAcceleration = ((double)delta_m * 100.0 * 4.0) / (double)acceleration_to_100;
			int newSpeed = speed - targetAcceleration;
			if (newSpeed < 0) newSpeed = 0;
			if (newSpeed != speed) {
				speed = newSpeed;
				time_stamp = m;
				actuatorsChanged = true;
			}
		}
		else {
			time_stamp = m;
		}
	}

	calculateActuators();
	printValues();
}

void GearBox::calculateActuators()
{

	if (forwardDirection) {

		int gap_start = gear2_StartSpeed - gear_actuator_trigger_gap;
		int gap_end = gear2_StartSpeed + gear_actuator_trigger_gap;

		if (gear == 0) {
			gear = 1;
			regulatorSpeed = 0;
		}
		if (gear == 1) {
			if (speed > gap_end) {
				gear = 2;
				calculateActuators();
				return;
			}
			else if (speed > gap_start) {
				regulatorSpeed = map(speed, gap_start, gap_end, gear1_start, gear1_max);
			}
			else {
				regulatorSpeed = map(speed, 0, gap_start, gear1_min, gear1_start);
			}
		}
		if (gear == 2) {
			if (speed < gap_start) {
				gear = 1;
				calculateActuators();
				return;
			}
			else if (speed < gap_end) {
				regulatorSpeed = map(speed, gap_start, gap_end, gear2_min, gear2_start);

			}
			else {
				regulatorSpeed = map(speed, gap_end, 100, gear2_start, gear2_max);
			}
		}
	}
	else {
		gear = 1;
		regulatorSpeed = map(speed, 0, 100, gearR_min, gearR_max);
	}
}

void GearBox::printValues()
{
	if (debug && actuatorsChanged) {
		Serial.print("speed=");
		Serial.print(speed);
		Serial.print("; forward=");
		Serial.print(forwardDirection);
		Serial.print("; gear=");
		Serial.print(gear);
		Serial.print("; reg=");
		Serial.println(regulatorSpeed);
	}
}
