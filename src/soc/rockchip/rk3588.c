/*
	Copyright (c) 2023 Radxa Ltd.
	Author: Nascs <nascs@radxa.com>

	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "rk3588.h"

#define GPIO_BANK_COUNT 5
#define IOMUX_COUNT 3

const uintptr_t rk3588_gpio_register_physical_address[MAX_REG_AREA] = {0xfd8a0000, 0xfec20000, 0xfec30000, 0xfec40000, 0xfec50000};

uintptr_t cru_ns_register_virtual_address = NULL;
uintptr_t pmu1_ioc_register_virtual_address = NULL;
uintptr_t pmu2_ioc_register_virtual_address = NULL;
uintptr_t bus_ioc_register_virtual_address = NULL;
#define CRU_NS_REGISTER_PHYSICAL_ADDRESS			0xfd7c0000
#define PMU1_IOC_REGISTER_PHYSICAL_ADDRESS		0xfd5f0000
#define PMU2_IOC_REGISTER_PHYSICAL_ADDRESS		0xfd5f4000
#define BUS_IOC_REGISTER_PHYSICAL_ADDRESS			0xfd5f8000	
#define GPIO_SWPORT_DR_L			0x0000	// output for the lower 16 bits of I/O port, GPIOX_AX GPIOX_BX
#define GPIO_SWPORT_DR_H			0x0004	// output for the higher 16 bits of I/O port, GPIOX_CX GPIOX_DX
#define GPIO_SWPORT_DDR_L			0x0008	// data direction for the lower bits of I/O port, GPIOX_AX GPIOX_BX
#define GPIO_SWPORT_DDR_H			0x000c	// data direction for the higher of I/O port,	GPIOX_CX GPIOX_DX
#define GPIO_EXT_PORT					0x0070	// external port data register, read
#define CRU_GATE_CON16				0x0840	// gpio 1 clock enable, bit14
#define CRU_GATE_CON17				0x0844 	// gpio 2 3 4 clock enable, bit0 bit2 bit4 
#define PMU1CRU_GATE_CON05		0x0814	// gpio0 clock enable, bit5
#define PMU1_IOC_GPIOXX_IOMUX_SEL_X			0x0000	// gpio0a0~gpio0a3 iomux
#define PMU1_IOC_GPIO0A_IOMUX_SEL_L			0x0000	// gpio0a0~gpio0a3 iomux
#define PMU1_IOC_GPIO0A_IOMUX_SEL_H			0x0004	// gpio0a4~gpio0a7 iomux
#define PMU1_IOC_GPIO0B_IOMUX_SEL_L			0x0008	// gpio0b0~gpio0b3 iomux
#define PMU2_IOC_GPIO0B_IOMUX_SEL_H			0x0000	// gpio0b4~gpio0b7 iomux
#define PMU2_IOC_GPIO0C_IOMUX_SEL_L			0x0004	// gpio0c0~gpio0c3 iomux
#define PMU2_IOC_GPIO0C_IOMUX_SEL_H			0x0008	// gpio0c4~gpio0c7 iomux		
#define PMU2_IOC_GPIO0D_IOMUX_SEL_L			0x000c	// gpio0d0~gpio0d3 iomux	
#define PMU2_IOC_GPIO0D_IOMUX_SEL_H			0x0010	// gpio0d4~gpio0d7 iomux
#define BUS_IOC_GPIO1A_IOMUX_L					0x0020	// gpio1a0~gpio1a3 iomux
#define BUS_IOC_GPIO1A_IOMUX_H					0x0024	// gpio1a4~gpio1a7 iomux								
#define BUS_IOC_GPIO1B_IOMUX_L					0x0028	// gpio1b0~gpio1b3 iomux
#define BUS_IOC_GPIO1B_IOMUX_H					0x002c	// gpio0c4~gpio0c7 iomux							
#define BUS_IOC_GPIO1C_IOMUX_L					0x0030	// gpio1c0~gpio1c3 iomux
#define BUS_IOC_GPIO1C_IOMUX_H					0x0034	// gpio1c4~gpio1c7 iomux			
#define BUS_IOC_GPIO1D_IOMUX_L					0x0038	// gpio1d0~gpio1d3 iomux
#define BUS_IOC_GPIO1D_IOMUX_H					0x003c	// gpio1d4~gpio1d7 iomux
#define BUS_IOC_GPIO2A_IOMUX_L					0x0040	// gpio2a0~gpio2a3 iomux
#define BUS_IOC_GPIO2A_IOMUX_H					0x0044	// gpio2a4~gpio2a7 iomux
#define BUS_IOC_GPIO2B_IOMUX_L					0x0048	// gpio2b0~gpio2b3 iomux
#define BUS_IOC_GPIO2B_IOMUX_H					0x004c	// gpio2b4~gpio2b7 iomux
#define BUS_IOC_GPIO2C_IOMUX_L					0x0050	// gpio2c0~gpio2c3 iomux
#define BUS_IOC_GPIO2C_IOMUX_H					0x0054	// gpio2c4~gpio2c7 iomux
#define BUS_IOC_GPIO2D_IOMUX_L					0x0058	// gpio2d0~gpio2d3 iomux
#define BUS_IOC_GPIO2D_IOMUX_H					0x005c	// gpio2d4~gpio2d7 iomux
#define BUS_IOC_GPIO3A_IOMUX_L					0x0060	// gpio3a0~gpio3a3 iomux
#define BUS_IOC_GPIO3A_IOMUX_H					0x0064	// gpio3a4~gpio3a7 iomux
#define BUS_IOC_GPIO3B_IOMUX_L					0x0068	// gpio3b0~gpio3b3 iomux
#define BUS_IOC_GPIO3B_IOMUX_H					0x006c	// gpio3b4~gpio3b7 iomux
#define BUS_IOC_GPIO3C_IOMUX_L					0x0070	// gpio3c0~gpio3c3 iomux
#define BUS_IOC_GPIO3C_IOMUX_H					0x0074	// gpio3c4~gpio3c7 iomux
#define BUS_IOC_GPIO3D_IOMUX_L					0x0078	// gpio3d0~gpio3d3 iomux
#define BUS_IOC_GPIO3D_IOMUX_H					0x007c	// gpio3s4~gpio3d7 iomux
#define BUS_IOC_GPIO4A_IOMUX_L					0x0080	// gpio4a0~gpio4a3 iomux
#define BUS_IOC_GPIO4A_IOMUX_H					0x0084	// gpio4a4~gpio4a7 iomux
#define BUS_IOC_GPIO4B_IOMUX_L					0x0088	// gpio4b0~gpio4b3 iomux
#define BUS_IOC_GPIO4B_IOMUX_H					0x008c	// gpio4b4~gpio4b7 iomux
#define BUS_IOC_GPIO4C_IOMUX_L					0x0090	// gpio4c0~gpio4c3 iomux
#define BUS_IOC_GPIO4C_IOMUX_H					0x0094	// gpio4c4~gpio4c7 iomux
#define BUS_IOC_GPIO4D_IOMUX_L					0x0098	// gpio4d0~gpio4d3 iomux
#define BUS_IOC_GPIO4D_IOMUX_H					0x009c	// gpio4d0~gpio4d3 iomux

struct soc_t *rk3588 = NULL;

static struct layout_t layout[] = {
	{"GPIO0_A0", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_L, 0}, {GPIO_SWPORT_DDR_L, 0}, {GPIO_SWPORT_DR_L, 0}, {GPIO_EXT_PORT, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A1", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_L, 4}, {GPIO_SWPORT_DDR_L, 1}, {GPIO_SWPORT_DR_L, 1}, {GPIO_EXT_PORT, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A2", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_L, 8}, {GPIO_SWPORT_DDR_L, 2}, {GPIO_SWPORT_DR_L, 2}, {GPIO_EXT_PORT, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A3", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_L, 12}, {GPIO_SWPORT_DDR_L, 3}, {GPIO_SWPORT_DR_L, 3}, {GPIO_EXT_PORT, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A4", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_H, 0}, {GPIO_SWPORT_DDR_L, 4}, {GPIO_SWPORT_DR_L, 4}, {GPIO_EXT_PORT, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A5", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_H, 4}, {GPIO_SWPORT_DDR_L, 5}, {GPIO_SWPORT_DR_L, 5}, {GPIO_EXT_PORT, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A6", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_H, 8}, {GPIO_SWPORT_DDR_L, 6}, {GPIO_SWPORT_DR_L, 6}, {GPIO_EXT_PORT, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A7", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0A_IOMUX_SEL_H, 12}, {GPIO_SWPORT_DDR_L, 7}, {GPIO_SWPORT_DR_L, 7}, {GPIO_EXT_PORT, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B0", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIOXX_IOMUX_SEL_X, 0}, {GPIO_SWPORT_DDR_L, 8}, {GPIO_SWPORT_DR_L, 8}, {GPIO_EXT_PORT, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B1", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0B_IOMUX_SEL_L, 4}, {GPIO_SWPORT_DDR_L, 9}, {GPIO_SWPORT_DR_L, 9}, {GPIO_EXT_PORT, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B2", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0B_IOMUX_SEL_L, 8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, {GPIO_EXT_PORT, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B3", 0, 0, {PMU1CRU_GATE_CON05, 5}, {PMU1_IOC_GPIO0B_IOMUX_SEL_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, {GPIO_EXT_PORT, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B4", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0B_IOMUX_SEL_H, 0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, {GPIO_EXT_PORT, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B5", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0B_IOMUX_SEL_H, 4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, {GPIO_EXT_PORT, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B6", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0B_IOMUX_SEL_H, 8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, {GPIO_EXT_PORT, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B7", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0B_IOMUX_SEL_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, {GPIO_EXT_PORT, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C0", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_L, 0}, {GPIO_SWPORT_DDR_H, 0}, {GPIO_SWPORT_DR_H, 0}, {GPIO_EXT_PORT, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C1", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_L, 4}, {GPIO_SWPORT_DDR_H, 1}, {GPIO_SWPORT_DR_H, 1}, {GPIO_EXT_PORT, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C2", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_L, 8}, {GPIO_SWPORT_DDR_H, 2}, {GPIO_SWPORT_DR_H, 2}, {GPIO_EXT_PORT, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C3", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_L, 12}, {GPIO_SWPORT_DDR_H, 3}, {GPIO_SWPORT_DR_H, 3}, {GPIO_EXT_PORT, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C4", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_H, 0}, {GPIO_SWPORT_DDR_H, 4}, {GPIO_SWPORT_DR_H, 4}, {GPIO_EXT_PORT, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C5", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_H, 4}, {GPIO_SWPORT_DDR_H, 5}, {GPIO_SWPORT_DR_H, 5}, {GPIO_EXT_PORT, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C6", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_H, 8}, {GPIO_SWPORT_DDR_H, 6}, {GPIO_SWPORT_DR_H, 6}, {GPIO_EXT_PORT, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C7", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0C_IOMUX_SEL_H, 12}, {GPIO_SWPORT_DDR_H, 7}, {GPIO_SWPORT_DR_H, 7}, {GPIO_EXT_PORT, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D0", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_L, 0}, {GPIO_SWPORT_DDR_H, 8}, {GPIO_SWPORT_DR_H, 8}, {GPIO_EXT_PORT, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D1", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_L, 4}, {GPIO_SWPORT_DDR_H, 9}, {GPIO_SWPORT_DR_H, 9}, {GPIO_EXT_PORT, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D2", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_L, 8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, {GPIO_EXT_PORT, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D3", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, {GPIO_EXT_PORT, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D4", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_H, 0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, {GPIO_EXT_PORT, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D5", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_H, 4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, {GPIO_EXT_PORT, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D6", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_H, 8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, {GPIO_EXT_PORT, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D7", 0, 1, {PMU1CRU_GATE_CON05, 5}, {PMU2_IOC_GPIO0D_IOMUX_SEL_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, {GPIO_EXT_PORT, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A0", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 0}, {GPIO_SWPORT_DR_L, 0}, {GPIO_EXT_PORT, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A1", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 1}, {GPIO_SWPORT_DR_L, 1}, {GPIO_EXT_PORT, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A2", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 2}, {GPIO_SWPORT_DR_L, 2}, {GPIO_EXT_PORT, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A3", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 3}, {GPIO_SWPORT_DR_L, 3}, {GPIO_EXT_PORT, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A4", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 4}, {GPIO_SWPORT_DR_L, 4}, {GPIO_EXT_PORT, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A5", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 5}, {GPIO_SWPORT_DR_L, 5}, {GPIO_EXT_PORT, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A6", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 6}, {GPIO_SWPORT_DR_L, 6}, {GPIO_EXT_PORT, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A7", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 7}, {GPIO_SWPORT_DR_L, 7}, {GPIO_EXT_PORT, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B0", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 8}, {GPIO_SWPORT_DR_L, 8}, {GPIO_EXT_PORT, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B1", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 9}, {GPIO_SWPORT_DR_L, 9}, {GPIO_EXT_PORT, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B2", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, {GPIO_EXT_PORT, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B3", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, {GPIO_EXT_PORT, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B4", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, {GPIO_EXT_PORT, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B5", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, {GPIO_EXT_PORT, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B6", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, {GPIO_EXT_PORT, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B7", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, {GPIO_EXT_PORT, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C0", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 0}, {GPIO_SWPORT_DR_H, 0}, {GPIO_EXT_PORT, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C1", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 1}, {GPIO_SWPORT_DR_H, 1}, {GPIO_EXT_PORT, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C2", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 2}, {GPIO_SWPORT_DR_H, 2}, {GPIO_EXT_PORT, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C3", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 3}, {GPIO_SWPORT_DR_H, 3}, {GPIO_EXT_PORT, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C4", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 4}, {GPIO_SWPORT_DR_H, 4}, {GPIO_EXT_PORT, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C5", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 5}, {GPIO_SWPORT_DR_H, 5}, {GPIO_EXT_PORT, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C6", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_H, 8}, {GPIO_SWPORT_DDR_H, 6}, {GPIO_SWPORT_DR_H, 6}, {GPIO_EXT_PORT, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C7", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 7}, {GPIO_SWPORT_DR_H, 7}, {GPIO_EXT_PORT, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D0", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 8}, {GPIO_SWPORT_DR_H, 8}, {GPIO_EXT_PORT, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D1", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 9}, {GPIO_SWPORT_DR_H, 9}, {GPIO_EXT_PORT, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D2", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, {GPIO_EXT_PORT, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D3", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, {GPIO_EXT_PORT, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D4", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, {GPIO_EXT_PORT, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D5", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, {GPIO_EXT_PORT, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D6", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_H, 8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, {GPIO_EXT_PORT, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D7", 1, 2, {CRU_GATE_CON16, 14}, {BUS_IOC_GPIO1D_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, {GPIO_EXT_PORT, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0}, 
	{"GPIO2_A0", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 0}, {GPIO_SWPORT_DR_L, 0}, {GPIO_EXT_PORT, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A1", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 1}, {GPIO_SWPORT_DR_L, 1}, {GPIO_EXT_PORT, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A2", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 2}, {GPIO_SWPORT_DR_L, 2}, {GPIO_EXT_PORT, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A3", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 3}, {GPIO_SWPORT_DR_L, 3}, {GPIO_EXT_PORT, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A4", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 4}, {GPIO_SWPORT_DR_L, 4}, {GPIO_EXT_PORT, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A5", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 5}, {GPIO_SWPORT_DR_L, 5}, {GPIO_EXT_PORT, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A6", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 6}, {GPIO_SWPORT_DR_L, 6}, {GPIO_EXT_PORT, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A7", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 7}, {GPIO_SWPORT_DR_L, 7}, {GPIO_EXT_PORT, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B0", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 8}, {GPIO_SWPORT_DR_L, 8}, {GPIO_EXT_PORT, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B1", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 9}, {GPIO_SWPORT_DR_L, 9}, {GPIO_EXT_PORT, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B2", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, {GPIO_EXT_PORT, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B3", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, {GPIO_EXT_PORT, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B4", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, {GPIO_EXT_PORT, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B5", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, {GPIO_EXT_PORT, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B6", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, {GPIO_EXT_PORT, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B7", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, {GPIO_EXT_PORT, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C0", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 0}, {GPIO_SWPORT_DR_H, 0}, {GPIO_EXT_PORT, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C1", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 1}, {GPIO_SWPORT_DR_H, 1}, {GPIO_EXT_PORT, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C2", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 2}, {GPIO_SWPORT_DR_H, 2}, {GPIO_EXT_PORT, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C3", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 3}, {GPIO_SWPORT_DR_H, 3}, {GPIO_EXT_PORT, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C4", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 4}, {GPIO_SWPORT_DR_H, 4}, {GPIO_EXT_PORT, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C5", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 5}, {GPIO_SWPORT_DR_H, 5}, {GPIO_EXT_PORT, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C6", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_H, 8}, {GPIO_SWPORT_DDR_H, 6}, {GPIO_SWPORT_DR_H, 6}, {GPIO_EXT_PORT, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C7", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 7}, {GPIO_SWPORT_DR_H, 7}, {GPIO_EXT_PORT, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D0", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 8}, {GPIO_SWPORT_DR_H, 8}, {GPIO_EXT_PORT, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D1", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 9}, {GPIO_SWPORT_DR_H, 9}, {GPIO_EXT_PORT, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D2", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, {GPIO_EXT_PORT, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D3", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, {GPIO_EXT_PORT, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D4", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, {GPIO_EXT_PORT, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D5", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, {GPIO_EXT_PORT, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D6", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_H, 8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, {GPIO_EXT_PORT, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D7", 2, 2, {CRU_GATE_CON17, 0}, {BUS_IOC_GPIO2D_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, {GPIO_EXT_PORT, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A0", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 0}, {GPIO_SWPORT_DR_L, 0}, {GPIO_EXT_PORT, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A1", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 1}, {GPIO_SWPORT_DR_L, 1}, {GPIO_EXT_PORT, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A2", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 2}, {GPIO_SWPORT_DR_L, 2}, {GPIO_EXT_PORT, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A3", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 3}, {GPIO_SWPORT_DR_L, 3}, {GPIO_EXT_PORT, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A4", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 4}, {GPIO_SWPORT_DR_L, 4}, {GPIO_EXT_PORT, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A5", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 5}, {GPIO_SWPORT_DR_L, 5}, {GPIO_EXT_PORT, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A6", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 6}, {GPIO_SWPORT_DR_L, 6}, {GPIO_EXT_PORT, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A7", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 7}, {GPIO_SWPORT_DR_L, 7}, {GPIO_EXT_PORT, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B0", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 8}, {GPIO_SWPORT_DR_L, 8}, {GPIO_EXT_PORT, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B1", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 9}, {GPIO_SWPORT_DR_L, 9}, {GPIO_EXT_PORT, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B2", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, {GPIO_EXT_PORT, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B3", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, {GPIO_EXT_PORT, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B4", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, {GPIO_EXT_PORT, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B5", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, {GPIO_EXT_PORT, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B6", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, {GPIO_EXT_PORT, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B7", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, {GPIO_EXT_PORT, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0}, 
	{"GPIO3_C0", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 0}, {GPIO_SWPORT_DR_H, 0}, {GPIO_EXT_PORT, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C1", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 1}, {GPIO_SWPORT_DR_H, 1}, {GPIO_EXT_PORT, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C2", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 2}, {GPIO_SWPORT_DR_H, 2}, {GPIO_EXT_PORT, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C3", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 3}, {GPIO_SWPORT_DR_H, 3}, {GPIO_EXT_PORT, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C4", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 4}, {GPIO_SWPORT_DR_H, 4}, {GPIO_EXT_PORT, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C5", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 5}, {GPIO_SWPORT_DR_H, 5}, {GPIO_EXT_PORT, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C6", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_H, 8}, {GPIO_SWPORT_DDR_H, 6}, {GPIO_SWPORT_DR_H, 6}, {GPIO_EXT_PORT, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C7", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 7}, {GPIO_SWPORT_DR_H, 7}, {GPIO_EXT_PORT, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D0", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3D_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 8}, {GPIO_SWPORT_DR_H, 8}, {GPIO_EXT_PORT, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D1", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3D_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 9}, {GPIO_SWPORT_DR_H, 9}, {GPIO_EXT_PORT, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D2", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3D_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, {GPIO_EXT_PORT, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D3", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, {GPIO_EXT_PORT, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D4", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3D_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, {GPIO_EXT_PORT, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D5", 3, 2, {CRU_GATE_CON17, 2}, {BUS_IOC_GPIO3D_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, {GPIO_EXT_PORT, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D6", 3, 2, {CRU_GATE_CON17, 2}, {GRF_UNDFEIND_IOMUX, 8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, {GPIO_EXT_PORT, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D7", 3, 2, {CRU_GATE_CON17, 2}, {GRF_UNDFEIND_IOMUX, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, {GPIO_EXT_PORT, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0}, 
	{"GPIO4_A0", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 0}, {GPIO_SWPORT_DR_L, 0}, {GPIO_EXT_PORT, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A1", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 1}, {GPIO_SWPORT_DR_L, 1}, {GPIO_EXT_PORT, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A2", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 2}, {GPIO_SWPORT_DR_L, 2}, {GPIO_EXT_PORT, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A3", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 3}, {GPIO_SWPORT_DR_L, 3}, {GPIO_EXT_PORT, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A4", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 4}, {GPIO_SWPORT_DR_L, 4}, {GPIO_EXT_PORT, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A5", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 5}, {GPIO_SWPORT_DR_L, 5}, {GPIO_EXT_PORT, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A6", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 6}, {GPIO_SWPORT_DR_L, 6}, {GPIO_EXT_PORT, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A7", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4A_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 7}, {GPIO_SWPORT_DR_L, 7}, {GPIO_EXT_PORT, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B0", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_L, 0}, {GPIO_SWPORT_DDR_L, 8}, {GPIO_SWPORT_DR_L, 8}, {GPIO_EXT_PORT, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B1", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_L, 4}, {GPIO_SWPORT_DDR_L, 9}, {GPIO_SWPORT_DR_L, 9}, {GPIO_EXT_PORT, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B2", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_L, 8}, {GPIO_SWPORT_DDR_L, 10}, {GPIO_SWPORT_DR_L, 10}, {GPIO_EXT_PORT, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B3", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_L, 12}, {GPIO_SWPORT_DDR_L, 11}, {GPIO_SWPORT_DR_L, 11}, {GPIO_EXT_PORT, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B4", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_H, 0}, {GPIO_SWPORT_DDR_L, 12}, {GPIO_SWPORT_DR_L, 12}, {GPIO_EXT_PORT, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B5", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_H, 4}, {GPIO_SWPORT_DDR_L, 13}, {GPIO_SWPORT_DR_L, 13}, {GPIO_EXT_PORT, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B6", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_H, 8}, {GPIO_SWPORT_DDR_L, 14}, {GPIO_SWPORT_DR_L, 14}, {GPIO_EXT_PORT, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B7", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4B_IOMUX_H, 12}, {GPIO_SWPORT_DDR_L, 15}, {GPIO_SWPORT_DR_L, 15}, {GPIO_EXT_PORT, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C0", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 0}, {GPIO_SWPORT_DR_H, 0}, {GPIO_EXT_PORT, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C1", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 1}, {GPIO_SWPORT_DR_H, 1}, {GPIO_EXT_PORT, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C2", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 2}, {GPIO_SWPORT_DR_H, 2}, {GPIO_EXT_PORT, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C3", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 3}, {GPIO_SWPORT_DR_H, 3}, {GPIO_EXT_PORT, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C4", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 4}, {GPIO_SWPORT_DR_H, 4}, {GPIO_EXT_PORT, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C5", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 5}, {GPIO_SWPORT_DR_H, 5}, {GPIO_EXT_PORT, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C6", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_H, 8}, {GPIO_SWPORT_DDR_H, 6}, {GPIO_SWPORT_DR_H, 6}, {GPIO_EXT_PORT, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C7", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4C_IOMUX_H, 12}, {GPIO_SWPORT_DDR_H, 7}, {GPIO_SWPORT_DR_H, 7}, {GPIO_EXT_PORT, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D0", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4D_IOMUX_L, 0}, {GPIO_SWPORT_DDR_H, 8}, {GPIO_SWPORT_DR_H, 8}, {GPIO_EXT_PORT, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D1", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4D_IOMUX_L, 4}, {GPIO_SWPORT_DDR_H, 9}, {GPIO_SWPORT_DR_H, 9}, {GPIO_EXT_PORT, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D2", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4D_IOMUX_L, 8}, {GPIO_SWPORT_DDR_H, 10}, {GPIO_SWPORT_DR_H, 10}, {GPIO_EXT_PORT, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D3", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4D_IOMUX_L, 12}, {GPIO_SWPORT_DDR_H, 11}, {GPIO_SWPORT_DR_H, 11}, {GPIO_EXT_PORT, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D4", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4D_IOMUX_H, 0}, {GPIO_SWPORT_DDR_H, 12}, {GPIO_SWPORT_DR_H, 12}, {GPIO_EXT_PORT, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D5", 4, 2, {CRU_GATE_CON17, 4}, {BUS_IOC_GPIO4D_IOMUX_H, 4}, {GPIO_SWPORT_DDR_H, 13}, {GPIO_SWPORT_DR_H, 13}, {GPIO_EXT_PORT, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D6", 4, 2, {CRU_GATE_CON17, 4}, {GRF_UNDFEIND_IOMUX, 8}, {GPIO_SWPORT_DDR_H, 14}, {GPIO_SWPORT_DR_H, 14}, {GPIO_EXT_PORT, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D7", 4, 2, {CRU_GATE_CON17, 4}, {GRF_UNDFEIND_IOMUX, 12}, {GPIO_SWPORT_DDR_H, 15}, {GPIO_SWPORT_DR_H, 15}, {GPIO_EXT_PORT, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
};

static int rk3588Setup(void) {
	if((rk3588->fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		wiringXLog(LOG_ERR, "wiringX failed to open /dev/mem for raw memory access");
		return -1;
	}
	for (int i = 0; i < GPIO_BANK_COUNT; i++) {
		if((rk3588->gpio[i] = (unsigned char *)rockchip_mmap(rk3588, rk3588->base_addr[i])) == NULL) {
			wiringXLog(LOG_ERR, "wiringX failed to map The %s %s GPIO memory address", rk3588->brand, rk3588->chip);
			return -1;
		}
	}

	if((cru_ns_register_virtual_address = (unsigned char *)rockchip_mmap(rk3588, CRU_NS_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk3588->brand, rk3588->chip);
		return -1;
	}
	if((pmu1_ioc_register_virtual_address = (unsigned char *)rockchip_mmap(rk3588, PMU1_IOC_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk3588->brand, rk3588->chip);
		return -1;
	}
	if((pmu2_ioc_register_virtual_address = (unsigned char *)rockchip_mmap(rk3588, PMU2_IOC_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk3588->brand, rk3588->chip);
		return -1;
	}
	if((bus_ioc_register_virtual_address = (unsigned char *)rockchip_mmap(rk3588, BUS_IOC_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s BUS_IOC memory address", rk3588->brand, rk3588->chip);
		return -1;
	}

	return 0;
}

static char *rk3588GetPinName(int pin) {
	return rockchipGetPinName(rk3588, pin);
}

static void rk3588SetMap(int *map, size_t size) {
	rockchipSetMap(rk3588, map, size);
}

static void rk3588SetIRQ(int *irq, size_t size) {
	rockchipSetIRQ(rk3588, irq, size);
}

struct layout_t * rk3588GetLayout(int i, int* mapping) {
	return rockchipGetLayout(rk3588, i, mapping);
}

static int rk3588DigitalWrite(int i, enum digital_value_t value) {
	struct layout_t *pin = NULL;
	unsigned int *out_reg = 0;

	if((pin = rockchipGetPinLayout(rk3588, i)) == NULL) {
		return -1;
	}

	if(pin->mode != PINMODE_OUTPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO%d is not set to output mode", rk3588->brand, rk3588->chip, i);
		return -1;
	}

	out_reg = (volatile unsigned int *)(rk3588->gpio[pin->bank] + pin->out.offset);

	if(value == HIGH) {
		REGISTER_SET_HIGH(out_reg, pin->out.bit, 1);
	} else if(value == LOW) {
		REGISTER_CLEAR_BITS(out_reg, pin->out.bit, 1);
	} else {
		wiringXLog(LOG_ERR, "invalid value %i for GPIO %i", value, i);
		return -1;
	}

	return 0;
}

static int rk3588DigitalRead(int i) {
	return rockchipDigitalRead(rk3588, i);
}

static int rk3588PinMode(int i, enum pinmode_t mode) {
	struct layout_t *pin = NULL;
	unsigned int *cru_reg = NULL;
	unsigned int *grf_reg = NULL;
	unsigned int *dir_reg = NULL;

	if( (pin = rockchipGetPinLayout(rk3588, i)) == NULL) {
		return -1;
	}

	cru_reg = (volatile unsigned int *)(cru_ns_register_virtual_address + pin->cru.offset);
	REGISTER_CLEAR_BITS(cru_reg, pin->cru.bit, 2);

	if(pin->iomux_num == 0) {
		grf_reg = (volatile unsigned int *)(pmu1_ioc_register_virtual_address + pin->grf.offset);
	} else if(pin->iomux_num == 1) {
		grf_reg = (volatile unsigned int *)(pmu2_ioc_register_virtual_address + pin->grf.offset);
	} else if(pin->iomux_num == 2) {
		grf_reg = (volatile unsigned int *)(bus_ioc_register_virtual_address + pin->grf.offset);
	} else {
		wiringXLog(LOG_ERR, "pin->iomux_num out of range %i, expect 0~2", i);
	}

	REGISTER_CLEAR_BITS(grf_reg, pin->grf.bit, 4);

	dir_reg = (volatile unsigned int *)(rk3588->gpio[pin->bank] + pin->direction.offset);
	if(mode == PINMODE_INPUT) {
		REGISTER_CLEAR_BITS(dir_reg, pin->direction.bit, 1);
	} else if(mode == PINMODE_OUTPUT) {
		REGISTER_SET_HIGH(dir_reg, pin->direction.bit, 1);
	} else {
		wiringXLog(LOG_ERR, "invalid pin mode %i for GPIO %i", mode, i);
		return -1;
	}

	pin->mode = mode;

	return 0;
}

static int rk3588ISR(int i, enum isr_mode_t mode) {
	return rockchipISR(rk3588, i, mode);
}

static int rk3588WaitForInterrupt(int i, int ms) {
	return rockchipWaitForInterrupt(rk3588, i, ms);
}

static int rk3588GC(void) {
	rockchipGC(rk3588);

	if(cru_ns_register_virtual_address != NULL) {
		munmap(cru_ns_register_virtual_address, rk3588->page_size);
		cru_ns_register_virtual_address = NULL;
	}
	if(pmu1_ioc_register_virtual_address != NULL) {
		munmap(pmu1_ioc_register_virtual_address, rk3588->page_size);
		pmu1_ioc_register_virtual_address = NULL;
	}
	if(pmu2_ioc_register_virtual_address != NULL) {
		munmap(pmu2_ioc_register_virtual_address, rk3588->page_size);
		pmu2_ioc_register_virtual_address = NULL;
	}
	if(bus_ioc_register_virtual_address != NULL) {
		munmap(bus_ioc_register_virtual_address, rk3588->page_size);
		bus_ioc_register_virtual_address = NULL;
	}
	for (int i = 0; i < GPIO_BANK_COUNT; i++) {
		if(rk3588->gpio[i] != NULL) {
			munmap(rk3588->gpio[i], rk3588->page_size);
			rk3588->gpio[i] = NULL;
		}
	}

	return 0;
}

static int rk3588SelectableFd(int i) {
	return rockchipSelectableFd(rk3588, i);
}

void rk3588Init(void) {
	soc_register(&rk3588, "Rockchip", "RK3588");

	rk3588->layout = layout;

	rk3588->support.isr_modes = ISR_MODE_RISING | ISR_MODE_FALLING | ISR_MODE_BOTH | ISR_MODE_NONE;
	rk3588->page_size = sysconf(_SC_PAGESIZE);
	memcpy(rk3588->base_addr, rk3588_gpio_register_physical_address, sizeof(rk3588_gpio_register_physical_address));

	rk3588->gc = &rk3588GC;
	rk3588->selectableFd = &rk3588SelectableFd;

	rk3588->pinMode = &rk3588PinMode;
	rk3588->setup = &rk3588Setup;
	rk3588->digitalRead = &rk3588DigitalRead;
	rk3588->digitalWrite = &rk3588DigitalWrite;
	rk3588->getPinName = &rk3588GetPinName;
	rk3588->setMap = &rk3588SetMap;
	rk3588->setIRQ = &rk3588SetIRQ;
	rk3588->isr = &rk3588ISR;
	rk3588->waitForInterrupt = &rk3588WaitForInterrupt;
}
