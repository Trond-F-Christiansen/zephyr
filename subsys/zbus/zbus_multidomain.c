/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zbus/zbus_multidomain.h>

void zbus_multidomain_wrap_domain_msg(struct generic_domain_msg *msg_wrapper, const char *domain,
				      const void *msg, size_t msg_size)
{
	__ASSERT(strlen(domain) < CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH,
		 "Domain string is too long, set CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH to "
		 "a larger value");
	__ASSERT(msg_size <= CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH,
		 "Message size exceeds maximum length, set "
		 "CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH to a larger value");

	msg_wrapper->domain_size = strlen(domain);
	strncpy(msg_wrapper->domain, domain, CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH - 1);
	msg_wrapper->domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH - 1] = '\0';
	msg_wrapper->msg_size = msg_size;
	memcpy(msg_wrapper->message_data, msg, msg_size);
}
