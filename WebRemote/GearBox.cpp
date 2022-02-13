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
		if (gear == 0) {
			gear = 1;
			regulatorSpeed = 0;
		}
		if (gear == 1) {
			if (speed > (gear2_StartSpeed + gear_actuator_trigger_gap)) {
				gear = 2;
			}
			else {
				regulatorSpeed = map(speed, 0, gear2_StartSpeed + gear_actuator_trigger_gap, gear1_min, gear1_max);
			}
		}
		if (gear == 2) {
			if (speed < (gear2_StartSpeed - gear_actuator_trigger_gap)) {
				gear = 1;
			}
			else {
				regulatorSpeed = map(speed, gear2_StartSpeed - gear_actuator_trigger_gap, 100, gear2_min, gear2_max);
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
