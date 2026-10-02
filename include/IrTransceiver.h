#pragma once

#include <Arduino.h>
#include <functional>
#include "IrProtocol.h"

class IrTransceiver {
public:
    static constexpr uint8_t PIN_TX = 4;
    static constexpr uint8_t PIN_RX = 5;

    using HitCallback = std::function<void(const IrPacket&)>;

    IrTransceiver();

    void begin(uint8_t myTeamId, uint8_t myPlayerId);
    bool fire(uint8_t damage = 1);
    void update();

    void setOnHitCallback(HitCallback cb) { _onHitCallback = cb; }
    void setImmune(bool immune) { _isImmune = immune; }
    bool isImmune() const { return _isImmune; }

    uint8_t getTeamId() const { return _myTeamId; }
    uint8_t getPlayerId() const { return _myPlayerId; }
    void setIdentity(uint8_t teamId, uint8_t playerId) {
        _myTeamId = teamId;
        _myPlayerId = playerId;
    }

    // Internal ISR handler
    void handleRxInterrupt();

private:
    uint8_t _myTeamId;
    uint8_t _myPlayerId;
    bool _isImmune;
    HitCallback _onHitCallback;

    void* _rmtTxObj; // rmt_obj_t* opaque handle

    // RX State Machine variables
    volatile uint32_t _lastEdgeTime;
    volatile uint16_t _rxBuffer;
    volatile uint8_t _rxBitCount;
    volatile bool _rxReceiving;
    volatile bool _newPacketReady;
    volatile uint16_t _receivedRaw;
};
