/*
   Copyright (c) 2023 Radxa Ltd.
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
#include "rock5a_v1_2.h"

struct platform_t *rock5a_v1_2 = NULL;

static int map[] = {
    /*  GPIO4_B3    GPIO4_A1    GPIO4_B2    GPIO4_B4  */
        139,		129,		138,		140,
    /*  GPIO1_A5    GPIO1_B0    GPIO1_B5    GPIO1_B3  */
         37,		 40,		 45,		 43,
    /*  GPIO1_D7    GPIO1_D6    GPIO1_A3    GPIO1_A4  */
         63,		 62,		 35,		 36,
	/*  GPIO1_A1     GPIO1_A0    GPIO1_A2   GPIO0_B5  */
         33,		 32,		 34,		 13,
    /*  GPIO0_B6                                      */
         14,		 -1,		 -1,		 -1,
	/*               GPIO1_B2    GPIO1_B1    GPIO1_B4  */
         -1,		 42,		 41,		 44,
	/*  GPIO4_A0     GPIO4_B0      GPIO4_A2     */
        128,		 -1,		136,		130,
    /*  GPIO4_A5   GPIO4_B1    GPIO0_C7    GPIO0_D0  */
        133,		137,		23,		24,

};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int rock5a_v1_2ValidGPIO(int pin) {
    if(pin >= 0 && pin < _sizeof(map)) {
        if(map[pin] == -1) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int rock5a_v1_2Setup(void) {
    rock5a_v1_2->soc->setup();
    rock5a_v1_2->soc->setMap(map,_sizeof(map));
    rock5a_v1_2->soc->setIRQ(map,_sizeof(map));
    return 0;
}

void rock5a_v1_2Init(void) {
    platform_register(&rock5a_v1_2,"rock5a_v1_2");

    rock5a_v1_2->soc = soc_get("Rockchip","RK3588");
    rock5a_v1_2->soc->setMap(map,_sizeof(map));

    rock5a_v1_2->digitalRead = rock5a_v1_2->soc->digitalRead;
    rock5a_v1_2->digitalWrite = rock5a_v1_2->soc->digitalWrite;
    rock5a_v1_2->pinMode = rock5a_v1_2->soc->pinMode;
    rock5a_v1_2->setup = &rock5a_v1_2Setup;

    rock5a_v1_2->isr = rock5a_v1_2->soc->isr;
    rock5a_v1_2->waitForInterrupt = rock5a_v1_2->soc->waitForInterrupt;

    rock5a_v1_2->selectableFd = rock5a_v1_2->soc->selectableFd;
    rock5a_v1_2->gc = rock5a_v1_2->soc->gc; 

    rock5a_v1_2->validGPIO = &rock5a_v1_2ValidGPIO;
}
