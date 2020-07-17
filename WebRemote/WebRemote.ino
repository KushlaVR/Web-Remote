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

#define MOSFET_OFF 1024
#define MOSFET_ON 0


#define pinLight D0

#define pinGun D1
#define pinCabin D2

//#define pinLeftMotor D4//лівий борт
//#define pinRightMotor D5//правий борт

#define pinLeftMotorA D4//правий борт
#define pinLeftMotorB D5//правий борт

#define pinRightMotorA D6//правий борт
#define pinRightMotorB D7//правий борт

#define pinTacho D3//ШИМ для турбіни
#define pinSmoke D8//ШИМ для димогенератора

enum Ignition {
	OFF = 0,
	ON = 1,
	RUN = 2
};

struct State {
	int left;
	int right;
	int rpm;
	int ignition;
} state;

void startStop_Pressed();
void reloadConfig();

ConfigStruct config;

char SSID[32];
char SSID_password[20];

IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
JoypadCollection joypads = JoypadCollection();

Blinker turbine = Blinker("Turbine");
VirtualButton startStop = VirtualButton(startStop_Pressed);

//Servo lefMotor = Servo();
//Servo rightMotor = Servo();


RoboEffects leftMotorEffect = RoboEffects();
MotorBase* leftMotor = nullptr;

RoboEffects rightMotorEffect = RoboEffects();
MotorBase* rightMotor = nullptr;

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
	webServer.on("/api/post", Post);

	leftMotor = new HBridge("Left motor", pinLeftMotorA, pinLeftMotorB, &leftMotorEffect);
	leftMotor->responder = &console;
	leftMotor->setWeight(config.inertion);
	leftMotor->reset();
	leftMotor->isEnabled = true;


	rightMotor = new HBridge("Right motor", pinRightMotorA, pinRightMotorB, &rightMotorEffect);
	rightMotor->responder = &console;
	rightMotor->setWeight(config.inertion);
	rightMotor->reset();
	rightMotor->isEnabled = true;

	turbine.Add(pinTacho, 0, MOSFET_ON)
		->Add(pinTacho, (1000 / config.turbine_frequency_min) / 2, MOSFET_OFF)
		->Add(pinTacho, 1000 / config.turbine_frequency_min, MOSFET_OFF);

	state.ignition = Ignition::OFF;

}

void reloadConfig() {
	leftMotor->setWeight(config.inertion);
	rightMotor->setWeight(config.inertion);
	turbine.item(1)->offset = (1000 / config.turbine_frequency_min) / 2;
	turbine.item(2)->offset = (1000 / config.turbine_frequency_min);
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
	String s = webServer.argName(0);
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

void startStop_Pressed() {
	state.ignition += 1;
	if (state.ignition > Ignition::RUN) {
		state.ignition = Ignition::OFF;
	}
	if (state.ignition == Ignition::OFF) {
		if (turbine.isRunning()) turbine.end();
		Serial.println("Engine stopped");
	}
	else if (state.ignition == Ignition::ON) {
		Serial.println("Engine Started");
		if (!turbine.isRunning()) turbine.begin();
	}
	else
	{
		Serial.println("Veichle Run");
		if (!turbine.isRunning()) turbine.begin();
	}
}

void loop()
{
	dnsServer.processNextRequest();
	joypads.loop();
	webServer.loop();

	if (joypads.getCount() > 0) {

		startStop.setValue(joypads.getValue("start"));

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
				analogWrite(pinSmoke, MOSFET_OFF - map(smoke, 0, 100, 0, 1024));
				//Turbo PWM
				turbine.item(0)->value = MOSFET_ON;//map(tacho, 0, 100, 1024 - config.turbine_min, config.turbine_max);
				//Turbo pulse freq
				int f = map(tacho, 0, 100, config.turbine_frequency_min, config.turbine_frequency_max);
				turbine.item(1)->offset = (1000 / f) / 2;
				turbine.item(2)->offset = (1000 / f);
			}
		}
		else
		{
			state.rpm = 0;
			turbine.item(0)->value = MOSFET_OFF;
			analogWrite(pinSmoke, MOSFET_OFF);
		}


		//Двигуни
		if (state.ignition >= Ignition::RUN) {
			if (left != state.left) {
				leftMotor->setSpeed(map(left, 0, 100, 0, 1024));
				state.left = left;
				console.print("left=");
				console.println(state.right);
			}

			if (right != state.right) {
				rightMotor->setSpeed(map(right, 0, 100, 0, 1024));
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
	turbine.loop();
	leftMotor->loop();
	rightMotor->loop();
	startStop.handle();
}