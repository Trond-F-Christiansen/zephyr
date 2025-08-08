/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/zbus_multidomain_ipc.h>

#include "common.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Define the ZBUS channel for multidomain communication */
ZBUS_CHAN_DEFINE(ipc_forwarder_channel, struct test_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* Add a listener for logging messages */
static void ipc_forwarder_channel_zbus_listener_cb(const struct zbus_channel *chan)
{
	const struct test_msg *msg = zbus_chan_const_msg(chan);

	LOG_INF("Received message from domain '%s': cnt=%d, str='%s'", zbus_chan_msg_domain(chan),
		msg->count, msg->string);
}
ZBUS_LISTENER_DEFINE(ipc_forwarder_channel_zbus_listener, ipc_forwarder_channel_zbus_listener_cb);
ZBUS_CHAN_ADD_OBS(ipc_forwarder_channel, ipc_forwarder_channel_zbus_listener, 0);

/* Define the IPC forwarder for multidomain communication */
ZBUS_MULTIDOMAIN_ADD_FORWARDER_IPC(ipc_forwarder_cpurad, ipc_forwarder_channel, cpuapp_cpurad_ipc);

int main(void)
{
	int ret = 0;

	LOG_INF("%s started", CONFIG_BOARD_TARGET);

	struct test_msg msg = {.count = 0, .string = "Hello from CPUAPP"};

	/* Periodically publish messages to the ZBUS channel */
	while (true) {
		ret = zbus_chan_pub(&ipc_forwarder_channel, &msg, K_MSEC(100));
		if (ret != 0) {
			LOG_ERR("Failed to publish message: %d", ret);
		}
		msg.count++;
		k_sleep(K_MSEC(5000));
	}
}
