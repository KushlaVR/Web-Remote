/*
 Name:		WemosRemote.ino
 Created:	10/23/2019 7:53:20 PM
 Author:	Віталік
*/



////////////////////////////////////////////// 
//        RemoteXY include library          // 
////////////////////////////////////////////// 

// определение режима соединения и подключение библиотеки RemoteXY  
#define REMOTEXY_MODE__ESP8266WIFI_LIB_POINT
#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h> 
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include <FS.h>
#include <Servo.h>

#include "Console.h"
#include "RoboconMotor.h"
#include "Stearing.h"
#include "Json.h"
#include "SerialController.h"
#include "Blinker.h"
#include "Button.h"
#include <RemoteXY.h> 
#include "WebUIController.h"
#include "SetupController.h"


#define MAX_PWM_VALUE 1024
#define MOSFET_OFF MAX_PWM_VALUE
#define MOSFET_ON 0

#define pinLight D0//Світло

#define pinGunMotor D1//Привід ствола
#define pinGunRollback D2//Відкат ствола

#define pinCabin D6//Привіт кабіни

#define pinLeftMotor D4//лівий борт
#define pinRightMotor D5//правий борт

#define pinTacho D7//Вихід тахометра

#define pinTurbine D8//турбіни
#define pinSmoke D3//ШИМ димогенератора


// конфигурация интерфейса  
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =
	{ 255,6,0,2,0,46,0,10,173,0,
	69,0,32,5,17,17,1,1,0,27,
	5,17,17,50,31,88,0,5,32,0,
	13,42,42,149,173,31,5,32,58,13,
	42,42,149,173,31,3,3,44,30,12,
	32,164,173 };

// структура определяет все переменные и события вашего интерфейса управления 
struct {

	// input variables
	uint8_t fire; // =1 если кнопка нажата, иначе =0 
	int8_t cabin_x; // =-100..100 координата x положения джойстика 
	int8_t cabin_y; // =-100..100 координата y положения джойстика 
	int8_t vehicle_x; // =-100..100 координата x положения джойстика 
	int8_t vehicle_y; // =-100..100 координата y положения джойстика 
	uint8_t engine; // =0 если переключатель в положении A, =1 если в положении B, =2 если в положении C, ... 

	  // output variables
	int16_t sound; // =0 нет звука, иначе ID звука, для примера 1001, смотри список звуков в приложении 

	  // other variable
	uint8_t connect_flag;  // =1 if wire connected, else =0 

} RemoteXY;
#pragma pack(pop)


enum Ignition {
	OFF = 0,
	ON = 1,
	RUN = 2
};


struct State {
	int left;
	int right;
	int cabin;//Швидкість приводу кабіни
	int gun_step;//Куди рухається ствол
	int gun_position;//Положення ствола

	unsigned long fireStart;//Початок вистрілу
	unsigned long firePeak;//Крайня точка вистрілу
	unsigned long fireEnd;//завершення вистрілу

	int rpm;
	int ignition;
	int handled_ignition;

	int light;

} state;

///////////////////////////////////////////// 
//           END RemoteXY include          // 
///////////////////////////////////////////// 

#define REMOTEXY_SERVER_PORT 6377 

bool connected = false;

void handle_StartStop();
void btnFire_Pressed();
void handle_Light();

void gun_Pressed();
void gun_Hold();

void turbine_write(int pin, int value);
void reloadConfig();

ConfigStruct config;

char SSID[32];
char SSID_password[20];

IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
//JoypadCollection joypads = JoypadCollection();

#define SMOKE_PWM_PERIOD 50
Blinker smokeGenerator = Blinker("Smoke");

VirtualBlinker turbineBlinker = VirtualBlinker("Turbine", turbine_write);
Servo turbine = Servo();

//VirtualButton btnStartStop = VirtualButton(startStop_Pressed);
VirtualButton btnFire = VirtualButton(btnFire_Pressed);
//VirtualButton btnLight = VirtualButton(handle_Light);

RoboEffects leftMotorEffect = RoboEffects();
MotorBase* leftMotor = nullptr;

RoboEffects rightMotorEffect = RoboEffects();
MotorBase* rightMotor = nullptr;

RoboEffects cabinMotorEffect = RoboEffects();
MotorBase* cabinMotor = nullptr;

Servo tachoOutput = Servo();

Servo gun = Servo();
Servo gunRollback = Servo();
VirtualButton gunStick = VirtualButton(gun_Pressed, gun_Hold);





void reloadConfig() {
	leftMotor->setWeight(config.inertion);
	rightMotor->setWeight(config.inertion);
	cabinMotor->setWeight(config.cabin_Inertion);
	turbineBlinker.item(1)->offset = (1000 / config.turbine_frequency_min) / 2;
	turbineBlinker.item(2)->offset = (1000 / config.turbine_frequency_min);
}




void handle_StartStop() {
	//state.ignition += 1;
	if (state.handled_ignition != state.ignition) {
		if (state.ignition > Ignition::RUN) {
			state.ignition = Ignition::OFF;
		}
		if (state.ignition == Ignition::OFF) {
			if (smokeGenerator.isRunning()) smokeGenerator.end();
			if (turbineBlinker.isRunning()) turbineBlinker.end();
			if (turbine.attached()) turbine.detach();
			digitalWrite(pinTurbine, 0);
			Serial.println("Engine stopped");
		}
		else if (state.ignition == Ignition::ON) {
			Serial.println("Engine Started");
			if (!smokeGenerator.isRunning()) smokeGenerator.begin();
			if (!turbineBlinker.isRunning()) turbineBlinker.begin();
			if (!turbine.attached()) turbine.attach(pinTurbine);
		}
		else
		{
			Serial.println("Veichle Run");
			if (!smokeGenerator.isRunning()) smokeGenerator.begin();
			if (!turbineBlinker.isRunning()) turbineBlinker.begin();
			if (!turbine.attached()) turbine.attach(pinTurbine);
		};
		state.handled_ignition = state.ignition;
	}
	
}

void btnFire_Pressed() {
	if (state.fireStart != 0) return;
	Serial.print("Fire!");
	state.fireStart = millis();
	state.firePeak = state.fireStart + config.fire_duration / 3;
	state.fireEnd = state.fireStart + config.fire_duration;

	Serial.print(state.fireStart);
	Serial.print(";");
	Serial.print(state.fireStart);
	Serial.print(";");
	Serial.println(state.fireStart);

}

void handle_Light() {
	//Serial.print("Light!");
	if (state.light == 0 && state.ignition > 0) {
		state.light = 1;
		analogWrite(pinLight, map(config.light, 0, 100, 0, MAX_PWM_VALUE));
	}
	else if (state.ignition == 0 && state.light != 0) {
		state.light = 0;
		analogWrite(pinLight, 0);
	}
}

void gun_MakeStep() {
	if (state.gun_step == 0) return;
	state.gun_position += state.gun_step;
	state.gun_position = constrain(state.gun_position, config.gun_min, config.gun_max);
	//joypads.setValue("gun", state.gun_position);
}

void gun_Pressed() {
	gun_MakeStep();
}

void gun_Hold() {
	gun_MakeStep();
}

void turbine_write(int pin, int value) {
	if (!turbine.attached()) return;
	if (value == 0) {
		int speed = map(config.turbine_min, -100, 100, 0, 180);
		turbine.write(speed);
	}
	else {
		int speed = map(config.turbine_max, -100, 100, 0, 180);
		turbine.write(speed);
	}
}

void handleVeichle() {
	//Поточні значення з пульта шофера
	//double left_y = joypads.getValue("left_y");
	//double right_y = joypads.getValue("right_y");

	//Поточні значення з універсального джойстика
	double vehicle_x = RemoteXY.vehicle_x;//joypads.getValue("vehicle_x");
	double vehicle_y = RemoteXY.vehicle_y;//joypads.getValue("vehicle_y");

	int left = 0;
	int right = 0;
	//Якщо на універсальному джойстику не нульові значення, то вони мають пріоритет
	if (vehicle_x != 0 || vehicle_y != 0) {
		if (vehicle_x > 0) {
			left = vehicle_y * (100.0 - vehicle_x) / 100.0;
			right = vehicle_y;
		}
		else if (vehicle_x < 0) {
			left = vehicle_y;
			right = vehicle_y * (100.0 + vehicle_x) / 100.0;
		}
		else {
			left = vehicle_y;
			right = vehicle_y;
		}
	}/*
	else {
		//Обробляємо робоче місце водія
		left = left_y;
		right = right_y;
	}*/

	if (left > 0 && right < 0) {
		right = 0;
	}
	if (right > 0 && left < 0) {
		left = 0;
	}


	int tacho;
	if (state.ignition >= Ignition::ON) {
		//Тахометр
		tacho = abs(left);
		if (abs(right) > tacho) tacho = abs(right);
		int rpm = map(tacho, 0, 100, 800, 2300);
		if (state.rpm != rpm) {
			state.rpm = rpm;
			//Semoke PWM
			int smoke = map(tacho, 0, 100, config.smoke_min, config.smoke_max);
			smoke = map(smoke, 0, 100, 0, SMOKE_PWM_PERIOD);
			smokeGenerator.item(1)->offset = smoke;

			//Turbo pulse freq
			int f = map(tacho, 0, 100, config.turbine_frequency_min, config.turbine_frequency_max);
			turbineBlinker.item(1)->offset = (1000 / f) / 2;
			turbineBlinker.item(2)->offset = (1000 / f);
		}

		if (!tachoOutput.attached()) tachoOutput.attach(pinTacho);
		tachoOutput.write(state.rpm);

	}
	else
	{
		state.rpm = 0;
		if (tachoOutput.attached()) tachoOutput.detach();
	}


	//Двигуни
	if (state.ignition >= Ignition::RUN) {
		if (left != state.left) {
			leftMotor->setSpeed(map(left, 0, 100, 0, 255));
			state.left = left;
			console.print("left=");
			console.println(state.right);
		}

		if (right != state.right) {
			rightMotor->setSpeed(map(right, 0, 100, 0, 255));
			state.right = right;
			console.print("right=");
			console.println(state.right);
		}
	}
	else
	{
		state.left = 0;
		state.right = 0;
	}

	//joypads.setValue("rpm", state.rpm);
	//joypads.setValue("left", state.left);
	//joypads.setValue("right", state.right);
}

void handleCabin() {
	double cabin_x = RemoteXY.cabin_x;//joypads.getValue("cabin_x");

	int cabin = 0;
	if (cabin_x > config.cabin_blind_zone) {
		cabin = map(cabin_x, config.cabin_blind_zone, 100, config.cabin_min, config.cabin_max);
	}
	else if (cabin_x < -config.cabin_blind_zone) {
		cabin = -map(-cabin_x, config.cabin_blind_zone, 100, config.cabin_min, config.cabin_max);
	}

	if (state.cabin != cabin) {
		cabinMotor->setSpeed(cabin);
		state.cabin = cabin;
	}
}

void handleGun() {
	double cabin_y = RemoteXY.cabin_y;//joypads.getValue("cabin_y");
	if (cabin_y > config.gun_blind_zone) {
		gunStick.setValue(HIGH);
		state.gun_step = -1;
	}
	else if (cabin_y < -config.gun_blind_zone) {
		gunStick.setValue(HIGH);
		state.gun_step = 1;
	}
	else
	{
		gunStick.setValue(LOW);
		state.gun_step = 0;
	}

	gun.write(state.gun_position);
}

void handleFire() {
	if (state.fireStart == 0) return;

	if (!gunRollback.attached()) {
		gunRollback.attach(pinGunRollback);
	}
	unsigned long m = millis();
	unsigned long duration = m - state.fireStart;
	unsigned long peakDuration = state.firePeak - state.fireStart;
	unsigned long endDuration = state.fireEnd - state.fireStart;

	int position = config.fire_min;
	if (m < state.firePeak) {
		position = map(duration, 0, peakDuration, config.fire_min, config.fire_max);
		gunRollback.write(position);
		RemoteXY.sound = config.fire_sound;
	}
	else if (m < state.fireEnd) {
		position = map(duration, peakDuration, endDuration, config.fire_max, config.fire_min);
		gunRollback.write(position);
	}
	else {
		state.fireStart = 0;
		RemoteXY.sound = 0;
		gunRollback.write(config.fire_min);
	}
}







void refreshConfig() {
	reloadConfig();
}


void setup()
{
	//state.serialEnabled = true;
	Serial.end();

	//pinMode(pinBuzzer, OUTPUT);
	//digitalWrite(pinBuzzer, LOW);

	Serial.begin(115200);
	Serial.println();
	Serial.println();
	console.output = &Serial;

	analogWriteRange(255);
	String s;
	if (!SPIFFS.begin()) {
		console.println(F("No file system!"));
		console.println(F("Fomating..."));
		if (SPIFFS.format())
			console.println(F("OK"));
		else {
			console.println(F("Fail.... rebooting..."));
			while (true);
		}
	}

	if (SPIFFS.exists("/intro.txt")) {
		File f = SPIFFS.open("/intro.txt", "r");
		s = f.readString();
		console.println(s.c_str());
	}
	else {
		console.println(("Starting..."));
	}

	setupController.cfg = &config;
	setupController.loadConfig();

	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());


	remotexy = new CRemoteXY(RemoteXY_CONF_PROGMEM, &RemoteXY, REMOTEXY_ACCESS_PASSWORD, SSID, SSID_password, REMOTEXY_SERVER_PORT);//RemoteXY_Init();

	console.println("Start");
	webServer.setup();
	webServer.apName = String(SSID);

	setupController.reloadConfig = &refreshConfig;


	leftMotor = new SpeedController("Left motor", pinLeftMotor, &leftMotorEffect);
	//leftMotor = new HBridge("Left motor", pinLeftMotorA, pinLeftMotorB, &leftMotorEffect);
	leftMotor->responder = &console;
	leftMotor->setWeight(config.inertion);
	leftMotor->reset();
	leftMotor->isEnabled = true;


	rightMotor = new SpeedController("Right motor", pinRightMotor, &rightMotorEffect);
	//rightMotor = new HBridge("Right motor", pinRightMotorA, pinRightMotorB, &rightMotorEffect);
	rightMotor->responder = &console;
	rightMotor->setWeight(config.inertion);
	rightMotor->reset();
	rightMotor->isEnabled = true;

	smokeGenerator.startupState = MOSFET_OFF;
	//smokeGenerator.debug = true;
	smokeGenerator.Add(pinSmoke, 0, MOSFET_ON)
		->Add(pinSmoke, 0, MOSFET_OFF)
		->Add(pinSmoke, SMOKE_PWM_PERIOD, MOSFET_OFF)
		->end()
		->printValues();


	turbineBlinker.Add(pinTurbine, 0, 1)
		->Add(pinTurbine, 0, 0)
		->Add(pinTurbine, 0, 0)
		->end()
		->printValues();


	cabinMotor = new SpeedController("Cabin", pinCabin, &cabinMotorEffect);
	cabinMotor->responder = &console;
	cabinMotor->setWeight(config.cabin_Inertion);
	cabinMotor->reset();
	cabinMotor->isEnabled = true;

	gunStick.bounce = 20;
	gunStick.holdInterval = 50;

	state.gun_position = config.gun_min + ((config.gun_max - config.gun_min) / 2);
	gun.attach(pinGunMotor);
	gun.write(state.gun_position);

	state.ignition = Ignition::OFF;
	pinMode(pinTurbine, OUTPUT);

	digitalWrite(pinLight, 0);
	pinMode(pinLight, OUTPUT);


}


void loop()
{
	RemoteXY_Handler();
	webServer.loop();

	if (RemoteXY.connect_flag == 1) {
		state.ignition = RemoteXY.engine;
		btnFire.setValue(RemoteXY.fire);
		handleVeichle();
		handleCabin();
		handleGun();
		handleFire();
	}
	else {
		state.ignition = 0;
	}

	smokeGenerator.loop();
	turbineBlinker.loop();
	leftMotor->loop();
	rightMotor->loop();
	cabinMotor->loop();
	//buttons
	handle_StartStop();
	btnFire.handle();
	handle_Light();
	gunStick.handle();

}
