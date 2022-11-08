/*
   Copyright (c) 2022 Radxa Ltd.
Author: Nascs <nascs@radxa.com>

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "rk356x.h"
#include "../../wiringx.h"
#include "../soc.h"
#include "../../i2c-dev.h"

#define GPIO_BANK_COUNT 5

const uintptr_t gpio_register_physical_address[MAX_REG_AREA] = {0xfdd60000, 0xfe740000, 0xfe750000, 0xfe760000, 0xfe770000};

uintptr_t cru_register_virtual_address = NULL;
uintptr_t pmucru_register_virtual_address = NULL;
uintptr_t pmugrf_register_virtual_address = NULL;
uintptr_t sysgrf_register_virtual_address = NULL;
#define CRU_REGISTER_PHYSICAL_ADDRESS       0xfdd20000
#define PMUCRU_REGISTER_PHYSICAL_ADDRESS    0xfdd00000
#define PMUGRF_REGISTER_PHYSICAL_ADDRESS    0xfdc20000	
#define SYSGRF_REGISTER_PHYSICAL_ADDRESS    0xfdc60000
#define CPUGRF_REGISTER_PHYSICAL_ADDRESS    0xfdc30000									            	
#define GPIO_SWPORT_DR_L         0x0000     // output for the lower 16 bits of I/O port, GPIOX_AX GPIOX_BX
#define GPIO_SWPORT_DR_H         0x0004     // output for the higher 16 bits of I/O port, GPIOX_CX GPIOX_DX
#define GPIO_SWPORT_DDR_L        0x0008     // data direction for the lower bits of I/O port, GPIOX_AX GPIOX_BX
#define GPIO_SWPORT_DDR_H        0x000C     // data direction for the higher of I/O port,  GPIOX_CX GPIOX_DX     io
#define CRU_GATE_CON31           0x037c     // gpio1 2 3 4 clock enable, bit2 bit4 bit6 bit8
#define PMUCRU_PMUGATE_CON01     0x0184     // gpio0 clock enable, bit9
#define PMU_GFR_GPIO0A_IOMUX_L	 0x0000		// gpio0a0~gpio0a3 iomux
#define PMU_GFR_GPIO0A_IOMUX_H	 0x0004		// gpio0a4~gpio0a7 iomux
#define PMU_GFR_GPIO0B_IOMUX_L	 0x0008		// gpio0b0~gpio0b3 iomux
#define PMU_GFR_GPIO0B_IOMUX_H	 0x000c		// gpio0b4~gpio0b7 iomux
#define PMU_GFR_GPIO0C_IOMUX_L	 0x0010		// gpio0c0~gpio0c3 iomux
#define PMU_GFR_GPIO0C_IOMUX_H	 0x0014		// gpio0c4~gpio0c7 iomux		
#define PMU_GFR_GPIO0D_IOMUX_L	 0x0018		// gpio0d0~gpio0d3 iomux	
#define PMU_GFR_GPIO0D_IOMUX_H	 0x001c		// gpio0d4~gpio0d7 iomux			
#define GRF_GPIO1A_IOMUX_L		 0x0000		// gpio1a0~gpio1a3 iomux
#define GRF_GPIO1A_IOMUX_H		 0x0004		// gpio1a4~gpio1a7 iomux								
#define GRF_GPIO1B_IOMUX_L		 0x0008		// gpio1b0~gpio1b3 iomux
#define GRF_GPIO1B_IOMUX_H		 0x000c		// gpio0c4~gpio0c7 iomux							
#define GRF_GPIO1C_IOMUX_L		 0x0010	    // gpio1c0~gpio1c3 iomux
#define GRF_GPIO1C_IOMUX_H		 0x0014		// gpio1c4~gpio1c7 iomux			
#define GRF_GPIO1D_IOMUX_L		 0x0018	    // gpio1d0~gpio1d3 iomux
#define GRF_GPIO1D_IOMUX_H		 0x001c     // gpio1d4~gpio1d7 iomux
#define GRF_GPIO2A_IOMUX_L		 0x0020     // gpio2a0~gpio2a3 iomux
#define GRF_GPIO2A_IOMUX_H		 0x0024     // gpio2a4~gpio2a7 iomux
#define GRF_GPIO2B_IOMUX_L		 0x0028     // gpio2b0~gpio2b3 iomux
#define GRF_GPIO2B_IOMUX_H		 0x002c     // gpio2b4~gpio2b7 iomux
#define GRF_GPIO2C_IOMUX_L		 0x0030     // gpio2c0~gpio2c3 iomux
#define GRF_GPIO2C_IOMUX_H		 0x0034     // gpio2c4~gpio2c7 iomux
#define GRF_GPIO2D_IOMUX_L		 0x0038     // gpio2d0~gpio2d3 iomux
#define GRF_GPIO2D_IOMUX_H		 0x003c     // gpio2d4~gpio2d7 iomux
#define GRF_GPIO3A_IOMUX_L		 0x0040     // gpio3a0~gpio3a3 iomux
#define GRF_GPIO3A_IOMUX_H		 0x0044     // gpio3a4~gpio3a7 iomux
#define GRF_GPIO3B_IOMUX_L		 0x0048     // gpio3b0~gpio3b3 iomux
#define GRF_GPIO3B_IOMUX_H		 0x004c     // gpio3b4~gpio3b7 iomux
#define GRF_GPIO3C_IOMUX_L		 0x0050     // gpio3c0~gpio3c3 iomux
#define GRF_GPIO3C_IOMUX_H		 0x0054     // gpio3c4~gpio3c7 iomux
#define GRF_GPIO3D_IOMUX_L		 0x0058     // gpio3d0~gpio3d3 iomux
#define GRF_GPIO3D_IOMUX_H		 0x005c     // gpio3s4~gpio3d7 iomux
#define GRF_GPIO4A_IOMUX_L		 0x0060     // gpio4a0~gpio4a3 iomux
#define GRF_GPIO4A_IOMUX_H		 0x0064     // gpio4a4~gpio4a7 iomux
#define GRF_GPIO4B_IOMUX_L		 0x0068     // gpio4b0~gpio4b3 iomux
#define GRF_GPIO4B_IOMUX_H		 0x006c     // gpio4b4~gpio4b7 iomux
#define GRF_GPIO4C_IOMUX_L		 0x0070     // gpio4c0~gpio4c3 iomux
#define GRF_GPIO4C_IOMUX_H		 0x0074     // gpio4c4~gpio4c7 iomux
#define GRF_GPIO4D_IOMUX_L		 0x0078     // gpio4d0~gpio4d3 iomux
#define GRF_GPIOXX_IOMUX_X       0x0000     // gpioXX iomux, which is nonextent
#define REGISTER_WRITE_MASK  16         // High 16 bits are write mask,

#define REGISTER_CLEAR_BITS(addr, bit, size) \
    (*addr = *addr & ~(~(-1 << size) << bit) | (~(-1 << size) << bit << REGISTER_WRITE_MASK))
#define REGISTER_SET_HIGH(addr, bit, clear_bit_num) \
	(*addr = *addr | (clear_bit_num << bit) | (clear_bit_num << bit << REGISTER_WRITE_MASK))

struct soc_t *rk356x = NULL;

static struct layout_t
{
    char *name;
    int bank;

    struct
    {
        unsigned long offset;
        unsigned long bit;
    } cru;

    struct
    {
        unsigned long offset;
        unsigned long bit;
    } grf;

    struct
    {
        unsigned long offset;
        unsigned long bit;
    } direction;

    struct
    {
        unsigned long offset;
        unsigned long bit;
    } data;

    int support;
    enum pinmode_t mode;
    int fd;

} layout[] = {
    {"GPIO0_A0", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L, 0}, {GPIO_SWPORT_DR_L, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A1", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L, 1}, {GPIO_SWPORT_DR_L, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A2", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L, 2}, {GPIO_SWPORT_DR_L, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A3", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 3}, {GPIO_SWPORT_DR_L, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A4", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L, 4}, {GPIO_SWPORT_DR_L, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A5", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L, 5}, {GPIO_SWPORT_DR_L, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A6", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L, 6}, {GPIO_SWPORT_DR_L, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_A7", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 7}, {GPIO_SWPORT_DR_L, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B0", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L, 8}, {GPIO_SWPORT_DR_L, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B1", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L, 9}, {GPIO_SWPORT_DR_L, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B2", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L,10}, {GPIO_SWPORT_DR_L,10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B3", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L,11}, {GPIO_SWPORT_DR_L,11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B4", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L,12}, {GPIO_SWPORT_DR_L,12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B5", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L,13}, {GPIO_SWPORT_DR_L,13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B6", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L,14}, {GPIO_SWPORT_DR_L,14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_B7", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L,15}, {GPIO_SWPORT_DR_L,15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C0", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  0}, {GPIO_SWPORT_DR_H,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C1", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  1}, {GPIO_SWPORT_DR_H,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C2", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H,  2}, {GPIO_SWPORT_DR_H,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C3", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H,  3}, {GPIO_SWPORT_DR_H,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C4", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H,  4}, {GPIO_SWPORT_DR_H,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C5", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H,  5}, {GPIO_SWPORT_DR_H,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C6", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H,  6}, {GPIO_SWPORT_DR_H,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_C7", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H,  7}, {GPIO_SWPORT_DR_H,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D0", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  8}, {GPIO_SWPORT_DR_H,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D1", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  9}, {GPIO_SWPORT_DR_H,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D2", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D3", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D4", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D5", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D6", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO0_D7", 0, {PMUCRU_PMUGATE_CON01, 9}, {PMU_GFR_GPIO0D_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0}, 
    {"GPIO1_A0", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  0}, {GPIO_SWPORT_DR_L,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A1", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  1}, {GPIO_SWPORT_DR_L,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A2", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L,  2}, {GPIO_SWPORT_DR_L,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A3", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L,  3}, {GPIO_SWPORT_DR_L,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A4", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L,  4}, {GPIO_SWPORT_DR_L,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A5", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L,  5}, {GPIO_SWPORT_DR_L,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A6", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L,  6}, {GPIO_SWPORT_DR_L,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_A7", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L,  7}, {GPIO_SWPORT_DR_L,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B0", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  8}, {GPIO_SWPORT_DR_L,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B1", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  9}, {GPIO_SWPORT_DR_L,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B2", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B3", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B4", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B5", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B6", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_B7", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},  
    {"GPIO1_C0", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  0}, {GPIO_SWPORT_DR_H,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C1", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  1}, {GPIO_SWPORT_DR_H,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C2", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H,  2}, {GPIO_SWPORT_DR_H,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C3", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H,  3}, {GPIO_SWPORT_DR_H,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C4", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H,  4}, {GPIO_SWPORT_DR_H,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C5", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H,  5}, {GPIO_SWPORT_DR_H,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C6", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H,  6}, {GPIO_SWPORT_DR_H,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_C7", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H,  7}, {GPIO_SWPORT_DR_H,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D0", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  8}, {GPIO_SWPORT_DR_H,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D1", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  9}, {GPIO_SWPORT_DR_H,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D2", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D3", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D4", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D5", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D6", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO1_D7", 1, {CRU_GATE_CON31, 2}, {GRF_GPIO1D_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A0", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  0}, {GPIO_SWPORT_DR_L,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A1", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  1}, {GPIO_SWPORT_DR_L,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A2", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L,  2}, {GPIO_SWPORT_DR_L,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A3", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L,  3}, {GPIO_SWPORT_DR_L,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A4", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L,  4}, {GPIO_SWPORT_DR_L,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A5", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L,  5}, {GPIO_SWPORT_DR_L,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A6", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L,  6}, {GPIO_SWPORT_DR_L,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_A7", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L,  7}, {GPIO_SWPORT_DR_L,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B0", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  8}, {GPIO_SWPORT_DR_L,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B1", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  9}, {GPIO_SWPORT_DR_L,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B2", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B3", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B4", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B5", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B6", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_B7", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C0", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  0}, {GPIO_SWPORT_DR_H,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C1", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  1}, {GPIO_SWPORT_DR_H,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C2", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H,  2}, {GPIO_SWPORT_DR_H,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C3", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H,  3}, {GPIO_SWPORT_DR_H,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C4", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H,  4}, {GPIO_SWPORT_DR_H,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C5", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H,  5}, {GPIO_SWPORT_DR_H,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C6", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H,  6}, {GPIO_SWPORT_DR_H,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_C7", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H,  7}, {GPIO_SWPORT_DR_H,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D0", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  8}, {GPIO_SWPORT_DR_H,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D1", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  9}, {GPIO_SWPORT_DR_H,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D2", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D3", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D4", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D5", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D6", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO2_D7", 2, {CRU_GATE_CON31, 4}, {GRF_GPIO2D_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A0", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  0}, {GPIO_SWPORT_DR_L,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A1", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  1}, {GPIO_SWPORT_DR_L,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A2", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L,  2}, {GPIO_SWPORT_DR_L,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A3", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L,  3}, {GPIO_SWPORT_DR_L,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A4", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L,  4}, {GPIO_SWPORT_DR_L,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A5", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L,  5}, {GPIO_SWPORT_DR_L,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A6", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L,  6}, {GPIO_SWPORT_DR_L,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_A7", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L,  7}, {GPIO_SWPORT_DR_L,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B0", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  8}, {GPIO_SWPORT_DR_L,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B1", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  9}, {GPIO_SWPORT_DR_L,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B2", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B3", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B4", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B5", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B6", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_B7", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0}, 
    {"GPIO3_C0", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  0}, {GPIO_SWPORT_DR_H,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C1", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  1}, {GPIO_SWPORT_DR_H,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C2", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H,  2}, {GPIO_SWPORT_DR_H,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C3", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H,  3}, {GPIO_SWPORT_DR_H,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C4", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H,  4}, {GPIO_SWPORT_DR_H,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C5", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H,  5}, {GPIO_SWPORT_DR_H,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C6", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H,  6}, {GPIO_SWPORT_DR_H,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_C7", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H,  7}, {GPIO_SWPORT_DR_H,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D0", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  8}, {GPIO_SWPORT_DR_H,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D1", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  9}, {GPIO_SWPORT_DR_H,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D2", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D3", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D4", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D5", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D6", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO3_D7", 3, {CRU_GATE_CON31, 6}, {GRF_GPIO3D_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},   
    {"GPIO4_A0", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  0}, {GPIO_SWPORT_DR_L,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A1", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  1}, {GPIO_SWPORT_DR_L,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A2", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L,  2}, {GPIO_SWPORT_DR_L,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A3", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L,  3}, {GPIO_SWPORT_DR_L,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A4", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L,  4}, {GPIO_SWPORT_DR_L,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A5", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L,  5}, {GPIO_SWPORT_DR_L,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A6", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L,  6}, {GPIO_SWPORT_DR_L,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_A7", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L,  7}, {GPIO_SWPORT_DR_L,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B0", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_L,  0}, {GPIO_SWPORT_DDR_L,  8}, {GPIO_SWPORT_DR_L,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B1", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_L,  4}, {GPIO_SWPORT_DDR_L,  9}, {GPIO_SWPORT_DR_L,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B2", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_L,  8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B3", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B4", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_H,  0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B5", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_H,  4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B6", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_H,  8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_B7", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},  
    {"GPIO4_C0", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_L,  0}, {GPIO_SWPORT_DDR_H,  0}, {GPIO_SWPORT_DR_H,  0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C1", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_L,  4}, {GPIO_SWPORT_DDR_H,  1}, {GPIO_SWPORT_DR_H,  1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C2", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_L,  8}, {GPIO_SWPORT_DDR_H,  2}, {GPIO_SWPORT_DR_H,  2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C3", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H,  3}, {GPIO_SWPORT_DR_H,  3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C4", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_H,  0}, {GPIO_SWPORT_DDR_H,  4}, {GPIO_SWPORT_DR_H,  4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C5", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_H,  4}, {GPIO_SWPORT_DDR_H,  5}, {GPIO_SWPORT_DR_H,  5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C6", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_H,  8}, {GPIO_SWPORT_DDR_H,  6}, {GPIO_SWPORT_DR_H,  6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_C7", 4, {CRU_GATE_CON31, 8}, {GRF_GPIO4C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H,  7}, {GPIO_SWPORT_DR_H,  7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D0", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X,  0}, {GPIO_SWPORT_DDR_H,  8}, {GPIO_SWPORT_DR_H,  8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D1", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X,  4}, {GPIO_SWPORT_DDR_H,  9}, {GPIO_SWPORT_DR_H,  9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D2", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X,  8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D3", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D4", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X,  0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D5", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X,  4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D6", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X,  8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
    {"GPIO4_D7", 4, {CRU_GATE_CON31, 8}, {GRF_GPIOXX_IOMUX_X, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
};

static int rk356xSetup(void)
{
    if ((rk356x->fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
        wiringXLog(LOG_ERR, "wiringX failed to open /dev/mem for raw memory access");
        return -1;
    }

    for (int i = 0; i < GPIO_BANK_COUNT; i++)
    {
        if ((rk356x->gpio[i] = (unsigned char *)mmap(0, rk356x->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk356x->fd, rk356x->base_addr[i])) == NULL)
        {
            wiringXLog(LOG_ERR, "wiringX failed to map The %s %s GPIO memory address", rk356x->brand, rk356x->chip);
            return -1;
        }
    }

    if ((cru_register_virtual_address = (unsigned char *)mmap(0, rk356x->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk356x->fd, CRU_REGISTER_PHYSICAL_ADDRESS)) == NULL)
    {
        wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk356x->brand, rk356x->chip);
        return -1;
    }
    if ((pmucru_register_virtual_address = (unsigned char *)mmap(0, rk356x->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk356x->fd, PMUCRU_REGISTER_PHYSICAL_ADDRESS)) == NULL)
    {
        wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk356x->brand, rk356x->chip);
        return -1;
    }
    if ((pmugrf_register_virtual_address = (unsigned char *)mmap(0, rk356x->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk356x->fd, PMUGRF_REGISTER_PHYSICAL_ADDRESS)) == NULL)
    {
        wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk356x->brand, rk356x->chip);
        return -1;
    }      
    if ((sysgrf_register_virtual_address = (unsigned char *)mmap(0, rk356x->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk356x->fd, SYSGRF_REGISTER_PHYSICAL_ADDRESS)) == NULL)
    {
        wiringXLog(LOG_ERR, "wiringX failed to map The %s %s GRF memory address", rk356x->brand, rk356x->chip);
        return -1;
    }

    return 0;
}

static char *rk356xGetPinName(int pin)
{
    return rk356x->layout[pin].name;
}

static void rk356xSetMap(int *map, size_t size)
{
    rk356x->map = map;
    rk356x->map_size = size;
}

static void rk356xSetIRQ(int *irq, size_t size)
{
    rk356x->irq = irq;
    rk356x->irq_size = size;
}

struct layout_t * rk356xGetLayout(int i, int* mapping)
{
    struct layout_t *pin = NULL;

    if (mapping == NULL)
    {
        wiringXLog(LOG_ERR, "The %s %s has not yet been mapped", rk356x->brand, rk356x->chip);
        return NULL;
    }
    if (wiringXValidGPIO(i) != 0)
    {
        wiringXLog(LOG_ERR, "The %i is not the right GPIO number !\n");
        return NULL;
    }
    if (rk356x->fd <= 0 || rk356x->gpio == NULL)
    {
        wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX", rk356x->brand, rk356x->chip);
        return NULL;
    }

    pin = &rk356x->layout[mapping[i]];

    if (pin->bank < 0 || pin->bank >= GPIO_BANK_COUNT)
    {
        wiringXLog(LOG_ERR, "pin->bank out of range: %i, expect 0-4", pin->bank);
        return NULL;
    }

    return pin;
}

#define rk356xGetPinLayout(i) (rk356xGetLayout(i, rk356x->map))
#define rk356xGetIrqLayout(i) (rk356xGetLayout(i, rk356x->irq))

static int rk356xDigitalWrite(int i, enum digital_value_t value)
{
    struct layout_t *pin = NULL;
    unsigned int *data_reg = 0;

    if ((pin = rk356xGetPinLayout(i)) == NULL)
    {
        return -1;
    }

    if (pin->mode != PINMODE_OUTPUT)
    {
        wiringXLog(LOG_ERR, "The %s %s GPIO%d is not set to output mode", rk356x->brand, rk356x->chip, i);
        return -1;
    }

    data_reg = (volatile unsigned int *)(rk356x->gpio[pin->bank] + pin->data.offset);
    if (value == HIGH)
    {
        REGISTER_SET_HIGH(data_reg, pin->data.bit, 1);
    }
    else if (value == LOW)
    {
        REGISTER_CLEAR_BITS(data_reg, pin->data.bit, 1);
    }
    else
    {
        wiringXLog(LOG_ERR, "invalid value %i for GPIO %i", value, i);
        return -1;
    }

    return 0;
}

static int rk356xDigitalRead(int i)
{
    struct layout_t *pin = NULL;
    unsigned int *data_reg = NULL;
    uint32_t val = 0;

    if ((pin = rk356xGetPinLayout(i)) == NULL)
    {
        return -1;
    }

    if (pin->mode != PINMODE_INPUT)
    {
        wiringXLog(LOG_ERR, "The %s %s GPIO%d is not set to input mode", rk356x->brand, rk356x->chip, i);
        return -1;
    }

    data_reg = (volatile unsigned int *)(rk356x->gpio[pin->bank] + pin->data.offset);
    val = *data_reg;

    return (int)((val & (1 << pin->data.bit)) >> pin->data.bit);
}

static int rk356xPinMode(int i, enum pinmode_t mode)
{
    struct layout_t *pin = NULL;
    unsigned int *cru_reg = NULL;
    unsigned int *grf_reg = NULL;
    unsigned int *dir_reg = NULL;

    if ((pin = rk356xGetPinLayout(i)) == NULL)
    {
        return -1;
    }

    cru_reg = (volatile unsigned int *)(cru_register_virtual_address + pin->cru.offset);
    REGISTER_CLEAR_BITS(cru_reg, pin->cru.bit, 2);

    if(pin->bank == 0 ) {
        grf_reg = (volatile unsigned int *)(pmugrf_register_virtual_address + pin->grf.offset);
    }
    else if (pin->bank >= 1 || pin->bank <= 4) {
        grf_reg = (volatile unsigned int *)(sysgrf_register_virtual_address + pin->grf.offset);
    }
    else {
        wiringXLog(LOG_ERR, "pin->bank out of range %i, expect 0~4", i);
    }
    REGISTER_CLEAR_BITS(grf_reg, pin->grf.bit, 2);

    dir_reg = (volatile unsigned int *)(rk356x->gpio[pin->bank] + pin->direction.offset);
    if (mode == PINMODE_INPUT) {
        REGISTER_CLEAR_BITS(dir_reg, pin->data.bit, 2);
    } else if (mode == PINMODE_OUTPUT) {
        REGISTER_SET_HIGH(dir_reg, pin->direction.bit, 0x1);
    }
    else {
        wiringXLog(LOG_ERR, "invalid pin mode %i for GPIO %i", mode, i);
        return -1;        
    }

    pin->mode = mode;

    return 0;
}

static int rk356xISR(int i, enum isr_mode_t mode)
{
    struct layout_t *pin = NULL;
    char path[PATH_MAX];

    if ((pin = rk356xGetIrqLayout(i)) == NULL)
    {
        return -1;
    }

    sprintf(path, "/sys/class/gpio/gpio%d", rk356x->irq[i]);
    if ((soc_sysfs_check_gpio(rk356x, path)) == -1)
    {
        sprintf(path, "/sys/class/gpio/export");
        if (soc_sysfs_gpio_export(rk356x, path, rk356x->irq[i]) == -1)
        {
            return -1;
        }
    }

    sprintf(path, "/sys/class/gpio/gpio%d/direction", rk356x->irq[i]);
    if (soc_sysfs_set_gpio_direction(rk356x, path, "in") == -1)
    {
        return -1;
    }

    sprintf(path, "/sys/class/gpio/gpio%d/edge", rk356x->irq[i]);
    if (soc_sysfs_set_gpio_interrupt_mode(rk356x, path, mode) == -1)
    {
        return -1;
    }

    sprintf(path, "/sys/class/gpio/gpio%d/value", rk356x->irq[i]);
    if ((pin->fd = soc_sysfs_gpio_reset_value(rk356x, path)) == -1)
    {
        return -1;
    }

    pin->mode = PINMODE_INTERRUPT;

    return 0;
}

static int rk356xWaitForInterrupt(int i, int ms)
{
    struct layout_t *pin = NULL;

    if ((pin = rk356xGetIrqLayout(i)) == NULL)
    {
        return -1;
    }

    if (pin->mode != PINMODE_INTERRUPT)
    {
        wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to interrupt mode", rk356x->brand, rk356x->chip, i);
        return -1;
    }

    return soc_wait_for_interrupt(rk356x, pin->fd, ms);
}

static int rk356xGC(void)
{
    struct layout_t *pin = NULL;
    char path[PATH_MAX];

    if (rk356x->map != NULL)
    {
        for (int i = 0; i < rk356x->map_size; i++)
        {
            pin = &rk356x->layout[rk356x->map[i]];
            if (pin->mode == PINMODE_OUTPUT)
            {
                pinMode(i, PINMODE_INPUT);
            }
            else if (pin->mode == PINMODE_INTERRUPT)
            {
                sprintf(path, "/sys/class/gpio/gpio%d", rk356x->irq[i]);
                if ((soc_sysfs_check_gpio(rk356x, path)) == 0)
                {
                    sprintf(path, "/sys/class/gpio/unexport");
                    soc_sysfs_gpio_unexport(rk356x, path, rk356x->irq[i]);
                }
            }

            if (pin->fd > 0)
            {
                close(pin->fd);
                pin->fd = 0;
            }
        }
    }

    if (cru_register_virtual_address != NULL)
    {
        munmap(cru_register_virtual_address, rk356x->page_size);
        cru_register_virtual_address = NULL;
    }

    for (int i = 0; i < GPIO_BANK_COUNT; i++)
    {
        if (rk356x->gpio[i] != NULL)
        {
            munmap(rk356x->gpio[i], rk356x->page_size);
            rk356x->gpio[i] = NULL;
        }
    }

    return 0;
}

static int rk356xSelectableFd(int i)
{
    struct layout_t *pin = NULL;

    if ((pin = rk356xGetIrqLayout(i)) == NULL)
    {
        return -1;
    }

    return pin->fd;
}

void rk356xInit(void)
{
    soc_register(&rk356x, "Rockchip", "RK356X");

    rk356x->layout = layout;

    rk356x->support.isr_modes = ISR_MODE_RISING | ISR_MODE_FALLING | ISR_MODE_BOTH | ISR_MODE_NONE;
    rk356x->page_size = (1024*64);
    memcpy(rk356x->base_addr, gpio_register_physical_address, sizeof(gpio_register_physical_address));

    rk356x->gc = &rk356xGC;
    rk356x->selectableFd = &rk356xSelectableFd;

    rk356x->pinMode = &rk356xPinMode;
    rk356x->setup = &rk356xSetup;
    rk356x->digitalRead = &rk356xDigitalRead;
    rk356x->digitalWrite = &rk356xDigitalWrite;
    rk356x->getPinName = &rk356xGetPinName;
    rk356x->setMap = &rk356xSetMap;
    rk356x->setIRQ = &rk356xSetIRQ;
    rk356x->isr = &rk356xISR;
    rk356x->waitForInterrupt = &rk356xWaitForInterrupt;
}