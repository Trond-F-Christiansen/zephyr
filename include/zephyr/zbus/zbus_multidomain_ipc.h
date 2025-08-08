/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_IPC_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_IPC_H_

#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/zbus_multidomain.h>
#include <zephyr/ipc/ipc_service.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus API
 * @defgroup zbus_apis Zbus APIs
 * @since 3.3.0
 * @version 1.0.0
 * @ingroup os_services
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Configuration structure for IPC service
 *
 * This structure holds the configuration parameters for setting up IPC service
 * It includes the IPC instance, endpoint, and configuration details.
 * It also includes a semaphore to signal when the IPC endpoint is bound.
 *
 */
struct zbus_multidomain_ipc_config {
	const struct device *ipc_instance;
	struct ipc_ept *ipc_ept;
	struct ipc_ept_cfg *ipc_ept_cfg;
	struct k_sem *ipc_bound_sem;
};

/**
 * @brief Configuration structure for IPC forwarder
 *
 * This structure holds the configuration parameters for the IPC forwarder.
 * It includes the channel to forward messages from, the IPC forwarder subscriber,
 * the IPC configuration.
 */
struct zbus_multidomain_ipc_forwarder_config {
	const struct zbus_channel *chan;
	const struct zbus_observer *ipc_forwarder_sub;
	struct zbus_multidomain_ipc_config *ipc_config;
};

/**
 * @brief Thread function for IPC forwarder
 *
 * IPC forwarder main thread that forwards messages from a ZBUS channel to an IPC endpoint.
 * It sets up the IPC endpoint with the provided configuration and waits for messages from the ZBUS
 * channel. When a message is received, it wraps the message in a generic domain message structure
 * and sends it via IPC.
 *
 * @param config Configuration for the IPC forwarder
 * @param arg1 Unused argument
 * @param arg2 Unused argument
 */
void zbus_multidomain_ipc_forwarder_thread(void *config, void *arg1, void *arg2);

/**
 * @brief macro to set up IPC configuration
 *
 * This macro sets up the IPC configuration for a given IPC endpoint.
 *
 * @param _name Name of the IPC endpoint
 * @param _chan IPC channel for forwarder to listen to
 * @param _zbus_subscriber ZBUS subscriber for the IPC forwarder
 * @param _ipc_dt_nodelabel Device tree label for the IPC instance
 */
#define _ZBUS_MULTIDOMAIN_IPC_CONF_SETUP(_name, _chan, _zbus_subscriber, _ipc_dt_nodelabel)        \
	static K_SEM_DEFINE(_name##_ipc_ept_bound_sem, 0, 1);                                      \
	static struct ipc_ept_cfg _name##_ipc_ept_cfg = {.name = "ipc_zbus_" #_name "_ept"};       \
	struct ipc_ept _name##_ipc_ept;                                                            \
	static struct zbus_multidomain_ipc_config _name##_ipc_config = {                           \
		.ipc_instance = DEVICE_DT_GET(DT_NODELABEL(_ipc_dt_nodelabel)),                    \
		.ipc_ept = &_name##_ipc_ept,                                                       \
		.ipc_ept_cfg = &_name##_ipc_ept_cfg,                                               \
		.ipc_bound_sem = &_name##_ipc_ept_bound_sem,                                       \
	};                                                                                         \
	struct zbus_multidomain_ipc_forwarder_config _name##_ipc_forwarder_config = {              \
		.chan = &_chan,                                                                    \
		.ipc_forwarder_sub = &_zbus_subscriber,                                            \
		.ipc_config = &_name##_ipc_config,                                                 \
	};

/** @endcond */

/**
 * @brief Macro to add an IPC forwarder to a ZBUS channel
 *
 * This macro sets up an IPC forwarder that listens to a ZBUS channel and forwards messages to an
 * IPC endpoint. It defines the necessary callbacks and configurations for the IPC endpoint and
 * starts a thread for the IPC forwarder. By adding forwarders with the same name and the
 * corresponding ipc_dt_nodelabel on two cores, messages will be forwarded between the two cores on
 * the given zbus channels.
 *
 * @note: Each forwarder must have a unique name, but they can share the same IPC endpoint and zbus
 * channel.
 *
 * @param _name Name of the IPC forwarder
 * @param _chan ZBUS channel to forward messages from
 * @param _ipc_dt_nodelabel Device tree label for the IPC instance
 */
#define ZBUS_MULTIDOMAIN_ADD_FORWARDER_IPC(_name, _chan, _ipc_dt_nodelabel)                        \
	ZBUS_MSG_SUBSCRIBER_DEFINE(_name##_forwarder);                                             \
	ZBUS_CHAN_ADD_OBS(_chan, _name##_forwarder, 1);                                            \
	_ZBUS_MULTIDOMAIN_IPC_CONF_SETUP(_name, _chan, _name##_forwarder, _ipc_dt_nodelabel);      \
	K_THREAD_DEFINE(_name##_forwarder_thread_id,                                               \
			CONFIG_ZBUS_MULTIDOMAIN_IPC_FORWARDER_THREAD_STACK_SIZE,                   \
			zbus_multidomain_ipc_forwarder_thread, &_name##_ipc_forwarder_config,      \
			NULL, NULL, CONFIG_ZBUS_MULTIDOMAIN_IPC_FORWARDER_THREAD_PRIORITY, 0, 0);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_IPC_H_ */
