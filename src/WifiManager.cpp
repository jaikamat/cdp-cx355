#include "WiFiManager.hpp"
#include <Arduino.h>

WiFiManager::WiFiManager(const char *ssid, const char *password)
    : ssid(ssid), password(password), server(80), mdns(udp) {}

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

    // Initialize mDNS
    Serial.print("Starting mDNS with hostname: ");
    Serial.println(hostname);
    if (mdns.begin(WiFi.localIP(), hostname))
    {
        Serial.print("mDNS responder started: ");
        Serial.print(hostname);
        Serial.println(".local");

        // Add service to mDNS-SD
        mdns.addServiceRecord("sony-remote._http", 80, MDNSServiceTCP);
    }
    else
    {
        Serial.println("Error setting up mDNS responder!");
    }

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
    Serial.print("Also available at: http://");
    Serial.print(hostname);
    Serial.println(".local");
}

WiFiServer &WiFiManager::getServer()
{
    return server; // Ensure this returns the correct reference
}

const char *WiFiManager::getHostname()
{
    return hostname;
}

void WiFiManager::runMDNS()
{
    mdns.run();
}
