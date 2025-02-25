#ifndef VU_H
#define VU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "u128.h"

struct vu_reg {
    union {
        uint128_t u128;
        uint64_t u64[2];
        uint32_t u32[4];
        float f[4];

        // Named fields
        struct {
            float x;
            float y;
            float z;
            float w;
        };
    };
};

struct vu_state {
    struct vu_reg vf[32];
    uint16_t vi[16];
    struct vu_reg acc;

    uint64_t micro_mem[0x800];
    uint128_t vu_mem[0x400];

    int micro_mem_size;
    int vu_mem_size;
    int id;

    uint32_t upper;
    uint32_t lower;

    union {
        uint32_t cr[16];

        struct {
            uint32_t status;
            uint32_t mac;
            uint32_t clip;
            uint32_t rsv0;
            union {
                uint32_t u32;
                float f;
            } r;
            uint32_t i;
            union {
                uint32_t u32;
                float f;
            } q;
            uint32_t rsv1;
            uint32_t rsv2;
            uint32_t rsv3;
            uint32_t tpc;
            uint32_t cmsar0;
            uint32_t fbrst;
            uint32_t vpu_stat;
            uint32_t rsv4;
            uint32_t cmsar1;
        };
    };

    struct vu_state* vu1;
};

struct vu_state* vu_create(void);
void vu_init(struct vu_state* vu, int id, struct vu_state* vu1);
void vu_destroy(struct vu_state* vu);

// Upper pipeline
void vu_i_abs(struct vu_state* vu);
void vu_i_add(struct vu_state* vu);
void vu_i_addi(struct vu_state* vu);
void vu_i_addq(struct vu_state* vu);
void vu_i_addx(struct vu_state* vu);
void vu_i_addy(struct vu_state* vu);
void vu_i_addz(struct vu_state* vu);
void vu_i_addw(struct vu_state* vu);
void vu_i_adda(struct vu_state* vu);
void vu_i_addai(struct vu_state* vu);
void vu_i_addaq(struct vu_state* vu);
void vu_i_addax(struct vu_state* vu);
void vu_i_adday(struct vu_state* vu);
void vu_i_addaz(struct vu_state* vu);
void vu_i_addaw(struct vu_state* vu);
void vu_i_sub(struct vu_state* vu);
void vu_i_subi(struct vu_state* vu);
void vu_i_subq(struct vu_state* vu);
void vu_i_subx(struct vu_state* vu);
void vu_i_suby(struct vu_state* vu);
void vu_i_subz(struct vu_state* vu);
void vu_i_subw(struct vu_state* vu);
void vu_i_suba(struct vu_state* vu);
void vu_i_subai(struct vu_state* vu);
void vu_i_subaq(struct vu_state* vu);
void vu_i_subax(struct vu_state* vu);
void vu_i_subay(struct vu_state* vu);
void vu_i_subaz(struct vu_state* vu);
void vu_i_subaw(struct vu_state* vu);
void vu_i_mul(struct vu_state* vu);
void vu_i_muli(struct vu_state* vu);
void vu_i_mulq(struct vu_state* vu);
void vu_i_mulx(struct vu_state* vu);
void vu_i_muly(struct vu_state* vu);
void vu_i_mulz(struct vu_state* vu);
void vu_i_mulw(struct vu_state* vu);
void vu_i_mula(struct vu_state* vu);
void vu_i_mulai(struct vu_state* vu);
void vu_i_mulaq(struct vu_state* vu);
void vu_i_mulax(struct vu_state* vu);
void vu_i_mulay(struct vu_state* vu);
void vu_i_mulaz(struct vu_state* vu);
void vu_i_mulaw(struct vu_state* vu);
void vu_i_madd(struct vu_state* vu);
void vu_i_maddi(struct vu_state* vu);
void vu_i_maddq(struct vu_state* vu);
void vu_i_maddx(struct vu_state* vu);
void vu_i_maddy(struct vu_state* vu);
void vu_i_maddz(struct vu_state* vu);
void vu_i_maddw(struct vu_state* vu);
void vu_i_madda(struct vu_state* vu);
void vu_i_maddai(struct vu_state* vu);
void vu_i_maddaq(struct vu_state* vu);
void vu_i_maddax(struct vu_state* vu);
void vu_i_madday(struct vu_state* vu);
void vu_i_maddaz(struct vu_state* vu);
void vu_i_maddaw(struct vu_state* vu);
void vu_i_msub(struct vu_state* vu);
void vu_i_msubi(struct vu_state* vu);
void vu_i_msubq(struct vu_state* vu);
void vu_i_msubx(struct vu_state* vu);
void vu_i_msuby(struct vu_state* vu);
void vu_i_msubz(struct vu_state* vu);
void vu_i_msubw(struct vu_state* vu);
void vu_i_msuba(struct vu_state* vu);
void vu_i_msubai(struct vu_state* vu);
void vu_i_msubaq(struct vu_state* vu);
void vu_i_msubax(struct vu_state* vu);
void vu_i_msubay(struct vu_state* vu);
void vu_i_msubaz(struct vu_state* vu);
void vu_i_msubaw(struct vu_state* vu);
void vu_i_max(struct vu_state* vu);
void vu_i_maxi(struct vu_state* vu);
void vu_i_maxx(struct vu_state* vu);
void vu_i_maxy(struct vu_state* vu);
void vu_i_maxz(struct vu_state* vu);
void vu_i_maxw(struct vu_state* vu);
void vu_i_mini(struct vu_state* vu);
void vu_i_minii(struct vu_state* vu);
void vu_i_minix(struct vu_state* vu);
void vu_i_miniy(struct vu_state* vu);
void vu_i_miniz(struct vu_state* vu);
void vu_i_miniw(struct vu_state* vu);
void vu_i_opmula(struct vu_state* vu);
void vu_i_opmsub(struct vu_state* vu);
void vu_i_nop(struct vu_state* vu);
void vu_i_ftoi0(struct vu_state* vu);
void vu_i_ftoi4(struct vu_state* vu);
void vu_i_ftoi12(struct vu_state* vu);
void vu_i_ftoi15(struct vu_state* vu);
void vu_i_itof0(struct vu_state* vu);
void vu_i_itof4(struct vu_state* vu);
void vu_i_itof12(struct vu_state* vu);
void vu_i_itof15(struct vu_state* vu);
void vu_i_clip(struct vu_state* vu);

// Lower pipeline
void vu_i_b(struct vu_state* vu);
void vu_i_bal(struct vu_state* vu);
void vu_i_div(struct vu_state* vu);
void vu_i_eatan(struct vu_state* vu);
void vu_i_eatanxy(struct vu_state* vu);
void vu_i_eatanxz(struct vu_state* vu);
void vu_i_eexp(struct vu_state* vu);
void vu_i_eleng(struct vu_state* vu);
void vu_i_ercpr(struct vu_state* vu);
void vu_i_erleng(struct vu_state* vu);
void vu_i_ersadd(struct vu_state* vu);
void vu_i_ersqrt(struct vu_state* vu);
void vu_i_esadd(struct vu_state* vu);
void vu_i_esin(struct vu_state* vu);
void vu_i_esqrt(struct vu_state* vu);
void vu_i_esum(struct vu_state* vu);
void vu_i_fcand(struct vu_state* vu);
void vu_i_fceq(struct vu_state* vu);
void vu_i_fcget(struct vu_state* vu);
void vu_i_fcor(struct vu_state* vu);
void vu_i_fcset(struct vu_state* vu);
void vu_i_fmand(struct vu_state* vu);
void vu_i_fmeq(struct vu_state* vu);
void vu_i_fmor(struct vu_state* vu);
void vu_i_fsand(struct vu_state* vu);
void vu_i_fseq(struct vu_state* vu);
void vu_i_fsor(struct vu_state* vu);
void vu_i_fsset(struct vu_state* vu);
void vu_i_iadd(struct vu_state* vu);
void vu_i_iaddi(struct vu_state* vu);
void vu_i_iaddiu(struct vu_state* vu);
void vu_i_iand(struct vu_state* vu);
void vu_i_ibeq(struct vu_state* vu);
void vu_i_ibgez(struct vu_state* vu);
void vu_i_ibgtz(struct vu_state* vu);
void vu_i_iblez(struct vu_state* vu);
void vu_i_ibltz(struct vu_state* vu);
void vu_i_ibne(struct vu_state* vu);
void vu_i_ilw(struct vu_state* vu);
void vu_i_ilwr(struct vu_state* vu);
void vu_i_ior(struct vu_state* vu);
void vu_i_isub(struct vu_state* vu);
void vu_i_isubiu(struct vu_state* vu);
void vu_i_isw(struct vu_state* vu);
void vu_i_iswr(struct vu_state* vu);
void vu_i_jalr(struct vu_state* vu);
void vu_i_jr(struct vu_state* vu);
void vu_i_lq(struct vu_state* vu);
void vu_i_lqd(struct vu_state* vu);
void vu_i_lqi(struct vu_state* vu);
void vu_i_mfir(struct vu_state* vu);
void vu_i_mfp(struct vu_state* vu);
void vu_i_move(struct vu_state* vu);
void vu_i_mr32(struct vu_state* vu);
void vu_i_mtir(struct vu_state* vu);
void vu_i_rget(struct vu_state* vu);
void vu_i_rinit(struct vu_state* vu);
void vu_i_rnext(struct vu_state* vu);
void vu_i_rsqrt(struct vu_state* vu);
void vu_i_rxor(struct vu_state* vu);
void vu_i_sq(struct vu_state* vu);
void vu_i_sqd(struct vu_state* vu);
void vu_i_sqi(struct vu_state* vu);
void vu_i_sqrt(struct vu_state* vu);
void vu_i_waitp(struct vu_state* vu);
void vu_i_waitq(struct vu_state* vu);
void vu_i_xgkick(struct vu_state* vu);
void vu_i_xitop(struct vu_state* vu);
void vu_i_xtop(struct vu_state* vu);

#ifdef __cplusplus
}
#endif

#endif