/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _MULTICORE_ZBUS_COMMON_H_
#define _MULTICORE_ZBUS_COMMON_H_

/* Shared struct definitions between cores. */
struct test_msg {
	uint8_t count;
	char string[32];
};

#endif /* _MULTICORE_ZBUS_COMMON_H_ */
