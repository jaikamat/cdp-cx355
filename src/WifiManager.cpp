#include "WiFiManager.hpp"
#include <Arduino.h>

WiFiManager::WiFiManager(const char *ssid, const char *password)
    : ssid(ssid), password(password), server(80) {}

void WiFiManager::connect()
{
    // Check for the WiFi module
    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("Communication with WiFi module failed!");
        while (true)
            ; // Halt execution
    }

    // Check firmware version
    String fv = WiFi.firmwareVersion();
    if (fv < WIFI_FIRMWARE_LATEST_VERSION)
    {
        Serial.println("Please upgrade the firmware");
    }

    // Attempt to connect to WiFi
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);

    while (WiFi.status() != WL_CONNECTED)
    {
        WiFi.begin(ssid, password);
        delay(10000); // Wait 10 seconds
    }

    server.begin();
    Serial.println("WiFi connected.");
    printStatus();
}

void WiFiManager::printStatus()
{
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    long rssi = WiFi.RSSI();
    Serial.print("Signal strength (RSSI): ");
    Serial.print(rssi);
    Serial.println(" dBm");

    Serial.print("Server running at: http://");
    Serial.println(ip);
}

WiFiServer &WiFiManager::getServer()
{
    return server; // Ensure this returns the correct reference
}
