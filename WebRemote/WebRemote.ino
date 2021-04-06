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

#define PIN_EXT1_BLINKER_LEFT 0
#define PIN_EXT1_BLINKER_RIGHT 1
#define PIN_EXT1_PARKING_LIGHT 2
#define PIN_EXT1_INTERIOR 3

#define lightOFF HIGH
#define lightON LOW


enum Ignition {
	OFF = 0,
	ON = 1,
	RUN = 2
};

struct State {
	int speed;

	bool alarm;

	bool parkingLight;
	bool headLight;
	bool highLight;
	bool cabinLight;
	bool interiorLight;

	int siren;

} state;


BenchMark input_X = BenchMark();
BenchMark input_Y = BenchMark();
BenchMark input_CH3 = BenchMark();
BenchMark input_CH4 = BenchMark();
BenchMark input_CH5 = BenchMark();
BenchMark input_CH6 = BenchMark();

BenchMark* input_Light = &input_CH4;
BenchMark* input_Siren = &input_CH3;
BenchMark* input_Cabin = &input_CH6;
BenchMark* input_Interior = &input_CH6;

DFRobotDFPlayerMini* myDFPlayer;
bool is_MP3_available = false;

PCF8574* portExt0;// = PCF8574(0x3F);
PCF8574* portExt1;// = PCF8574(0x3F);

extBlinker* stopLight;
extBlinker* leftLight;
extBlinker* rightLight;
extBlinker* BackLight;
extBlinker* alarmBlinker;

extBlinker* sirenBlinker;


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


void btnSiren_Press();
void btnSiren_Hold();
void btnSiren_Release();
VirtualButton btnSiren = VirtualButton(btnSiren_Press, btnSiren_Hold, btnSiren_Release);

void btnSirenSound_Press();
void btnSirenSound_Hold();
void btnSirenSound_Release();
VirtualButton btnSirenSound = VirtualButton(btnSirenSound_Press, btnSirenSound_Hold, btnSirenSound_Release);

void btnInterior_Press();
void btnInterior_Hold();
void btnInterior_Release();
VirtualButton btnInterior = VirtualButton(btnInterior_Press, btnInterior_Hold, btnInterior_Release);

void btnCabin_Press();
void btnCabin_Hold();
void btnCabin_Release();
VirtualButton btnCabin = VirtualButton(btnCabin_Press, btnCabin_Hold, btnCabin_Release);



bool interruptAttached = false;

void ICACHE_RAM_ATTR  pinServo_X_CHANGE() {
	if (digitalRead(PIN_SERVO_X))
		input_X.Start();
	else
		input_X.Stop();
}

void ICACHE_RAM_ATTR  pinServo_Y_CHANGE() {
	if (digitalRead(PIN_SERVO_Y))
		input_Y.Start();
	else
		input_Y.Stop();
}

void ICACHE_RAM_ATTR  pinServo_CH3_CHANGE() {
	if (digitalRead(PIN_SERVO_CH3))
		input_CH3.Start();
	else
		input_CH3.Stop();
}
/*
void ICACHE_RAM_ATTR  pinServo_CH4_CHANGE() {
	if (digitalRead(PIN_SERVO_CH4))
		input_CH4.Start();
	else
		input_CH4.Stop();
}*/


void ICACHE_RAM_ATTR  pinServo_CH5_CHANGE() {
	if (digitalRead(PIN_SERVO_CH4))
		input_CH5.Start();
	else
		input_CH5.Stop();
}

void ICACHE_RAM_ATTR  pinServo_CH6_CHANGE() {
	if (digitalRead(PIN_SERVO_CH6))
		input_CH6.Start();
	else
		input_CH6.Stop();
}



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


void btnSiren_Press() {
	if (!sirenBlinker->isRunning()) sirenBlinker->begin();
};
void btnSiren_Hold() {};
void btnSiren_Release() {
	if (sirenBlinker->isRunning()) sirenBlinker->end();
};

void btnSirenSound_Press() {
	if (is_MP3_available) {
		myDFPlayer->loop(1);
	}
};
void btnSirenSound_Hold() {};
void btnSirenSound_Release() {
	if (is_MP3_available) {
		myDFPlayer->stop();
	}
};


void btnInterior_Press() {
	if (state.interiorLight == 1) {
		state.interiorLight = 0;
		portExt1->set(PIN_EXT1_INTERIOR, lightOFF);
	}
	else {
		state.interiorLight = 1;
		portExt1->set(PIN_EXT1_INTERIOR, lightON);
	}
};
void btnInterior_Hold() {
};
void btnInterior_Release() {
};


void btnCabin_Press() {
	if (state.cabinLight == 1) {
		state.cabinLight = 0;
		portExt0->set(PIN_EXT0_CABIN, lightOFF);
	}
	else {
		state.cabinLight = 1;
		portExt0->set(PIN_EXT0_CABIN, lightON);
	}
};
void btnCabin_Hold() {
};
void btnCabin_Release() {
};

void reloadConfig() {

	stopLight->item(2)->offset = config.stop_light_duration;
	BackLight->item(1)->offset = config.back_light_timeout;

	input_X.IN_max = config.ch1_max;
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
	input_CH6.IN_min = config.ch6_min;

}


void setupBlinkers() {


	stopLight = new extBlinker("Stop light", portExt0);
	leftLight = new extBlinker("Left light", portExt0);
	rightLight = new extBlinker("Right light", portExt0);
	BackLight = new extBlinker("Back light", portExt0);
	alarmBlinker = new extBlinker("Alarm light", portExt0);
	sirenBlinker = new extBlinker("Siren light", portExt1);

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


	sirenBlinker
		->Add(PIN_EXT1_BLINKER_LEFT, 0, lightON)
		->Add(PIN_EXT1_BLINKER_RIGHT, 0, lightOFF)
		->Add(PIN_EXT1_BLINKER_LEFT, 500, lightOFF)
		->Add(PIN_EXT1_BLINKER_RIGHT, 500, lightON)
		->Add(PIN_EXT1_BLINKER_LEFT, 1000, lightOFF)
		->Add(PIN_EXT1_BLINKER_RIGHT, 1000, lightOFF)
		->repeat = true;
	sirenBlinker->offLevel = lightOFF;

}

void setup()
{
	Serial.begin(9600);
	console.output = &Serial;
	console.println();
	console.println();
	console.println();

	myDFPlayer = new DFRobotDFPlayerMini();


	myDFPlayer->begin(Serial);


	/*if (!myDFPlayer->begin(Serial)) {
		console.println(F("MP3 init failed!"));
		is_MP3_available = false;
	}
	else {*/
	is_MP3_available = true;
	myDFPlayer->setTimeOut(500); //Set serial communictaion time out 500ms
	delay(100);
	myDFPlayer->volume(30);
	delay(100);
	myDFPlayer->outputDevice(DFPLAYER_DEVICE_SD);
	delay(100);
	//myDFPlayer->play(1);
	//myDFPlayer->pause();
//}

	delay(100);
	console.println(F("MP3 ready"));

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

	portExt1 = new PCF8574(config.port_addr1);
	portExt1->begin(PIN_I2C_SDA, PIN_I2C_SCL);
	portExt1->write8(0x00);

	delay(1000);
	portExt0->write8(0xFF);
	portExt1->write8(0xFF);

	setupBlinkers();

	reloadConfig();

}

void printValues(JsonString* out)
{
	out->beginObject();
	out->AddValue("ch1_val", String(input_X.ImpulsLength));
	out->AddValue("ch2_val", String(input_Y.ImpulsLength));
	out->AddValue("ch3_val", String(input_CH3.ImpulsLength));
	out->AddValue("ch4_val", String(input_CH4.ImpulsLength));
	out->AddValue("ch5_val", String(input_CH5.ImpulsLength));
	out->AddValue("ch6_val", String(input_CH6.ImpulsLength));
	out->endObject();
}

void Values_Get() {
	JsonString ret = JsonString();
	printValues(&ret);
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


void handleVeichle() {
	/*
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
	*/
}


void handleStearing() {
	if (!input_X.isValid()) {
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
	int center = 90;
	int leftGap = center - input_X.OUT_min;
	int rightGap = center - input_X.OUT_max;

	//Фактичне відхилення 
	int leftLimit = (leftGap * config.turn_light_limit) / 100;
	int rightLimit = (rightGap * config.turn_light_limit) / 100;

	int delta = center - input_X.pos;

	//Serial.printf("delta=%i;ll=%i;rl=%i;c=%i\n", delta, leftLimit, rightLimit, center);

	if (delta > leftLimit) {
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
	else if (delta < rightLimit) {
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


void handleSpeed() {
	if (!input_Y.isValid()) {
		state.speed = 0;
		if (BackLight->isRunning()) {
			Serial.println("BackLight end");
			BackLight->end();
		}
		if (stopLight->isRunning()) {
			stopLight->end();
		}
		return;
	}
	int center = 90;
	int forwardGap = center - input_Y.OUT_min;
	int reverceGap = center - input_Y.OUT_max;


	int forward_limit = (forwardGap * config.reverce_limit) / 100;
	int reverce_limit = (reverceGap * config.reverce_limit) / 100;


	int speed = input_Y.pos - center;
	if (input_Y.isChanged) {
		Serial.printf("delta=%i;f=%i;r=%i;c=%i\n", speed, forward_limit, reverce_limit, center);
	}

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
		if (abs(state.speed) > 5)//передуваємо в русі
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
	}

}


void handleHeadLight() {
	if (input_Light->pos > 90)
		btnLight.setValue(HIGH);
	else
		btnLight.setValue(LOW);

	if (state.parkingLight) {
		portExt0->write(PIN_EXT0_PARKING_LIGHT, lightON);
		portExt1->write(PIN_EXT1_PARKING_LIGHT, lightON);
		/*if (state.fogLight)
			portExt->write(bitFogLight, lightON);
		else
			portExt->write(bitFogLight, lightOFF);*/
	}
	else {
		portExt0->write(PIN_EXT0_PARKING_LIGHT, lightOFF);
		portExt1->write(PIN_EXT1_PARKING_LIGHT, lightOFF);
		//portExt->write(bitFogLight, lightOFF);
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

void handleSiren() {
	if (input_Siren->isValid()) {
		if (input_Siren->pos > 45)
			btnSiren.setValue(HIGH);
		else
			btnSiren.setValue(LOW);

		if (input_Siren->pos > 135)
			btnSirenSound.setValue(HIGH);
		else
			btnSirenSound.setValue(LOW);
	}
	else {
		btnSiren.setValue(LOW);
		btnSirenSound.setValue(LOW);
	}
}

void handleCabin() {
	if (input_Cabin->isValid()) {
		if (input_Cabin->pos > 135) {
			btnCabin.setValue(HIGH);
		}
		else {
			btnCabin.setValue(LOW);
		}
	}
	else {
		btnCabin.setValue(LOW);
	}
}

void handleInterior() {
	if (input_Interior->isValid()) {
		if (input_Cabin->pos < 45) {
			btnInterior.setValue(HIGH);
		}
		else {
			btnInterior.setValue(LOW);
		}
	}
	else {
		btnInterior.setValue(LOW);
	}
}

unsigned long last_CH4_read = 0;

void loop()
{

	if (!interruptAttached) {
		Serial.println("interrupt attached");
		attachInterrupt(digitalPinToInterrupt(PIN_SERVO_X), pinServo_X_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_SERVO_Y), pinServo_Y_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_SERVO_CH3), pinServo_CH3_CHANGE, CHANGE);
		//attachInterrupt(digitalPinToInterrupt(PIN_SERVO_CH4), pinServo_CH4_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_SERVO_CH5), pinServo_CH5_CHANGE, CHANGE);
		attachInterrupt(digitalPinToInterrupt(PIN_SERVO_CH6), pinServo_CH6_CHANGE, CHANGE);
		interruptAttached = true;
	}

	dnsServer.processNextRequest();
	joypads.loop();
	webServer.loop();

	if (digitalRead(PIN_SERVO_CH4) == LOW && ((millis() - last_CH4_read) > 300)) {
		last_CH4_read = millis();
		while (digitalRead(PIN_SERVO_CH4) == LOW && (millis() - last_CH4_read) < 25) {};
		input_CH4.Start();
		while (digitalRead(PIN_SERVO_CH4) == HIGH && (millis() - last_CH4_read) < 25) {};
		input_CH4.Stop();
	}

	input_X.loop();
	input_Y.loop();
	input_CH3.loop();
	input_CH4.loop();
	input_CH5.loop();
	input_CH6.loop();

	if (input_X.isChanged) {
		Serial.printf("Servo X = %i (%i)\n", input_X.pos, input_X.ImpulsLength);
	}
	if (input_Y.isChanged) {
		Serial.printf("Servo Y = %i (%i)\n", input_Y.pos, input_Y.ImpulsLength);
	}
	if (input_CH3.isChanged) {
		Serial.printf("Servo CH3 = %i (%i)\n", input_CH3.pos, input_CH3.ImpulsLength);
	}
	if (input_CH4.isChanged) {
		Serial.printf("Servo CH4 = %i (%i)\n", input_CH4.pos, input_CH4.ImpulsLength);
	}
	if (input_CH5.isChanged) {
		Serial.printf("Servo CH5 = %i (%i)\n", input_CH5.pos, input_CH5.ImpulsLength);
	}
	if (input_CH6.isChanged) {
		Serial.printf("Servo CH6 = %i (%i)\n", input_CH6.pos, input_CH6.ImpulsLength);
	}

	if (joypads.getCount() > 0) {
		//btnStartStop.setValue(joypads.getValue("start"));
		//btnFire.setValue(joypads.getValue("fire"));
		//btnLight.setValue(joypads.getValue("light"));
		handleVeichle();
	}

	handleStearing();
	handleSpeed();
	handleHeadLight();
	handleSiren();
	handleCabin();
	handleInterior();

	stopLight->loop();
	leftLight->loop();
	rightLight->loop();
	BackLight->loop();
	alarmBlinker->loop();
	sirenBlinker->loop();

}