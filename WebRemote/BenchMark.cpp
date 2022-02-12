#include "BenchMark.h"



BenchMark::BenchMark()
{
}


BenchMark::~BenchMark()
{
}

void BenchMark::SetFakeValue(ulong v)
{
	if (v > 180)
	{
		length = v;
	}
	else
	{
		length = 1000 + ((v * 1000.0) / 180.0);
	}
	lastMeasure = millis();
}

void BenchMark::Start()
{
	lastMeasure = millis();
	m = micros();
}

void BenchMark::Stop()
{
	length = micros() - m;
}

bool BenchMark::isActual()
{
	return ((millis() - lastMeasure) <= inteval);
}

void BenchMark::loop()
{
	ulong newPos = 0;
	if (ImpulsLength != length) {
		ImpulsLength = length;
		newPos = convert(ImpulsLength);

		if (pos != newPos) {
			isChanged = true;
			pos = newPos;
		}
		else
		{
			isChanged = false;
		}
	}
	else
		isChanged = false;
}

int BenchMark::convert(ulong value)
{
	if (IN_min < IN_max) {
		if (value < IN_min) {
			return OUT_min;
		}
		else if (value > IN_max) {
			return OUT_max;
		}
		else {
			return map(value, IN_min, IN_max, OUT_min, OUT_max);
		}
	}
	else
	{
		if (value < IN_max) {
			return OUT_max;
		}
		else if (value > IN_min) {
			return OUT_min;
		}
		else {
			return map(value, IN_min, IN_max, OUT_min, OUT_max);
		}
	}
}

bool BenchMark::isValid()
{
	if (IN_min < IN_max)
		return (ImpulsLength >= IN_min) && (ImpulsLength <= IN_max);
	else
		return (ImpulsLength >= IN_max) && (ImpulsLength <= IN_min);

}
