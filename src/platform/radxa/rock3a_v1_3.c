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
#include "rock3a_v1_3.h"

struct platform_t *rock3a_v1_3 = NULL;

static int map[] = {
    /*  GPIO3_C4     GPIO3_A3     GPIO3_C5    GPIO0_C0    */
		116,			 99,		117,		 16,
    /*  GPIO0_B6     GPIO3_B2     GPIO0_C1    GPIO0_B5    */  
		 14,			106,		 17,         13,
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
    /*   GPIO3_A6     GPIO3_A5     GPIO0_B4      GPIO0_B3  */
        102,            101,         12,         11
};

#define _sizeof(arr) (sizeof(arr) / sizeof(arr[0]))

static int rock3a_v1_3_ValidGPIO(int pin) {
    if(pin >= 0 && pin < _sizeof(map)) {
        if(map[pin] == -1) {
            return -1;
        }
        return 0;
    } else {
        return -1;
    }
}

static int rock3a_v1_3_Setup(void) {
    rock3a_v1_3->soc->setup();
    rock3a_v1_3->soc->setMap(map,_sizeof(map));
    rock3a_v1_3->soc->setIRQ(map,_sizeof(map));
    return 0;
}

void rock3a_v1_3_init(void) {
    platform_register(&rock3a_v1_3,"rock3a_v1_3");

    rock3a_v1_3->soc = soc_get("Rockchip","RK356X");
    rock3a_v1_3->soc->setMap(map,_sizeof(map));

    rock3a_v1_3->digitalRead = rock3a_v1_3->soc->digitalRead;
    rock3a_v1_3->digitalWrite = rock3a_v1_3->soc->digitalWrite;
    rock3a_v1_3->pinMode = rock3a_v1_3->soc->pinMode;
    rock3a_v1_3->setup = &rock3a_v1_3_Setup;

    rock3a_v1_3->isr = rock3a_v1_3->soc->isr;
    rock3a_v1_3->waitForInterrupt = rock3a_v1_3->soc->waitForInterrupt;

    rock3a_v1_3->selectableFd = rock3a_v1_3->soc->selectableFd;
    rock3a_v1_3->gc = rock3a_v1_3->soc->gc; 

    rock3a_v1_3->validGPIO = &rock3a_v1_3_ValidGPIO;
}
