#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#include "Json.h"
#include "Console.h"
#include "Collection.h"
#include "WebUIController.h"
#include "Joypad.h"
#include "Button.h"

char SSID[32] = "ONE_BUTTON";
char SSID_password[20] = "12345678";
IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;

JoypadCollection joypads = JoypadCollection();

int button_counter = 0;

void btnOne_Press()
{
  button_counter++;
  joypads.setValue("button_counter", button_counter);
}

VirtualButton buttonOne(btnOne_Press);

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

void setup()
{
  // put your setup code here, to run once:
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

  console.println(("Starting..."));

  WiFi.begin();
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
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

  buttonOne.condition = 1;
}

void loop()
{
  // put your main code here, to run repeatedly:
  dnsServer.processNextRequest();
  joypads.loop();
  webServer.loop();

  if (joypads.getCount() > 0)
  {
    buttonOne.setValue(joypads.getValue("button_one"));
  }
  else
  {
    // noone connected...
  }
}
