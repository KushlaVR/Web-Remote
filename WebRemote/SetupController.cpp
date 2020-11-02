#include "SetupController.h"



SetupController::SetupController()
{
	webServer.on("/api/setup", HTTPMethod::HTTP_GET, Setup_Get);
	webServer.on("/api/setup", HTTPMethod::HTTP_POST, Setup_Post);
}

SetupController::~SetupController()
{
}

void SetupController::loadConfig()
{
	String s;
	JsonString cfg = "";
	File cfgFile;
	if (!SPIFFS.exists("/config.json")) {
		console.println(("Default setting loaded..."));
		cfg.beginObject();
		cfg.AddValue("ssid", "WEMOS");
		cfg.AddValue("password", "12345678");

		cfg.AddValue("min_speed", "20");
		cfg.AddValue("inertion", "800");

		cfg.AddValue("turbine_min", "0");
		cfg.AddValue("turbine_max", "20");

		cfg.AddValue("turbine_frequency_min", "3");
		cfg.AddValue("turbine_frequency_max", "10");

		cfg.AddValue("smoke_min", "20");
		cfg.AddValue("smoke_max", "100");

		cfg.AddValue("gun_min", "20");
		cfg.AddValue("gun_max", "40");

		//cabin
		cfg.AddValue("cabin_min", "20");
		cfg.AddValue("cabin_max", "40");
		cfg.AddValue("cabin_inertion", "800");

		cfg.AddValue("fire_min", "20");
		cfg.AddValue("fire_max", "40");
		cfg.AddValue("fire_duration", "1000");
		cfg.AddValue("fire_led_start", "100");
		cfg.AddValue("fire_led_end", "800");
		cfg.AddValue("fire_led_pwm", "50");

		cfg.AddValue("light", "50");

		cfg.endObject();

		cfgFile = SPIFFS.open("/config.json", "w");
		cfgFile.print(cfg.c_str());
		cfgFile.flush();
		cfgFile.close();
	}
	else {
		console.println(("Reading config..."));
		cfgFile = SPIFFS.open("/config.json", "r");
		s = cfgFile.readString();
		cfg = JsonString(s.c_str());
		cfgFile.close();
	}

	this->cfg->ssid = String(cfg.getValue("ssid"));
	this->cfg->password = String(cfg.getValue("password"));

	this->cfg->min_speed = cfg.getInt("min_speed");
	this->cfg->inertion = cfg.getInt("inertion");

	this->cfg->gun_min = cfg.getInt("gun_min");
	this->cfg->gun_max = cfg.getInt("gun_max");

	this->cfg->cabin_min = cfg.getInt("cabin_min");
	this->cfg->cabin_max = cfg.getInt("cabin_max");
	this->cfg->cabin_Inertion = cfg.getInt("cabin_inertion");

	this->cfg->fire_min = cfg.getInt("fire_min");
	this->cfg->fire_max = cfg.getInt("fire_max");
	this->cfg->fire_duration = cfg.getInt("fire_duration");
	this->cfg->fire_led_start = cfg.getInt("fire_led_start");
	this->cfg->fire_led_end = cfg.getInt("fire_led_end");
	this->cfg->fire_led_pwm = cfg.getInt("fire_led_pwm");

	this->cfg->turbine_min = cfg.getInt("turbine_min");
	this->cfg->turbine_max = cfg.getInt("turbine_max");

	this->cfg->turbine_frequency_min = cfg.getInt("turbine_frequency_min");
	this->cfg->turbine_frequency_max = cfg.getInt("turbine_frequency_max");

	this->cfg->smoke_min = cfg.getInt("smoke_min");
	this->cfg->smoke_max = cfg.getInt("smoke_max");
	
	this->cfg->light= cfg.getInt("light");

}

void SetupController::saveConfig()
{
	JsonString  out = JsonString();
	printConfig(&out);
	File cfgFile = SPIFFS.open("/config.json", "w");
	cfgFile.print(out.c_str());
	cfgFile.flush();
	cfgFile.close();
	if (setupController.reloadConfig != nullptr) setupController.reloadConfig();
}

void SetupController::printConfig(JsonString* out)
{
	out->beginObject();
	out->AddValue("ssid", cfg->ssid);
	out->AddValue("password", cfg->password);

	out->AddValue("min_speed", String(cfg->min_speed));
	out->AddValue("inertion", String(cfg->inertion));

	out->AddValue("gun_min", String(cfg->gun_min));
	out->AddValue("gun_max", String(cfg->gun_max));

	out->AddValue("cabin_min", String(cfg->cabin_min));
	out->AddValue("cabin_max", String(cfg->cabin_max));
	out->AddValue("cabin_inertion", String(cfg->cabin_Inertion));

	out->AddValue("fire_min", String(cfg->fire_min));
	out->AddValue("fire_max", String(cfg->fire_max));
	out->AddValue("fire_duration", String(cfg->fire_duration));
	out->AddValue("fire_led_start", String(cfg->fire_led_start));
	out->AddValue("fire_led_end", String(cfg->fire_led_end));
	out->AddValue("fire_led_pwm", String(cfg->fire_led_pwm));

	out->AddValue("turbine_min", String(cfg->turbine_min));
	out->AddValue("turbine_max", String(cfg->turbine_max));

	out->AddValue("turbine_frequency_min", String(cfg->turbine_frequency_min));
	out->AddValue("turbine_frequency_max", String(cfg->turbine_frequency_max));

	out->AddValue("smoke_min", String(cfg->smoke_min));
	out->AddValue("smoke_max", String(cfg->smoke_max));
	
	out->AddValue("light", String(cfg->light));

	out->endObject();
}

void SetupController::Setup_Get()
{
	JsonString ret = JsonString();
	setupController.printConfig(&ret);
	webServer.jsonOk(&ret);
}

void SetupController::Setup_Post()
{

	if (webServer.hasArg("ssid")) { setupController.cfg->ssid = webServer.arg("ssid"); }
	if (webServer.hasArg("password")) { setupController.cfg->password = webServer.arg("password"); }

	if (webServer.hasArg("min_speed")) { setupController.cfg->min_speed = webServer.arg("min_speed").toInt(); }
	if (webServer.hasArg("inertion")) { setupController.cfg->inertion = webServer.arg("inertion").toInt(); }

	if (webServer.hasArg("gun_min")) { setupController.cfg->gun_min = webServer.arg("gun_min").toInt(); }
	if (webServer.hasArg("gun_max")) { setupController.cfg->gun_max = webServer.arg("gun_max").toInt(); }

	if (webServer.hasArg("cabin_min")) { setupController.cfg->cabin_min = webServer.arg("cabin_min").toInt(); }
	if (webServer.hasArg("cabin_max")) { setupController.cfg->cabin_max = webServer.arg("cabin_max").toInt(); }
	if (webServer.hasArg("cabin_inertion")) { setupController.cfg->cabin_Inertion = webServer.arg("cabin_inertion").toInt(); }

	if (webServer.hasArg("fire_min")) { setupController.cfg->fire_min = webServer.arg("fire_min").toInt(); }
	if (webServer.hasArg("fire_max")) { setupController.cfg->fire_max = webServer.arg("fire_max").toInt(); }
	if (webServer.hasArg("fire_duration")) { setupController.cfg->fire_duration = webServer.arg("fire_duration").toInt(); }
	if (webServer.hasArg("fire_led_start")) { setupController.cfg->fire_led_start = webServer.arg("fire_led_start").toInt(); }
	if (webServer.hasArg("fire_led_end")) { setupController.cfg->fire_led_end = webServer.arg("fire_led_end").toInt(); }
	if (webServer.hasArg("fire_led_pwm")) { setupController.cfg->fire_led_pwm = webServer.arg("fire_led_pwm").toInt(); }

	if (webServer.hasArg("turbine_min")) { setupController.cfg->turbine_min = webServer.arg("turbine_min").toInt(); }
	if (webServer.hasArg("turbine_max")) { setupController.cfg->turbine_max = webServer.arg("turbine_max").toInt(); }

	if (webServer.hasArg("turbine_frequency_min")) { setupController.cfg->turbine_frequency_min = webServer.arg("turbine_frequency_min").toInt(); }
	if (webServer.hasArg("turbine_frequency_max")) { setupController.cfg->turbine_frequency_max = webServer.arg("turbine_frequency_max").toInt(); }

	if (webServer.hasArg("smoke_min")) { setupController.cfg->smoke_min = webServer.arg("smoke_min").toInt(); }
	if (webServer.hasArg("smoke_max")) { setupController.cfg->smoke_max = webServer.arg("smoke_max").toInt(); }
	
	if (webServer.hasArg("light")) { setupController.cfg->light = webServer.arg("light").toInt(); }

	setupController.saveConfig();

	webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
	webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
	webServer.client().stop(); // Stop is needed because we sent no content length
}


SetupController setupController;