/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zbus/zbus_multidomain.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

void zbus_multidomain_wrap_domain_msg(struct generic_domain_msg *msg_wrapper, const char *domain,
				      const void *msg, size_t msg_size, const char *chan_name)
{

	msg_wrapper->domain_size = strlen(domain);
	strncpy(msg_wrapper->domain, domain, CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH - 1);
	msg_wrapper->domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
	if(msg_wrapper->domain_size >= CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH) {
		LOG_WRN("Domain name too long: %zu, got truncated to %s", 
			msg_wrapper->domain_size, msg_wrapper->domain);
	}

	msg_wrapper->msg_size = msg_size;
	if (msg_size > CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH) {
		LOG_ERR("Message size %zu exceeds maximum length %d, aborting",
			msg_size, CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH);
		return;
	}
	memcpy(msg_wrapper->message_data, msg, msg_size);

	msg_wrapper->chan_name_len = strlen(chan_name);
	if(msg_wrapper->chan_name_len >= CONFIG_ZBUS_MULTIDOMAIN_MAX_CHANNEL_NAME_LENGTH) {
		LOG_WRN("Channel name too long: %zu, aborting",
			msg_wrapper->chan_name_len);
		return;
	}

	strncpy(msg_wrapper->chan_name, chan_name, CONFIG_ZBUS_MULTIDOMAIN_MAX_CHANNEL_NAME_LENGTH - 1);
	msg_wrapper->chan_name[CONFIG_ZBUS_MULTIDOMAIN_MAX_CHANNEL_NAME_LENGTH - 1] = '\0';
}