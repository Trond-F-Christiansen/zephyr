/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_TYPES_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_TYPES_H_

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

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
 * @brief Type of the proxy agent.
 *
 * This enum defines the types of proxy agents that can be used in a multi-domain
 * Zbus setup. Each type corresponds to a different communication backend.
 */
enum zbus_multidomain_type {
	ZBUS_MULTIDOMAIN_TYPE_UART,
	ZBUS_MULTIDOMAIN_TYPE_IPC
};

/**
 * @brief Message structure for the proxy agent.
 *
 * This structure represents a message that is sent or received by the proxy agent.
 * It contains the size of the message, the actual message data, and the channel name
 * associated with the message.
 */
struct zbus_proxy_agent_msg {
	/* The size of the message */
	size_t message_size;

	/* The channel associated with the message */
	uint8_t message_data[CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE];

	/* The length of the channel name */
	size_t channel_name_len;

	/* The name of the channel */
	char channel_name[CONFIG_ZBUS_MULTIDOMAIN_CHANNEL_NAME_SIZE];
} __packed;

/**
 * @brief Proxy agent API structure.
 *
 */
struct zbus_proxy_agent_api {

	/**
	 * @brief Initialize the backend for the proxy agent.
	 *
	 * This function is called to initialize the backend specific to the proxy agent.
	 *
	 * @param config Pointer to the backend specific configuration.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_init)(void *config);

	/**
	 * @brief Send a message through the proxy agent.
	 *
	 * This function is called to send a message through the proxy agent.
	 *
	 * @param config Pointer to the backend specific configuration.
	 * @param msg Pointer to the message to be sent.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_send)(void *config, struct zbus_proxy_agent_msg *msg);

	/**
	 * @brief Callback function for receiving messages.
	 *
	 * This function is called when a message is received by the backend.
	 *
	 * @param msg Pointer to the received message.
	 * @return int 0 on success, negative error code on failure.
	 */
	int (*backend_msg_recv_cb)(struct zbus_proxy_agent_msg *msg);
} __packed;

/**
 * @brief Configuration structure for the proxy agent.
 *
 * This structure holds the configuration for a proxy agent, including its name,
 * type, backend specific API, and backend specific configuration.
 */
struct zbus_proxy_agent_config {
	/* The name of the proxy agent */
	const char *name;

	/* The type of the proxy agent */
	enum zbus_multidomain_type type;

	/* Pointer to the backend specific API */
	const struct zbus_proxy_agent_api *api;

	/* Pointer to the backend specific configuration */
	void *backend_config;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_TYPES_H_ */
