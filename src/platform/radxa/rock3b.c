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
#include "rock3b.h"

struct platform_t *rock3b = NULL;

static int map[] = {
    /*  GPIO3_C4     GPIO3_A3     GPIO3_C5    GPIO0_C0    */
		116,             99,        117,         16,
    /*  GPIO0_B6     GPIO3_B2     GPIO0_C1    GPIO0_B5    */  
         14,            106,         17,         13,
    /*  GPIO1_A0     GPIO1_A1     GPIO4_C6    GPIO4_D1    */
         32,             33,        150,        153,
    /*  GPIO4_C3     GPIO4_C5     GPIO4_C2    GPIO0_D1    */
        147,            149,        146,         25,
    /*  GPIO0_D0                                          */
         24,             -1,         -1,         -1,
    /*               GPIO2_D7     GPIO3_A0     GPIO3_C3   */
         -1,             95,         96,        115,
    /*  GPIO3_A4                  GPIO3_C2     GPIO3_A2   */
        100,             -1,        114,         98,
    /*   GPIO3_A6     GPIO3_A5     GPIO0_B4      GPIO4_B3  */
        102,            101,         12,         11
};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int rock3bValidGPIO(int pin) {
    if(pin >= 0 && pin < _sizeof(map)) {
        if(map[pin] == -1) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int rock3bSetup(void) {
    rock3b->soc->setup();
    rock3b->soc->setMap(map,_sizeof(map));
    rock3b->soc->setIRQ(map,_sizeof(map));
    return 0;
}

void rock3bInit(void) {
    platform_register(&rock3b,"rock3b");

    rock3b->soc = soc_get("Rockchip","RK356X");
    rock3b->soc->setMap(map,_sizeof(map));

    rock3b->digitalRead = rock3b->soc->digitalRead;
    rock3b->digitalWrite = rock3b->soc->digitalWrite;
    rock3b->pinMode = rock3b->soc->pinMode;
    rock3b->setup = &rock3bSetup;

    rock3b->isr = rock3b->soc->isr;
    rock3b->waitForInterrupt = rock3b->soc->waitForInterrupt;

    rock3b->selectableFd = rock3b->soc->selectableFd;
    rock3b->gc = rock3b->soc->gc; 

    rock3b->validGPIO = &rock3bValidGPIO;
}
