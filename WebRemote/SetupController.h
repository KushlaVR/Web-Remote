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
	int ch4_center;
	int ch4_max;

	int port_addr0;

	int turn_light_limit;
	int reverce_limit;

	int stop_light_duration;//в мілісекундах
	int back_light_timeout;//в мілісекундах

	int gearbox_mode;//0 - manual; 1 - auto

	int gearN; //Положення сервопривода вибору передач - позиція N
	int gear1;//Положення сервопривода вибору передач - передача 1
	int gear2;//Положення сервопривода вибору передач - передача 2

	int gear1_min;//Мінімальні оберти першої передачі
	int gear1_max;//Максимальні оберти першої передачі

	int gear2_min;//Мінімальні оберти другої передачі
	int gear2_max;//Максимальні оберти другої передачі

	int gearR_min;//Мінімальні оберти задньої передачі
	int gearR_max;//Максимальні оберти задньої передачі

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