#pragma once

#include <Arduino.h>

struct IrPacket {
    uint8_t teamId;    // 0 - 15 (e.g., 1 = Blue, 2 = Red)
    uint8_t playerId;  // 0 - 15 (Tank ID)
    uint8_t damage;    // 1 - 15 (Damage dealt per hit)
    uint8_t checksum;  // 4-bit XOR checksum

    static uint8_t calcChecksum(uint8_t team, uint8_t player, uint8_t dmg) {
        return (team ^ player ^ dmg) & 0x0F;
    }

    static uint16_t encode(uint8_t team, uint8_t player, uint8_t dmg) {
        uint8_t cs = calcChecksum(team, player, dmg);
        return ((team & 0x0F) << 12) |
               ((player & 0x0F) << 8) |
               ((dmg & 0x0F) << 4) |
               (cs & 0x0F);
    }

    static bool decode(uint16_t raw, IrPacket &out) {
        out.teamId = (raw >> 12) & 0x0F;
        out.playerId = (raw >> 8) & 0x0F;
        out.damage = (raw >> 4) & 0x0F;
        out.checksum = raw & 0x0F;
        return (out.checksum == calcChecksum(out.teamId, out.playerId, out.damage));
    }
};
