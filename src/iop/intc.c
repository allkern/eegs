#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intc.h"

struct ps2_iop_intc* ps2_iop_intc_create(void) {
    return malloc(sizeof(struct ps2_iop_intc));
}

void ps2_iop_intc_init(struct ps2_iop_intc* intc, struct iop_state* iop) {
    memset(intc, 0, sizeof(struct ps2_iop_intc));

    intc->iop = iop;
    intc->ctrl = 1;
}

void ps2_iop_intc_irq(struct ps2_iop_intc* intc, int dev) {
    intc->stat |= dev;

    if (intc->ctrl && (intc->stat & intc->mask)) {
        iop_set_irq_pending(intc->iop);
    }
}

void ps2_iop_intc_destroy(struct ps2_iop_intc* intc) {
    free(intc);
}

uint64_t ps2_iop_intc_read32(struct ps2_iop_intc* intc, uint32_t addr) {
    int o = intc->ctrl && (intc->stat & intc->mask);

    uint32_t ctrl = intc->ctrl;

    switch (addr) {
        case 0x1f801070: return intc->stat;
        case 0x1f801074: return intc->mask;
        case 0x1f801078: intc->ctrl = 0; break;
    }

    int n = intc->ctrl && (intc->stat & intc->mask);

    if (!n) {
        intc->iop->cop0_r[COP0_CAUSE] &= ~SR_IM2;
    } else {
        if (n & !o)
            iop_set_irq_pending(intc->iop);
    }

    return ctrl;
}

void ps2_iop_intc_write32(struct ps2_iop_intc* intc, uint32_t addr, uint64_t data) {
    int o = intc->ctrl && (intc->stat & intc->mask);

    switch (addr) {
        case 0x1f801070: intc->stat &= data; break;
        case 0x1f801074: intc->mask = data; break;
        case 0x1f801078: intc->ctrl = data; break;
    }

    int n = intc->ctrl && (intc->stat & intc->mask);

    if (!n) {
        intc->iop->cop0_r[COP0_CAUSE] &= ~SR_IM2;
    } else {
        if (n & !o)
            iop_set_irq_pending(intc->iop);
    }
}