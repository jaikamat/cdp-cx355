#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include "WiFiS3.h"
#include <WiFiUdp.h>
#include <ArduinoMDNS.h>

// Owns the WiFi association, the HTTP server socket and the mDNS responder.
//
// Lifecycle:
//   connect()          - called once from setup(); blocks until the network
//                        is joined, then starts the server and mDNS.
//   ensureConnected()  - called every loop(); every LINK_CHECK_INTERVAL_MS it
//                        asks the modem whether the link is still up and, if
//                        not, re-joins. It also (re)binds mDNS whenever the
//                        IP address appears or changes, because WiFi.begin()
//                        returns when associated, which is before DHCP has
//                        handed out an address.
//   runMDNS()          - called every loop(); services mDNS queries.
//
// Timing contract: nothing in here may block longer than the watchdog period
// configured in main.cpp. WiFi.begin() is the only long call, and connect()
// caps it with WiFi.setTimeout(WIFI_JOIN_TIMEOUT_MS).
class WiFiManager
{
public:
    WiFiManager(const char *ssid, const char *password);

    void connect();
    void ensureConnected();
    void printStatus();
    WiFiServer &getServer();
    const char *getHostname();
    void runMDNS();

private:
    // Upper bound for one WiFi.begin() call. Keep below WATCHDOG_TIMEOUT_MS.
    static constexpr unsigned long WIFI_JOIN_TIMEOUT_MS = 4000;
    // How often ensureConnected() polls the modem for link state.
    static constexpr unsigned long LINK_CHECK_INTERVAL_MS = 5000;
    // How long connect() waits for DHCP after association before giving up
    // and leaving the mDNS bind to ensureConnected(). Only used in setup(),
    // where the watchdog is not yet running.
    static constexpr unsigned long DHCP_WAIT_MS = 10000;

    void startMDNS(const IPAddress &ip);
    void bindMDNSIfIpChanged();

    const char *ssid;
    const char *password;
    WiFiServer server;
    const char *hostname = "sony-remote";
    WiFiUDP udp;
    MDNS mdns;
    bool serviceRecordAdded = false;
    IPAddress mdnsBoundIp;          // 0.0.0.0 until mDNS has been bound
    unsigned long lastLinkCheckMs = 0;
};

#endif // WIFI_MANAGER_HPP
