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

    // WiFi.begin() blocks until it either joins or this timeout passes.
    // The default is 10 s, which is longer than the watchdog allows.
    WiFi.setTimeout(WIFI_JOIN_TIMEOUT_MS);

    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);

    // The watchdog is not running yet during setup(), so looping here
    // indefinitely is safe. There is nothing useful to do without a network.
    while (WiFi.begin(ssid, password) != WL_CONNECTED)
    {
        Serial.println("Join failed, retrying...");
    }

    server.begin();
    Serial.println("WiFi connected.");

    // Associated, but DHCP may still be running. Give it a few seconds so
    // mDNS can be bound before setup() returns; if it is still not done,
    // ensureConnected() binds it on a later poll.
    unsigned long start = millis();
    while (WiFi.localIP() == IPAddress(0, 0, 0, 0) && millis() - start < DHCP_WAIT_MS)
    {
        delay(100);
    }
    bindMDNSIfIpChanged();
}

// Re-join the network if the association was lost (router reboot, AP channel
// change, signal dropout). Cheap when the link is up: one modem status query
// every LINK_CHECK_INTERVAL_MS. When the link is down this blocks for at most
// WIFI_JOIN_TIMEOUT_MS per call, then returns so loop() keeps running; the
// next call retries.
//
// The HTTP server socket lives on the ESP32 and survives re-association, so
// it is not restarted. mDNS is rebound because the IP address may have changed.
void WiFiManager::ensureConnected()
{
    unsigned long now = millis();
    if (now - lastLinkCheckMs < LINK_CHECK_INTERVAL_MS)
        return;
    lastLinkCheckMs = now;

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi link lost. Reconnecting...");
        if (WiFi.begin(ssid, password) != WL_CONNECTED)
        {
            Serial.println("Reconnect failed; will retry.");
            return;
        }
        Serial.println("WiFi reconnected.");
        // DHCP is probably still running; the check below binds mDNS on
        // this poll if the address is already here, otherwise on a later one.
    }

    bindMDNSIfIpChanged();
}

// Bind (or rebind) mDNS when the IP address becomes known or changes.
// Does nothing while the address is still 0.0.0.0 (DHCP pending) or when it
// matches the address mDNS is already answering with. Advertising 0.0.0.0
// makes "sony-remote.local" resolve to nothing, so the zero check matters.
void WiFiManager::bindMDNSIfIpChanged()
{
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0) || ip == mdnsBoundIp)
        return;
    startMDNS(ip);
    printStatus();
}

// (Re)start the mDNS responder on `ip`. Safe to call more than once: the
// multicast socket is closed first (a no-op on first use), and the HTTP
// service record is only registered the first time because the MDNS object
// keeps its record table across begin() calls.
void WiFiManager::startMDNS(const IPAddress &ip)
{
    udp.stop();

    Serial.print("Starting mDNS with hostname: ");
    Serial.println(hostname);
    if (mdns.begin(ip, hostname))
    {
        mdnsBoundIp = ip;
        Serial.print("mDNS responder started: ");
        Serial.print(hostname);
        Serial.print(".local -> ");
        Serial.println(ip);

        if (!serviceRecordAdded)
        {
            mdns.addServiceRecord("sony-remote._http", 80, MDNSServiceTCP);
            serviceRecordAdded = true;
        }
    }
    else
    {
        Serial.println("Error setting up mDNS responder!");
    }
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
    return server;
}

const char *WiFiManager::getHostname()
{
    return hostname;
}

void WiFiManager::runMDNS()
{
    mdns.run();
}
