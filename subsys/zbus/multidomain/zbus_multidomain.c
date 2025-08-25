#include <zephyr/zbus/multidomain/zbus_multidomain.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

int zbus_proxy_agent_set_recv_cb(struct zbus_proxy_agent_config *config,
				int (*recv_cb)(struct zbus_proxy_agent_msg *msg))
{
	if (config == NULL || config->api == NULL || config->backend_config == NULL) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}

	if (config->api->backend_set_recv_cb == NULL) {
		LOG_ERR("Backend set receive callback function is not defined");
		return -ENOSYS;
	}

	int ret = config->api->backend_set_recv_cb(config->backend_config, recv_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set receive callback for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	LOG_DBG("Receive callback set successfully for proxy agent %s", config->name);
	return 0;
}

int zbus_proxy_agent_init(struct zbus_proxy_agent_config *config)
{
	if (config == NULL || config->api == NULL || config->backend_config == NULL) {
		LOG_ERR("Invalid proxy agent configuration");
		return -EINVAL;
	}

	if (config->api->backend_init == NULL) {
		LOG_ERR("Backend init function is not defined");
		return -ENOSYS;
	}

	int ret = config->api->backend_init(config->backend_config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize backend for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	LOG_DBG("Proxy agent %s of type %d initialized successfully", config->name, config->type);
	return 0;
}

int zbus_proxy_agent_send(const struct zbus_proxy_agent_config *config,
			  struct zbus_proxy_agent_msg *msg)
{
	if (config == NULL || config->api == NULL || msg == NULL) {
		LOG_ERR("Invalid parameters for sending message");
		return -EINVAL;
	}

	if (config->api->backend_send == NULL) {
		LOG_ERR("Backend send function is not defined");
		return -ENOSYS;
	}

	int ret = config->api->backend_send((void *)config->backend_config, msg);
	if (ret < 0) {
		LOG_ERR("Failed to send message via proxy agent %s: %d", config->name, ret);
		return ret;
	}

	LOG_DBG("Message sent successfully via proxy agent %s", config->name);
	return 0;
}

int zbus_proxy_agent_msg_recv_cb(struct zbus_proxy_agent_msg *msg)
{
	if (msg == NULL) {
		LOG_ERR("Received NULL message in callback");
		return -EINVAL;
	}

	/* Find corresponding channel by name TODO: Would create less overhead if using ID */
	const struct zbus_channel *chan = zbus_chan_from_name(msg->channel_name);
	if (chan == NULL) {
		LOG_ERR("No channel found for message with name %s", msg->channel_name);
		return -ENOENT;
	}
	if (!chan->is_shadow_channel) {
		LOG_ERR("Channel %s is not a shadow channel, cannot process message", chan->name);
		return -EPERM;
	}

	int ret = zbus_chan_pub_shadow(chan, msg->message_data, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to publish shadow message on channel %s: %d", chan->name, ret);
		return ret;
	}

	LOG_DBG("Published message on shadow channel %s", chan->name);

	return 0;
}

int zbus_proxy_agent_thread(struct zbus_proxy_agent_config *config,
			    const struct zbus_observer *subscriber)
{
	if (config == NULL) {
		LOG_ERR("Invalid proxy agent configuration for thread");
		return -EINVAL;
	}

	LOG_DBG("Starting thread for proxy agent %s", config->name);

	int ret = zbus_proxy_agent_set_recv_cb(config, zbus_proxy_agent_msg_recv_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set receive callback for proxy agent %s: %d", config->name, ret);
		return ret;
	}

	ret = zbus_proxy_agent_init(config);
	if (ret < 0) {
		LOG_ERR("Failed to initialize proxy agent %s: %d", config->name, ret);
		return ret;
	}

	uint8_t message_data[CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE] = {0};

	const struct zbus_channel *chan;
	while (!zbus_sub_wait_msg(subscriber, &chan, &message_data, K_FOREVER)) {

		if (ZBUS_CHANNEL_IS_SHADOW(chan)) {
			LOG_ERR("Forwarding of shadow channel %s, not suported by proxy agent",
				chan->name);
			continue;
		}

		struct zbus_proxy_agent_msg msg = {
			.message_size = chan->message_size,
		};
		memcpy(msg.message_data, message_data, msg.message_size);
		msg.channel_name_len = strlen(chan->name);
		strncpy(msg.channel_name, chan->name, sizeof(msg.channel_name) - 1);
		msg.channel_name[sizeof(msg.channel_name) - 1] = '\0'; // Ensure null termination

		ret = zbus_proxy_agent_send(config, &msg);
		if (ret < 0) {
			LOG_ERR("Failed to send message via proxy agent %s: %d", config->name, ret);
			return ret;
		}
	}
	return 0;
}
