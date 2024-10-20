#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bus.h"

struct ee_bus* ee_bus_create(void) {
    return malloc(sizeof(struct ee_bus));
}

void ee_bus_init(struct ee_bus* bus, const char* bios_path) {
    memset(bus, 0, sizeof(struct ee_bus));

    bus->ee_ram = ps2_ram_create();

    ps2_ram_init(bus->ee_ram, RAM_SIZE_32MB);
}

void ee_bus_init_bios(struct ee_bus* bus, struct ps2_bios* bios) {
    bus->bios = bios;
}

void ee_bus_init_iop_ram(struct ee_bus* bus, struct ps2_ram* iop_ram) {
    bus->iop_ram = iop_ram;
}

void ee_bus_init_sif(struct ee_bus* bus, struct ps2_sif* sif) {
    bus->sif = sif;
}

void ee_bus_destroy(struct ee_bus* bus) {
    ps2_ram_destroy(bus->ee_ram);

    free(bus);
}

#define MAP_READ(b, l, u, d, n) \
    if ((addr >= l) && (addr <= u)) return ps2_ ## d ## _read ## b (bus->n, addr - l);

#define MAP_WRITE(b, l, u, d, n) \
    if ((addr >= l) && (addr <= u)) { ps2_ ## d ## _write ## b (bus->n, addr - l, data); return; }

uint64_t ee_bus_read8(void* udata, uint32_t addr) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_READ(8, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_READ(8, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_READ(8, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 8-bit read from physical address 0x%08x\n", addr);

    return 0;
}

uint64_t ee_bus_read16(void* udata, uint32_t addr) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_READ(16, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_READ(16, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_READ(16, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 16-bit read from physical address 0x%08x\n", addr);

    return 0;
}

static int timer0 = 0;

uint64_t ee_bus_read32(void* udata, uint32_t addr) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_READ(32, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_READ(32, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_READ(32, 0x1FC00000, 0x1FFFFFFF, bios, bios);
    MAP_READ(32, 0x1000F200, 0x1000F26F, sif, sif);

    switch (addr) {
        case 0x10000000: return timer0++;
        case 0x1000F430: return 0;
        case 0x1000F440: {
            uint8_t sop = (bus->mch_ricm >> 6) & 0xF;
            uint8_t sa = (bus->mch_ricm >> 16) & 0xFFF;

            if (!sop) {
                switch (sa) {
                    case 0x21: {
                        if (bus->rdram_sdevid < 2) {
                            bus->rdram_sdevid++;

                            return 0x1F;
                        }

                        return 0;
                    } break;

                    case 0x23: return 0x0D0D;
                    case 0x24: return 0x0090;
                    case 0x40: return bus->mch_ricm & 0x1F;
                }
            }

            return 0;
        } break;
    }

    // printf("bus: Unhandled 32-bit read from physical address 0x%08x\n", addr);

    return 0;
}

uint64_t ee_bus_read64(void* udata, uint32_t addr) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_READ(64, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_READ(64, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_READ(64, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 64-bit read from physical address 0x%08x\n", addr);

    return 0;
}

uint128_t ee_bus_read128(void* udata, uint32_t addr) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_READ(128, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_READ(128, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_READ(128, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 128-bit read from physical address 0x%08x\n", addr);

    return (uint128_t)0;
}

void ee_bus_write8(void* udata, uint32_t addr, uint64_t data) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_WRITE(8, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_WRITE(8, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_WRITE(8, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    if (addr == 0x1000f180) { putchar(data & 0xff); return; }

    printf("bus: Unhandled 8-bit write to physical address 0x%08x (0x%02lx)\n", addr, data);
}

void ee_bus_write16(void* udata, uint32_t addr, uint64_t data) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_WRITE(16, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_WRITE(16, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_WRITE(16, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 16-bit write to physical address 0x%08x (0x%04lx)\n", addr, data);
}

void ee_bus_write32(void* udata, uint32_t addr, uint64_t data) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_WRITE(32, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_WRITE(32, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_WRITE(32, 0x1FC00000, 0x1FFFFFFF, bios, bios);
    MAP_WRITE(32, 0x1000F200, 0x1000F26F, sif, sif);

    switch (addr) {
        case 0x1000f200: { printf("EE-side SIF_MSCOM write %08lx\n", data); } break;
        case 0x1000f220: { printf("EE-side SIF_MSFLG write %08lx\n", data); } break;
        case 0x1000f230: { printf("EE-side SIF_SMFLG write %08lx\n", data); } break;
        case 0x1000f430: {
            uint8_t sa = (data >> 16) & 0xFFF;
            uint8_t sbc = (data >> 6) & 0xF;

            if ((sa == 0x21) && (sbc == 0x1) && ((bus->mch_drd >> 7) & 1) == 0)
                bus->rdram_sdevid = 0;

            bus->mch_ricm = data & ~0x80000000;
        } return;
        case 0x1000f440: {
            bus->mch_drd = data;
        } return;
    }

    // printf("bus: Unhandled 32-bit write to physical address 0x%08x (0x%08lx)\n", addr, data);
}

void ee_bus_write64(void* udata, uint32_t addr, uint64_t data) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_WRITE(64, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_WRITE(64, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_WRITE(64, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 64-bit write to physical address 0x%08x (0x%08lx%08lx)\n", addr, data >> 32, data & 0xffffffff);
}

void ee_bus_write128(void* udata, uint32_t addr, uint128_t data) {
    struct ee_bus* bus = (struct ee_bus*)udata;

    MAP_WRITE(128, 0x00000000, 0x01FFFFFF, ram, ee_ram);
    MAP_WRITE(128, 0x1C000000, 0x1C1FFFFF, ram, iop_ram);
    MAP_WRITE(128, 0x1FC00000, 0x1FFFFFFF, bios, bios);

    // printf("bus: Unhandled 128-bit write to physical address 0x%08x (0x%08x%08x%08x%08x)\n", addr, data.u32[3], data.u32[2], data.u32[1], data.u32[0]);
}
