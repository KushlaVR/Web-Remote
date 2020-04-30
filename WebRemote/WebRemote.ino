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



IPAddress apIP = IPAddress(192, 168, 4, 1);
IPAddress netMsk = IPAddress(255, 255, 255, 0);

const byte DNS_PORT = 53;
DNSServer dnsServer;

WiFiServer server(8888);
WiFiClient client;


void setup()
{
    Serial.begin(115200);
    WiFi.begin();
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Test", "1234567890");
    server.begin();


    /* Setup the DNS server redirecting all the domains to the apIP */
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", apIP);


    Serial.println("");
    Serial.println(WiFi.localIP());
}

void loop()
{
    dnsServer.processNextRequest();

    if (!client.connected())
    {
        client = server.available();
    }
    else
    {
        if (client.available() > 0)
        {
            Serial.write(client.read());
        }
    }
}