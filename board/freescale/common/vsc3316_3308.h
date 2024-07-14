/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __VSC_CROSSBAR_H_
#define __VSC_CROSSBAR_H	1_

#include <common.h>
#include <i2c.h>
#include <errno.h>

int vsc_if_enable(unsigned int vsc_addr);
int vsc3316_config(unsigned int vsc_addr, const int8_t con_arr[][2],
		unsigned int num_con);
int vsc3308_config(unsigned int vsc_addr, const int8_t con_arr[][2],
		unsigned int num_con);
void vsc_wp_config(unsigned int vsc_addr);

#endif	/* __VSC_CROSSBAR_H_ */
