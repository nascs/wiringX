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
#include <sys/ioctl.h>
#include <signal.h>

#include "../../soc/soc.h"
#include "../../wiringx.h"
#include "../platform.h"
#include "radxa_zero2.h"

struct platform_t *radxa_zero2 = NULL;

static int map[] = {
	/* GPIOZ_3 GPIOA_1 GPIOZ_4 GPIOZ_5	*/
			430,         477,         431,         432,
	/* GPIOZ_8	GPIOZ_6	GPIOZ_9	GPIOZ_2	*/
			435,         433,         436,         429,
	/* GPIOA_14 GPIOA_15 GPIOH_6		*/
			490,         491,         439,          -1,
	/* GPIOH_4 GPIOH_5 GPIOH_7 GPIOAO_0*/
			447,         449,         450,         412,
	/* GPIOAO_1                        */
			413,          -1,          -1,          -1,
	/*	        GPIOAO_7 GPIOA_4 GPIOAO_8*/
			 -1,         419,         480,         420,
	/* GPIOA_2 GPIOAO_9 GPIOA_0 GPIOA_3 */
			478,         421,         476,         479,
	/* GPIOA_5 GPIOZ_7 GPIOZ_0 GPIOZ_1	*/
			481,         434,         427,         428
};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int radxa_zero2ValidGPIO(int pin) {
	if(pin >= 0 && pin < _sizeof(map)) {
		if(map[pin] == -1) {
			return -1;
		}
		return 0;
	} else {
		return -1;
	}
}

static int radxa_zero2Setup(void) {
	radxa_zero2->soc->setup();
	radxa_zero2->soc->setMap(map, _sizeof(map));
	radxa_zero2->soc->setIRQ(map, _sizeof(map));
	return 0;
}

void radxa_zero2Init(void) {
	platform_register(&radxa_zero2, "radxa_zero2");

	radxa_zero2->soc = soc_get("Rockchip", "RK3399");
	radxa_zero2->soc->setMap(map, _sizeof(map));

	radxa_zero2->digitalRead = radxa_zero2->soc->digitalRead;
	radxa_zero2->digitalWrite = radxa_zero2->soc->digitalWrite;
	radxa_zero2->pinMode = radxa_zero2->soc->pinMode;
	radxa_zero2->setup = &radxa_zero2Setup;

	radxa_zero2->isr = radxa_zero2->soc->isr;
	radxa_zero2->waitForInterrupt = radxa_zero2->soc->waitForInterrupt;

	radxa_zero2->selectableFd = radxa_zero2->soc->selectableFd;
	radxa_zero2->gc = radxa_zero2->soc->gc;

	radxa_zero2->validGPIO = &radxa_zero2ValidGPIO;
}
