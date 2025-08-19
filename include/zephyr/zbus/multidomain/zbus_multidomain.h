/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_

#include <zephyr/kernel.h>
#include <string.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/multidomain/zbus_multidomain_types.h>

#include <zephyr/zbus/multidomain/zbus_multidomain_uart.h>

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

/**
 * @brief Set up a proxy agent using the provided configuration.
 *
 * Starts the proxy agent thread and initializes the necessary resources.
 *
 * @param _name The name of the proxy agent.
 * @param _type The type of the proxy agent (enum zbus_multidomain_type)
 * @param _nodelabel The device node label for the proxy agent.
 */
#define ZBUS_PROXY_AGENT_DEFINE(_name, _type, _nodelabel)                                          \
	_ZBUS_GENERATE_BACKEND_CONFIG(_name, _type, _nodelabel);                                   \
	struct zbus_proxy_agent_config _name##_config = {.name = #_name,                           \
							 .type = _type,                            \
							 .api = _ZBUS_GET_API(_type),              \
							 .backend_config =                         \
								 _ZBUS_GET_CONFIG(_name, _type)};  \
	ZBUS_MSG_SUBSCRIBER_DEFINE(_name##_subscriber);                                            \
	K_THREAD_DEFINE(_name##_thread_id, 1024, zbus_proxy_agent_thread, &_name##_config,         \
			&_name##_subscriber, NULL, 7, 0, 0);

/**
 * @brief Add a channel to the proxy agent.
 *
 * @param _name The name of the proxy agent.
 * @param _chan The channel to be added.
 */
#define ZBUS_PROXY_ADD_CHANNEL(_name, _chan) ZBUS_CHAN_ADD_OBS(_chan, _name##_subscriber, 0);

/**
 * @brief Callback function for receiving messages.
 *
 * This function is called when a message is received by the backend.
 *
 * @param msg pointer to the received message.
 * @return int 0 on success, negative error code on failure.
 */
int zbus_proxy_agent_msg_recv_cb(struct zbus_proxy_agent_msg *msg);

/**
 * @brief Thread function for the proxy agent.
 *
 * This function runs in a separate thread and continuously listens for messages
 * on the zbus observer. It processes incoming messages and forwards them
 * to the appropriate backend for sending.
 *
 * @param config Pointer to the configuration structure for the proxy agent.
 * @param subscriber Pointer to the zbus observer that the proxy agent listens to.
 * @return negative error code on failure.
 */
int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber);

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Initialize the proxy agent.
 *
 * This function initializes the proxy agent with the provided configuration
 * and backend specific settings.
 *
 * @param config Pointer to the configuration structure for the proxy agent.
 * @return int 0 on success, negative error code on failure.
 */
int zbus_proxy_agent_init(struct zbus_proxy_agent_config *config);

/**
 * @brief Send a message through the proxy agent.
 *
 * @param config Pointer to the configuration structure for the proxy agent.
 * @param msg pointer to the message to be sent.
 * @return int 0 on success, negative error code on failure.
 */
int zbus_proxy_agent_send(const struct zbus_proxy_agent_config *config,
			  struct zbus_proxy_agent_msg *msg);

/**
 * @brief Macros to generate backend specific configurations for the proxy agent.
 *
 * This macro generates the backend specific configurations based on the type of
 * the proxy agent.
 *
 * @param _name The name of the proxy agent.
 * @param _type The type of the proxy agent (enum zbus_multidomain_type).
 * @param _nodelabel The device node label for the proxy agent.
 *
 * @note This macro finds the matching backend configuration macro from the
 * backend specific header files. Requires the backend specific header files to
 * define the macros in the format `_ZBUS_GENERATE_BACKEND_CONFIG_<type>(_name, _nodelabel)`.
 */
#define _ZBUS_GENERATE_BACKEND_CONFIG(_name, _type, _nodelabel)                                    \
	_ZBUS_GENERATE_BACKEND_CONFIG_##_type(_name, _nodelabel)

/**
 * @brief Generic macros to get the API and configuration for the specified type of proxy agent.
 *
 * These macros are used to retrieve the API and configuration for the specified type of
 * proxy agent. The type is specified as an argument to the macro.
 *
 * @param _type The type of the proxy agent (enum zbus_multidomain_type).
 * @param _name The name of the proxy agent.
 *
 * @note These macros are used to retrieve the API and configuration for the specified type of
 * proxy agent. Requires the backend specific header files to define the macros in the format
 * `_ZBUS_GET_API_<type>()` and `_ZBUS_GET_CONFIG_<name, type>()`.
 */
#define _ZBUS_GET_API(_type)           _ZBUS_GET_API_##_type()
#define _ZBUS_GET_CONFIG(_name, _type) _ZBUS_GET_CONFIG_##_type(_name)

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_ */
