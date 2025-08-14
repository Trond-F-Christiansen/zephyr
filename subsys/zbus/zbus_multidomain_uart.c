#include <zephyr/zbus/zbus_multidomain_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_uart, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

void zbus_multidomain_uart_cb(const struct device *dev, struct uart_event *evt, void *config)
{
	int ret;
	struct zbus_multidomain_uart_config *uart_config = (struct zbus_multidomain_uart_config *)config;

	switch (evt->type) {
	case UART_TX_DONE:
		LOG_DBG("UART TX done");
		k_sem_give(uart_config->tx_busy_sem);
		break;
	case UART_TX_ABORTED:
		LOG_ERR("UART TX aborted");
		k_sem_give(uart_config->tx_busy_sem);
		break;

	case UART_RX_RDY:
		LOG_HEXDUMP_DBG(evt->data.rx.buf, evt->data.rx.len, "RX Buffer");
		if (evt->data.rx.len < sizeof(struct generic_domain_msg)) {
			LOG_DBG("Received data too short for domain message, discarding: %d bytes",
				evt->data.rx.len);
			return;
		}

		struct generic_domain_msg *uart_msg_in =
			(struct generic_domain_msg *)evt->data.rx.buf;
		char received_domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH + 5];

		/* Synchronize uart messages with rx buffers. */ 
		/* TODO: This solution is bad, drops first couple of messages */
		/* basic validation of the received message */
		if (uart_msg_in->domain_size == 0 ||
		    uart_msg_in->domain_size >= CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH ||
			uart_msg_in->msg_size >= CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH ||
			uart_msg_in->chan_name_len == 0 ||
			uart_msg_in->chan_name_len >= CONFIG_ZBUS_MULTIDOMAIN_MAX_CHANNEL_NAME_LENGTH) {
			LOG_ERR("Invalid message received: domain_size=%zu, msg_size=%zu, "
				"chan_name_len=%zu",
				uart_msg_in->domain_size, uart_msg_in->msg_size,
				uart_msg_in->chan_name_len);

           /* Force UART RX reset to restore synchronization */
            ret = uart_rx_disable(dev);
            if (ret < 0) {
                LOG_ERR("Failed to disable RX for reset: %d", ret);
            }
			return;
		}

		/* Adding "_uart" suffix to the domain name to differentiate in case of uart between
		 * same domains */
		if (uart_msg_in->domain_size >= CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH) {
			LOG_WRN("Domain name too long: %zu, truncating", uart_msg_in->domain_size);
		}
		snprintf(received_domain, sizeof(received_domain), "%s_uart", uart_msg_in->domain);

		/* get zbus channel from the channel name  and publish the message */
		const struct zbus_channel *chan =
			zbus_chan_from_name(uart_msg_in->chan_name);
		if (chan == NULL) {
			LOG_WRN("Channel %s not found, cannot publish message", uart_msg_in->chan_name);
			return;
		} else {
		}

		ret = zbus_chan_pub_domain(chan, uart_msg_in->message_data, received_domain,
					   K_MSEC(CONFIG_ZBUS_MULTIDOMAIN_FORWARDER_ZBUS_TIMEOUT));
		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", chan->name, ret);
		} else {
			LOG_DBG("UART message published on channel %s with domain %s",
				chan->name, received_domain);
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
		LOG_DBG("UART RX disabled, re-enabling");
		ret = uart_rx_enable(dev, uart_config->async_rx_buffer[0],
				     sizeof(uart_config->async_rx_buffer[0]),
				     SYS_FOREVER_US);
		if (ret < 0) {
			LOG_ERR("Failed to re-enable RX: %d", ret);
		}
		break;
	default:
		break;
	}
}

int zbus_multidomain_uart_setup(struct zbus_multidomain_uart_config *uart_config)
{
	int ret;
	const struct device *uart_dev = uart_config->uart_dev;

	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device %s is not ready", uart_dev->name);
		return -ENODEV;
	}

	ret = uart_callback_set(uart_dev, zbus_multidomain_uart_cb, uart_config);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	ret = uart_rx_enable(uart_dev, uart_config->async_rx_buffer[0],
			     sizeof(uart_config->async_rx_buffer[0]),
			     SYS_FOREVER_US);
	if (ret < 0) {
		LOG_ERR("Failed to enable RX: %d", ret);
		return ret;
	}

	LOG_DBG("UART forwarder initialized for device %s", uart_dev->name);
	return 0;
}

void zbus_multidomain_uart_forwarder_thread(void *config, void *msg_subscriber, void *arg2)
{

	ARG_UNUSED(arg2);

	if (config == NULL || msg_subscriber == NULL) {
		LOG_ERR("Invalid configuration or subscriber, cannot start forwarder thread");
		return;
	}

	struct zbus_multidomain_uart_config *uart_config = (struct zbus_multidomain_uart_config *)config;
	const struct zbus_observer *subscriber = (const struct zbus_observer *)msg_subscriber;

	uint8_t msg_buffer[CONFIG_ZBUS_MULTIDOMAIN_MAX_MESSAGE_LENGTH];
	char origin_domain[CONFIG_ZBUS_MULTIDOMAIN_MAX_DOMAIN_NAME_LENGTH];

	LOG_DBG("ZBUS UART forwarder thread started");

	int ret = zbus_multidomain_uart_setup(uart_config);
	if (ret < 0) {
		LOG_ERR("Failed to set up UART forwarder: %d", ret);
		return;
	}

	/* Wait for messages from the ZBUS channel */
	const struct zbus_channel *chan;
	while (!zbus_sub_wait_msg_with_domain(subscriber, &chan, msg_buffer,
					      origin_domain, K_FOREVER)) {

		/* Skip forwarding messages from other domains to avoid looping */
		if (strcmp(origin_domain, CONFIG_BOARD_QUALIFIERS) != 0) {
			LOG_DBG("Skipping message from domain %s to avoid loop", origin_domain);
			continue;
		}

		ret = k_sem_take(uart_config->tx_busy_sem, K_FOREVER);
		if (ret < 0) {
			LOG_ERR("Failed to take TX busy semaphore: %d", ret);
			continue;
		}

		/* Wrap the message in a generic domain message structure */
		struct generic_domain_msg uart_msg_out = {0};
		zbus_multidomain_wrap_domain_msg(&uart_msg_out, origin_domain, msg_buffer,
						 zbus_chan_msg_size(chan), zbus_chan_name(chan));

		/* Send the message via UART */
		ret = uart_tx(uart_config->uart_dev, (uint8_t *)&uart_msg_out,
			      sizeof(uart_msg_out), SYS_FOREVER_US);
		if (ret < 0) {
			LOG_ERR("Failed to send message on %s: %d",
				uart_config->uart_dev->name, ret);
		} else {
			LOG_DBG("%s sent: %s (size: %zu)", uart_config->uart_dev->name,
				uart_msg_out.domain, uart_msg_out.msg_size);
		}
	}
}
