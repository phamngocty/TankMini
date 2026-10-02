#include "IrTransceiver.h"
#include <esp32-hal-rmt.h>

static IrTransceiver* s_instance = nullptr;

static void IRAM_ATTR rxPinChangeIsr() {
    if (s_instance) {
        s_instance->handleRxInterrupt();
    }
}

IrTransceiver::IrTransceiver()
    : _myTeamId(1),
      _myPlayerId(1),
      _isImmune(false),
      _onHitCallback(nullptr),
      _rmtTxObj(nullptr),
      _lastEdgeTime(0),
      _rxBuffer(0),
      _rxBitCount(0),
      _rxReceiving(false),
      _newPacketReady(false),
      _receivedRaw(0) {
    s_instance = this;
}

void IrTransceiver::begin(uint8_t myTeamId, uint8_t myPlayerId) {
    _myTeamId = myTeamId;
    _myPlayerId = myPlayerId;

    // 1. Configure RMT TX for 38kHz IR Output on GPIO 4
    rmt_obj_t* rmt = rmtInit(PIN_TX, RMT_TX_MODE, RMT_MEM_64);
    if (rmt) {
        rmtSetTick(rmt, 1000); // 1 tick = 1000ns (1us)
        // 38kHz carrier with 33% duty cycle at 80MHz clock (High = 700 cycles, Low = 1405 cycles)
        rmtSetCarrier(rmt, true, 1, 1405, 700);
        _rmtTxObj = (void*)rmt;
    }

    // 2. Configure TSOP38238 Active-LOW Input on GPIO 5
    pinMode(PIN_RX, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_RX), rxPinChangeIsr, CHANGE);
}

bool IrTransceiver::fire(uint8_t damage) {
    if (!_rmtTxObj) return false;

    uint16_t encoded = IrPacket::encode(_myTeamId, _myPlayerId, damage);

    // Frame: 1 Leader (4.5ms H + 4.5ms L) + 16 Data Bits + 1 Stop Bit = 18 items
    rmt_data_t items[18];

    // Leader pulse
    items[0].duration0 = 4500;
    items[0].level0 = 1;
    items[0].duration1 = 4500;
    items[0].level1 = 0;

    // 16 Data bits (MSB first)
    for (int i = 0; i < 16; i++) {
        bool bit = (encoded >> (15 - i)) & 0x01;
        items[i + 1].duration0 = 560;
        items[i + 1].level0 = 1;
        items[i + 1].duration1 = bit ? 1690 : 560;
        items[i + 1].level1 = 0;
    }

    // Stop bit
    items[17].duration0 = 560;
    items[17].level0 = 1;
    items[17].duration1 = 0;
    items[17].level1 = 0;

    return rmtWrite((rmt_obj_t*)_rmtTxObj, items, 18);
}

void IRAM_ATTR IrTransceiver::handleRxInterrupt() {
    uint32_t now = micros();
    uint32_t dur = now - _lastEdgeTime;
    _lastEdgeTime = now;

    bool pin = digitalRead(PIN_RX);

    // TSOP38238 is active LOW.
    // pin == HIGH means a carrier burst (LOW state) just ended.
    // pin == LOW means an idle space (HIGH state) just ended.

    if (pin == HIGH) {
        // Just finished a LOW carrier mark
        if (dur >= 3800 && dur <= 5200) {
            // Potential leader mark (4500us)
            _rxReceiving = true;
            _rxBitCount = 0;
            _rxBuffer = 0;
        }
    } else {
        // Just finished a HIGH space
        if (_rxReceiving) {
            if (_rxBitCount == 0 && (dur < 3500 || dur > 5500)) {
                // Invalid leader space (expected ~4500us)
                _rxReceiving = false;
                return;
            }

            if (_rxBitCount > 0) {
                if (dur >= 300 && dur <= 900) {
                    // Bit 0 (~560us space)
                    _rxBuffer = (_rxBuffer << 1);
                    _rxBitCount++;
                } else if (dur >= 1300 && dur <= 2100) {
                    // Bit 1 (~1690us space)
                    _rxBuffer = (_rxBuffer << 1) | 0x01;
                    _rxBitCount++;
                } else {
                    // Noise or frame error
                    _rxReceiving = false;
                    _rxBitCount = 0;
                    return;
                }

                if (_rxBitCount == 16) {
                    _receivedRaw = _rxBuffer;
                    _newPacketReady = true;
                    _rxReceiving = false;
                    _rxBitCount = 0;
                }
            } else {
                // Just verified leader space, next bits will be payload
                _rxBitCount = 1; // start recording bits
            }
        }
    }
}

void IrTransceiver::update() {
    if (_newPacketReady) {
        _newPacketReady = false;
        uint16_t raw = _receivedRaw;

        IrPacket packet;
        if (IrPacket::decode(raw, packet)) {
            // Friendly fire rejection: Ignore hits from own team
            if (packet.teamId != _myTeamId) {
                // If not immune (I-Frame), trigger hit callback
                if (!_isImmune && _onHitCallback) {
                    _onHitCallback(packet);
                }
            }
        }
    }
}
