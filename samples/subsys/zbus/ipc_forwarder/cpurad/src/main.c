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
ZBUS_CHAN_DEFINE(ipc_forwarder_remote_channel, struct test_msg, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* Define the IPC forwarder for multidomain communication */
ZBUS_MULTIDOMAIN_ADD_FORWARDER_IPC(ipc_forwarder_cpurad, ipc_forwarder_remote_channel,
				   cpuapp_cpurad_ipc);

/* Define the subscriber for echoing messages */
ZBUS_SUBSCRIBER_DEFINE(zbus_echo_subscriber, 2);
ZBUS_CHAN_ADD_OBS(ipc_forwarder_remote_channel, zbus_echo_subscriber, 0);

static void zbus_echo_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_INF("Echo thread started for channel: %s", ipc_forwarder_remote_channel.name);

	const struct zbus_channel *chan;
	struct test_msg msg;

	while (!zbus_sub_wait(&zbus_echo_subscriber, &chan, K_FOREVER)) {
		if (&ipc_forwarder_remote_channel == chan) {
			int ret = zbus_chan_read(chan, &msg, K_MSEC(100));
			if (ret != 0) {
				LOG_ERR("Failed to read from channel: %d", ret);
				continue;
			}

			/* For echoing it is necessary to ignore messages originating from this
			 * domain to avoid infinitely echoing the same message back to itself.
			 */
			if (strcmp(zbus_chan_msg_domain(chan), CONFIG_BOARD_QUALIFIERS) == 0) {
				LOG_DBG("Ignoring message from own domain");
				continue;
			}

			LOG_INF("Echoing message from domain '%s': cnt=%d, str='%s'",
				zbus_chan_msg_domain(chan), msg.count, msg.string);

			/* Echo the message back to the ZBUS channel */
			ret = zbus_chan_pub(&ipc_forwarder_remote_channel, &msg, K_MSEC(100));
			if (ret < 0) {
				LOG_ERR("Failed to publish echo message: %d", ret);
			}
		} else {
			LOG_WRN("Received message from unknown channel");
		}
	}
}
K_THREAD_DEFINE(zbus_echo_thread_id, CONFIG_MAIN_STACK_SIZE, zbus_echo_thread, NULL, NULL, NULL,
		CONFIG_MAIN_THREAD_PRIORITY, 0, 0);

int main(void)
{
	LOG_INF("%s started", CONFIG_BOARD_TARGET);
}
