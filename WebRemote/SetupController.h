#pragma once
#pragma once
#include "WebUIController.h"

enum PotentiometerLinearity {
	Linear,
	X2_div_X
};



struct ConfigStruct {
public:
	String ssid;
	String password;

	int ch1_min;
	int ch1_center;
	int ch1_max;

	int ch2_min;
	int ch2_center;
	int ch2_max;

	int ch3_min;
	int ch3_center;
	int ch3_max;

	int ch4_min;
	int ch4_max;

	int ch5_min;
	int ch5_center;
	int ch5_max;

	int ch6_min;
	int ch6_center;
	int ch6_max;

	int port_addr0;
	int port_addr1;

	int turn_light_limit;
	int reverce_limit;

	int gear0;
	int gear1;
	int gear2;

	int stop_light_duration;//в мілісекундах
	int back_light_timeout;//в мілісекундах
};

typedef void(*myFunctionPointer) ();

class SetupController
{
public:
	ConfigStruct* cfg;

	SetupController();
	~SetupController();

	void loadConfig();
	void saveConfig();
	void printConfig(JsonString* out);

	static void Setup_Get();
	static void Setup_Post();

	myFunctionPointer reloadConfig = nullptr;
};


extern SetupController setupController;