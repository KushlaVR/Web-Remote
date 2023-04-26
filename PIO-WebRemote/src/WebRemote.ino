/*
 Name:		WebRemote.ino
 Created:	4/23/2020 11:30:33 PM
 Author:	Віталік
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h>
#include "Json.h"
#include "Button.h"
#include "Console.h"
#include "Collection.h"
#include "WebUIController.h"
#include "SetupController.h"
#include "Joypad.h"
#include "RoboconMotor.h"
#include "Blinker.h"


#define MAX_PWM_VALUE 1024
#define MOSFET_OFF MAX_PWM_VALUE
#define MOSFET_ON 0

#define pinLight D0//Світло

#define pinGunMotor D1//Привід ствола
#define pinGunRollback D2//Відкат ствола

#define pinCabin D6//Привіт кабіни

#define pinLeftMotor D4//лівий борт
#define pinRightMotor D5//правий борт

//#define pinTacho D7//Вихід тахометра
#define pinFireLed D7//Вихід тахометра

#define pinTurbine D8//турбіни
#define pinSmoke D3//ШИМ димогенератора

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

	unsigned long fireAnimationStart;//Початок вистрілу
	unsigned long fireStart;//Початок вистрілу
	unsigned long firePeak;//Крайня точка вистрілу
	unsigned long fireEnd;//завершення вистрілу
	unsigned long fireLedStart;//Засвічуємо діод
	unsigned long fireLedEnd;//гасимо діод
	unsigned long fireLedPWM;//гасимо діод

	int rpm;
	int ignition;

	int light;

} state;

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
JoypadCollection joypads = JoypadCollection();

#define SMOKE_PWM_PERIOD 50
Blinker smokeGenerator = Blinker("Smoke");

VirtualBlinker turbineBlinker = VirtualBlinker("Turbine", turbine_write);
Servo turbine = Servo();

VirtualButton btnStartStop = VirtualButton(handle_StartStop);
VirtualButton btnFire = VirtualButton(btnFire_Pressed);
VirtualButton btnLight = VirtualButton(handle_Light);

RoboEffects leftMotorEffect = RoboEffects();
MotorBase* leftMotor = nullptr;

RoboEffects rightMotorEffect = RoboEffects();
MotorBase* rightMotor = nullptr;

RoboEffects cabinMotorEffect = RoboEffects();
MotorBase* cabinMotor = nullptr;

//Servo tachoOutput = Servo();

Servo gun = Servo();
Servo gunRollback = Servo();
VirtualButton gunStick = VirtualButton(gun_Pressed, gun_Hold);


void setup()
{
	Serial.begin(115200);
	console.output = &Serial;
	console.println();
	console.println();
	console.println();

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
	setupController.reloadConfig = reloadConfig;
	setupController.loadConfig();

	WiFi.begin();
	WiFi.disconnect();
	WiFi.mode(WIFI_AP);

	s = config.ssid + "_" + WiFi.macAddress();
	s.replace(":", "");
	strcpy(&SSID[0], s.c_str());

	s = config.password;
	strcpy(&SSID_password[0], s.c_str());

	WiFi.softAP(SSID, SSID_password);

	/* Setup the DNS server redirecting all the domains to the apIP */
	dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
	dnsServer.start(DNS_PORT, "*", apIP);

	console.println("");
	console.println(apIP.toString());

	webServer.setup();
	webServer.on("/api/EventSourceName", EventSourceName);
	webServer.on("/api/events", Events);
	webServer.on("/api/post", HTTPMethod::HTTP_POST, Post);

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

	btnStartStop.bounce = 0;
	btnFire.bounce = 0;
	btnLight.bounce = 0;

	pinMode(pinFireLed, OUTPUT);

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

void reloadConfig() {
	leftMotor->setWeight(config.inertion);
	rightMotor->setWeight(config.inertion);
	cabinMotor->setWeight(config.cabin_Inertion);
	turbineBlinker.item(1)->offset = (1000 / config.turbine_frequency_min) / 2;
	turbineBlinker.item(2)->offset = (1000 / config.turbine_frequency_min);
}


void EventSourceName() {
	console.println(webServer.uri());
	webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	webServer.sendHeader("Pragma", "no-cache");
	webServer.sendHeader("Expires", "-1");
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

	Joypad* j = new Joypad();
	j->client = webServer.client();
	j->clientIP = webServer.client().remoteIP();
	joypads.add(j);

	String ret = "http://" + apIP.toString() + ":80/api/events?{\"client\":\"" + String(j->id) + "\"}";
	yield();
	webServer.send(200, "text/plain", ret);
	console.println(ret);

}

void Events() {
	console.println(webServer.uri());
	WiFiClient client = webServer.client();

	String s = webServer.argName(0);
	JsonString json = "";
	json += s;
	int id = json.getInt("client");

	//console.printf("client:%i", id);

	Joypad* j = joypads.getById(id);
	if (j == nullptr) {
		console.printf("Unauthorized client %i\n", id);
		webServer.handleNotFound();
		return;
	}
	if (client.remoteIP() != j->clientIP) {
		console.printf("wrong IP", id);
		joypads.remove(j);
		webServer.handleNotFound();
		return;
	}
	j->client = client;
	client.setNoDelay(true);
	client.setSync(true);
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN); // the payload can go on forever
	webServer.sendContent_P(PSTR("HTTP/1.1 200 OK\nContent-Type: text/event-stream;\nConnection: keep-alive\nCache-Control: no-cache\nAccess-Control-Allow-Origin: *\n\n"));
	console.flush();
}

void Post() {
	if (webServer.hasArg("plain")) {
		String s = webServer.arg("plain");
		JsonString json = "";
		json += s;
		int id = json.getInt("client");

		//console.printf("client:%i\n", id);

		Joypad* j = joypads.getById(id);
		if (j == nullptr) {
			webServer.handleNotFound();
			return;
		}
		webServer.Ok();
		if (j->processParcel(&json)) {
			joypads.updateValuesFrom(j);
		}
	}
	else
	{
		webServer.Ok();
	}
}

void handle_StartStop() {
	state.ignition += 1;
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
}

void btnFire_Pressed() {
	if (state.fireAnimationStart != 0) return;
	
	state.fireAnimationStart = millis();
	state.fireStart = state.fireAnimationStart + config.fire_rollback_start;
	state.firePeak = state.fireAnimationStart + config.fire_rollback_peak;
	state.fireEnd = state.fireAnimationStart + config.fire_rollback_end;
	state.fireLedStart = state.fireAnimationStart + config.fire_led_start;
	state.fireLedEnd = state.fireAnimationStart + config.fire_led_end;
	state.fireLedPWM = map(config.fire_led_pwm, 0, 100, 0, MAX_PWM_VALUE);

	Serial.print("Fire! start=");
	Serial.print(state.fireStart);
	Serial.print("; peak=");
	Serial.print(state.firePeak);
	Serial.print("; end=");
	Serial.print(state.fireEnd);
	Serial.print("; led-start=");
	Serial.print(state.fireLedStart);
	Serial.print("; led-end=");
	Serial.print(state.fireLedEnd);
	Serial.print("; PWM=");
	Serial.println(state.fireLedPWM);

}

void handle_Light() {
	Serial.print("Light!");
	if (state.light == 0) {
		state.light = 1;
		analogWrite(pinLight, map(config.light, 0, 100, 0, MAX_PWM_VALUE));
	}
	else {
		state.light = 0;
		analogWrite(pinLight, 0);
	}
}

void gun_MakeStep() {
	if (state.gun_step == 0) return;
	state.gun_position += state.gun_step;
	state.gun_position = constrain(state.gun_position, config.gun_min, config.gun_max);
	joypads.setValue("gun", state.gun_position);
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
	double left_y = joypads.getValue("left_y");
	double right_y = joypads.getValue("right_y");

	//Поточні значення з універсального джойстика
	double vehicle_x = joypads.getValue("vehicle_x");
	double vehicle_y = joypads.getValue("vehicle_y");

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
	}
	else {
		//Обробляємо робоче місце водія
		left = left_y;
		right = right_y;
	}

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

		//if (!tachoOutput.attached()) tachoOutput.attach(pinTacho);
		//tachoOutput.write(state.rpm);

	}
	else
	{
		state.rpm = 0;
		//if (tachoOutput.attached()) tachoOutput.detach();
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

	joypads.setValue("rpm", state.rpm);
	joypads.setValue("left", state.left);
	joypads.setValue("right", state.right);
}

void handleCabin() {
	double cabin_x = joypads.getValue("cabin_x");

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
	double cabin_y = joypads.getValue("cabin_y");
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
	if (state.fireAnimationStart == 0) return;

	if (!gunRollback.attached()) {
		gunRollback.attach(pinGunRollback);
	}
	unsigned long m = millis();

	if (m > state.fireStart) {

		unsigned long duration = m - state.fireStart;
		unsigned long peakDuration = state.firePeak - state.fireStart;
		unsigned long endDuration = state.fireEnd - state.fireStart;
		
		int position = config.fire_min;

		if (m < state.firePeak) {
			position = map(duration, 0, peakDuration, config.fire_min, config.fire_max);
			Serial.print("+");
			gunRollback.write(position);
		}
		else if (m < state.fireEnd) {
			position = map(duration, peakDuration, endDuration, config.fire_max, config.fire_min);
			Serial.print("-");
			gunRollback.write(position);
		}
		else {
			gunRollback.write(config.fire_min);
		}

		Serial.print(position); Serial.print("; "); Serial.print(duration); Serial.println();
	}
	

	if (m < state.fireLedStart) {
		analogWrite(pinFireLed, 0);
	}
	else if (m < state.fireLedEnd) {
		analogWrite(pinFireLed, state.fireLedPWM);
	}
	else {
		analogWrite(pinFireLed, 0);
	}

	if (m > state.fireEnd && m > state.fireLedEnd) {
		state.fireAnimationStart = 0;
		//gunRollback.write(config.fire_min);
		//gunRollback.detach();
	}
}

void loop()
{
	dnsServer.processNextRequest();
	joypads.loop();
	webServer.loop();

	if (joypads.getCount() > 0) {
		btnStartStop.setValue(joypads.getValue("start"));
		btnFire.setValue(joypads.getValue("fire"));
		btnLight.setValue(joypads.getValue("light"));
		handleVeichle();
		handleCabin();
		handleGun();
		handleFire();
	}

	smokeGenerator.loop();
	turbineBlinker.loop();
	leftMotor->loop();
	rightMotor->loop();
	cabinMotor->loop();
	//buttons
	btnStartStop.handle();
	btnFire.handle();
	btnLight.handle();
	gunStick.handle();
}
