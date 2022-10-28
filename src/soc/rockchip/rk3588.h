/*
	Copyright (c) 2022 Radxa Ltd.
        Author: Nascs <nascs@radxa.com>

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef __WIRINGX_ROCKCHIP3588_H
#define __WIRINGX_ROCKCHIP3588_H

#include "../soc.h"
#include "../../wiringx.h"

extern struct soc_t *rk3588;

void rk3588Init(void);

#endif
