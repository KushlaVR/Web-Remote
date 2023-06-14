#include "SetupController.h"

SetupController::SetupController()
{
}

SetupController::~SetupController()
{
}

void SetupController::setup()
{
	webServer.on("/api/setup", HTTPMethod::HTTP_GET, Setup_Get);
	webServer.on("/api/setup", HTTPMethod::HTTP_POST, Setup_Post);
}

void SetupController::Setup_Get()
{
	JsonString ret = JsonString();
	setupController.buildConfig(&ret);
	webServer.jsonOk(&ret);
}

void SetupController::Setup_Post()
{
	int cnt = webServer.args();
	JsonString ret = JsonString();
	ret.beginObject();
	for (int i = 0; i < cnt; i++)
	{
		ret.AddValue(webServer.argName(i), webServer.arg(i));
	}
	ret.endObject();

	if (setupController.saveConfig != nullptr)
		setupController.saveConfig(&ret);

	webServer.sendHeader("Location", String("http://") + WebUIController::ipToString(webServer.client().localIP()), true);
	webServer.send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
	webServer.client().stop();			   // Stop is needed because we sent no content length
}

SetupController setupController;
