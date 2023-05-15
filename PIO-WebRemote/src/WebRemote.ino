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

#define MOTOR_A D6
#define MOTOR_B D7
#define SERVO_PIN D5

enum Ignition
{
	OFF = 0,
	ON = 1,
	RUN = 2
};

struct State
{
	int stearing;
	int speed;

	int rpm;
	int ignition;

	int parkingLight;
	int headLight;
	int highLight;
	int stopLight;
	int leftTurnLight;
	int rightTurnLight;
	int reverseLight;

} state;

void reloadConfig();

ConfigStruct config;

char SSID[32];
char SSID_password[20];

IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
JoypadCollection joypads = JoypadCollection();

// Blinker smokeGenerator = Blinker("Smoke");

void handle_StartStop();
void btnParkingLight_Pressed();
void btnHeadLight_Pressed();
void btnHighLight_Pressed();

Servo stearing = Servo();

VirtualButton btnStartStop = VirtualButton(handle_StartStop);
VirtualButton btnParkingLight = VirtualButton(btnParkingLight_Pressed);
VirtualButton btnHeadLight = VirtualButton(btnHeadLight_Pressed);
VirtualButton btnHighLight = VirtualButton(btnHighLight_Pressed);

RoboEffects motorEffect = RoboEffects();
MotorBase *motor = nullptr;

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker stopLight = Blinker("Stop light");
Blinker backLight = Blinker("Back light");

void setup()
{
	Serial.begin(115200);
	console.output = &Serial;
	console.println();
	console.println();
	console.println();

	String s;
	if (!SPIFFS.begin())
	{
		console.println(F("No file system!"));
		console.println(F("Fomating..."));
		if (SPIFFS.format())
			console.println(F("OK"));
		else
		{
			console.println(F("Fail.... rebooting..."));
			while (true)
				;
		}
	}

	if (SPIFFS.exists("/intro.txt"))
	{
		File f = SPIFFS.open("/intro.txt", "r");
		s = f.readString();
		console.println(s.c_str());
	}
	else
	{
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

	motor = new HBridge("Main motor", MOTOR_A, MOTOR_B, &motorEffect);
	motor->responder = &console;
	motor->setWeight(config.inertion);
	motor->reset();
	motor->isEnabled = true;

	stearing.attach(SERVO_PIN);

	btnStartStop.bounce = 0;

	state.ignition = Ignition::OFF;
}

void reloadConfig()
{
	// leftMotor->setWeight(config.inertion);
	// rightMotor->setWeight(config.inertion);
	// cabinMotor->setWeight(config.cabin_Inertion);
	// turbineBlinker.item(1)->offset = (1000 / config.turbine_frequency_min) / 2;
	// turbineBlinker.item(2)->offset = (1000 / config.turbine_frequency_min);
}

void EventSourceName()
{
	console.println(webServer.uri());
	webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
	webServer.sendHeader("Pragma", "no-cache");
	webServer.sendHeader("Expires", "-1");
	webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);

	Joypad *j = new Joypad();
	j->client = webServer.client();
	j->clientIP = webServer.client().remoteIP();
	joypads.add(j);

	String ret = "http://" + apIP.toString() + ":80/api/events?{\"client\":\"" + String(j->id) + "\"}";
	yield();
	webServer.send(200, "text/plain", ret);
	console.println(ret);
}

void Events()
{
	console.println(webServer.uri());
	WiFiClient client = webServer.client();

	String s = webServer.argName(0);
	JsonString json = "";
	json += s;
	int id = json.getInt("client");

	// console.printf("client:%i", id);

	Joypad *j = joypads.getById(id);
	if (j == nullptr)
	{
		console.printf("Unauthorized client %i\n", id);
		webServer.handleNotFound();
		return;
	}
	if (client.remoteIP() != j->clientIP)
	{
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

void Post()
{
	if (webServer.hasArg("plain"))
	{
		String s = webServer.arg("plain");
		JsonString json = "";
		json += s;
		int id = json.getInt("client");

		// console.printf("client:%i\n", id);

		Joypad *j = joypads.getById(id);
		if (j == nullptr)
		{
			webServer.handleNotFound();
			return;
		}
		webServer.Ok();
		if (j->processParcel(&json))
		{
			joypads.updateValuesFrom(j);
		}
	}
	else
	{
		webServer.Ok();
	}
}

void handle_StartStop()
{
	state.ignition += 1;
	if (state.ignition > Ignition::RUN)
	{
		state.ignition = Ignition::OFF;
	}
	if (state.ignition == Ignition::OFF)
	{
		Serial.println("Engine stopped");
	}
	else if (state.ignition == Ignition::ON)
	{
		Serial.println("Engine Started");
	}
	else
	{
		Serial.println("Veichle Run");
	};
}

void btnParkingLight_Pressed()
{
	Serial.print("Parking Light!");
	if (state.parkingLight == 0)
	{
		state.parkingLight = 1;
	}
	else
	{
		state.parkingLight = 0;
		state.headLight = 0;
		state.highLight = 0;
	}
}

void btnHeadLight_Pressed()
{
	Serial.print("Head Light!");
	if (state.headLight == 0)
	{
		state.parkingLight = 1;
		state.headLight = 1;
		state.highLight = 0;
	}
	else
	{
		state.parkingLight = 1;
		state.headLight = 0;
		state.highLight = 0;
	}
}

void btnHighLight_Pressed()
{
	Serial.print("High Light!");
	if (state.highLight == 0)
	{
		state.headLight = 0;
		state.highLight = 1;
	}
	else
	{
		state.headLight = 1;
		state.highLight = 0;
	}
}

void handleVeichle()
{
	double speed = joypads.getValue("motor_y");
	double stearing = joypads.getValue("stearing_x");

	if (state.ignition >= Ignition::ON)
	{
		state.stearing = map(stearing, -100, 100, 0, 180);
	}
	else
	{
		state.stearing = 90;
	}

	// Двигуни
	if (state.ignition >= Ignition::RUN)
	{
		// Тахометр
		int tacho;
		tacho = abs(speed);
		int rpm = map(tacho, 0, 100, 800, 2300);
		if (state.rpm != rpm)
		{
			state.rpm = rpm;
		}
		motor->setSpeed(map(speed, -100, 100, -255, 255));
	}
	else
	{
		state.speed = 0;
	}

	joypads.setValue("rpm", state.rpm);
}

void loop()
{
	dnsServer.processNextRequest();
	joypads.loop();
	webServer.loop();

	if (joypads.getCount() > 0)
	{
		state.ignition = Ignition::RUN;
		btnStartStop.setValue(joypads.getValue("start"));
		btnParkingLight.setValue(joypads.getValue("pos_light"));
		btnHeadLight.setValue(joypads.getValue("l_light"));
		btnHighLight.setValue(joypads.getValue("h_light"));
		handleVeichle();
	}
	else
	{
		state.ignition = Ignition::OFF;
	}

	motor->loop();
	stearing.write(state.stearing);
	leftLight.loop();
	rightLight.loop();
	stopLight.loop();
	backLight.loop();
}
