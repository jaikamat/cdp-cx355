#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <ArduinoMDNS.h>

class WiFiManager
{
public:
    WiFiManager(const char *ssid, const char *password);

    void connect();
    void printStatus();
    WiFiServer &getServer();
    const char* getHostname();
    void runMDNS();

private:
    const char *ssid;
    const char *password;
    WiFiServer server;
    const char *hostname = "sony-remote";
    WiFiUDP udp;
    MDNS mdns;
};

#endif // WIFI_MANAGER_HPP
