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
#include <Wire.h>
#include "Json.h"
#include "Button.h"
#include "Console.h"
#include "Collection.h"
#include "WebUIController.h"
#include "SetupController.h"
#include "Joypad.h"
#include "RoboconMotor.h"
#include "Stearing.h"
#include "Blinker.h"
#include "PCF8574.h"
#include "BenchMark.h"
#include "DFRobotDFPlayerMini.h"


/*

+X - повороти

+Y - газ

+На кнопку 4 канала можно вывести свет. Как раньше делали, первое нажатие габариты, второе ближний, третье дальний. Долгое нажатие- аварийка.

+На крутилку 3 канала хочу сделать включение проблесковых маячков (малый поворот) и маячки плюс сирена(большой поворот)

 +А на 6 канал включение света в кабине(первое положение) и в будке(второе положение)

*/

#define MAX_PWM_VALUE 1024
#define MOSFET_OFF MAX_PWM_VALUE
#define MOSFET_ON 0

#define PIN_SERVO_X 14//ok
#define PIN_SERVO_Y 12//ok
#define PIN_SERVO_CH3 13//ok
#define PIN_SERVO_CH4 16//OK D0 - no interrupt
#define PIN_SERVO_CH5 2//D4 - 
#define PIN_SERVO_CH6 15

#define PIN_I2C_SCL D1 //pcf8574
#define PIN_I2C_SDA D2 //pcf8574

#define PIN_EXT0_STOP 3
#define PIN_EXT0_BACK 2
#define PIN_EXT0_LEFT 0
#define PIN_EXT0_RIGHT 1
#define PIN_EXT0_FOG 4
#define PIN_EXT0_HIGH_LIGHT 5
#define PIN_EXT0_HEAD_LIGHT 6
#define PIN_EXT0_PARKING_LIGHT 7

#define PIN_EXT0_CABIN PIN_EXT0_FOG


#define lightOFF HIGH
#define lightON LOW


enum Ignition {
	OFF = 0,
	ON = 1,
	RUN = 2
};

struct State {
	int stearing;
	int speed;

	bool alarm;

	bool parkingLight;
	bool headLight;
	bool highLight;
	bool cabinLight;
	bool interiorLight;

} state;


RoboEffects motorEffect = RoboEffects();
MotorBase* motor = nullptr;//= RoboMotor("motor", pinMotorA, pinMotorB, &motorEffect);
Stearing stearingServo = Stearing(PIN_SERVO_X);


PCF8574* portExt0;// = PCF8574(0x3F);

extBlinker* stopLight;
extBlinker* leftLight;
extBlinker* rightLight;
extBlinker* BackLight;
extBlinker* alarmBlinker;

void reloadConfig();

ConfigStruct config;

char SSID[32];
char SSID_password[20];

IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
JoypadCollection joypads = JoypadCollection();


void btnLight_Press();
void btnLight_Hold();
void btnLight_Release();
VirtualButton btnLight = VirtualButton(btnLight_Press, btnLight_Hold, btnLight_Release);

bool alarmChanged = false;
void btnLight_Press() {
	Serial.println("btnLight_Press");
}

void btnLight_Hold() {
	Serial.println("btnLight_Hold");
	if (state.alarm)
		state.alarm = false;
	else
		state.alarm = true;
	alarmChanged = true;
}

void btnLight_Release() {
	Serial.println("btnLight_Release");
	if (alarmChanged) {
		alarmChanged = false;
		return;
	}
	if (state.parkingLight == false) {
		state.parkingLight = true;
		state.headLight = false;
		state.highLight = false;
		Serial.println("parking");
	}
	else if (state.headLight == false) {
		state.parkingLight = true;
		state.headLight = true;
		state.highLight = false;
		Serial.println("head");
	}
	else if (state.highLight == false) {
		state.parkingLight = true;
		state.headLight = true;
		state.highLight = true;
		Serial.println("high");
	}
	else {
		state.parkingLight = false;
		state.headLight = false;
		state.highLight = false;
		Serial.println("off");
	}

}


void setupMotor() {
	if (motor != nullptr) {
		motor->isEnabled = false;
		motor->reset();
		motor->loop();
		delete motor;
		motor = nullptr;
	}
	if (motor == nullptr) {
		motor = new SpeedController("Speed reg Y", PIN_SERVO_Y, &motorEffect);
	}

	motor->responder = &console;
	motor->setWeight(config.inertion);
	motor->reset();
	motor->isEnabled = true;
}



void reloadConfig() {

	stopLight->item(2)->offset = config.stop_light_duration;
	BackLight->item(1)->offset = config.back_light_timeout;

	stearingServo.max_left = config.ch1_max;
	stearingServo.max_right = config.ch1_min;
	stearingServo.center = config.ch1_center;
	stearingServo.setPosition(0, (PotentiometerLinearity)config.stearing_linearity);
	stearingServo.isEnabled = true;

	setupMotor();

	/*input_X.IN_max = config.ch1_max;
	input_X.IN_center = config.ch1_center;
	input_X.IN_min = config.ch1_min;

	input_Y.IN_max = config.ch2_max;
	input_Y.IN_center = config.ch2_center;
	input_Y.IN_min = config.ch2_min;

	input_CH3.IN_max = config.ch3_max;
	input_CH3.IN_center = config.ch3_center;
	input_CH3.IN_min = config.ch3_min;

	input_CH4.IN_max = config.ch4_max;
	input_CH4.IN_center = config.ch4_min + ((config.ch4_max - config.ch4_min) / 2);
	input_CH4.IN_min = config.ch4_min;

	input_CH5.IN_max = config.ch5_max;
	input_CH5.IN_center = config.ch5_center;
	input_CH5.IN_min = config.ch5_min;

	input_CH6.IN_max = config.ch6_max;
	input_CH6.IN_center = config.ch6_center;
	input_CH6.IN_min = config.ch6_min;*/

}


void setupBlinkers() {


	stopLight = new extBlinker("Stop light", portExt0);
	leftLight = new extBlinker("Left light", portExt0);
	rightLight = new extBlinker("Right light", portExt0);
	BackLight = new extBlinker("Back light", portExt0);
	alarmBlinker = new extBlinker("Alarm light", portExt0);
	//sirenBlinker = new extBlinker("Siren light", portExt1);

	stopLight
		->Add(PIN_EXT0_STOP, 0, lightOFF)
		->Add(PIN_EXT0_STOP, 0, lightON)
		->Add(PIN_EXT0_STOP, config.stop_light_duration, lightOFF)
		->repeat = false;
	stopLight->debug = true;
	stopLight->offLevel = lightOFF;

	//Налаштування поворотників
	leftLight
		->Add(PIN_EXT0_LEFT, 0, lightON)
		->Add(PIN_EXT0_LEFT, 500, lightOFF)
		->Add(PIN_EXT0_LEFT, 1000, lightOFF);
	leftLight->offLevel = lightOFF;
	//leftLight.debug = true;
	//serialController.leftLight = &leftLight;

	rightLight
		->Add(PIN_EXT0_RIGHT, 0, lightON)
		->Add(PIN_EXT0_RIGHT, 500, lightOFF)
		->Add(PIN_EXT0_RIGHT, 1000, lightOFF);
	rightLight->offLevel = lightOFF;
	//rightLight.debug = true;
	//serialController.rightLight = &rightLight;


	BackLight
		->Add(PIN_EXT0_BACK, 0, lightON)
		->Add(PIN_EXT0_BACK, 500, lightON)
		->repeat = false;
	BackLight->offLevel = lightOFF;

	alarmBlinker
		->Add(PIN_EXT0_LEFT, 0, lightON)
		->Add(PIN_EXT0_RIGHT, 0, lightON)
		->Add(PIN_EXT0_LEFT, 500, lightOFF)
		->Add(PIN_EXT0_RIGHT, 500, lightOFF)
		->Add(PIN_EXT0_LEFT, 1000, lightOFF)
		->Add(PIN_EXT0_RIGHT, 1000, lightOFF)
		->repeat = true;
	alarmBlinker->offLevel = lightOFF;


	//sirenBlinker
	//	->Add(PIN_EXT1_BLINKER_LEFT, 0, lightON)
	//	->Add(PIN_EXT1_BLINKER_RIGHT, 0, lightOFF)
	//	->Add(PIN_EXT1_BLINKER_LEFT, 500, lightOFF)
	//	->Add(PIN_EXT1_BLINKER_RIGHT, 500, lightON)
	//	->Add(PIN_EXT1_BLINKER_LEFT, 1000, lightOFF)
	//	->Add(PIN_EXT1_BLINKER_RIGHT, 1000, lightOFF)
	//	->repeat = true;
	//sirenBlinker->offLevel = lightOFF;

}

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
	webServer.on("/api/values", HTTPMethod::HTTP_GET, Values_Get);
	webServer.on("/api/EventSourceName", EventSourceName);
	webServer.on("/api/events", Events);
	webServer.on("/api/post", HTTPMethod::HTTP_POST, Post);



	portExt0 = new PCF8574(config.port_addr0);
	portExt0->begin(PIN_I2C_SDA, PIN_I2C_SCL);
	portExt0->write8(0x00);


	delay(1000);
	portExt0->write8(0xFF);

	setupBlinkers();

	reloadConfig();

}
//
//void printValues(JsonString* out)
//{
//	out->beginObject();
//	out->AddValue("ch1_val", String(input_X.ImpulsLength));
//	out->AddValue("ch2_val", String(input_Y.ImpulsLength));
//	out->AddValue("ch3_val", String(input_CH3.ImpulsLength));
//	out->AddValue("ch4_val", String(input_CH4.ImpulsLength));
//	out->AddValue("ch5_val", String(input_CH5.ImpulsLength));
//	out->AddValue("ch6_val", String(input_CH6.ImpulsLength));
//	out->endObject();
//}

void Values_Get() {
	JsonString ret = JsonString();
	//printValues(&ret);
	webServer.jsonOk(&ret);
}

void EventSourceName() {
	webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	webServer.sendHeader("Pragma", "no-cache");
	webServer.sendHeader("Expires", "-1");
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

	Joypad* j = new Joypad();
	j->client = webServer.client();
	j->clientIP = webServer.client().remoteIP();
	joypads.add(j);

	String ret = "http://" + apIP.toString() + ":80/api/events?{\"client\":\"" + String(j->id) + "\"}";

	webServer.send(200, "text/plain", ret);

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

		console.printf("client:%i\n", id);

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


void handleVeichle() {
	//joypads.setValue("rpm", state.rpm);
	//joypads.setValue("left", state.left);
	//joypads.setValue("right", state.right);

	int speed = joypads.getValue("pot_right_y");
	state.stearing = joypads.getValue("pot_left_x");

	btnLight.setValue(joypads.getValue("light"));

	handleStearing();
	handleSpeed(speed);
	handleHeadLight();
}


void handleStearing() {

	if (!stearingServo.isEnabled) stearingServo.isEnabled = true;
	stearingServo.setPosition(state.stearing, (PotentiometerLinearity)config.stearing_linearity);


	if (state.stearing == 0) {
		if (leftLight->isRunning())
		{
			Serial.println("left end");
			leftLight->end();
		}
		if (rightLight->isRunning())
		{
			Serial.println("right end");
			rightLight->end();
		}
		if (state.alarm) {
			if (!alarmBlinker->isRunning()) alarmBlinker->begin();
		}
		else {
			if (alarmBlinker->isRunning()) alarmBlinker->end();
		}

		return;
	}

	//Проміжки включення правого/лівого поворота
	int center = 0;

	int leftLimit = center - 20;
	int rightLimit = center + 20;

	int delta = center + state.stearing;

	//Serial.printf("delta=%i;ll=%i;rl=%i;c=%i\n", delta, leftLimit, rightLimit, center);

	if (delta < leftLimit) {
		if (rightLight->isRunning()) {
			Serial.println("right end");
			rightLight->end();
		}
		if (!leftLight->isRunning())
		{
			Serial.println("left begin");
			leftLight->begin();
		}
	}
	else if (delta > rightLimit) {
		if (leftLight->isRunning())
		{
			Serial.println("left end");
			leftLight->end();
		}
		if (!rightLight->isRunning())
		{
			Serial.println("right begin");
			rightLight->begin();
		}
	}
	else {
		if (leftLight->isRunning())
		{
			Serial.println("left end");
			leftLight->end();
		}
		if (rightLight->isRunning())
		{
			Serial.println("right end");
			rightLight->end();
		}
	}

	if (state.alarm && !leftLight->isRunning() && !rightLight->isRunning()) {
		if (!alarmBlinker->isRunning()) alarmBlinker->begin();
	}
	else {
		if (alarmBlinker->isRunning()) alarmBlinker->end();
	}

}


void handleSpeed(int speed) {


	int forward_limit = 10;
	int reverce_limit = -5;
	if (speed > forward_limit) {
		if (BackLight->isRunning()) {
			Serial.println("BackLight end");
			BackLight->end();
		}
	}
	else if (speed < reverce_limit) {
		if (!BackLight->isRunning()) {
			Serial.println("BackLight begin");
			BackLight->begin();
		}
	}


	if (state.speed != speed) {
		//швидкість змінилась
		if (abs(state.speed) > 5)//перебуваємо в русі
		{
			if (abs(speed) < 5) {//Зупинка
				stopLight->begin();
			}
			else
			{
				if (abs(speed) > abs(state.speed)) {//Швидкість зросла
					stopLight->end();
				}
				else {
					if (abs(state.speed - speed) > 5) {//Швидкість впала більше ніж на 10
						stopLight->begin();
					}
				}
			}
		}
		state.speed = speed;

		if (!motor->isEnabled) motor->isEnabled = true;
		motor->setSpeed(state.speed);

	}

	
}


void handleHeadLight() {


	if (state.parkingLight) {
		portExt0->write(PIN_EXT0_PARKING_LIGHT, lightON);
	}
	else {
		portExt0->write(PIN_EXT0_PARKING_LIGHT, lightOFF);
	}

	if (state.headLight)
		portExt0->write(PIN_EXT0_HEAD_LIGHT, lightON);
	else
		portExt0->write(PIN_EXT0_HEAD_LIGHT, lightOFF);

	if (state.highLight)
		portExt0->write(PIN_EXT0_HIGH_LIGHT, lightON);
	else
		portExt0->write(PIN_EXT0_HIGH_LIGHT, lightOFF);

}


void loop()
{
	dnsServer.processNextRequest();
	joypads.loop();
	webServer.loop();

	if (joypads.getCount() > 0) {
		handleVeichle();
	}

	stearingServo.loop();
	motor->loop();

	btnLight.handle();

	stopLight->loop();
	leftLight->loop();
	rightLight->loop();
	BackLight->loop();
	alarmBlinker->loop();

}