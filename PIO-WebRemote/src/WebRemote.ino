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
#define SERVO_HALF_ANGLE 90

#define LEFT_TURN_LIGHT_PIN D1
#define RIGHT_TURN_LIGHT_PIN D2
#define HEAD_LIGHT_PIN D4
#define STOP_LIGHT_PIN D8
#define REVERCE_LIGHT_PIN D3
#define BATTERY_PIN A0

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
	int last_speed_for_stop_light;

	int rpm;
	int ignition;

	int parkingLight;
	int headLight;
	int highLight;
	int stopLight;
	int leftTurnLight;
	int rightTurnLight;
	int reverseLight;

	unsigned long last_battery_read = 0;
	int battary_level = 50;
	int battary_mv = 7200;

} state;

struct Config
{

	String ssid = "WEB_REMOTE";
	String password = "12345678";

	int min_speed = 10;
	int inertion = 800;

	int stearing_Center = 90;
	int stearing_Left = 90 + SERVO_HALF_ANGLE;
	int stearing_Right = 90 - SERVO_HALF_ANGLE;

	int turnLight_Left = 90 + (SERVO_HALF_ANGLE * 0.5F);  // Condition to enable turn light
	int turnLight_Right = 90 - (SERVO_HALF_ANGLE * 0.5F); // Condition to enable turn light

	int PWM_TurnLight = 60;
	int PWM_ParkingLight = 40;
	int PWM_HeadLight = 100;
	int PWM_HighLight = 220;

	int PWM_StopLigh = 100;
	int stopLightDuration = 3000;
	int StopLightCondidion = 10;

	int PWM_BackLigh = 100;
	int backLightDuration = 1000;

	int battery_read_inteval = 10000;

	int battery_ADC_min = 561; // ADC value for minimum battery voltage
	int battery_ADC_max = 785; // ADC value for maximum battery voltage

	int battery_mv_min = 6000; // min battery voltage in millivolts
	int battery_mv_max = 8400; // max battery voltage in millivolts
};

void reloadConfig();

Config config;

char SSID[32];
char SSID_password[20];

IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;
JoypadCollection joypads = JoypadCollection();

void handle_StartStop();
void btnParkingLight_Pressed();
void btnHeadLight_Pressed();
void btnHighLight_Pressed();
void btnHighLight_Released();

Servo stearing = Servo();

VirtualButton btnStartStop = VirtualButton(handle_StartStop);
VirtualButton btnParkingLight = VirtualButton(btnParkingLight_Pressed);
VirtualButton btnHeadLight = VirtualButton(btnHeadLight_Pressed);
VirtualButton btnHighLight = VirtualButton(btnHighLight_Pressed, NULL, btnHighLight_Released);

RoboEffects motorEffect = RoboEffects();
MotorBase *motor = nullptr;

Blinker leftLight = Blinker("Left light");
Blinker rightLight = Blinker("Right light");
Blinker stopLight = Blinker("Stop light");
Blinker backLight = Blinker("Back light");

void printConfig()
{
	JsonString cfg = "";
	buildConfig(&cfg);
	Serial.println(cfg.c_str());
}

void loadConfig(JsonString *json)
{
	if (json->containsKey("ssid"))
		config.ssid = String(json->getValue("ssid"));
	if (json->containsKey("password"))
		config.password = String(json->getValue("password"));
	if (json->containsKey("min_speed"))
		config.min_speed = json->getInt("min_speed");
	if (json->containsKey("inertion"))
		config.inertion = json->getInt("inertion");
	if (json->containsKey("stearing_Center"))
		config.stearing_Center = json->getInt("stearing_Center");
	if (json->containsKey("stearing_Left"))
		config.stearing_Left = json->getInt("stearing_Left");
	if (json->containsKey("stearing_Right"))
		config.stearing_Right = json->getInt("stearing_Right");
	if (json->containsKey("turnLight_Left"))
		config.turnLight_Left = json->getInt("turnLight_Left");
	if (json->containsKey("turnLight_Right"))
		config.turnLight_Right = json->getInt("turnLight_Right");
	if (json->containsKey("PWM_BackLigh"))
		config.PWM_BackLigh = json->getInt("PWM_BackLigh");
	if (json->containsKey("PWM_HeadLight"))
		config.PWM_HeadLight = json->getInt("PWM_HeadLight");
	if (json->containsKey("PWM_HighLight"))
		config.PWM_HighLight = json->getInt("PWM_HighLight");
	if (json->containsKey("PWM_ParkingLight"))
		config.PWM_ParkingLight = json->getInt("PWM_ParkingLight");
	if (json->containsKey("PWM_TurnLight"))
		config.PWM_TurnLight = json->getInt("PWM_TurnLight");
	if (json->containsKey("PWM_StopLigh"))
		config.PWM_StopLigh = json->getInt("PWM_StopLigh");
	if (json->containsKey("stopLightDuration"))
		config.stopLightDuration = json->getInt("stopLightDuration");
	if (json->containsKey("StopLightCondidion"))
		config.StopLightCondidion = json->getInt("StopLightCondidion");
	if (json->containsKey("backLightDuration"))
		config.backLightDuration = json->getInt("backLightDuration");
	if (json->containsKey("battery_read_inteval"))
		config.battery_read_inteval = json->getInt("battery_read_inteval");
	if (json->containsKey("battery_ADC_min"))
		config.battery_ADC_min = json->getInt("battery_ADC_min");
	if (json->containsKey("battery_ADC_max"))
		config.battery_ADC_max = json->getInt("battery_ADC_max");
	if (json->containsKey("battery_mv_min"))
		config.battery_mv_min = json->getInt("battery_mv_min");
	if (json->containsKey("battery_mv_max"))
		config.battery_mv_max = json->getInt("battery_mv_max");
}

void buildConfig(JsonString *json)
{
	json->beginObject();
	json->AddValue("ssid", config.ssid);
	json->AddValue("password", config.password);

	json->AddValue("min_speed", String(config.min_speed));
	json->AddValue("inertion", String(config.inertion));

	json->AddValue("stearing_Center", String(config.stearing_Center));
	json->AddValue("stearing_Left", String(config.stearing_Left));
	json->AddValue("stearing_Right", String(config.stearing_Right));
	json->AddValue("turnLight_Left", String(config.turnLight_Left));
	json->AddValue("turnLight_Right", String(config.turnLight_Right));

	json->AddValue("PWM_BackLigh", String(config.PWM_BackLigh));
	json->AddValue("PWM_HeadLight", String(config.PWM_HeadLight));
	json->AddValue("PWM_HighLight", String(config.PWM_HighLight));
	json->AddValue("PWM_ParkingLight", String(config.PWM_ParkingLight));
	json->AddValue("PWM_TurnLight", String(config.PWM_TurnLight));
	json->AddValue("PWM_StopLigh", String(config.PWM_StopLigh));
	json->AddValue("stopLightDuration", String(config.stopLightDuration));
	json->AddValue("StopLightCondidion", String(config.StopLightCondidion));
	json->AddValue("backLightDuration", String(config.backLightDuration));

	json->AddValue("battery_read_inteval", String(config.battery_read_inteval));
	json->AddValue("battery_ADC_min", String(config.battery_ADC_min));
	json->AddValue("battery_ADC_max", String(config.battery_ADC_max));
	json->AddValue("battery_mv_min", String(config.battery_mv_min));
	json->AddValue("battery_mv_max", String(config.battery_mv_max));

	json->endObject();
}

void readConfig()
{
	String s;
	JsonString cfg = "";
	File cfgFile;
	if (!SPIFFS.exists("/config.json"))
	{
		console.println(("Default setting loaded..."));
		saveConfig();
	}
	console.println(("Reading config..."));
	cfgFile = SPIFFS.open("/config.json", "r");
	s = cfgFile.readString();
	cfg = JsonString(s.c_str());
	cfgFile.close();
	loadConfig(&cfg);
}

void saveConfig()
{
	String path = "/config.json";
	JsonString content = "";
	buildConfig(&content);
	File file = SPIFFS.open(path, "w");
	file.write(content.c_str());
	file.flush();
	file.close();
}

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

	readConfig();
	printConfig();

	pinMode(HEAD_LIGHT_PIN, OUTPUT);
	pinMode(STOP_LIGHT_PIN, OUTPUT);
	pinMode(REVERCE_LIGHT_PIN, OUTPUT);
	pinMode(BATTERY_PIN, INPUT);

	setupController.setup();
	setupController.buildConfig = buildConfig;
	setupController.saveConfig = [](JsonString *json)
	{
		loadConfig(json);
		saveConfig();
		printConfig();
	};

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

	leftLight.Add(LEFT_TURN_LIGHT_PIN, 0, LOW);
	leftLight.Add(LEFT_TURN_LIGHT_PIN, 0, config.PWM_TurnLight);
	leftLight.Add(LEFT_TURN_LIGHT_PIN, 500, LOW);
	leftLight.Add(LEFT_TURN_LIGHT_PIN, 1000, LOW);
	leftLight.attachWriteEvent([](int pin, int value)
							   { joypads.setValue("left_on", value); });
	leftLight.debug = true;

	rightLight.Add(RIGHT_TURN_LIGHT_PIN, 0, LOW);
	rightLight.Add(RIGHT_TURN_LIGHT_PIN, 0, config.PWM_TurnLight);
	rightLight.Add(RIGHT_TURN_LIGHT_PIN, 500, LOW);
	rightLight.Add(RIGHT_TURN_LIGHT_PIN, 1000, LOW);
	rightLight.attachWriteEvent([](int pin, int value)
								{ joypads.setValue("right_on", value); });
	rightLight.debug = true;

	stopLight.Add(STOP_LIGHT_PIN, 0, LOW);
	stopLight.Add(STOP_LIGHT_PIN, 0, config.PWM_StopLigh);
	stopLight.Add(STOP_LIGHT_PIN, config.stopLightDuration, LOW);
	stopLight.repeat = false;
	stopLight.attachWriteEvent([](int pin, int value)
							   { joypads.setValue("stop_on", value); });
	stopLight.debug = true;

	backLight.Add(REVERCE_LIGHT_PIN, 0, LOW);
	backLight.Add(REVERCE_LIGHT_PIN, 0, config.PWM_BackLigh);
	backLight.Add(REVERCE_LIGHT_PIN, config.backLightDuration, LOW);
	backLight.repeat = true;
	backLight.attachWriteEvent([](int pin, int value)
							   { joypads.setValue("back_on", value); });
	backLight.debug = true;

	btnStartStop.bounce = 0;

	state.ignition = Ignition::OFF;
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

void setParkingLight(int value)
{
	if (state.parkingLight == value)
		return;
	state.parkingLight = value;
	if (value == 0)
		joypads.setValue("pos_on", 0);
	else
		joypads.setValue("pos_on", 1);
}

void setHeadLight(int value)
{
	if (state.headLight == value)
		return;
	state.headLight = value;
	if (value == 0)
		joypads.setValue("lo_on", 0);
	else
		joypads.setValue("lo_on", 1);
}

void setHighLight(int value)
{
	if (state.highLight == value)
		return;
	state.highLight = value;
	if (value == 0)
		joypads.setValue("hi_on", 0);
	else
		joypads.setValue("hi_on", 1);
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
	Serial.println("Parking Light!");
	if (state.parkingLight == 0)
	{
		setParkingLight(1);
	}
	else
	{
		setParkingLight(0);
		setHeadLight(0);
		setHighLight(0);
	}
}

void btnHeadLight_Pressed()
{
	Serial.println("Head Light!");
	if (state.headLight == 0)
	{
		setParkingLight(1);
		setHeadLight(1);
		setHighLight(0);
	}
	else
	{
		setParkingLight(1);
		setHeadLight(0);
		setHighLight(0);
	}
}

void btnHighLight_Pressed()
{
	Serial.println("High Light!");
	if (state.headLight)
	{
		if (state.highLight == 0)
		{
			setHighLight(1);
		}
		else
		{
			setHighLight(0);
		}
	}
	else
	{
		setHighLight(1);
	}
}

void btnHighLight_Released()
{
	if (!state.headLight)
		setHighLight(0);
}

void handleVeichle()
{
	double speed = joypads.getValue("motor_y");
	double stearing = joypads.getValue("stearing_x");

	if (state.ignition >= Ignition::ON)
	{
		state.stearing = map(stearing, -100, 100, config.stearing_Right, config.stearing_Left);
	}
	else
	{
		state.stearing = config.stearing_Center;
	}

	if (state.stearing > config.turnLight_Left)
	{
		if (!leftLight.isRunning())
			leftLight.begin();
		if (rightLight.isRunning())
			rightLight.end();
	}
	else
	{
		if (leftLight.isRunning())
			leftLight.end();
	}

	if (state.stearing < config.turnLight_Right)
	{
		if (!rightLight.isRunning())
			rightLight.begin();
		if (leftLight.isRunning())
			leftLight.end();
	}
	else
	{
		if (rightLight.isRunning())
			rightLight.end();
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
		state.speed = speed;
		motor->setSpeed(map(state.speed, -100, 100, -255, 255));

		// if speen decrease -> turn stop ligh ON
		int deltaSpeed = abs(state.speed) - state.last_speed_for_stop_light;
		if (deltaSpeed < -config.StopLightCondidion)
		{
			state.last_speed_for_stop_light = abs(state.speed);
			stopLight.begin();
		}
		else if (deltaSpeed >= config.StopLightCondidion)
		{
			state.last_speed_for_stop_light = abs(state.speed);
			stopLight.end();
		}

		if (state.speed < 0)
		{
			backLight.repeat = true;
			backLight.begin();
		}
		else
		{
			backLight.repeat = false;
		}
	}
	else
	{
		state.speed = 0;
	}

	if (state.highLight == 1)
	{
		analogWrite(HEAD_LIGHT_PIN, config.PWM_HighLight);
	}
	else if (state.headLight == 1)
	{
		analogWrite(HEAD_LIGHT_PIN, config.PWM_HeadLight);
	}
	else if (state.parkingLight == 1)
	{
		analogWrite(HEAD_LIGHT_PIN, config.PWM_ParkingLight);
	}
	else
	{
		analogWrite(HEAD_LIGHT_PIN, 0);
	}

	joypads.setValue("rpm", state.rpm);
	joypads.setValue("speed", abs(state.speed));
}

void handleBattery()
{

	if (millis() - state.last_battery_read > config.battery_read_inteval)
	{
		state.last_battery_read = millis();
		int battary_adc = analogRead(BATTERY_PIN);
		state.battary_level = map(battary_adc, config.battery_ADC_min, config.battery_ADC_max, 0, 100);
		state.battary_mv = map(battary_adc, config.battery_ADC_min, config.battery_ADC_max, config.battery_mv_min, config.battery_mv_max);
		joypads.setValue("bat_v", (double)state.battary_mv / 1000.0);
		joypads.setValue("bat_level", state.battary_level);
		Serial.print("Bat V=");
		Serial.print(state.battary_mv);
		Serial.print(" lvl=");
		Serial.print(state.battary_level);
		Serial.print(" adc=");
		Serial.println(battary_adc);
	}
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
		state.stearing = config.stearing_Center;
		state.ignition = Ignition::OFF;
	}

	handleBattery();

	motor->loop();
	stearing.write(state.stearing);
	leftLight.loop();
	rightLight.loop();
	stopLight.loop();
	backLight.loop();
}
