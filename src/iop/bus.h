#ifndef IOP_BUS_H
#define IOP_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "u128.h"

#include "shared/ram.h"
#include "shared/sif.h"
#include "shared/bios.h"

#include "dma.h"
#include "intc.h"
#include "timers.h"
#include "cdvd.h"
#include "sio2.h"
#include "spu2.h"
#include "usb.h"
#include "dev9.h"
#include "speed.h"
#include "fw.h"

struct iop_bus {
    struct ps2_bios* bios;
    struct ps2_bios* rom1;
    struct ps2_bios* rom2;
    struct ps2_ram* iop_ram;
    struct ps2_ram* iop_spr;
    struct ps2_sif* sif;
    struct ps2_iop_dma* dma;
    struct ps2_iop_intc* intc;
    struct ps2_iop_timers* timers;
    struct ps2_cdvd* cdvd;
    struct ps2_sio2* sio2;
    struct ps2_spu2* spu2;
    struct ps2_usb* usb;
    struct ps2_fw* fw;
    struct ps2_dev9* dev9;
    struct ps2_speed* speed;
};

void iop_bus_init_bios(struct iop_bus* bus, struct ps2_bios* bios);
void iop_bus_init_rom1(struct iop_bus* bus, struct ps2_bios* rom1);
void iop_bus_init_rom2(struct iop_bus* bus, struct ps2_bios* rom2);
void iop_bus_init_iop_ram(struct iop_bus* bus, struct ps2_ram* iop_ram);
void iop_bus_init_iop_spr(struct iop_bus* bus, struct ps2_ram* iop_spr);
void iop_bus_init_sif(struct iop_bus* bus, struct ps2_sif* sif);
void iop_bus_init_dma(struct iop_bus* bus, struct ps2_iop_dma* dma);
void iop_bus_init_intc(struct iop_bus* bus, struct ps2_iop_intc* intc);
void iop_bus_init_timers(struct iop_bus* bus, struct ps2_iop_timers* timers);
void iop_bus_init_cdvd(struct iop_bus* bus, struct ps2_cdvd* cdvd);
void iop_bus_init_sio2(struct iop_bus* bus, struct ps2_sio2* sio2);
void iop_bus_init_spu2(struct iop_bus* bus, struct ps2_spu2* spu2);
void iop_bus_init_usb(struct iop_bus* bus, struct ps2_usb* usb);
void iop_bus_init_fw(struct iop_bus* bus, struct ps2_fw* fw);
void iop_bus_init_dev9(struct iop_bus* bus, struct ps2_dev9* dev9);
void iop_bus_init_speed(struct iop_bus* bus, struct ps2_speed* speed);

#ifdef __cplusplus
}
#endif

#endif