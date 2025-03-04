#pragma once

#include <stdint.h>

#include "ee.h"
#include "shared/ram.h"

#define EE_SR_CU  0xf0000000
#define EE_SR_DEV 0x08000000
#define EE_SR_BEV 0x04000000
#define EE_SR_CH  0x00040000
#define EE_SR_EDI 0x00020000
#define EE_SR_EIE 0x00010000
#define EE_SR_IM7 0x00008000
#define EE_SR_BEM 0x00001000
#define EE_SR_IM3 0x00000800
#define EE_SR_IM2 0x00000400
#define EE_SR_KSU 0x00000018
#define EE_SR_ERL 0x00000004
#define EE_SR_EXL 0x00000002
#define EE_SR_IE  0x00000001

#define EE_CAUSE_BD   0x80000000
#define EE_CAUSE_BD2  0x40000000
#define EE_CAUSE_CE   0x30000000
#define EE_CAUSE_EXC2 0x00070000
#define EE_CAUSE_IP7  0x00008000
#define EE_CAUSE_IP3  0x00000800
#define EE_CAUSE_IP2  0x00000400
#define EE_CAUSE_EXC  0x0000007c

#define CAUSE_EXC1_INT  (0 << 2)
#define CAUSE_EXC1_MOD  (1 << 2)
#define CAUSE_EXC1_TLBL (2 << 2)
#define CAUSE_EXC1_TLBS (3 << 2)
#define CAUSE_EXC1_ADEL (4 << 2)
#define CAUSE_EXC1_ADES (5 << 2)
#define CAUSE_EXC1_IBE  (6 << 2)
#define CAUSE_EXC1_DBE  (7 << 2)
#define CAUSE_EXC1_SYS  (8 << 2)
#define CAUSE_EXC1_BP   (9 << 2)
#define CAUSE_EXC1_RI   (10 << 2)
#define CAUSE_EXC1_CPU  (11 << 2)
#define CAUSE_EXC1_OV   (12 << 2)
#define CAUSE_EXC1_TR   (13 << 2)

#define CAUSE_EXC2_RES   (0 << 16)
#define CAUSE_EXC2_NMI   (1 << 16)
#define CAUSE_EXC2_PERFC (2 << 16)
#define CAUSE_EXC2_DBG   (3 << 16)

#define EE_VEC_RESET   0xbfc00000
#define EE_VEC_TLBR    0x00000000
#define EE_VEC_COUNTER 0x00000080
#define EE_VEC_DEBUG   0x00000100
#define EE_VEC_COMMON  0x00000180
#define EE_VEC_IRQ     0x00000200

union ee_fpu_reg {
    float f;
    uint32_t u32;
    int32_t s32;
};

#include <vector>
#include <unordered_map>

struct ee_instruction {
    void (*fn)(struct ee_state*, ee_instruction*);

    uint32_t rs;
    uint32_t fs;
    uint32_t rt;
    uint32_t rd;
    uint32_t fd;
    uint32_t sa;
    uint32_t i16;
    uint32_t i26;
    uint32_t si26;
    uint32_t si16;
};

struct ee_basic_block {
    uint32_t addr;
    uint32_t end_addr;

    std::vector <ee_instruction> instructions;
};

#ifdef _EE_USE_INTRINSICS
#define EE_ALIGNED16 __attribute__((aligned(16)))
#else
#define EE_ALIGNED16
#endif

struct ee_state {
    struct ee_bus_s bus;

    int block_end = 0;
    ee_basic_block block;
    std::unordered_map <uint32_t, ee_basic_block> block_cache;

    uint128_t r[32] EE_ALIGNED16 = { 0 };
    uint128_t hi EE_ALIGNED16 = { 0 };
    uint128_t lo EE_ALIGNED16 = { 0 };

    uint64_t total_cycles;

    uint32_t prev_pc = 0;
    uint32_t pc = 0;
    uint32_t next_pc = 0;
    uint32_t opcode = 0;
    uint64_t sa = 0;
    int branch = 0, branch_taken = 0, delay_slot = 0;

    struct ps2_ram* scratchpad = nullptr;

    int cpcond0 = 0;

    union {
        uint32_t cop0_r[32] = { 0 };
    
        struct {
            uint32_t index;
            uint32_t random;
            uint32_t entrylo0;
            uint32_t entrylo1;
            uint32_t context;
            uint32_t pagemask;
            uint32_t wired;
            uint32_t unused7;
            uint32_t badvaddr;
            uint32_t count;
            uint32_t entryhi;
            uint32_t compare;
            uint32_t status;
            uint32_t cause;
            uint32_t epc;
            uint32_t prid;
            uint32_t config;
            uint32_t unused16;
            uint32_t unused17;
            uint32_t unused18;
            uint32_t unused19;
            uint32_t unused20;
            uint32_t unused21;
            uint32_t badpaddr;
            uint32_t debug;
            uint32_t perf;
            uint32_t unused25;
            uint32_t unused26;
            uint32_t taglo;
            uint32_t taghi;
            uint32_t errorepc;
            uint32_t unused30;
            uint32_t unused31;
        };
    };

    union ee_fpu_reg f[32];
    union ee_fpu_reg a;

    uint32_t fcr;

    struct vu_state* vu0;
    struct vu_state* vu1;
};

#undef EE_ALIGNED16