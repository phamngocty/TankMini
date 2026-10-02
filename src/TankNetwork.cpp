#include "TankNetwork.h"
#include "WebDashboard.h"
#include <ArduinoJson.h>
#include <esp_wifi.h>

static TankNetwork* s_tankNetwork = nullptr;
static uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

TankNetwork::TankNetwork()
    : _engine(nullptr),
      _teamId(1),
      _playerId(1),
      _mode(NET_MODE_STANDALONE),
      _server(80),
      _ws("/ws"),
      _savedSsid(""),
      _savedPass(""),
      _lastHudBroadcast(0),
      _lastStateSnapshot(""),
      _staConnecting(false),
      _lastStaCheck(0),
      _disconnectTime(0),
      _activeChannel(WIFI_CHANNEL),
      _lastBeaconTime(0),
      _lastRosterTime(0) {
    memset(_peers, 0, sizeof(_peers));
}

void TankNetwork::begin(TankGameEngine* engine, uint8_t teamId, uint8_t playerId) {
    _engine = engine;
    _teamId = teamId;
    _playerId = playerId;

    // Load custom profile if saved in NVS
    String savedName;
    uint8_t savedTeam = teamId;
    uint8_t savedPlayer = playerId;
    if (loadProfile(savedName, savedTeam, savedPlayer)) {
        _teamId = savedTeam;
        _playerId = savedPlayer;
        if (_engine) {
            _engine->setProfile(savedName, _teamId, _playerId);
        }
        Serial.printf("[NET] Loaded profile from NVS: Callsign='%s', Team=%d, ID=%d\n",
                      savedName.c_str(), _teamId, _playerId);
    }

    // 1. Khởi động tức thì SoftAP trong 200ms (Hybrid Always-On: luôn sẵn sàng lái)
    initSoftAp();
    initWebServer();
    initEspNow();

    // 2. Kết nối Wi-Fi nhà chạy ngầm trong nền (non-blocking) nếu có cấu hình NVS
    loadWifiConfig(_savedSsid, _savedPass);
    if (_savedSsid.length() > 0) {
        Serial.printf("[NET] Found saved Wi-Fi '%s' in NVS. Starting non-blocking background connection...\n", _savedSsid.c_str());
        WiFi.begin(_savedSsid.c_str(), _savedPass.c_str());
        _staConnecting = true;
    } else {
        Serial.println("[NET] No saved Wi-Fi in NVS. Ready in Standalone Mode.");
    }
}

void TankNetwork::initSoftAp() {
    WiFi.mode(WIFI_AP_STA); // AP_STA cho phép chạy song song SoftAP và ESP-NOW/STA

    char apName[32];
    snprintf(apName, sizeof(apName), "Tank_%s_%02d", (_teamId == 1) ? "Blue" : "Red", _playerId);

    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);

    WiFi.softAPConfig(apIP, apIP, netMsk);
    WiFi.softAP(apName, nullptr, WIFI_CHANNEL);

    // Bật Captive Portal DNS 192.168.4.1
    _dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    _dnsServer.start(DNS_PORT, "*", apIP);
    Serial.printf("[NET] Instant SoftAP '%s' active @ 192.168.4.1 (Channel %d)\n", apName, WIFI_CHANNEL);
}

bool TankNetwork::loadWifiConfig(String& ssid, String& pass) {
    Preferences prefs;
    if (prefs.begin("tank_wifi", true)) {
        ssid = prefs.getString("ssid", "");
        pass = prefs.getString("pass", "");
        prefs.end();
    }
    return (ssid.length() > 0);
}

void TankNetwork::saveWifiConfig(const String& ssid, const String& pass) {
    Preferences prefs;
    if (prefs.begin("tank_wifi", false)) {
        prefs.putString("ssid", ssid);
        prefs.putString("pass", pass);
        prefs.end();
        _savedSsid = ssid;
        _savedPass = pass;
        Serial.printf("[NET] Wi-Fi config saved to NVS: SSID='%s'\n", ssid.c_str());
    }
}

void TankNetwork::clearWifiConfig() {
    Preferences prefs;
    if (prefs.begin("tank_wifi", false)) {
        prefs.clear();
        prefs.end();
        _savedSsid = "";
        _savedPass = "";
        Serial.println("[NET] Wi-Fi config cleared from NVS.");
    }
}

bool TankNetwork::loadProfile(String& name, uint8_t& teamId, uint8_t& playerId) {
    Preferences prefs;
    if (prefs.begin("tank_prof", true)) {
        name = prefs.getString("name", "");
        teamId = prefs.getUChar("team", teamId);
        playerId = prefs.getUChar("player", playerId);
        prefs.end();
        return (name.length() > 0);
    }
    return false;
}

void TankNetwork::saveProfile(const String& name, uint8_t teamId, uint8_t playerId) {
    Preferences prefs;
    if (prefs.begin("tank_prof", false)) {
        prefs.putString("name", name);
        prefs.putUChar("team", teamId);
        prefs.putUChar("player", playerId);
        prefs.end();
        Serial.printf("[NET] Profile saved to NVS: Callsign='%s', Team=%d, ID=%d\n",
                      name.c_str(), teamId, playerId);
    }
}

void TankNetwork::clearProfile() {
    Preferences prefs;
    if (prefs.begin("tank_prof", false)) {
        prefs.clear();
        prefs.end();
        Serial.println("[NET] Tank profile cleared from NVS.");
    }
}

void TankNetwork::scanWifiNetworks(AsyncWebSocketClient* client) {
    Serial.println("[NET] Scanning for nearby Wi-Fi networks...");
    int n = WiFi.scanNetworks(false, false);
    StaticJsonDocument<768> doc;
    doc["type"] = "wifi_list";
    JsonArray arr = doc.createNestedArray("networks");
    for (int i = 0; i < n && i < 15; ++i) {
        String ssid = WiFi.SSID(i);
        if (ssid.length() > 0) {
            arr.add(ssid);
        }
    }
    String out;
    serializeJson(doc, out);
    if (client) {
        client->text(out);
    }
    WiFi.scanDelete();
    Serial.printf("[NET] Scan completed: found %d networks.\n", n);
}

void TankNetwork::initEspNow() {
    if (esp_now_init() == ESP_OK) {
        s_tankNetwork = this;
        esp_now_register_recv_cb([](const uint8_t* mac, const uint8_t* data, int len) {
            if (s_tankNetwork) {
                s_tankNetwork->handleEspNowRx(mac, data, len);
            }
        });

        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
        peerInfo.channel = _activeChannel;
        peerInfo.encrypt = false;
        esp_now_add_peer(&peerInfo);
        Serial.printf("[NET] ESP-NOW active on Wi-Fi Channel %d.\n", _activeChannel);
    }
}

void TankNetwork::handleEspNowRx(const uint8_t* mac, const uint8_t* data, int len) {
    if (!data || len <= 0) return;
    uint8_t msgType = data[0];

    if (msgType == ESP_NOW_MSG_KILL && len >= (int)sizeof(EspNowKillMsg)) {
        const EspNowKillMsg* kMsg = (const EspNowKillMsg*)data;
        if (_ws.count() > 0) {
            StaticJsonDocument<256> doc;
            doc["type"] = "kill";
            doc["killerTeam"] = kMsg->killerTeam;
            doc["killerPlayer"] = kMsg->killerPlayer;
            doc["victimTeam"] = kMsg->victimTeam;
            doc["victimPlayer"] = kMsg->victimPlayer;
            String out;
            serializeJson(doc, out);
            _ws.textAll(out);
        }
    } else if (msgType == ESP_NOW_MSG_BEACON && len >= (int)sizeof(EspNowBeaconMsg)) {
        const EspNowBeaconMsg* bMsg = (const EspNowBeaconMsg*)data;
        // Don't track self
        if (bMsg->teamId == _teamId && bMsg->playerId == _playerId) {
            return;
        }

        uint32_t now = millis();
        int foundIdx = -1;
        int emptyIdx = -1;

        for (size_t i = 0; i < MAX_PEERS; ++i) {
            if (_peers[i].active && _peers[i].teamId == bMsg->teamId && _peers[i].playerId == bMsg->playerId) {
                foundIdx = i;
                break;
            }
            if (!_peers[i].active && emptyIdx == -1) {
                emptyIdx = i;
            }
        }

        int targetIdx = (foundIdx != -1) ? foundIdx : emptyIdx;
        if (targetIdx != -1) {
            _peers[targetIdx].active = true;
            _peers[targetIdx].teamId = bMsg->teamId;
            _peers[targetIdx].playerId = bMsg->playerId;
            _peers[targetIdx].hp = bMsg->hp;
            _peers[targetIdx].maxHp = bMsg->maxHp;
            _peers[targetIdx].state = bMsg->state;
            _peers[targetIdx].lastSeen = now;
            strncpy(_peers[targetIdx].callsign, bMsg->callsign, sizeof(_peers[targetIdx].callsign) - 1);
            _peers[targetIdx].callsign[sizeof(_peers[targetIdx].callsign) - 1] = '\0';
        }
    }
}

void TankNetwork::broadcastBeacon() {
    if (!_engine) return;
    EspNowBeaconMsg bMsg;
    bMsg.msgType = ESP_NOW_MSG_BEACON;
    bMsg.teamId = _teamId;
    bMsg.playerId = _playerId;
    bMsg.hp = _engine->getHp();
    bMsg.maxHp = _engine->getMaxHp();
    bMsg.state = (uint8_t)_engine->getState();
    memset(bMsg.callsign, 0, sizeof(bMsg.callsign));
    strncpy(bMsg.callsign, _engine->getCallsign().c_str(), sizeof(bMsg.callsign) - 1);

    esp_now_send(BROADCAST_MAC, (uint8_t*)&bMsg, sizeof(bMsg));
}

void TankNetwork::broadcastRoster() {
    if (_ws.count() == 0) return;
    uint32_t now = millis();

    // Prune peers inactive for >4 seconds
    for (size_t i = 0; i < MAX_PEERS; ++i) {
        if (_peers[i].active && (now - _peers[i].lastSeen > 4000)) {
            _peers[i].active = false;
        }
    }

    StaticJsonDocument<768> doc;
    doc["type"] = "roster";
    doc["myTeam"] = _teamId;
    doc["myPlayerId"] = _playerId;

    JsonArray arr = doc.createNestedArray("peers");
    uint8_t blueCount = (_teamId == 1) ? 1 : 0;
    uint8_t redCount = (_teamId == 2) ? 1 : 0;

    for (size_t i = 0; i < MAX_PEERS; ++i) {
        if (_peers[i].active) {
            JsonObject p = arr.createNestedObject();
            p["team"] = _peers[i].teamId;
            p["id"] = _peers[i].playerId;
            p["name"] = _peers[i].callsign;
            p["hp"] = _peers[i].hp;
            p["maxHp"] = _peers[i].maxHp;
            const char* stateNames[] = {"LOBBY", "BATTLE", "STUN", "RELOAD", "DEAD"};
            p["state"] = (_peers[i].state <= 4) ? stateNames[_peers[i].state] : "BATTLE";

            if (_peers[i].teamId == 1) blueCount++;
            else if (_peers[i].teamId == 2) redCount++;
        }
    }
    doc["blueCount"] = blueCount;
    doc["redCount"] = redCount;

    String out;
    serializeJson(doc, out);
    _ws.textAll(out);
}

void TankNetwork::syncEspNowChannel(uint8_t channel) {
    if (channel == 0 || channel == _activeChannel) return;
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    esp_now_del_peer(BROADCAST_MAC);
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
    peerInfo.channel = channel;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
    _activeChannel = channel;
    Serial.printf("[NET] ESP-NOW & Wi-Fi Channel synced to Channel %d\n", channel);
}

void TankNetwork::broadcastNetworkStatus() {
    if (_ws.count() == 0) return;
    StaticJsonDocument<256> doc;
    doc["type"] = "net_status";
    doc["mode"] = (_mode == NET_MODE_ARENA) ? "arena" : "standalone";
    doc["ip"] = WiFi.localIP().toString();
    doc["ap_ip"] = "192.168.4.1";
    doc["channel"] = _activeChannel;
    String out;
    serializeJson(doc, out);
    _ws.textAll(out);
}

void TankNetwork::initWebServer() {
    _ws.onEvent([this](AsyncWebSocket* s, AsyncWebSocketClient* c, AwsEventType t, void* a, uint8_t* d, size_t l) {
        this->onWsEvent(s, c, t, a, d, l);
    });
    _server.addHandler(&_ws);

    // Serve Cockpit Web HUD (Streaming from PROGMEM directly to avoid 82KB heap allocation)
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/html", (const uint8_t*)INDEX_HTML, sizeof(INDEX_HTML) - 1);
        response->addHeader("Cache-Control", "no-cache");
        request->send(response);
    });

    // Captive Portal Redirects for iOS & Android
    _server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    _server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    _server.on("/canonical.html", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->redirect("/");
    });
    _server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Microsoft NCSI");
    });

    _server.onNotFound([](AsyncWebServerRequest* request) {
        request->redirect("/");
    });

    _server.begin();
}

void TankNetwork::onWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                            AwsEventType type, void* arg, uint8_t* data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        // Gửi trạng thái game HUD và trạng thái mạng khi client kết nối
        if (_engine) {
            client->text(_engine->getStatusJson());
        }
        StaticJsonDocument<256> netDoc;
        netDoc["type"] = "net_status";
        netDoc["mode"] = (_mode == NET_MODE_ARENA) ? "arena" : "standalone";
        netDoc["ip"] = WiFi.localIP().toString();
        netDoc["ap_ip"] = "192.168.4.1";
        netDoc["channel"] = _activeChannel;
        String netOut;
        serializeJson(netDoc, netOut);
        client->text(netOut);
        broadcastRoster();
    } else if (type == WS_EVT_DATA) {
        AwsFrameInfo* info = (AwsFrameInfo*)arg;
        if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
            StaticJsonDocument<384> doc;
            DeserializationError err = deserializeJson(doc, data, len);
            if (!err) {
                const char* msgType = doc["type"];
                if (msgType) {
                    if (strcmp(msgType, "drive") == 0 && _engine) {
                        int16_t left = doc["left"] | 0;
                        int16_t right = doc["right"] | 0;
                        _engine->handleDrive(left, right);
                    } else if (strcmp(msgType, "fire") == 0 && _engine) {
                        _engine->handleFire();
                        broadcastHud();
                    } else if (strcmp(msgType, "reload") == 0 && _engine) {
                        _engine->handleManualReload();
                        broadcastHud();
                    } else if (strcmp(msgType, "ulti") == 0 && _engine) {
                        _engine->handleUlti();
                        broadcastHud();
                    } else if (strcmp(msgType, "reset") == 0 && _engine) {
                        _engine->resetGame();
                        broadcastHud();
                    } else if (strcmp(msgType, "engine") == 0 && _engine) {
                        bool start = doc["start"] | false;
                        _engine->setEngineStarted(start);
                        broadcastHud();
                    } else if (strcmp(msgType, "engine_crank") == 0 && _engine) {
                        _engine->triggerIgnitionCrank();
                    } else if (strcmp(msgType, "set_profile") == 0) {
                        const char* name = doc["name"];
                        uint8_t newTeam = doc["team"] | _teamId;
                        uint8_t newPlayer = doc["id"] | _playerId;
                        String nameStr = name ? String(name) : "";
                        if (nameStr.length() == 0) {
                            nameStr = (newTeam == 1) ? ("Tiger-" + String(newPlayer)) : ("Dragon-" + String(newPlayer));
                        }
                        _teamId = newTeam;
                        _playerId = newPlayer;
                        saveProfile(nameStr, _teamId, _playerId);
                        if (_engine) {
                            _engine->setProfile(nameStr, _teamId, _playerId);
                        }
                        if (client) {
                            client->text("{\"type\":\"profile_status\",\"status\":\"saved\"}");
                        }
                        broadcastHud();
                        broadcastBeacon();
                        broadcastRoster();
                    } else if (strcmp(msgType, "scan_wifi") == 0) {
                        scanWifiNetworks(client);
                    } else if (strcmp(msgType, "save_wifi") == 0) {
                        const char* ssid = doc["ssid"];
                        const char* pass = doc["pass"] | "";
                        if (ssid && strlen(ssid) > 0) {
                            saveWifiConfig(String(ssid), String(pass));
                            if (client) {
                                client->text("{\"type\":\"wifi_status\",\"status\":\"saved\",\"msg\":\"Đã lưu Wi-Fi. Đang kết nối trong nền...\"}");
                            }
                            // Thử kết nối ngay trong nền, không cần reboot
                            WiFi.begin(ssid, pass);
                            _staConnecting = true;
                        }
                    } else if (strcmp(msgType, "reset_wifi") == 0) {
                        clearWifiConfig();
                        WiFi.disconnect(false, true);
                        _mode = NET_MODE_STANDALONE;
                        syncEspNowChannel(WIFI_CHANNEL);
                        if (client) {
                            client->text("{\"type\":\"wifi_status\",\"status\":\"cleared\",\"msg\":\"Đã quên Wi-Fi. Xe duy trì SoftAP 192.168.4.1 (Kênh 1)\"}");
                        }
                        broadcastNetworkStatus();
                    }
                }
            }
        }
    }
}

void TankNetwork::broadcastHud() {
    if (!_engine || _ws.count() == 0) return;
    String status = _engine->getStatusJson();
    _ws.textAll(status);
}

void TankNetwork::broadcastKillFeed(uint8_t killerTeam, uint8_t killerPlayer, uint8_t victimTeam, uint8_t victimPlayer) {
    EspNowKillMsg msg;
    msg.msgType = ESP_NOW_MSG_KILL;
    msg.killerTeam = killerTeam;
    msg.killerPlayer = killerPlayer;
    msg.victimTeam = victimTeam;
    msg.victimPlayer = victimPlayer;

    esp_now_send(BROADCAST_MAC, (uint8_t*)&msg, sizeof(msg));
}

void TankNetwork::update() {
    uint32_t now = millis();

    // 1. Luôn xử lý Captive Portal DNS queries (SoftAP luôn chạy song song)
    _dnsServer.processNextRequest();

    _ws.cleanupClients();

    // 2. Kiểm tra trạng thái Wi-Fi nhà phi chặn (Non-blocking STA Monitor & Auto-Recovery)
    if (now - _lastStaCheck >= 500) {
        _lastStaCheck = now;
        if (_savedSsid.length() > 0) {
            if (WiFi.status() == WL_CONNECTED) {
                if (_mode != NET_MODE_ARENA) {
                    _mode = NET_MODE_ARENA;
                    _staConnecting = false;
                    _disconnectTime = 0;
                    uint8_t rChannel = WiFi.channel();
                    Serial.printf("[NET] Arena Wi-Fi Connected! IP: %s | Router Channel: %d\n",
                                  WiFi.localIP().toString().c_str(), rChannel);
                    syncEspNowChannel(rChannel);
                    broadcastNetworkStatus();
                }
            } else {
                if (_mode == NET_MODE_ARENA) {
                    if (_disconnectTime == 0) {
                        _disconnectTime = now;
                    } else if (now - _disconnectTime >= 2000) {
                        Serial.println("[NET] Lost Arena Wi-Fi for >2s! Auto-fallback to Channel 1 Standalone.");
                        _mode = NET_MODE_STANDALONE;
                        syncEspNowChannel(WIFI_CHANNEL);
                        broadcastNetworkStatus();
                    }
                }
            }
        }
    }

    // 3. Broadcast HUD updates to clients every 100ms if changed
    if (now - _lastHudBroadcast >= 100) {
        _lastHudBroadcast = now;
        if (_engine && _ws.count() > 0) {
            String currentStatus = _engine->getStatusJson();
            if (currentStatus != _lastStateSnapshot) {
                _lastStateSnapshot = currentStatus;
                _ws.textAll(currentStatus);
            }
        }
    }

    // 4. ESP-NOW Beacon & Team Roster update every 1000ms
    if (now - _lastBeaconTime >= 1000) {
        _lastBeaconTime = now;
        broadcastBeacon();
        broadcastRoster();
    }
}
