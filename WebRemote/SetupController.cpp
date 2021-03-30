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
		cfg.AddValue("ssid", "GAZ13");
		cfg.AddValue("password", "12345678");

		cfg.AddValue("ch1_min", "1114");
		cfg.AddValue("ch1_max", "2314");

		cfg.AddValue("ch2_min", "900");
		cfg.AddValue("ch2_max", "2075");

		cfg.AddValue("ch3_min", "1007");
		cfg.AddValue("ch3_max", "2014");

		cfg.AddValue("ch4_min", "1007");
		cfg.AddValue("ch4_max", "2014");

		cfg.AddValue("ch5_min", "1007");
		cfg.AddValue("ch5_max", "2014");

		cfg.AddValue("ch6_min", "1007");
		cfg.AddValue("ch6_max", "2014");

		cfg.AddValue("turn_light_limit", "50");
		cfg.AddValue("reverce_limit", "5");

		cfg.AddValue("stop_light_duration", "2000");
		cfg.AddValue("back_light_timeout", "500");

		cfg.AddValue("port_addr0", "39");
		cfg.AddValue("port_addr1", "119");

		cfg.AddValue("gear0", "90");
		cfg.AddValue("gear1", "60");
		cfg.AddValue("gear2", "120");

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

	this->cfg->ch1_min = cfg.getInt("ch1_min");
	this->cfg->ch1_max = cfg.getInt("ch1_max");
	this->cfg->ch1_center = this->cfg->ch1_min + ((this->cfg->ch1_max - this->cfg->ch1_min) / 2);

	this->cfg->ch2_min = cfg.getInt("ch2_min");
	this->cfg->ch2_max = cfg.getInt("ch2_max");
	this->cfg->ch2_center = this->cfg->ch2_min + ((this->cfg->ch2_max - this->cfg->ch2_min) / 2);

	this->cfg->ch3_min = cfg.getInt("ch3_min");
	this->cfg->ch3_max = cfg.getInt("ch3_max");

	this->cfg->ch4_min = cfg.getInt("ch4_min");
	this->cfg->ch4_max = cfg.getInt("ch4_max");

	this->cfg->ch5_min = cfg.getInt("ch5_min");
	this->cfg->ch5_max = cfg.getInt("ch5_max");
	this->cfg->ch5_center = this->cfg->ch5_min + ((this->cfg->ch5_max - this->cfg->ch5_min) / 2);

	this->cfg->ch6_min = cfg.getInt("ch6_min");
	this->cfg->ch6_max = cfg.getInt("ch6_max");
	this->cfg->ch6_center = this->cfg->ch6_min + ((this->cfg->ch6_max - this->cfg->ch6_min) / 2);

	this->cfg->turn_light_limit = cfg.getInt("turn_light_limit");
	this->cfg->reverce_limit = cfg.getInt("reverce_limit");
	this->cfg->port_addr0 = cfg.getInt("port_addr0");
	this->cfg->port_addr1 = cfg.getInt("port_addr1");

	this->cfg->stop_light_duration = cfg.getInt("stop_light_duration");
	this->cfg->back_light_timeout = cfg.getInt("back_light_timeout");

	this->cfg->gear0 = cfg.getInt("gear0");
	this->cfg->gear1 = cfg.getInt("gear1");
	this->cfg->gear2 = cfg.getInt("gear2");


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

	out->AddValue("ch1_min", String(cfg->ch1_min));
	out->AddValue("ch1_center", String(cfg->ch1_center));
	out->AddValue("ch1_max", String(cfg->ch1_max));

	out->AddValue("ch2_min", String(cfg->ch2_min));
	out->AddValue("ch2_center", String(cfg->ch2_center));
	out->AddValue("ch2_max", String(cfg->ch2_max));

	out->AddValue("ch3_min", String(cfg->ch3_min));
	out->AddValue("ch3_max", String(cfg->ch3_max));

	out->AddValue("ch4_min", String(cfg->ch4_min));
	out->AddValue("ch4_max", String(cfg->ch4_max));

	out->AddValue("ch5_min", String(cfg->ch5_min));
	out->AddValue("ch5_max", String(cfg->ch5_max));

	out->AddValue("ch6_min", String(cfg->ch6_min));
	out->AddValue("ch6_max", String(cfg->ch6_max));

	out->AddValue("turn_light_limit", String(cfg->turn_light_limit));
	out->AddValue("reverce_limit", String(cfg->reverce_limit));
	out->AddValue("port_addr0", String(cfg->port_addr0));
	out->AddValue("port_addr1", String(cfg->port_addr1));

	out->AddValue("stop_light_duration", String(cfg->stop_light_duration));
	out->AddValue("back_light_timeout", String(cfg->back_light_timeout));

	out->AddValue("gear0", String(cfg->gear0));
	out->AddValue("gear1", String(cfg->gear1));
	out->AddValue("gear2", String(cfg->gear2));
	
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

	if (webServer.hasArg("ch1_min")) { setupController.cfg->ch1_min = webServer.arg("ch1_min").toInt(); }
	if (webServer.hasArg("ch1_max")) { setupController.cfg->ch1_max = webServer.arg("ch1_max").toInt(); }
	setupController.cfg->ch1_center = setupController.cfg->ch1_min + ((setupController.cfg->ch1_max - setupController.cfg->ch1_min) / 2);

	if (webServer.hasArg("ch2_min")) { setupController.cfg->ch2_min = webServer.arg("ch2_min").toInt(); }
	if (webServer.hasArg("ch2_max")) { setupController.cfg->ch2_max = webServer.arg("ch2_max").toInt(); }
	setupController.cfg->ch2_center = setupController.cfg->ch2_min + ((setupController.cfg->ch2_max - setupController.cfg->ch2_min) / 2);

	if (webServer.hasArg("ch3_min")) { setupController.cfg->ch3_min = webServer.arg("ch3_min").toInt(); }
	if (webServer.hasArg("ch3_max")) { setupController.cfg->ch3_max = webServer.arg("ch3_max").toInt(); }

	if (webServer.hasArg("ch4_min")) { setupController.cfg->ch4_min = webServer.arg("ch4_min").toInt(); }
	if (webServer.hasArg("ch4_max")) { setupController.cfg->ch4_max = webServer.arg("ch4_max").toInt(); }

	if (webServer.hasArg("ch5_min")) { setupController.cfg->ch5_min = webServer.arg("ch5_min").toInt(); }
	if (webServer.hasArg("ch5_max")) { setupController.cfg->ch5_max = webServer.arg("ch5_max").toInt(); }

	if (webServer.hasArg("ch6_min")) { setupController.cfg->ch6_min = webServer.arg("ch6_min").toInt(); }
	if (webServer.hasArg("ch6_max")) { setupController.cfg->ch6_max = webServer.arg("ch6_max").toInt(); }

	if (webServer.hasArg("turn_light_limit")) { setupController.cfg->turn_light_limit = webServer.arg("turn_light_limit").toInt(); }
	if (webServer.hasArg("reverce_limit")) { setupController.cfg->reverce_limit = webServer.arg("reverce_limit").toInt(); }
	if (webServer.hasArg("port_addr0")) { setupController.cfg->port_addr0 = webServer.arg("port_addr0").toInt(); }
	if (webServer.hasArg("port_addr1")) { setupController.cfg->port_addr1 = webServer.arg("port_addr1").toInt(); }

	if (webServer.hasArg("stop_light_duration")) { setupController.cfg->stop_light_duration = webServer.arg("stop_light_duration").toInt(); }
	if (webServer.hasArg("back_light_timeout")) { setupController.cfg->back_light_timeout = webServer.arg("back_light_timeout").toInt(); }

	if (webServer.hasArg("gear0")) { setupController.cfg->gear0 = webServer.arg("gear0").toInt(); }
	if (webServer.hasArg("gear1")) { setupController.cfg->gear1 = webServer.arg("gear1").toInt(); }
	if (webServer.hasArg("gear2")) { setupController.cfg->gear2 = webServer.arg("gear2").toInt(); }

	setupController.saveConfig();

	webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
	webServer.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
	webServer.client().stop(); // Stop is needed because we sent no content length
}


SetupController setupController;