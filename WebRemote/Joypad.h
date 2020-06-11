#pragma once

#include "Console.h"
#include "Collection.h"
#include "Json.h"
#include <WiFiClient.h>
#include "WebUIController.h"


class Joypadfield : public Item {
public:
	String name;
	double value = 0.0;
	double sent = 0.0;

	Joypadfield(String name);
	~Joypadfield() {};

	bool changed() { return value != sent; }
};

class Joypad :public Item
{
public:
	Joypad();
	~Joypad() {};

	int id;
	IPAddress clientIP;
	WiFiClient client;
	unsigned long report;

	Collection* fields = nullptr;

	bool keepAlive();

	bool processParcel(JsonString* json);
	bool sendValues();

	bool changed();
};

class JoypadCollection : public Collection {
private:
	Collection* fields = nullptr;

public:
	JoypadCollection();
	~JoypadCollection() {};

	unsigned long keepAliveInterval = 5000;
	unsigned long reportAliveInterval = 500;

	Joypad* getById(int id);
	void updateValuesFrom(Joypad * source);
	void setValue(String name, double value);
	double getValue(String name);
	bool setValue(Collection * fields, String name, double value);
	void loop();

};