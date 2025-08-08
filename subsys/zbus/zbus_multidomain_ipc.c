/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zbus/zbus_multidomain_ipc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_ipc, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

void zbus_multidomain_ipc_bound_cb(void *config)
{
	struct zbus_multidomain_ipc_forwarder_config *conf =
		(struct zbus_multidomain_ipc_forwarder_config *)config;

	LOG_DBG("IPC forwarder bound to endpoint %s", conf->ipc_config->ipc_ept_cfg->name);
	k_sem_give(conf->ipc_config->ipc_bound_sem);
}

void zbus_multidomain_ipc_recv_cb(const void *data, size_t len, void *config)
{
	struct zbus_multidomain_ipc_forwarder_config *conf =
		(struct zbus_multidomain_ipc_forwarder_config *)config;
	const struct generic_domain_msg *ipc_msg_in = (const struct generic_domain_msg *)data;

	LOG_DBG("IPC forwarder received message from domain: %s", ipc_msg_in->domain);

	int ret = zbus_chan_pub_domain(conf->chan, ipc_msg_in->message_data, ipc_msg_in->domain,
				       K_MSEC(CONFIG_ZBUS_MULTIDOMAIN_FORWARDER_ZBUS_TIMEOUT));
	if (ret < 0) {
		LOG_ERR("Failed to publish message to channel %s: %d", conf->chan->name, ret);
		return;
	}
}

void zbus_multidomain_ipc_error_cb(const char *message, void *config)
{
	struct zbus_multidomain_ipc_forwarder_config *conf =
		(struct zbus_multidomain_ipc_forwarder_config *)config;

	LOG_ERR("IPC forwarder error on endpoint %s: %s", conf->ipc_config->ipc_ept_cfg->name,
		message);
}

int zbus_multidomain_ipc_setup(struct zbus_multidomain_ipc_config *ipc_config)
{
	int ret = 0;

	if (!device_is_ready(ipc_config->ipc_instance)) {
		LOG_ERR("IPC instance not ready");
		return -ENODEV;
	}

	ret = ipc_service_open_instance(ipc_config->ipc_instance);
	if (ret < 0 && ret != -EALREADY) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	LOG_DBG("IPC instance %s opened", ipc_config->ipc_instance->name);

	if (!ipc_config->ipc_ept || !ipc_config->ipc_ept_cfg || !ipc_config->ipc_bound_sem) {
		LOG_ERR("Invalid IPC endpoint or configuration");
		return -EINVAL;
	}

	ret = ipc_service_register_endpoint(ipc_config->ipc_instance, ipc_config->ipc_ept,
					    ipc_config->ipc_ept_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

	LOG_DBG("IPC endpoint %s registered on instance %s", ipc_config->ipc_ept_cfg->name,
		ipc_config->ipc_instance->name);

	ret = k_sem_take(ipc_config->ipc_bound_sem,
			 K_MSEC(CONFIG_ZBUS_MULTIDOMAIN_FORWARDER_IPC_TIMEOUT));
	if (ret < 0) {
		LOG_ERR("Failed to take IPC bound semaphore: %d", ret);
		return ret;
	}

	LOG_DBG("IPC-service ZBUS endpoint %s bound", ipc_config->ipc_ept_cfg->name);

	return 0;
}

void zbus_multidomain_ipc_forwarder_thread(void *config, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);

	int ret = 0;

	struct zbus_multidomain_ipc_forwarder_config *conf =
		(struct zbus_multidomain_ipc_forwarder_config *)config;
	uint8_t msg_buffer[CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH];
	char origin_domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH];

	LOG_DBG("ZBUS IPC forwarder thread started for channel: %s", conf->chan->name);

	conf->ipc_config->ipc_ept_cfg->cb.bound = zbus_multidomain_ipc_bound_cb;
	conf->ipc_config->ipc_ept_cfg->cb.received = zbus_multidomain_ipc_recv_cb;
	conf->ipc_config->ipc_ept_cfg->cb.error = zbus_multidomain_ipc_error_cb;
	conf->ipc_config->ipc_ept_cfg->priv = conf;

	ret = zbus_multidomain_ipc_setup(conf->ipc_config);
	if (ret < 0) {
		LOG_ERR("Failed to set up IPC endpoint: %d", ret);
		return;
	}

	const struct zbus_channel *chan;

	while (!zbus_sub_wait_msg_with_domain(conf->ipc_forwarder_sub, &chan, msg_buffer,
					      origin_domain, K_FOREVER))
	{
		if (chan != conf->chan) {
			LOG_WRN("Received message from unknown channel: %s", chan->name);
			continue;
		}

		LOG_DBG("IPC forwarder: Message received from domain: %s", origin_domain);

		/* Skip forwarding messages from other domains to avoid looping */
		if (strcmp(origin_domain, CONFIG_BOARD_QUALIFIERS) != 0) {
			LOG_DBG("Skipping forwarding for message from other domain. origin: %s",
				origin_domain);
			continue;
		}

		struct generic_domain_msg ipc_msg_out;
        
		zbus_multidomain_wrap_domain_msg(&ipc_msg_out, origin_domain, msg_buffer,
						 zbus_chan_msg_size(chan));

		ret = ipc_service_send(conf->ipc_config->ipc_ept, &ipc_msg_out,
				       sizeof(ipc_msg_out));
		if (ret < 0) {
			LOG_ERR("Failed to send message via IPC: %d", ret);
			continue;
		}
		LOG_DBG("Sent %d bytes to IPC endpoint %s", ret,
			conf->ipc_config->ipc_ept->instance->name);
	}
}
