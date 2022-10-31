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

#include "rk3399.h"
#include "../../wiringx.h"
#include "../soc.h"
#include "../../i2c-dev.h"

#define GPIO_BANK_COUNT 5

const static uintptr_t gpio_register_physical_address[MAX_REG_AREA] = {0xff720000, 0xff730000, 0xff780000, 0xff788000, 0xff790000};
#define GPIO_SWPORTA_DR			0x0000	// GPIO data register offset
#define GPIO_SWPORTA_DDR		0x0004	// GPIO direction control register offset

static uintptr_t cru_register_virtual_address = NULL;
static uintptr_t pmugrf_register_virtual_address = NULL;
static uintptr_t grf_register_virtual_address = NULL;
#define CRU_REGISTER_PHYSICAL_ADDRESS			0xff760000
#define PMUGRF_REGISTER_PHYSICAL_ADDRESS	0xff320000
#define GRF_REGISTER_PHYSICAL_ADDRESS			0xff770000
#define PMUCRU_CLKGATE_CON1	0x0104	// Internal clock gating register for GPIO 0/1
#define CRU_CLKGATE_CON31		0x037c	// Internal clock gating register for GPIO 2/3/4
#define GRF_UNDFEIND_IOMUX	0xffff	//
#define PMUGRF_GPIO0A_IOMUX	0x0000	//GPIO0A iomux control
#define PMUGRF_GPIO0B_IOMUX	0x0004	//GPIO0B iomux control
#define PMUGRF_GPIO0C_IOMUX	GRF_UNDFEIND_IOMUX	//GPIO0C is undefined on the hardware
#define PMUGRF_GPIO0D_IOMUX	GRF_UNDFEIND_IOMUX	//GPIO0D is undefined on the hardware
#define PMUGRF_GPIO1A_IOMUX	0x0010	//GPIO1A iomux control
#define PMUGRF_GPIO1B_IOMUX	0x0014	//GPIO1B iomux control
#define PMUGRF_GPIO1C_IOMUX	0x0018	//GPIO1C iomux control
#define PMUGRF_GPIO1D_IOMUX	0x001C	//GPIO1D iomux control
#define GRF_GPIO2A_IOMUX		0xe000	//GPIO2A iomux control
#define GRF_GPIO2B_IOMUX		0xe004	//GPIO2B iomux control
#define GRF_GPIO2C_IOMUX		0xe008	//GPIO2C iomux control
#define GRF_GPIO2D_IOMUX		0xe00c	//GPIO2D iomux control
#define GRF_GPIO3A_IOMUX		0xe010	//GPIO3A iomux control
#define GRF_GPIO3B_IOMUX		0xe014	//GPIO3B iomux control
#define GRF_GPIO3C_IOMUX		0xe018	//GPIO3C iomux control
#define GRF_GPIO3D_IOMUX		0xe01c	//GPIO3D iomux control
#define GRF_GPIO4A_IOMUX		0xe020	//GPIO4A iomux control
#define GRF_GPIO4B_IOMUX		0xe024	//GPIO4B iomux control
#define GRF_GPIO4C_IOMUX		0xe028	//GPIO4C iomux control
#define GRF_GPIO4D_IOMUX		0xe02c	//GPIO4D iomux control
#define REGISTER_WRITE_MASK	16			// High 16 bits are write mask,
// with 1 enabling the coressponding lower bit
// Set lower bit to 0 and higher bit (write mask) to 1

#define REGISTER_CLEAR_BITS(addr, bit, size) \
	(*addr = *addr & ~(~(-1 << size) << bit) | (~(-1 << size) << bit << REGISTER_WRITE_MASK))
#define REGISTER_GET_BITS(addr, bit, size) \
	((*addr & ~(-1 << size) << bit) >> (bit - size))

struct soc_t *rk3399 = NULL;

static struct layout_t {
	char *name;
	int bank;
	struct {
		unsigned long offset;
		unsigned long bit;
	} cru;
	struct {
		unsigned long offset;
		unsigned long bit;
	} grf;
	struct {
		unsigned long offset;
		unsigned long bit;
	} direction;
	struct {
		unsigned long offset;
		unsigned long bit;
	} data;
	int support;
	enum pinmode_t mode;
	int fd;
} layout[] = {
	{"GPIO0_A0", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 0}, {GPIO_SWPORTA_DDR, 0}, {GPIO_SWPORTA_DR, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A1", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 2}, {GPIO_SWPORTA_DDR, 1}, {GPIO_SWPORTA_DR, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A2", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 4}, {GPIO_SWPORTA_DDR, 2}, {GPIO_SWPORTA_DR, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A3", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 6}, {GPIO_SWPORTA_DDR, 3}, {GPIO_SWPORTA_DR, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A4", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 8}, {GPIO_SWPORTA_DDR, 4}, {GPIO_SWPORTA_DR, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A5", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 10}, {GPIO_SWPORTA_DDR, 5}, {GPIO_SWPORTA_DR, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A6", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 12}, {GPIO_SWPORTA_DDR, 6}, {GPIO_SWPORTA_DR, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_A7", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0A_IOMUX, 14}, {GPIO_SWPORTA_DDR, 7}, {GPIO_SWPORTA_DR, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B0", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 0}, {GPIO_SWPORTA_DDR, 8}, {GPIO_SWPORTA_DR, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B1", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 2}, {GPIO_SWPORTA_DDR, 9}, {GPIO_SWPORTA_DR, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B2", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 4}, {GPIO_SWPORTA_DDR, 10}, {GPIO_SWPORTA_DR, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B3", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 6}, {GPIO_SWPORTA_DDR, 11}, {GPIO_SWPORTA_DR, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B4", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 8}, {GPIO_SWPORTA_DDR, 12}, {GPIO_SWPORTA_DR, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B5", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 10}, {GPIO_SWPORTA_DDR, 13}, {GPIO_SWPORTA_DR, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B6", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 12}, {GPIO_SWPORTA_DDR, 14}, {GPIO_SWPORTA_DR, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_B7", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0B_IOMUX, 14}, {GPIO_SWPORTA_DDR, 15}, {GPIO_SWPORTA_DR, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C0", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 0}, {GPIO_SWPORTA_DDR, 16}, {GPIO_SWPORTA_DR, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C1", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 2}, {GPIO_SWPORTA_DDR, 17}, {GPIO_SWPORTA_DR, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C2", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 4}, {GPIO_SWPORTA_DDR, 18}, {GPIO_SWPORTA_DR, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C3", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 6}, {GPIO_SWPORTA_DDR, 19}, {GPIO_SWPORTA_DR, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C4", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 8}, {GPIO_SWPORTA_DDR, 20}, {GPIO_SWPORTA_DR, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C5", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 10}, {GPIO_SWPORTA_DDR, 21}, {GPIO_SWPORTA_DR, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C6", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 12}, {GPIO_SWPORTA_DDR, 22}, {GPIO_SWPORTA_DR, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_C7", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0C_IOMUX, 14}, {GPIO_SWPORTA_DDR, 23}, {GPIO_SWPORTA_DR, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D0", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 0}, {GPIO_SWPORTA_DDR, 24}, {GPIO_SWPORTA_DR, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D1", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 2}, {GPIO_SWPORTA_DDR, 25}, {GPIO_SWPORTA_DR, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D2", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 4}, {GPIO_SWPORTA_DDR, 26}, {GPIO_SWPORTA_DR, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D3", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 6}, {GPIO_SWPORTA_DDR, 27}, {GPIO_SWPORTA_DR, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D4", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 8}, {GPIO_SWPORTA_DDR, 28}, {GPIO_SWPORTA_DR, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D5", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 10}, {GPIO_SWPORTA_DDR, 29}, {GPIO_SWPORTA_DR, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D6", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 12}, {GPIO_SWPORTA_DDR, 30}, {GPIO_SWPORTA_DR, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO0_D7", 0, {PMUCRU_CLKGATE_CON1, 3}, {PMUGRF_GPIO0D_IOMUX, 14}, {GPIO_SWPORTA_DDR, 31}, {GPIO_SWPORTA_DR, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A0", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 0}, {GPIO_SWPORTA_DDR, 0}, {GPIO_SWPORTA_DR, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A1", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 2}, {GPIO_SWPORTA_DDR, 1}, {GPIO_SWPORTA_DR, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A2", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 4}, {GPIO_SWPORTA_DDR, 2}, {GPIO_SWPORTA_DR, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A3", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 6}, {GPIO_SWPORTA_DDR, 3}, {GPIO_SWPORTA_DR, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A4", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 8}, {GPIO_SWPORTA_DDR, 4}, {GPIO_SWPORTA_DR, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A5", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 10}, {GPIO_SWPORTA_DDR, 5}, {GPIO_SWPORTA_DR, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A6", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 12}, {GPIO_SWPORTA_DDR, 6}, {GPIO_SWPORTA_DR, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_A7", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1A_IOMUX, 14}, {GPIO_SWPORTA_DDR, 7}, {GPIO_SWPORTA_DR, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B0", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 0}, {GPIO_SWPORTA_DDR, 8}, {GPIO_SWPORTA_DR, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B1", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 2}, {GPIO_SWPORTA_DDR, 9}, {GPIO_SWPORTA_DR, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B2", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 4}, {GPIO_SWPORTA_DDR, 10}, {GPIO_SWPORTA_DR, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B3", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 6}, {GPIO_SWPORTA_DDR, 11}, {GPIO_SWPORTA_DR, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B4", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 8}, {GPIO_SWPORTA_DDR, 12}, {GPIO_SWPORTA_DR, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B5", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 10}, {GPIO_SWPORTA_DDR, 13}, {GPIO_SWPORTA_DR, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B6", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 12}, {GPIO_SWPORTA_DDR, 14}, {GPIO_SWPORTA_DR, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_B7", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1B_IOMUX, 14}, {GPIO_SWPORTA_DDR, 15}, {GPIO_SWPORTA_DR, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C0", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 0}, {GPIO_SWPORTA_DDR, 16}, {GPIO_SWPORTA_DR, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C1", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 2}, {GPIO_SWPORTA_DDR, 17}, {GPIO_SWPORTA_DR, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C2", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 4}, {GPIO_SWPORTA_DDR, 18}, {GPIO_SWPORTA_DR, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C3", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 6}, {GPIO_SWPORTA_DDR, 19}, {GPIO_SWPORTA_DR, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C4", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 8}, {GPIO_SWPORTA_DDR, 20}, {GPIO_SWPORTA_DR, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C5", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 10}, {GPIO_SWPORTA_DDR, 21}, {GPIO_SWPORTA_DR, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C6", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 12}, {GPIO_SWPORTA_DDR, 22}, {GPIO_SWPORTA_DR, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_C7", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1C_IOMUX, 14}, {GPIO_SWPORTA_DDR, 23}, {GPIO_SWPORTA_DR, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D0", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 0}, {GPIO_SWPORTA_DDR, 24}, {GPIO_SWPORTA_DR, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D1", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 2}, {GPIO_SWPORTA_DDR, 25}, {GPIO_SWPORTA_DR, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D2", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 4}, {GPIO_SWPORTA_DDR, 26}, {GPIO_SWPORTA_DR, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D3", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 6}, {GPIO_SWPORTA_DDR, 27}, {GPIO_SWPORTA_DR, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D4", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 8}, {GPIO_SWPORTA_DDR, 28}, {GPIO_SWPORTA_DR, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D5", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 10}, {GPIO_SWPORTA_DDR, 29}, {GPIO_SWPORTA_DR, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D6", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 12}, {GPIO_SWPORTA_DDR, 30}, {GPIO_SWPORTA_DR, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO1_D7", 1, {PMUCRU_CLKGATE_CON1, 4}, {PMUGRF_GPIO1D_IOMUX, 14}, {GPIO_SWPORTA_DDR, 31}, {GPIO_SWPORTA_DR, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A0", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 0}, {GPIO_SWPORTA_DDR, 0}, {GPIO_SWPORTA_DR, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A1", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 2}, {GPIO_SWPORTA_DDR, 1}, {GPIO_SWPORTA_DR, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A2", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 4}, {GPIO_SWPORTA_DDR, 2}, {GPIO_SWPORTA_DR, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A3", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 6}, {GPIO_SWPORTA_DDR, 3}, {GPIO_SWPORTA_DR, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A4", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 8}, {GPIO_SWPORTA_DDR, 4}, {GPIO_SWPORTA_DR, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A5", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 10}, {GPIO_SWPORTA_DDR, 5}, {GPIO_SWPORTA_DR, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A6", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 12}, {GPIO_SWPORTA_DDR, 6}, {GPIO_SWPORTA_DR, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_A7", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2A_IOMUX, 14}, {GPIO_SWPORTA_DDR, 7}, {GPIO_SWPORTA_DR, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B0", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 0}, {GPIO_SWPORTA_DDR, 8}, {GPIO_SWPORTA_DR, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B1", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 2}, {GPIO_SWPORTA_DDR, 9}, {GPIO_SWPORTA_DR, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B2", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 4}, {GPIO_SWPORTA_DDR, 10}, {GPIO_SWPORTA_DR, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B3", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 6}, {GPIO_SWPORTA_DDR, 11}, {GPIO_SWPORTA_DR, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B4", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 8}, {GPIO_SWPORTA_DDR, 12}, {GPIO_SWPORTA_DR, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B5", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 10}, {GPIO_SWPORTA_DDR, 13}, {GPIO_SWPORTA_DR, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B6", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 12}, {GPIO_SWPORTA_DDR, 14}, {GPIO_SWPORTA_DR, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_B7", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2B_IOMUX, 14}, {GPIO_SWPORTA_DDR, 15}, {GPIO_SWPORTA_DR, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C0", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 0}, {GPIO_SWPORTA_DDR, 16}, {GPIO_SWPORTA_DR, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C1", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 2}, {GPIO_SWPORTA_DDR, 17}, {GPIO_SWPORTA_DR, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C2", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 4}, {GPIO_SWPORTA_DDR, 18}, {GPIO_SWPORTA_DR, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C3", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 6}, {GPIO_SWPORTA_DDR, 19}, {GPIO_SWPORTA_DR, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C4", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 8}, {GPIO_SWPORTA_DDR, 20}, {GPIO_SWPORTA_DR, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C5", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 10}, {GPIO_SWPORTA_DDR, 21}, {GPIO_SWPORTA_DR, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C6", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 12}, {GPIO_SWPORTA_DDR, 22}, {GPIO_SWPORTA_DR, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_C7", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2C_IOMUX, 14}, {GPIO_SWPORTA_DDR, 23}, {GPIO_SWPORTA_DR, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D0", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 0}, {GPIO_SWPORTA_DDR, 24}, {GPIO_SWPORTA_DR, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D1", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 2}, {GPIO_SWPORTA_DDR, 25}, {GPIO_SWPORTA_DR, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D2", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 4}, {GPIO_SWPORTA_DDR, 26}, {GPIO_SWPORTA_DR, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D3", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 6}, {GPIO_SWPORTA_DDR, 27}, {GPIO_SWPORTA_DR, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D4", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 10}, {GPIO_SWPORTA_DDR, 28}, {GPIO_SWPORTA_DR, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D5", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 12}, {GPIO_SWPORTA_DDR, 29}, {GPIO_SWPORTA_DR, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D6", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 14}, {GPIO_SWPORTA_DDR, 30}, {GPIO_SWPORTA_DR, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO2_D7", 2, {CRU_CLKGATE_CON31, 3}, {GRF_GPIO2D_IOMUX, 16}, {GPIO_SWPORTA_DDR, 31}, {GPIO_SWPORTA_DR, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A0", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 0}, {GPIO_SWPORTA_DDR, 0}, {GPIO_SWPORTA_DR, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A1", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 2}, {GPIO_SWPORTA_DDR, 1}, {GPIO_SWPORTA_DR, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A2", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 4}, {GPIO_SWPORTA_DDR, 2}, {GPIO_SWPORTA_DR, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A3", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 6}, {GPIO_SWPORTA_DDR, 3}, {GPIO_SWPORTA_DR, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A4", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 8}, {GPIO_SWPORTA_DDR, 4}, {GPIO_SWPORTA_DR, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A5", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 10}, {GPIO_SWPORTA_DDR, 5}, {GPIO_SWPORTA_DR, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A6", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 12}, {GPIO_SWPORTA_DDR, 6}, {GPIO_SWPORTA_DR, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_A7", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3A_IOMUX, 14}, {GPIO_SWPORTA_DDR, 7}, {GPIO_SWPORTA_DR, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B0", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 0}, {GPIO_SWPORTA_DDR, 8}, {GPIO_SWPORTA_DR, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B1", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 2}, {GPIO_SWPORTA_DDR, 9}, {GPIO_SWPORTA_DR, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B2", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 4}, {GPIO_SWPORTA_DDR, 10}, {GPIO_SWPORTA_DR, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B3", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 6}, {GPIO_SWPORTA_DDR, 11}, {GPIO_SWPORTA_DR, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B4", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 8}, {GPIO_SWPORTA_DDR, 12}, {GPIO_SWPORTA_DR, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B5", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 10}, {GPIO_SWPORTA_DDR, 13}, {GPIO_SWPORTA_DR, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B6", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 12}, {GPIO_SWPORTA_DDR, 14}, {GPIO_SWPORTA_DR, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_B7", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3B_IOMUX, 14}, {GPIO_SWPORTA_DDR, 15}, {GPIO_SWPORTA_DR, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C0", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 0}, {GPIO_SWPORTA_DDR, 16}, {GPIO_SWPORTA_DR, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C1", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 2}, {GPIO_SWPORTA_DDR, 17}, {GPIO_SWPORTA_DR, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C2", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 4}, {GPIO_SWPORTA_DDR, 18}, {GPIO_SWPORTA_DR, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C3", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 6}, {GPIO_SWPORTA_DDR, 19}, {GPIO_SWPORTA_DR, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C4", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 8}, {GPIO_SWPORTA_DDR, 20}, {GPIO_SWPORTA_DR, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C5", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 10}, {GPIO_SWPORTA_DDR, 21}, {GPIO_SWPORTA_DR, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C6", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 12}, {GPIO_SWPORTA_DDR, 22}, {GPIO_SWPORTA_DR, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_C7", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3C_IOMUX, 14}, {GPIO_SWPORTA_DDR, 23}, {GPIO_SWPORTA_DR, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D0", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 0}, {GPIO_SWPORTA_DDR, 24}, {GPIO_SWPORTA_DR, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D1", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 2}, {GPIO_SWPORTA_DDR, 25}, {GPIO_SWPORTA_DR, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D2", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 4}, {GPIO_SWPORTA_DDR, 26}, {GPIO_SWPORTA_DR, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D3", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 6}, {GPIO_SWPORTA_DDR, 27}, {GPIO_SWPORTA_DR, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D4", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 8}, {GPIO_SWPORTA_DDR, 28}, {GPIO_SWPORTA_DR, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D5", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 10}, {GPIO_SWPORTA_DDR, 29}, {GPIO_SWPORTA_DR, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D6", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 12}, {GPIO_SWPORTA_DDR, 30}, {GPIO_SWPORTA_DR, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO3_D7", 3, {CRU_CLKGATE_CON31, 4}, {GRF_GPIO3D_IOMUX, 14}, {GPIO_SWPORTA_DDR, 31}, {GPIO_SWPORTA_DR, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A0", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 0}, {GPIO_SWPORTA_DDR, 0}, {GPIO_SWPORTA_DR, 0}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A1", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 2}, {GPIO_SWPORTA_DDR, 1}, {GPIO_SWPORTA_DR, 1}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A2", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 4}, {GPIO_SWPORTA_DDR, 2}, {GPIO_SWPORTA_DR, 2}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A3", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 6}, {GPIO_SWPORTA_DDR, 3}, {GPIO_SWPORTA_DR, 3}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A4", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 8}, {GPIO_SWPORTA_DDR, 4}, {GPIO_SWPORTA_DR, 4}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A5", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 10}, {GPIO_SWPORTA_DDR, 5}, {GPIO_SWPORTA_DR, 5}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A6", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 12}, {GPIO_SWPORTA_DDR, 6}, {GPIO_SWPORTA_DR, 6}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_A7", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4A_IOMUX, 14}, {GPIO_SWPORTA_DDR, 7}, {GPIO_SWPORTA_DR, 7}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B0", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 0}, {GPIO_SWPORTA_DDR, 8}, {GPIO_SWPORTA_DR, 8}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B1", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 2}, {GPIO_SWPORTA_DDR, 9}, {GPIO_SWPORTA_DR, 9}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B2", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 4}, {GPIO_SWPORTA_DDR, 10}, {GPIO_SWPORTA_DR, 10}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B3", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 6}, {GPIO_SWPORTA_DDR, 11}, {GPIO_SWPORTA_DR, 11}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B4", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 8}, {GPIO_SWPORTA_DDR, 12}, {GPIO_SWPORTA_DR, 12}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B5", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 10}, {GPIO_SWPORTA_DDR, 13}, {GPIO_SWPORTA_DR, 13}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B6", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 12}, {GPIO_SWPORTA_DDR, 14}, {GPIO_SWPORTA_DR, 14}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_B7", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4B_IOMUX, 14}, {GPIO_SWPORTA_DDR, 15}, {GPIO_SWPORTA_DR, 15}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C0", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 0}, {GPIO_SWPORTA_DDR, 16}, {GPIO_SWPORTA_DR, 16}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C1", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 2}, {GPIO_SWPORTA_DDR, 17}, {GPIO_SWPORTA_DR, 17}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C2", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 4}, {GPIO_SWPORTA_DDR, 18}, {GPIO_SWPORTA_DR, 18}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C3", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 6}, {GPIO_SWPORTA_DDR, 19}, {GPIO_SWPORTA_DR, 19}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C4", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 8}, {GPIO_SWPORTA_DDR, 20}, {GPIO_SWPORTA_DR, 20}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C5", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 10}, {GPIO_SWPORTA_DDR, 21}, {GPIO_SWPORTA_DR, 21}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C6", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 12}, {GPIO_SWPORTA_DDR, 22}, {GPIO_SWPORTA_DR, 22}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_C7", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4C_IOMUX, 14}, {GPIO_SWPORTA_DDR, 23}, {GPIO_SWPORTA_DR, 23}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D0", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 0}, {GPIO_SWPORTA_DDR, 24}, {GPIO_SWPORTA_DR, 24}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D1", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 2}, {GPIO_SWPORTA_DDR, 25}, {GPIO_SWPORTA_DR, 25}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D2", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 4}, {GPIO_SWPORTA_DDR, 26}, {GPIO_SWPORTA_DR, 26}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D3", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 6}, {GPIO_SWPORTA_DDR, 27}, {GPIO_SWPORTA_DR, 27}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D4", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 8}, {GPIO_SWPORTA_DDR, 28}, {GPIO_SWPORTA_DR, 28}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D5", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 10}, {GPIO_SWPORTA_DDR, 29}, {GPIO_SWPORTA_DR, 29}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D6", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 12}, {GPIO_SWPORTA_DDR, 30}, {GPIO_SWPORTA_DR, 30}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
	{"GPIO4_D7", 4, {CRU_CLKGATE_CON31, 5}, {GRF_GPIO4D_IOMUX, 14}, {GPIO_SWPORTA_DDR, 31}, {GPIO_SWPORTA_DR, 31}, FUNCTION_DIGITAL, PINMODE_NOT_SET, 0},
};

static int rk3399Setup(void) {
	int i = 0;

	if((rk3399->fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		wiringXLog(LOG_ERR, "wiringX failed to open /dev/mem for raw memory access");
		return -1;
	}
	for(i = 0; i < GPIO_BANK_COUNT; i++) {
		if((rk3399->gpio[i] = (unsigned char *)mmap(0, rk3399->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk3399->fd, rk3399->base_addr[i])) == NULL) {
			wiringXLog(LOG_ERR, "wiringX failed to map The %s %s GPIO memory address", rk3399->brand, rk3399->chip);
			return -1;
		}
	}
	if((cru_register_virtual_address = (unsigned char *)mmap(0, rk3399->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk3399->fd, CRU_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s CRU memory address", rk3399->brand, rk3399->chip);
		return -1;
	}
	if((pmugrf_register_virtual_address = (unsigned char *)mmap(0, rk3399->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk3399->fd, PMUGRF_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s PMUGRF memory address", rk3399->brand, rk3399->chip);
		return -1;
	}
	if((grf_register_virtual_address = (unsigned char *)mmap(0, rk3399->page_size, PROT_READ | PROT_WRITE, MAP_SHARED, rk3399->fd, GRF_REGISTER_PHYSICAL_ADDRESS)) == NULL) {
		wiringXLog(LOG_ERR, "wiringX failed to map The %s %s GRF memory address", rk3399->brand, rk3399->chip);
		return -1;
	}

	return 0;
}

static char *rk3399GetPinName(int pin) {
	return rk3399->layout[pin].name;
}

static void rk3399SetMap(int *map, size_t size) {
	rk3399->map = map;
	rk3399->map_size = size;
}

static void rk3399SetIRQ(int *irq, size_t size) {
	rk3399->irq = irq;
	rk3399->irq_size = size;
}

struct layout_t *rk3399GetLayout(int i, int *mapping) {
	struct layout_t *pin = NULL;
	unsigned int *grf_reg = NULL;
	unsigned int iomux_value = 0;

	if(mapping == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped", rk3399->brand, rk3399->chip);
		return NULL;
	}
	if(wiringXValidGPIO(i) != 0) {
		wiringXLog(LOG_ERR, "The %i is not the right GPIO number");
		return NULL;
	}
	if(rk3399->fd <= 0 || rk3399->gpio == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX", rk3399->brand, rk3399->chip);
		return NULL;
	}

	pin = &rk3399->layout[mapping[i]];
	if(pin->bank < 0 || pin->bank >= GPIO_BANK_COUNT) {
		wiringXLog(LOG_ERR, "pin->bank out of range: %i, expect 0~4", pin->bank);
		return NULL;
	}

	if(pin->grf.offset == GRF_UNDFEIND_IOMUX) {
		wiringXLog(LOG_ERR, "Pin %i is mapped to undefined pin on the hardware", i);
		return NULL;
	}

	return pin;
}

#define rk3399GetPinLayout(i) (rk3399GetLayout(i, rk3399->map))
#define rk3399GetIrqLayout(i) (rk3399GetLayout(i, rk3399->irq))

static int rk3399DigitalWrite(int i, enum digital_value_t value) {
	struct layout_t *pin = NULL;
	unsigned int *data_reg = 0;

	if((pin = rk3399GetPinLayout(i)) == NULL) {
		return -1;
	}

	if(pin->mode != PINMODE_OUTPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO%d is not set to output mode", rk3399->brand, rk3399->chip, i);
		return -1;
	}

	data_reg = (volatile unsigned int *)(rk3399->gpio[pin->bank] + pin->data.offset);
	if(value == HIGH) {
		*data_reg |= (1 << (pin->data.bit));
	} else if(value == LOW) {
		*data_reg &= ~(1 << (pin->data.bit));
	} else {
		wiringXLog(LOG_ERR, "invalid value %i for GPIO %i", value, i);
		return -1;
	}

	return 0;
}

static int rk3399DigitalRead(int i) {
	struct layout_t *pin = NULL;
	unsigned int *data_reg = NULL;
	uint32_t val = 0;

	if((pin = rk3399GetPinLayout(i)) == NULL) {
		return -1;
	}

	if(pin->mode != PINMODE_INPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO%d is not set to input mode", rk3399->brand, rk3399->chip, i);
		return -1;
	}

	data_reg = (volatile unsigned int *)(rk3399->gpio[pin->bank] + pin->data.offset);
	val = *data_reg;

	return (int)((val & (1 << pin->data.bit)) >> pin->data.bit);
}

static int rk3399PinMode(int i, enum pinmode_t mode) {
	struct layout_t *pin = NULL;
	unsigned int *cru_reg = NULL;
	unsigned int *grf_reg = NULL;
	unsigned int *dir_reg = NULL;
	unsigned int mask = 0;

	if((pin = rk3399GetPinLayout(i)) == NULL) {
		return -1;
	}

	cru_reg = (volatile unsigned int *)(cru_register_virtual_address + pin->cru.offset);
	// set to low to enable the clock for GPIO bank
	*cru_reg = REGISTER_CLEAR_BITS(cru_reg, pin->cru.bit, 1);

	if(pin->bank == 0 || pin->bank == 1) {
		grf_reg = (volatile unsigned int *)(pmugrf_register_virtual_address + pin->grf.offset);
	} else {
		grf_reg = (volatile unsigned int *)(grf_register_virtual_address + pin->grf.offset);
	}
	*grf_reg = REGISTER_CLEAR_BITS(grf_reg, pin->grf.bit, 2);

	dir_reg = (volatile unsigned int *)(rk3399->gpio[pin->bank] + pin->direction.offset);
	if(mode == PINMODE_INPUT) {
		*dir_reg &= ~(1 << pin->direction.bit);
	} else if(mode == PINMODE_OUTPUT) {
		*dir_reg |= (1 << pin->direction.bit);
	} else {
		wiringXLog(LOG_ERR, "invalid pin mode %i for GPIO %i", mode, i);
		return -1;
	}

	pin->mode = mode;

	return 0;
}

static int rk3399ISR(int i, enum isr_mode_t mode) {
	struct layout_t *pin = NULL;
	char path[PATH_MAX];
	memset(path, 0, sizeof(path));

	if((pin = rk3399GetIrqLayout(i)) == NULL) {
		return -1;
	}

	sprintf(path, "/sys/class/gpio/gpio%d", rk3399->irq[i]);
	if((soc_sysfs_check_gpio(rk3399, path)) == -1) {
		sprintf(path, "/sys/class/gpio/export");
		if(soc_sysfs_gpio_export(rk3399, path, rk3399->irq[i]) == -1) {
			return -1;
		}
	}

	sprintf(path, "/sys/class/gpio/gpio%d/direction", rk3399->irq[i]);
	if(soc_sysfs_set_gpio_direction(rk3399, path, "in") == -1) {
		return -1;
	}

	sprintf(path, "/sys/class/gpio/gpio%d/edge", rk3399->irq[i]);
	if(soc_sysfs_set_gpio_interrupt_mode(rk3399, path, mode) == -1) {
		return -1;
	}

	sprintf(path, "/sys/class/gpio/gpio%d/value", rk3399->irq[i]);
	if((pin->fd = soc_sysfs_gpio_reset_value(rk3399, path)) == -1) {
		return -1;
	}

	pin->mode = PINMODE_INTERRUPT;

	return 0;
}

static int rk3399WaitForInterrupt(int i, int ms) {
	struct layout_t *pin = NULL;

	if((pin = rk3399GetIrqLayout(i)) == NULL) {
		return -1;
	}

	if(pin->mode != PINMODE_INTERRUPT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to interrupt mode", rk3399->brand, rk3399->chip, i);
		return -1;
	}

	return soc_wait_for_interrupt(rk3399, pin->fd, ms);
}

static int rk3399GC(void) {
	struct layout_t *pin = NULL;
	char path[PATH_MAX];
	int i = 0;
	memset(path, 0, sizeof(path));

	if(rk3399->map != NULL) {
		for(i = 0; i < rk3399->map_size; i++) {
			pin = &rk3399->layout[rk3399->map[i]];
			if(pin->mode == PINMODE_OUTPUT) {
				pinMode(i, PINMODE_INPUT);
			} else if(pin->mode == PINMODE_INTERRUPT) {
				sprintf(path, "/sys/class/gpio/gpio%d", rk3399->irq[i]);
				if((soc_sysfs_check_gpio(rk3399, path)) == 0) {
					sprintf(path, "/sys/class/gpio/unexport");
					soc_sysfs_gpio_unexport(rk3399, path, rk3399->irq[i]);
				}
			}

			if(pin->fd > 0) {
				close(pin->fd);
				pin->fd = 0;
			}
		}
	}

	if(cru_register_virtual_address != NULL) {
		munmap(cru_register_virtual_address, rk3399->page_size);
		cru_register_virtual_address = NULL;
	}
	if(pmugrf_register_virtual_address != NULL) {
		munmap(pmugrf_register_virtual_address, rk3399->page_size);
		pmugrf_register_virtual_address = NULL;
	}
	if(grf_register_virtual_address != NULL) {
		munmap(grf_register_virtual_address, rk3399->page_size);
		grf_register_virtual_address = NULL;
	}
	for(i = 0; i < GPIO_BANK_COUNT; i++) {
		if(rk3399->gpio[i] != NULL) {
			munmap(rk3399->gpio[i], rk3399->page_size);
			rk3399->gpio[i] = NULL;
		}
	}

	return 0;
}

static int rk3399SelectableFd(int i) {
	struct layout_t *pin = NULL;

	if((pin = rk3399GetIrqLayout(i)) == NULL) {
		return -1;
	}

	return pin->fd;
}

void rk3399Init(void) {
	soc_register(&rk3399, "Rockchip", "RK3399");

	rk3399->layout = layout;

	rk3399->support.isr_modes = ISR_MODE_RISING | ISR_MODE_FALLING | ISR_MODE_BOTH | ISR_MODE_NONE;
	rk3399->page_size = sysconf(_SC_PAGESIZE);
	memcpy(rk3399->base_addr, gpio_register_physical_address, sizeof(gpio_register_physical_address));

	rk3399->gc = &rk3399GC;
	rk3399->selectableFd = &rk3399SelectableFd;
	rk3399->pinMode = &rk3399PinMode;
	rk3399->setup = &rk3399Setup;
	rk3399->digitalRead = &rk3399DigitalRead;
	rk3399->digitalWrite = &rk3399DigitalWrite;
	rk3399->getPinName = &rk3399GetPinName;
	rk3399->setMap = &rk3399SetMap;
	rk3399->setIRQ = &rk3399SetIRQ;
	rk3399->isr = &rk3399ISR;
	rk3399->waitForInterrupt = &rk3399WaitForInterrupt;
}
