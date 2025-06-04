#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "speed.h"

struct ps2_speed* ps2_speed_create(void) {
    return malloc(sizeof(struct ps2_speed));
}

void ps2_speed_init(struct ps2_speed* speed) {
    memset(speed, 0, sizeof(struct ps2_speed));

    speed->rev3 |= SPD_CAPS_FLASH | SPD_CAPS_ATA | SPD_CAPS_DVR;
}

void ps2_speed_destroy(struct ps2_speed* speed) {
    free(speed);
}

uint64_t ps2_speed_read8(struct ps2_speed* speed, uint32_t addr) {
    switch (addr & 0xfff) {
        case 0x10000000 & 0xfff: return speed->rev;
        case 0x10000002 & 0xfff: return speed->rev1;
        case 0x10000004 & 0xfff: return speed->rev3;
    }

    printf("speed: read8 %08x\n", addr); // exit(1);
}
uint64_t ps2_speed_read16(struct ps2_speed* speed, uint32_t addr) {
    switch (addr & 0xfff) {
        case 0x10000000 & 0xfff: return speed->rev;
        case 0x10000002 & 0xfff: return speed->rev1;
        case 0x10000004 & 0xfff: return speed->rev3;
        case 0x1000002a & 0xfff: return speed->intr_mask;
    }

    printf("speed: read16 %08x\n", addr); // exit(1);
}
uint64_t ps2_speed_read32(struct ps2_speed* speed, uint32_t addr) {
    printf("speed: read32 %08x\n", addr); // exit(1);
}
void ps2_speed_write8(struct ps2_speed* speed, uint32_t addr, uint64_t data) {
    printf("speed: write8 %08x %08x\n", addr, data); // exit(1);
}
void ps2_speed_write16(struct ps2_speed* speed, uint32_t addr, uint64_t data) {
    switch (addr & 0xfff) {
        // case 0x10000000 & 0xfff: return speed->rev;
        // case 0x10000002 & 0xfff: return speed->rev1;
        // case 0x10000004 & 0xfff: return speed->rev3;
        case 0x1000002a & 0xfff: speed->intr_mask = data; return;
    }

    printf("speed: write16 %08x %08x\n", addr, data); // exit(1);
}
void ps2_speed_write32(struct ps2_speed* speed, uint32_t addr, uint64_t data) {
    printf("speed: write32 %08x %08x\n", addr, data); // exit(1);
}