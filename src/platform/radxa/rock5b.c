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
#include "rock5b.h"

struct platform_t *rock5b = NULL;

static int map[] = {
    /*  GPIO3_C1    GPIO3_B5    GPIO3_B7    GPIO3_C0  */
        113,		109,		111,		112,
    /*  GPIO3_A4    GPIO4_C4                GPIO3_C3  */
        100,		128,		 -1,		115,
    /*  GPIO4_B3    GPIO4_B2    GPIO1_B4    GPIO1_B5  */
        139,		138,		 44,		 45,
	/*  GPIO1_B2    GPIO1_B1    GPIO1_B3    GPIO0_B5  */
         42,		 41,		 43,		 13,
    /*  GPIO0_B6                                      */
         14,		 -1,		 -1,		 -1,
	/*              GPIO1_D7    GPIO1_B7    GPIO3_A7  */
         -1,		 63,		 47,		103,
	/*  GPIO3_B6    GPIO0_A0    GPIO3_C3    GPIO3_B1  */
        110,		  0,		111,		105,
    /*  GPIO3_B2    GPIO3_B3    GPIO4_C6    GPIO4_C5  */
        106,		107,		150,		149,

};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int rock5bValidGPIO(int pin) {
    if(pin >= 0 && pin < _sizeof(map)) {
        if(map[pin] == -1) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int rock5bSetup(void) {
    rock5b->soc->setup();
    rock5b->soc->setMap(map,_sizeof(map));
    rock5b->soc->setIRQ(map,_sizeof(map));
    return 0;
}

void rock5bInit(void) {
    platform_register(&rock5b,"rock5b");

    rock5b->soc = soc_get("Rockchip","RK3588");
    rock5b->soc->setMap(map,_sizeof(map));

    rock5b->digitalRead = rock5b->soc->digitalRead;
    rock5b->digitalWrite = rock5b->soc->digitalWrite;
    rock5b->pinMode = rock5b->soc->pinMode;
    rock5b->setup = &rock5bSetup;

    rock5b->isr = rock5b->soc->isr;
    rock5b->waitForInterrupt = rock5b->soc->waitForInterrupt;

    rock5b->selectableFd = rock5b->soc->selectableFd;
    rock5b->gc = rock5b->soc->gc; 

    rock5b->validGPIO = &rock5bValidGPIO;
}
