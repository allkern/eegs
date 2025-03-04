#ifndef EE_H
#define EE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "u128.h"

#include "vu.h"

struct ee_bus_s {
    void* udata;
    uint64_t (*read8)(void* udata, uint32_t addr);
    uint64_t (*read16)(void* udata, uint32_t addr);
    uint64_t (*read32)(void* udata, uint32_t addr);
    uint64_t (*read64)(void* udata, uint32_t addr);
    uint128_t (*read128)(void* udata, uint32_t addr);
    void (*write8)(void* udata, uint32_t addr, uint64_t data);
    void (*write16)(void* udata, uint32_t addr, uint64_t data);
    void (*write32)(void* udata, uint32_t addr, uint64_t data);
    void (*write64)(void* udata, uint32_t addr, uint64_t data);
    void (*write128)(void* udata, uint32_t addr, uint128_t data);
};

struct ee_state;

struct ee_state* ee_create(void);
void ee_init(struct ee_state* ee, struct vu_state* vu0, struct vu_state* vu1, struct ee_bus_s bus);
void ee_cycle(struct ee_state* ee);
void ee_reset(struct ee_state* ee);
void ee_destroy(struct ee_state* ee);
void ee_set_int0(struct ee_state* ee, int v);
void ee_set_int1(struct ee_state* ee, int v);
void ee_set_cpcond0(struct ee_state* ee, int v);

struct ps2_ram* ee_get_spr(struct ee_state* ee);
uint32_t ee_get_pc(struct ee_state* ee);
uint32_t ee_get_next_pc(struct ee_state* ee);
uint32_t ee_get_opcode(struct ee_state* ee);

#ifdef __cplusplus
}
#endif

#undef EE_ALIGNED16

#endif