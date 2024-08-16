/*
  Copyright (c) 2024 Radxa Ltd.
  Author: Nascs <nascs@radxa.com>
          ZHANG Yuntian <yt@radxa.com>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "rock2.h"

struct platform_t *rock2 = NULL;

static int map[] = {
	/*	GPIO4_B7	GPIO1_B5    GPIO4_C0    GPIO4_C6	*/
			92,			19,			93,			99,
	/*	GPIO4_B0	GPIO4_B1	GPIO1_B2	GPIO4_A6	*/
			85,			86,			87,			83,
	/*	GPIO4_A0	GPIO4_A1	GPIO4_B6	GPIO4_C1	*/
			77,			78,			91,			94,
	/*	GPIO4_B2	GPIO4_B3	GPIO4_B4	GPIO4_D0	*/
			87,			88,			89,			101,
	/*	GPIO4_C7										*/
			100,		-1,			-1,			-1,
	/*	-1			GPIO4_B5	GPIO1_B0	GPIO1_B1	*/
			-1,			90,			14,			15,
	/*	GPIO1_B6	-1			GPIO4_C3	GPIO1_B4	*/
			20,			-1,			96,			18,
	/*	GPIO1_B7	GPIO1_C0	GPIO4_A2	GPIO4_A3	*/
			21,			22,			79,			80,
};

static int rock2ValidGPIO(int pin) {
	return radxaValidGPIO(pin, map, _sizeof(map));
}

static int rock2Setup(void) {
	return radxaSetup(rock2, map, _sizeof(map));
}

void rock2Init(void) {
	platform_register(&rock2, "rock2");

	rock2->soc = soc_get("Rockchip", "RK3528");
	rock2->soc->setMap(map, _sizeof(map));

	radxaInit(rock2);
	rock2->setup = &rock2Setup;

	rock2->validGPIO = &rock2ValidGPIO;
}
