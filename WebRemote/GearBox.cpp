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
	if (acceleratorPedalPosition <= speed) {
		speed = acceleratorPedalPosition;
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
				Serial.print("Speed=>");
				Serial.println(speed);
				time_stamp = m;
			}
		}
	}
}
