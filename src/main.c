/*
 * Copyright (c) 2023 RSA FG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "dali.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

int main(void)
{
	LOG_INF("starting application");

	while (1)
	{
		LOG_INF("switch off");
		dali_send_data(BROADCAST_C, OFF_C);
		k_sleep(K_SECONDS(1));

		LOG_INF("switch on");
		dali_send_data(BROADCAST_C, ON_C);
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
