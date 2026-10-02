#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <esp_now.h>
#include <Preferences.h>
#include "TankGameEngine.h"

enum NetworkMode {
    NET_MODE_STANDALONE = 0,
    NET_MODE_ARENA
};

enum EspNowMsgType {
    ESP_NOW_MSG_KILL = 0x01,
    ESP_NOW_MSG_BEACON = 0x02
};

struct EspNowKillMsg {
    uint8_t msgType; // 0x01 = KILL_FEED
    uint8_t killerTeam;
    uint8_t killerPlayer;
    uint8_t victimTeam;
    uint8_t victimPlayer;
};

struct EspNowBeaconMsg {
    uint8_t msgType;    // 0x02 = BEACON
    uint8_t teamId;     // 1 = Blue, 2 = Red
    uint8_t playerId;   // 1 - 15
    uint8_t hp;         // 0 - 5
    uint8_t maxHp;      // 5
    uint8_t state;      // 0..4 TankState
    char callsign[16];  // e.g. "Tiger-01"
};

struct PeerInfo {
    uint8_t teamId;
    uint8_t playerId;
    uint8_t hp;
    uint8_t maxHp;
    uint8_t state;
    char callsign[16];
    uint32_t lastSeen;
    bool active;
};

class TankNetwork {
public:
    enum {
        WIFI_CHANNEL = 1,
        DNS_PORT = 53,
        MAX_PEERS = 12
    };

    TankNetwork();

    void begin(TankGameEngine* engine, uint8_t teamId, uint8_t playerId);
    void update();

    void broadcastHud();
    void broadcastKillFeed(uint8_t killerTeam, uint8_t killerPlayer, uint8_t victimTeam, uint8_t victimPlayer);
    void broadcastBeacon();
    void broadcastRoster();

    bool loadWifiConfig(String& ssid, String& pass);
    void saveWifiConfig(const String& ssid, const String& pass);
    void clearWifiConfig();
    void scanWifiNetworks(AsyncWebSocketClient* client);
    void syncEspNowChannel(uint8_t channel);
    void broadcastNetworkStatus();

    bool loadProfile(String& name, uint8_t& teamId, uint8_t& playerId);
    void saveProfile(const String& name, uint8_t teamId, uint8_t playerId);
    void clearProfile();

    void handleEspNowRx(const uint8_t* mac, const uint8_t* data, int len);

    NetworkMode getMode() const { return _mode; }
    String getSavedSsid() const { return _savedSsid; }
    uint8_t getTeamId() const { return _teamId; }
    uint8_t getPlayerId() const { return _playerId; }

private:
    void initSoftAp();
    void initWebServer();
    void initEspNow();
    void onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                   AwsEventType type, void* arg, uint8_t* data, size_t len);

    TankGameEngine* _engine;
    uint8_t _teamId;
    uint8_t _playerId;

    NetworkMode _mode;
    DNSServer _dnsServer;
    AsyncWebServer _server;
    AsyncWebSocket _ws;

    String _savedSsid;
    String _savedPass;

    uint32_t _lastHudBroadcast;
    String _lastStateSnapshot;

    bool _staConnecting;
    uint32_t _lastStaCheck;
    uint32_t _disconnectTime;
    uint8_t _activeChannel;

    // ESP-NOW Mesh & Team Tracking
    PeerInfo _peers[MAX_PEERS];
    uint32_t _lastBeaconTime;
    uint32_t _lastRosterTime;
};
