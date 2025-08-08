#include <zephyr/zbus/zbus_multidomain_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_uart, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

void zbus_multidomain_uart_cb(const struct device *dev, struct uart_event *evt, void *config)
{
	int ret;
	struct zbus_multidomain_uart_forwarder_config *conf =
		(struct zbus_multidomain_uart_forwarder_config *)config;
	struct zbus_multidomain_uart_config *uart_config = conf->uart_config;

	switch (evt->type) {
	case UART_RX_RDY:
		LOG_DBG("UART RX ready: length %d bytes", evt->data.rx.len);
		LOG_HEXDUMP_DBG(evt->data.rx.buf, evt->data.rx.len, "RX Buffer");
		if (evt->data.rx.len < sizeof(struct generic_domain_msg)) {
			LOG_DBG("Received data too short for domain message, discarding: %d bytes",
				evt->data.rx.len);
			return;
		}

		struct generic_domain_msg *uart_msg_in =
			(struct generic_domain_msg *)evt->data.rx.buf;
		char received_domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH + 5];

		/* Adding "_uart" suffix to the domain name to differentiate in case of uart between
		 * same domains */
		if (uart_msg_in->domain_size >= CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH) {
			LOG_WRN("Domain name too long: %zu, truncating", uart_msg_in->domain_size);
		}
		snprintf(received_domain, sizeof(received_domain), "%s_uart", uart_msg_in->domain);

		/* Publish the message to the ZBUS channel */
		ret = zbus_chan_pub_domain(conf->chan, uart_msg_in->message_data, received_domain,
					   K_MSEC(CONFIG_ZBUS_MULTIDOMAIN_FORWARDER_ZBUS_TIMEOUT));
		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", conf->chan->name, ret);
		} else {
			LOG_DBG("UART message published on channel %s with domain %s",
				conf->chan->name, received_domain);
		}
		break;
	case UART_RX_BUF_REQUEST:
		LOG_DBG("Providing RX buffer %d", uart_config->async_rx_buffer_idx);
		ret = uart_rx_buf_rsp(
			dev, uart_config->async_rx_buffer[uart_config->async_rx_buffer_idx],
			sizeof(uart_config->async_rx_buffer[0]));
		if (ret < 0) {
			LOG_ERR("Failed to provide RX buffer: %d", ret);
		}
		uart_config->async_rx_buffer_idx = (uart_config->async_rx_buffer_idx + 1) %
						   CONFIG_ZBUS_MULTIDOMAIN_UART_RX_BUFFER_COUNT;
		break;
	case UART_RX_DISABLED:
		LOG_WRN("UART RX disabled, re-enabling");
		ret = uart_rx_enable(dev, uart_config->async_rx_buffer[0],
				     sizeof(uart_config->async_rx_buffer[0]),
				     CONFIG_ZBUS_MULTIDOMAIN_FORWARDER_UART_TIMEOUT *
					     USEC_PER_MSEC);
		if (ret < 0) {
			LOG_ERR("Failed to re-enable RX: %d", ret);
		}
		break;
	default:
		LOG_DBG("Unused UART event: %d", evt->type);
		break;
	}
}

int zbus_multidomain_uart_setup(struct zbus_multidomain_uart_forwarder_config *conf)
{
	int ret;
	const struct device *uart_dev = conf->uart_config->uart_dev;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device %s is not ready", uart_dev->name);
		return -ENODEV;
	}

	ret = uart_callback_set(uart_dev, zbus_multidomain_uart_cb, conf);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	ret = uart_rx_enable(uart_dev, conf->uart_config->async_rx_buffer[0],
			     sizeof(conf->uart_config->async_rx_buffer[0]),
			     CONFIG_ZBUS_MULTIDOMAIN_FORWARDER_UART_TIMEOUT * USEC_PER_MSEC);
	if (ret < 0) {
		LOG_ERR("Failed to enable RX: %d", ret);
		return ret;
	}

	LOG_DBG("UART forwarder initialized for device %s", uart_dev->name);
	return 0;
}

void zbus_multidomain_uart_forwarder_thread(void *config, void *arg1, void *arg2)
{
	struct zbus_multidomain_uart_forwarder_config *conf =
		(struct zbus_multidomain_uart_forwarder_config *)config;
	uint8_t msg_buffer[CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH];
	char origin_domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH];

	LOG_DBG("ZBUS UART forwarder thread started for channel: %s", conf->chan->name);

	int ret = zbus_multidomain_uart_setup(conf);
	if (ret < 0) {
		LOG_ERR("Failed to set up UART forwarder: %d", ret);
		return;
	}

	/* Wait for messages from the ZBUS channel */
	const struct zbus_channel *chan;
	while (!zbus_sub_wait_msg_with_domain(conf->uart_forwarder_sub, &chan, msg_buffer,
					      origin_domain, K_FOREVER)) {
		if (chan != conf->chan) {
			LOG_WRN("Received message from unknown channel: %s", chan->name);
			continue;
		}

		LOG_DBG("UART forwarder: message received from domain: %s", origin_domain);

		/* Skip forwarding messages from other domains to avoid looping */
		if (strcmp(origin_domain, CONFIG_BOARD_QUALIFIERS) != 0) {
			LOG_DBG("Skipping message from domain %s to avoid loop", origin_domain);
			continue;
		}

		/* Wrap the message in a generic domain message structure */
		struct generic_domain_msg uart_msg_out;
		zbus_multidomain_wrap_domain_msg(&uart_msg_out, origin_domain, msg_buffer,
						 zbus_chan_msg_size(chan));

		/* Send the message via UART */
		ret = uart_tx(conf->uart_config->uart_dev, (uint8_t *)&uart_msg_out,
			      sizeof(uart_msg_out), SYS_FOREVER_US);
		if (ret < 0) {
			LOG_ERR("Failed to send message on %s: %d",
				conf->uart_config->uart_dev->name, ret);
		} else {
			LOG_DBG("%s sent: %s (size: %zu)", conf->uart_config->uart_dev->name,
				uart_msg_out.domain, uart_msg_out.msg_size);
		}
	}
}
