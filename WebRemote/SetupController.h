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

	int min_speed;
	int inertion;

	int cabin_Inertion;
	int cabin_min;
	int cabin_max;
	int cabin_blind_zone = 30;

	int gun_min;
	int gun_max;
	int gun_blind_zone = 30;
	//unsigned long gun_duration;

	int fire_min;
	int fire_max;
	int fire_duration;

	int turbine_min;//0..180
	int turbine_max;//0..180
	int turbine_frequency_min;
	int turbine_frequency_max;

	int smoke_min;//0..100
	int smoke_max;//0..100

	int light;//0..100%

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