#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "WiFiS3.h"

class WiFiManager
{
public:
    WiFiManager(const char *ssid, const char *password);

    void connect();
    void printStatus();
    WiFiServer &getServer();

private:
    const char *ssid;
    const char *password;
    WiFiServer server;
};

#endif // WIFI_MANAGER_HPP
