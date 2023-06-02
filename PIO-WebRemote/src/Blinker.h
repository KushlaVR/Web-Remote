#pragma once

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif
#include "Console.h"

class BlinkerItem
{
public:
	int iteration;
	BlinkerItem();
	~BlinkerItem(){};
	BlinkerItem *next;
	int pin;
	unsigned long offset;
	int value;
};

class Blinker
{
	BlinkerItem *first;
	BlinkerItem *last;
	BlinkerItem *current;

	String name;
	unsigned long startTime = 0;

protected:
	void (*onWrite)(int pin, int value);

public:
	bool debug = false;
	bool repeat = true;
	int startupState = 0;
	Blinker(String name);
	~Blinker();
	void loop();
	Blinker *Add(int pin, unsigned long offset, int value);
	Blinker *begin()
	{
		current = first;
		startTime = millis();
		return this;
	};
	Blinker *end();
	virtual void write(int pin, int value);
	void printValues();
	BlinkerItem *item(int index);
	bool isRunning() { return startTime != 0; };
	void attachWriteEvent(void (*onWrite)(int pin, int value));
};

class Beeper : public Blinker
{

public:
	Beeper(String name) : Blinker(name) {}
	~Beeper() {}
	virtual void write(int pin, int value);
};

class VirtualBlinker : public Blinker
{

public:
	VirtualBlinker(String name, void (*writeMethode)(int pin, int value)) : Blinker(name)
	{
		this->writeMethode = writeMethode;
	}
	~VirtualBlinker() {}

	void (*writeMethode)(int pin, int value);

	virtual void write(int pin, int value);
};
