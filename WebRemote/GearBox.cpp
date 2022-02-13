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
	ulong m = millis();
	actuatorsChanged = false;
	if (acceleratorPedalPosition <= speed) {
		if (speed != acceleratorPedalPosition) {
			speed = acceleratorPedalPosition;
			actuatorsChanged = true;
		}
		time_stamp = m;
	}
	else {
		ulong delta_m = m - time_stamp;
		if (delta_m > 10) {
			double targetAcceleration = ((double)delta_m * 100.0) / (double)acceleration_to_100;
			int newSpeed = speed + targetAcceleration;
			if (newSpeed > acceleratorPedalPosition) newSpeed = acceleratorPedalPosition;
			if (newSpeed != speed) {
				speed = newSpeed;
				time_stamp = m;
				actuatorsChanged = true;
			}
		}
	}
	calculateActuators();
	printValues();
}

void GearBox::calculateActuators()
{
	if (forwardDirection) {

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
