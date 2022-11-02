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
#include "radxa_e23.h"

struct platform_t *radxa_e23 = NULL;

static int map[] = {
    /*  GPIO3_A1     GPIO3_B4     GPIO0_C4    GPIO0_C6    */
         97,            108,         20,         22,
    /*                            GPIO2_C6    GPIO0_C5    */  
         -1,             -1,         86,         21,
    /*  GPIO1_A0     GPIO1_A1     GPIO4_A6    GPIO4_A7    */
         32,             33,        134,        135,
    /*  GPIO4_B2     GPIO4_B0     GPIO4_B3    GPIO0_D1    */
        138,            136,        139,         25,
    /*  GPIO0_D0                                          */
         24,             -1,         -1,         -1,
    /*               GPIO3_C1     GPIO3_C2     GPIO0_B7   */
         -1,            113,        114,         15,
    /*  GPIO0_D5     GPIO0_D6     GPIO0_C3     GPIO0_B4   */
         29,             30,         19,         12,
    /*  GPIO0_C1     GPIO3_B3     GPIO0_B6      GPIO4_B3  */
         17,             11,         14,         13
};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int radxa_e23ValidGPIO(int pin) {
    if(pin >= 0 && pin < _sizeof(map)) {
        if(map[pin] == -1) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int radxa_e23Setup(void) {
    radxa_e23->soc->setup();
    radxa_e23->soc->setMap(map,_sizeof(map));
    radxa_e23->soc->setIRQ(map,_sizeof(map));
    return 0;
}

void radxa_e23Init(void) {
    platform_register(&radxa_e23,"radxa_e23");

    radxa_e23->soc = soc_get("Rockchip","RK356X");
    radxa_e23->soc->setMap(map,_sizeof(map));

    radxa_e23->digitalRead = radxa_e23->soc->digitalRead;
    radxa_e23->digitalWrite = radxa_e23->soc->digitalWrite;
    radxa_e23->pinMode = radxa_e23->soc->pinMode;
    radxa_e23->setup = &radxa_e23Setup;

    radxa_e23->isr = radxa_e23->soc->isr;
    radxa_e23->waitForInterrupt = radxa_e23->soc->waitForInterrupt;

    radxa_e23->selectableFd = radxa_e23->soc->selectableFd;
    radxa_e23->gc = radxa_e23->soc->gc; 

    radxa_e23->validGPIO = &radxa_e23ValidGPIO;
}
