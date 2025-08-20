/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/zbus/multidomain/zbus_multidomain_ipc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_ipc, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

void zbus_multidomain_ipc_bound_cb(void *config)
{
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	k_sem_give(&ipc_config->ept_bound_sem);
	LOG_DBG("IPC endpoint %s bound", ipc_config->ept_cfg->name);
}

void zbus_multidomain_ipc_error_cb(const char *error_msg, void *config)
{
	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;
	LOG_ERR("IPC error: %s on endpoint %s", error_msg, ipc_config->ept_cfg->name);
}

void zbus_multidomain_ipc_recv_cb(const void *data, size_t len, void *config)
{
	if (data == NULL || len == 0) {
		LOG_ERR("Received empty data on IPC endpoint");
		return;
	}

	if (len != sizeof(struct zbus_proxy_agent_msg)) {
		LOG_ERR("Invalid message size: expected %zu, got %zu",
			sizeof(struct zbus_proxy_agent_msg), len);
		return;
	}

	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	struct zbus_proxy_agent_msg *msg = (struct zbus_proxy_agent_msg *)data;

	/* Call the common message reception callback */
	int ret = zbus_proxy_agent_msg_recv_cb(msg);
	if (ret < 0) {
		LOG_ERR("Failed to process received message on IPC endpoint %s: %d",
			ipc_config->ept_cfg->name, ret);
	}
}

int zbus_multidomain_ipc_backend_init(void *config)
{
	int ret;

	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	ret = k_sem_init(&ipc_config->ept_bound_sem, 0, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize IPC endpoint bound semaphore: %d", ret);
		return ret;
	}

	LOG_DBG("Initialized IPC endpoint bound semaphore for %s", ipc_config->ept_cfg->name);

	if (!device_is_ready(ipc_config->dev)) {
		LOG_ERR("IPC device %s is not ready", ipc_config->dev->name);
		return -ENODEV;
	}

	/** Set up IPC endpoint configuration */
	ipc_config->ept_cfg->cb.received = zbus_multidomain_ipc_recv_cb;
	ipc_config->ept_cfg->cb.error = zbus_multidomain_ipc_error_cb;
	ipc_config->ept_cfg->cb.bound = zbus_multidomain_ipc_bound_cb;
	ipc_config->ept_cfg->priv = ipc_config;

	ret = ipc_service_open_instance(ipc_config->dev);
	if (ret < 0) {
		LOG_ERR("Failed to open IPC instance %s: %d", ipc_config->dev->name, ret);
		return ret;
	} else {
		LOG_DBG("Opened IPC instance for device %s", ipc_config->dev->name);
	}

	ret = ipc_service_register_endpoint(ipc_config->dev, &ipc_config->ipc_ept,
					    ipc_config->ept_cfg);
	if (ret < 0) {
		LOG_ERR("Failed to register IPC endpoint %s: %d", ipc_config->ept_cfg->name, ret);
		return ret;
	} else {
		LOG_DBG("Registered IPC endpoint %s", ipc_config->ept_cfg->name);
	}

	ret = k_sem_take(&ipc_config->ept_bound_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to wait for IPC endpoint %s to be bound: %d",
			ipc_config->ept_cfg->name, ret);
		return ret;
	} else {
		LOG_DBG("IPC endpoint %s is bound", ipc_config->ept_cfg->name);
	}

	LOG_DBG("ZBUS Multidomain IPC initialized for device %s with endpoint %s",
		ipc_config->dev->name, ipc_config->ept_cfg->name);

	return 0;
}

int zbus_multidomain_ipc_backend_send(void *config, struct zbus_proxy_agent_msg *msg)
{
	int ret;

	struct zbus_multidomain_ipc_config *ipc_config =
		(struct zbus_multidomain_ipc_config *)config;

	if (msg == NULL || msg->message_size == 0) {
		LOG_ERR("Invalid message to send on IPC endpoint %s", ipc_config->ept_cfg->name);
		return -EINVAL;
	}

	ret = ipc_service_send(&ipc_config->ipc_ept, (void *)msg, sizeof(*msg));
	if (ret < 0) {
		LOG_ERR("Failed to send message on IPC endpoint %s: %d", ipc_config->ept_cfg->name,
			ret);
		return ret;
	}

	LOG_DBG("Sent message of size %zu on IPC endpoint %s", ret, ipc_config->ept_cfg->name);

	return 0;
}

/* Define the IPC backend API */
const struct zbus_proxy_agent_api zbus_multidomain_ipc_api = {
	.backend_init = zbus_multidomain_ipc_backend_init,
	.backend_send = zbus_multidomain_ipc_backend_send};
