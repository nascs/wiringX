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
#include "radxa_e25.h"

struct platform_t *radxa_e25 = NULL;

static int map[] = {
    /*  GPIO3_C4     GPIO3_A3     GPIO3_C5    GPIO3_C1    */
        116,             99,        117,        113,
    /*  GPIO2_D2     GPIO0_C6     GPIO3_A1    GPIO3_B7    */  
         90,             22,         97,        111,
    /*  GPIO1_A0     GPIO1_A1     GPIO4_C6    GPIO3_C0    */
         32,             33,        150,        112,
    /*  GPIO2_D1    GPIO2_D0     GPIO2_D3    GPIO3_C2    */
         89,             88,         91,         114,
    /*  GPIO3_C3                                          */
        115,             -1,         -1,         -1,
         -1,             -1,         -1,         -1,
         -1,             -1,         -1,         -1,
         -1,             -1,         -1,         -1,
};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int radxa_e25ValidGPIO(int pin) {
    if(pin >= 0 && pin < _sizeof(map)) {
        if(map[pin] == -1) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int radxa_e25Setup(void) {
    radxa_e25->soc->setup();
    radxa_e25->soc->setMap(map,_sizeof(map));
    radxa_e25->soc->setIRQ(map,_sizeof(map));
    return 0;
}

void radxa_e25Init(void) {
    platform_register(&radxa_e25,"radxa_e25");

    radxa_e25->soc = soc_get("Rockchip","RK356X");
    radxa_e25->soc->setMap(map,_sizeof(map));

    radxa_e25->digitalRead = radxa_e25->soc->digitalRead;
    radxa_e25->digitalWrite = radxa_e25->soc->digitalWrite;
    radxa_e25->pinMode = radxa_e25->soc->pinMode;
    radxa_e25->setup = &radxa_e25Setup;

    radxa_e25->isr = radxa_e25->soc->isr;
    radxa_e25->waitForInterrupt = radxa_e25->soc->waitForInterrupt;

    radxa_e25->selectableFd = radxa_e25->soc->selectableFd;
    radxa_e25->gc = radxa_e25->soc->gc; 

    radxa_e25->validGPIO = &radxa_e25ValidGPIO;
}
