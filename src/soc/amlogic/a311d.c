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

#include "a311d.h"
#include "../../wiringx.h"
#include "../soc.h"

#define PERIPHS_PIN_MUX_0 0xb0	//BOOT_0 ~ BOOT_7  pinmux, 0xff6346c0, 1
#define PERIPHS_PIN_MUX_1 0xb1	//BOOT_8 ~ BOOT_15 pinmux, 0xff6346c4, 2
#define PERIPHS_PIN_MUX_3 0xb3  //GPIOX_0 ~ GPIOX7 pinmux, 0xff6346cc, 4
#define PERIPHS_PIN_MUX_4 0xb4  //GPIOX_8 ~ GPIOX15 pinmux, 0xff6346d0, 5
#define PERIPHS_PIN_MUX_5 0xb5  //GPIOX_16 ~ GPIOX19 pinmux, 0xff6346d4, 6
#define PERIPHS_PIN_MUX_6 0xb6  //GPIOZ_0 ~ GPIOZ_7 pinmux
#define PERIPHS_PIN_MUX_7 0xb7  //GPIOZ_8 ~ GPIO_15 pinmux,
#define PERIPHS_PIN_MUX_9 0xb9  //GPIOC_0 ~ GPIOC_7 pinmux, 0xff6346e4, 3
#define PERIPHS_PIN_MUX_B 0xbb  //GPIOH_0 ~ GPIOH_7 pinmux, 0xff6346ec, 7
#define PERIPHS_PIN_MUX_C 0xbc  //GPIOH_8 pinmux, 0xff6346f0, 8
#define PERIPHS_PIN_MUX_D 0xbd  //GPIOA_0 ~ GPIO7 pinmux, 0xff6346f4
#define PERIPHS_PIN_MUX_E 0xbe  //GPIOA_8 ~ GPIOA_15 pinmux, 0xff6346f4
#define AO_RTI_PINMUX_REG0 0x05  //GPIOAO_0 ~ GPIOAO_7 pinmux, 0xff800014, 9
#define AO_RTI_PINMUX_REG1 0x06  //GPIOAO_8 ~ GPIOAO_11 pinmux[0:15], GPIOE_0 ~ GPIOE_2 pinmux[16:24], 0xff800018, 10
#define PREG_PAD_GPIO0_EN_N 0x10 //BOOT_0 ~ BOOT_15, direction
#define PREG_PAD_GPIO1_EN_N 0x13 //GPIOC_0 ~ GPIOC_7, direction
#define PREG_PAD_GPIO2_EN_N 0x16 //GPIOX_0 ~ GPIOX_19, direction
#define PREG_PAD_GPIO3_EN_N 0x19 //GPIOH_0 ~ GPIOH_8, direction
#define PREG_PAD_GPIO4_EN_N 0x1c //GPIOZ_0 ~ GPIOZ_15, direction
#define PREG_PAD_GPIO5_EN_N 0x20 //GPIOA_0 ~ GPIOA_15, direction
#define AO_GPIO_O_EN_N 0x09  //GPIOAO, direction, [0,11] [16:18] RW、OUTPUT_ENABLE, [12,15] [19,31] R 、reserved
#define PREG_PAD_GPIO0_O 0x11  //BOOT_0 ~ BOOT_15, data
#define PREG_PAD_GPIO1_O 0x14  //GPIOC_0 ~ GPIOC_7, data
#define PREG_PAD_GPIO2_O 0x17  //GPIOX_0 ~ GPIOX_19, data
#define PREG_PAD_GPIO3_O 0x1a  //GPIOH_4 ~ GPIOH_7, data
#define PREG_PAD_GPIO4_O 0x1d  //GPIOZ_0 ~ GPIOZ_15, data
#define PREG_PAD_GPIO5_O 0x21  //GPIOA_0 ~ GPIOA_15, data
#define AO_GPIO_O 0x0d  //GPIOAO data, 
#define REGISTER_CLEAR_BITS(addr, bit) \
	*addr |= (1 << bit)
#define REGISTER_SET_BITS(addr, bit) \
	*addr &= ~(1 << bit)

struct soc_t *amlogicA311d = NULL;

static struct layout_t {
	char *name;
    long baseAddrMark;

	struct {
		unsigned long offset;
		unsigned long bit;
	} pinmux;

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
	// GPIOAO_x
	{ "GPIOAO_0", 1, {AO_RTI_PINMUX_REG0, 0}, {AO_GPIO_O_EN_N, 0}, {AO_GPIO_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_1", 1, {AO_RTI_PINMUX_REG0, 4}, {AO_GPIO_O_EN_N, 1}, {AO_GPIO_O, 1}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_2", 1, {AO_RTI_PINMUX_REG0, 8}, {AO_GPIO_O_EN_N, 2}, {AO_GPIO_O, 2}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_3", 1, {AO_RTI_PINMUX_REG0, 12}, {AO_GPIO_O_EN_N, 3}, {AO_GPIO_O, 3}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_4", 1, {AO_RTI_PINMUX_REG0, 16}, {AO_GPIO_O_EN_N, 4}, {AO_GPIO_O, 4}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_5", 1, {AO_RTI_PINMUX_REG0, 20}, {AO_GPIO_O_EN_N, 5}, {AO_GPIO_O, 5}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_6", 1, {AO_RTI_PINMUX_REG0, 24}, {AO_GPIO_O_EN_N, 6}, {AO_GPIO_O, 6}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_7", 1, {AO_RTI_PINMUX_REG0, 28}, {AO_GPIO_O_EN_N, 7}, {AO_GPIO_O, 7}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_8", 1, {AO_RTI_PINMUX_REG1, 0}, {AO_GPIO_O_EN_N, 8}, {AO_GPIO_O, 8}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_9", 1, {AO_RTI_PINMUX_REG1, 4}, {AO_GPIO_O_EN_N, 9}, {AO_GPIO_O, 9}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_10", 1, {AO_RTI_PINMUX_REG1, 8}, {AO_GPIO_O_EN_N, 10}, {AO_GPIO_O, 10}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOAO_11", 1, {AO_RTI_PINMUX_REG1, 12}, {AO_GPIO_O_EN_N, 11}, {AO_GPIO_O, 11}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	// GPIOE_x
	{ "GPIOE_0", 1, {AO_RTI_PINMUX_REG1, 16}, {AO_GPIO_O_EN_N, 16}, {AO_GPIO_O, 16}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOE_1", 1, {AO_RTI_PINMUX_REG1, 20}, {AO_GPIO_O_EN_N, 17}, {AO_GPIO_O, 17}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOE_0", 1, {AO_RTI_PINMUX_REG1, 24}, {AO_GPIO_O_EN_N, 18}, {AO_GPIO_O, 18}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	// GPIOZ_x
	{ "GPIOZ_0", 0, {PERIPHS_PIN_MUX_6, 0}, {PREG_PAD_GPIO4_EN_N, 0}, {PREG_PAD_GPIO4_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_1", 0, {PERIPHS_PIN_MUX_6, 4}, {PREG_PAD_GPIO4_EN_N, 1}, {PREG_PAD_GPIO4_O, 1}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_2", 0, {PERIPHS_PIN_MUX_6, 8}, {PREG_PAD_GPIO4_EN_N, 2}, {PREG_PAD_GPIO4_O, 2}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_3", 0, {PERIPHS_PIN_MUX_6, 12}, {PREG_PAD_GPIO4_EN_N, 3}, {PREG_PAD_GPIO4_O, 3}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_4", 0, {PERIPHS_PIN_MUX_6, 16}, {PREG_PAD_GPIO4_EN_N, 4}, {PREG_PAD_GPIO4_O, 4}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_5", 0, {PERIPHS_PIN_MUX_6, 20}, {PREG_PAD_GPIO4_EN_N, 5}, {PREG_PAD_GPIO4_O, 5}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_6", 0, {PERIPHS_PIN_MUX_6, 24}, {PREG_PAD_GPIO4_EN_N, 6}, {PREG_PAD_GPIO4_O, 6}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_7", 0, {PERIPHS_PIN_MUX_6, 28}, {PREG_PAD_GPIO4_EN_N, 7}, {PREG_PAD_GPIO4_O, 7}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_8", 0, {PERIPHS_PIN_MUX_7, 0}, {PREG_PAD_GPIO4_EN_N, 8}, {PREG_PAD_GPIO4_O, 8}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_9", 0, {PERIPHS_PIN_MUX_7, 4}, {PREG_PAD_GPIO4_EN_N, 9}, {PREG_PAD_GPIO4_O, 9}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_10", 0, {PERIPHS_PIN_MUX_7, 8}, {PREG_PAD_GPIO4_EN_N, 10}, {PREG_PAD_GPIO4_O, 10}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_11", 0, {PERIPHS_PIN_MUX_7, 12}, {PREG_PAD_GPIO4_EN_N, 11}, {PREG_PAD_GPIO4_O, 11}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_12", 0, {PERIPHS_PIN_MUX_7, 16}, {PREG_PAD_GPIO4_EN_N, 12}, {PREG_PAD_GPIO4_O, 12}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_13", 0, {PERIPHS_PIN_MUX_7, 20}, {PREG_PAD_GPIO4_EN_N, 13}, {PREG_PAD_GPIO4_O, 13}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_14", 0, {PERIPHS_PIN_MUX_7, 24}, {PREG_PAD_GPIO4_EN_N, 14}, {PREG_PAD_GPIO4_O, 14}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOZ_15", 0, {PERIPHS_PIN_MUX_7, 28}, {PREG_PAD_GPIO4_EN_N, 15}, {PREG_PAD_GPIO4_O, 15}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	// GPIOH_x, data register, to be determined
	{ "GPIOH_0", 0, {PERIPHS_PIN_MUX_B, 0}, {PREG_PAD_GPIO3_EN_N, 0}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_1", 0, {PERIPHS_PIN_MUX_B, 4}, {PREG_PAD_GPIO3_EN_N, 1}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_2", 0, {PERIPHS_PIN_MUX_B, 8}, {PREG_PAD_GPIO3_EN_N, 2}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_3", 0, {PERIPHS_PIN_MUX_B, 12}, {PREG_PAD_GPIO3_EN_N, 3}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_4", 0, {PERIPHS_PIN_MUX_B, 16}, {PREG_PAD_GPIO3_EN_N, 4}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_5", 0, {PERIPHS_PIN_MUX_B, 20}, {PREG_PAD_GPIO3_EN_N, 5}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_6", 0, {PERIPHS_PIN_MUX_B, 24}, {PREG_PAD_GPIO3_EN_N, 6}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_7", 0, {PERIPHS_PIN_MUX_B, 28}, {PREG_PAD_GPIO3_EN_N, 7}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOH_8", 0, {PERIPHS_PIN_MUX_C, 0}, {PREG_PAD_GPIO3_EN_N, 8}, {PREG_PAD_GPIO3_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	// BOOT_x
	{ "BOOT_0", 0, {PERIPHS_PIN_MUX_0, 0}, {PREG_PAD_GPIO0_EN_N, 0}, {PREG_PAD_GPIO0_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_1", 0, {PERIPHS_PIN_MUX_0, 4}, {PREG_PAD_GPIO0_EN_N, 1}, {PREG_PAD_GPIO0_O, 1}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_2", 0, {PERIPHS_PIN_MUX_0, 8}, {PREG_PAD_GPIO0_EN_N, 2}, {PREG_PAD_GPIO0_O, 2}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_3", 0, {PERIPHS_PIN_MUX_0, 12}, {PREG_PAD_GPIO0_EN_N, 3}, {PREG_PAD_GPIO0_O, 3}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_4", 0, {PERIPHS_PIN_MUX_0, 16}, {PREG_PAD_GPIO0_EN_N, 4}, {PREG_PAD_GPIO0_O, 4}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_5", 0, {PERIPHS_PIN_MUX_0, 20}, {PREG_PAD_GPIO0_EN_N, 5}, {PREG_PAD_GPIO0_O, 5}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_6", 0, {PERIPHS_PIN_MUX_0, 24}, {PREG_PAD_GPIO0_EN_N, 6}, {PREG_PAD_GPIO0_O, 6}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_7", 0, {PERIPHS_PIN_MUX_0, 28}, {PREG_PAD_GPIO0_EN_N, 7}, {PREG_PAD_GPIO0_O, 7}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_8", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 8}, {PREG_PAD_GPIO0_O, 8}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_9", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 9}, {PREG_PAD_GPIO0_O, 9}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_10", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 10}, {PREG_PAD_GPIO0_O, 10}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_11", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 11}, {PREG_PAD_GPIO0_O, 11}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_12", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 12}, {PREG_PAD_GPIO0_O, 12}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_13", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 13}, {PREG_PAD_GPIO0_O, 13}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_14", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 14}, {PREG_PAD_GPIO0_O, 14}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "BOOT_15", 0, {PERIPHS_PIN_MUX_1, 0}, {PREG_PAD_GPIO0_EN_N, 15}, {PREG_PAD_GPIO0_O, 15}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	// GPIOC_x
	{ "GPIOC_0", 0, {PERIPHS_PIN_MUX_9, 0}, {PREG_PAD_GPIO1_EN_N, 0}, {PREG_PAD_GPIO1_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_1", 0, {PERIPHS_PIN_MUX_9, 4}, {PREG_PAD_GPIO1_EN_N, 1}, {PREG_PAD_GPIO1_O, 1}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_2", 0, {PERIPHS_PIN_MUX_9, 8}, {PREG_PAD_GPIO1_EN_N, 2}, {PREG_PAD_GPIO1_O, 2}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_3", 0, {PERIPHS_PIN_MUX_9, 12}, {PREG_PAD_GPIO1_EN_N, 3}, {PREG_PAD_GPIO1_O, 3}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_4", 0, {PERIPHS_PIN_MUX_9, 16}, {PREG_PAD_GPIO1_EN_N, 4}, {PREG_PAD_GPIO1_O, 4}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_5", 0, {PERIPHS_PIN_MUX_9, 20}, {PREG_PAD_GPIO1_EN_N, 5}, {PREG_PAD_GPIO1_O, 5}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_6", 0, {PERIPHS_PIN_MUX_9, 24}, {PREG_PAD_GPIO1_EN_N, 6}, {PREG_PAD_GPIO1_O, 6}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOC_7", 0, {PERIPHS_PIN_MUX_9, 28}, {PREG_PAD_GPIO1_EN_N, 7}, {PREG_PAD_GPIO1_O, 7}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },	
	// GPIOA_x
	{ "GPIOA_0", 0, {PERIPHS_PIN_MUX_D, 0}, {PREG_PAD_GPIO5_EN_N, 0}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_1", 0, {PERIPHS_PIN_MUX_D, 4}, {PREG_PAD_GPIO5_EN_N, 1}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_2", 0, {PERIPHS_PIN_MUX_D, 8}, {PREG_PAD_GPIO5_EN_N, 2}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_3", 0, {PERIPHS_PIN_MUX_D, 12}, {PREG_PAD_GPIO5_EN_N, 3}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_4", 0, {PERIPHS_PIN_MUX_D, 16}, {PREG_PAD_GPIO5_EN_N, 4}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_5", 0, {PERIPHS_PIN_MUX_D, 20}, {PREG_PAD_GPIO5_EN_N, 5}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_6", 0, {PERIPHS_PIN_MUX_D, 24}, {PREG_PAD_GPIO5_EN_N, 6}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_7", 0, {PERIPHS_PIN_MUX_D, 28}, {PREG_PAD_GPIO5_EN_N, 7}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_8", 0, {PERIPHS_PIN_MUX_E, 0}, {PREG_PAD_GPIO5_EN_N, 8}, {PREG_PAD_GPIO5_O, 8}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_9", 0, {PERIPHS_PIN_MUX_E, 4}, {PREG_PAD_GPIO5_EN_N, 9}, {PREG_PAD_GPIO5_O, 9}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_10", 0, {PERIPHS_PIN_MUX_E, 8}, {PREG_PAD_GPIO5_EN_N, 10}, {PREG_PAD_GPIO5_O, 10}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_11", 0, {PERIPHS_PIN_MUX_E, 12}, {PREG_PAD_GPIO5_EN_N, 11}, {PREG_PAD_GPIO5_O, 11}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_12", 0, {PERIPHS_PIN_MUX_E, 16}, {PREG_PAD_GPIO5_EN_N, 12}, {PREG_PAD_GPIO5_O, 12}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_13", 0, {PERIPHS_PIN_MUX_E, 20}, {PREG_PAD_GPIO5_EN_N, 13}, {PREG_PAD_GPIO5_O, 13}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_14", 0, {PERIPHS_PIN_MUX_E, 24}, {PREG_PAD_GPIO5_EN_N, 14}, {PREG_PAD_GPIO5_O, 14}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOA_15", 0, {PERIPHS_PIN_MUX_E, 28}, {PREG_PAD_GPIO5_EN_N, 15}, {PREG_PAD_GPIO5_O, 15}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	// GPIOX_x
	{ "GPIOX_0", 0, {PERIPHS_PIN_MUX_3, 0}, {PREG_PAD_GPIO5_EN_N, 0}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_1", 0, {PERIPHS_PIN_MUX_3, 4}, {PREG_PAD_GPIO5_EN_N, 1}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_2", 0, {PERIPHS_PIN_MUX_3, 8}, {PREG_PAD_GPIO5_EN_N, 2}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_3", 0, {PERIPHS_PIN_MUX_3, 12}, {PREG_PAD_GPIO5_EN_N, 3}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_4", 0, {PERIPHS_PIN_MUX_3, 16}, {PREG_PAD_GPIO5_EN_N, 4}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_5", 0, {PERIPHS_PIN_MUX_3, 20}, {PREG_PAD_GPIO5_EN_N, 5}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_6", 0, {PERIPHS_PIN_MUX_3, 24}, {PREG_PAD_GPIO5_EN_N, 6}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_7", 0, {PERIPHS_PIN_MUX_3, 28}, {PREG_PAD_GPIO5_EN_N, 7}, {PREG_PAD_GPIO5_O, 0}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_8", 0, {PERIPHS_PIN_MUX_4, 0}, {PREG_PAD_GPIO5_EN_N, 8}, {PREG_PAD_GPIO5_O, 8}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_9", 0, {PERIPHS_PIN_MUX_4, 4}, {PREG_PAD_GPIO5_EN_N, 9}, {PREG_PAD_GPIO5_O, 9}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_10", 0, {PERIPHS_PIN_MUX_4, 8}, {PREG_PAD_GPIO5_EN_N, 10}, {PREG_PAD_GPIO5_O, 10}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_11", 0, {PERIPHS_PIN_MUX_4, 12}, {PREG_PAD_GPIO5_EN_N, 11}, {PREG_PAD_GPIO5_O, 11}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_12", 0, {PERIPHS_PIN_MUX_4, 16}, {PREG_PAD_GPIO5_EN_N, 12}, {PREG_PAD_GPIO5_O, 12}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_13", 0, {PERIPHS_PIN_MUX_4, 20}, {PREG_PAD_GPIO5_EN_N, 13}, {PREG_PAD_GPIO5_O, 13}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_14", 0, {PERIPHS_PIN_MUX_4, 24}, {PREG_PAD_GPIO5_EN_N, 14}, {PREG_PAD_GPIO5_O, 14}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_15", 0, {PERIPHS_PIN_MUX_4, 28}, {PREG_PAD_GPIO5_EN_N, 15}, {PREG_PAD_GPIO5_O, 15}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_15", 0, {PERIPHS_PIN_MUX_5, 28}, {PREG_PAD_GPIO5_EN_N, 16}, {PREG_PAD_GPIO5_O, 16}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_15", 0, {PERIPHS_PIN_MUX_5, 28}, {PREG_PAD_GPIO5_EN_N, 17}, {PREG_PAD_GPIO5_O, 17}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_15", 0, {PERIPHS_PIN_MUX_5, 28}, {PREG_PAD_GPIO5_EN_N, 18}, {PREG_PAD_GPIO5_O, 18}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
	{ "GPIOX_15", 0, {PERIPHS_PIN_MUX_5, 28}, {PREG_PAD_GPIO5_EN_N, 19}, {PREG_PAD_GPIO5_O, 19}, FUNCTION_DIGITAL | FUNCTION_INTERRUPT, PINMODE_NOT_SET, 0 },
};

static int amlogicA311dSetup(void) {
	if((amlogicA311d->fd = open("/dev/mem", O_RDWR | O_SYNC )) < 0) {
		wiringXLog(LOG_ERR, "wiringX failed to open /dev/mem for raw memory access");
		return -1;
	}

	if((amlogicA311d->gpio[0] = mmap(0, amlogicA311d->page_size, PROT_READ|PROT_WRITE, MAP_SHARED, amlogicA311d->fd, amlogicA311d->base_addr[0])) == MAP_FAILED) {
		wiringXLog(LOG_ERR, "wiringX failed to map the %s %s GPIO memory address", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}

	if((amlogicA311d->gpio[1] = mmap(0, amlogicA311d->page_size, PROT_READ|PROT_WRITE, MAP_SHARED, amlogicA311d->fd, amlogicA311d->base_addr[1])) == MAP_FAILED) {
		wiringXLog(LOG_ERR, "wiringX failed to map the %s %s GPIO memory address", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}

	return 0;
}

static long amlogicFinalAddress(long baseAddress, long offset) {
	static long finalAddress = 0;
	finalAddress = baseAddress + offset * 4;
	return finalAddress;
}

static char *amlogicA311dGetPinName(int pin) {
	return amlogicA311d->layout[pin].name;
}

static void amlogicA311dSetMap(int *map, size_t size) {
	amlogicA311d->map = map;
	amlogicA311d->map_size = size;
}

static void amlogicA311dSetIRQ(int *irq, size_t size) {
	amlogicA311d->irq = irq;
	amlogicA311d->irq_size = size;
}

static int amlogicA311dDigitalWrite(int i, enum digital_value_t value) {
	struct layout_t *pin = NULL;
	unsigned long data_reg = 0;

	if(amlogicA311d->map == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}
	if(amlogicA311d->fd <= 0 || amlogicA311d->gpio[0] == NULL || amlogicA311d->gpio[1] == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}

	pin = &amlogicA311d->layout[amlogicA311d->map[i]];
	if(pin->mode != PINMODE_OUTPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to output mode", amlogicA311d->brand, amlogicA311d->chip, i);
		return -1;
	}

	if(pin->baseAddrMark == 0) {
		data_reg = (volatile unsigned long *)(amlogicA311d->gpio[0] + pin->direction.offset * 4);
	} else if(pin->baseAddrMark == 1) {
		data_reg = (volatile unsigned long *)(amlogicA311d->gpio[1] + pin->direction.offset * 4);
	} else {
		wiringXLog("invalid baseAddrMark %i, expect 0 or 1", pin->baseAddrMark);
	}

	if(value == HIGH) {
		*data_reg = REGISTER_SET_BITS(data_reg, pin->data.bit);
	} else if(value == LOW) {
		*data_reg = REGISTER_CLEAR_BITS(data_reg, pin->data.bit);
	} else {
		wiringXLog(LOG_ERR, "invalid value %i for GPIO %i", value, i);
		return -1;
	}

	return 0;
}

static int amlogicA311dDigitalRead(int i) {
	struct layout_t *pin = NULL;
	unsigned int *data_reg = NULL;
	uint32_t val = 0;

	if(pin->mode != PINMODE_INPUT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO%d is not set to input mode", amlogicA311d->brand, amlogicA311d->chip, i);
		return -1;
	}

	if(pin->baseAddrMark == 0) {
		data_reg = (volatile unsigned long *)(amlogicA311d->gpio[0] + pin->direction.offset * 4);
	} else if(pin->baseAddrMark == 1) {
		data_reg = (volatile unsigned long *)(amlogicA311d->gpio[1] + pin->direction.offset * 4);
	} else {
		wiringXLog("invalid baseAddrMark %i, expect 0 or 1", pin->baseAddrMark);
	}

	val = *data_reg;

	return (int)((val & (1 << pin->data.bit)) >> pin->data.bit);
}

static int amlogicA311dPinMode(int i, enum pinmode_t mode) {
	struct layout_t *pin = NULL;
	unsigned long pinmux_reg = 0;
	unsigned long dir_reg = 0;
	uint32_t val = 0;

	if(amlogicA311d->map == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}
	if(amlogicA311d->fd <= 0 || amlogicA311d->gpio[0] == NULL || amlogicA311d->gpio[1] == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}

	pin = &amlogicA311d->layout[amlogicA311d->map[i]];
	
	if(pin->baseAddrMark == 0) {
		pinmux_reg = (volatile unsigned long *)(amlogicA311d->gpio[0] + pin->pinmux.offset * 4);
		dir_reg = (volatile unsigned long *)(amlogicA311d->gpio[0] + pin->direction.offset * 4);
	} else if(pin->baseAddrMark == 1) {
		pinmux_reg = (volatile unsigned long *)(amlogicA311d->gpio[1] + pin->pinmux.offset * 4);
		dir_reg = (volatile unsigned long *)(amlogicA311d->gpio[1] + pin->direction.offset * 4);
	} else {
		wiringXLog("invalid baseAddrMark %i, expect 0 or 1", pin->baseAddrMark);
	}
	*pinmux_reg = REGISTER_CLEAR_BITS(pinmux_reg, pin->pinmux.bit);

	if(mode == PINMODE_OUTPUT) {
		dir_reg = REGISTER_SET_BITS(dir_reg, pin->direction.bit);
	} else if(mode == PINMODE_INPUT) {
		REGISTER_CLEAR_BITS(pinmux_reg, pin->direction.bit);
	}

	pin->mode = mode;

	return 0;
}

static int amlogicA311dISR(int i, enum isr_mode_t mode) {
	struct layout_t *pin = NULL;
	char path[PATH_MAX];

	if(amlogicA311d->irq == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}
	if(amlogicA311d->fd <= 0 || amlogicA311d->gpio[0] == NULL || amlogicA311d->gpio[1] == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}

	pin = &amlogicA311d->layout[amlogicA311d->irq[i]];

	sprintf(path, "/sys/class/gpio/gpio%d", amlogicA311d->irq[i]);
	if((soc_sysfs_check_gpio(amlogicA311d, path)) == -1) {
		sprintf(path, "/sys/class/gpio/export");
		if(soc_sysfs_gpio_export(amlogicA311d, path, amlogicA311d->irq[i]) == -1) {
			return -1;
		}
	}

	sprintf(path, "/sys/class/gpio/gpio%d/direction", amlogicA311d->irq[i]);
	if(soc_sysfs_set_gpio_direction(amlogicA311d, path, "in") == -1) {
		return -1;
	}

	sprintf(path, "/sys/class/gpio/gpio%d/edge", amlogicA311d->irq[i]);
	if(soc_sysfs_set_gpio_interrupt_mode(amlogicA311d, path, mode) == -1) {
		return -1;
	}

	sprintf(path, "/sys/class/gpio/gpio%d/value", amlogicA311d->irq[i]);
	if((pin->fd = soc_sysfs_gpio_reset_value(amlogicA311d, path)) == -1) {
		return -1;
	}
	pin->mode = PINMODE_INTERRUPT;

	return 0;
}

static int amlogicA311dWaitForInterrupt(int i, int ms) {
	struct layout_t *pin = &amlogicA311d->layout[amlogicA311d->irq[i]];

	if(pin->mode != PINMODE_INTERRUPT) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d is not set to interrupt mode", amlogicA311d->brand, amlogicA311d->chip, i);
		return -1;
	}
	if(pin->fd <= 0) {
		wiringXLog(LOG_ERR, "The %s %s GPIO %d has not been opened for reading", amlogicA311d->brand, amlogicA311d->chip, i);
		return -1;
	}

	return soc_wait_for_interrupt(amlogicA311d, pin->fd, ms);
}

static int amlogicA311dGC(void) {
	struct layout_t *pin = NULL;
	char path[PATH_MAX];
	int i = 0;

	if(amlogicA311d->map != NULL) {
		for(i=0;i<amlogicA311d->map_size;i++) {
			pin = &amlogicA311d->layout[amlogicA311d->map[i]];
			if(pin->mode == PINMODE_OUTPUT) {
				pinMode(i, PINMODE_INPUT);
			} else if(pin->mode == PINMODE_INTERRUPT) {
				sprintf(path, "/sys/class/gpio/gpio%d", amlogicA311d->irq[i]);
				if((soc_sysfs_check_gpio(amlogicA311d, path)) == 0) {
					sprintf(path, "/sys/class/gpio/unexport");
					soc_sysfs_gpio_unexport(amlogicA311d, path, i);
				}
			}
			if(pin->fd > 0) {
				close(pin->fd);
				pin->fd = 0;
			}
		}
	}
	if(amlogicA311d->gpio[0] != NULL) {
		munmap(amlogicA311d->gpio[0], amlogicA311d->page_size);
	}
	return 0;
}

static int amlogicA311dSelectableFd(int i) {
	struct layout_t *pin = NULL;

	if(amlogicA311d->irq == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been mapped", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}
	if(amlogicA311d->fd <= 0 || amlogicA311d->gpio[0] == NULL || amlogicA311d->gpio[1] == NULL) {
		wiringXLog(LOG_ERR, "The %s %s has not yet been setup by wiringX", amlogicA311d->brand, amlogicA311d->chip);
		return -1;
	}

	pin = &amlogicA311d->layout[amlogicA311d->irq[i]];
	return pin->fd;
}

void amlogicA311dInit(void) {
	soc_register(&amlogicA311d, "Amlogic", "A311D");

	amlogicA311d->layout = layout;

	amlogicA311d->support.isr_modes = ISR_MODE_RISING | ISR_MODE_FALLING | ISR_MODE_BOTH | ISR_MODE_NONE;

	amlogicA311d->page_size = (4*1024);
	amlogicA311d->base_addr[0] = 0xff634400;
	amlogicA311d->base_addr[1] = 0xff800000;

	amlogicA311d->gc = &amlogicA311dGC;
	amlogicA311d->selectableFd = &amlogicA311dSelectableFd;

	amlogicA311d->pinMode = &amlogicA311dPinMode;
	amlogicA311d->setup = &amlogicA311dSetup;
	amlogicA311d->digitalRead = &amlogicA311dDigitalRead;
	amlogicA311d->digitalWrite = &amlogicA311dDigitalWrite;
	amlogicA311d->getPinName = &amlogicA311dGetPinName;
	amlogicA311d->setMap = &amlogicA311dSetMap;
	amlogicA311d->setIRQ = &amlogicA311dSetIRQ;
	amlogicA311d->isr = &amlogicA311dISR;
	amlogicA311d->waitForInterrupt = &amlogicA311dWaitForInterrupt;
}
