/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_

#include <zephyr/kernel.h>
#include <string.h>

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
 * @brief Struct to wrap a message with domain information.
 *
 * This structure is used to wrap a message with its associated domain name and size.
 * It is used for multidomain communication where messages need to be tagged with their
 * originating domain.
 */
struct generic_domain_msg {
	char domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH];
	size_t domain_size;
	size_t msg_size;
	uint8_t message_data[CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH];
};

#define ZBUS_MULTIDOMAIN_WRAPPED_DOMAIN_MSG_SIZE (sizeof(struct generic_domain_msg))

/**
 * @brief Wrap a message with domain information.
 *
 * This function wraps a message with its domain name and size into a
 * `generic_domain_msg` structure, allowing it to be sent as a single structured message
 *
 * @param msg_wrapper pointer to the `generic_domain_msg` structure to fill
 * @param domain pointer to the domain name string
 * @param msg pointer to the message data
 * @param msg_size size of the message data
 *
 * @note The domain must not exceed `CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH`
 *       and the message size must not exceed `CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH`.
 *       The function ensures that the domain string is null-terminated.
 */
void zbus_multidomain_wrap_domain_msg(struct generic_domain_msg *msg_wrapper, const char *domain,
				      const void *msg, size_t msg_size);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_H_ */
