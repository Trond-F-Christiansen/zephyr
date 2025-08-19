#include <zephyr/zbus/multidomain/zbus_multidomain_uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(zbus_multidomain_uart, CONFIG_ZBUS_MULTIDOMAIN_LOG_LEVEL);

void zbus_multidomain_uart_backend_cb(const struct device *dev, struct uart_event *evt,
				      void *config)
{
	int ret;
	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&uart_config->tx_busy_sem);
		break;

	case UART_TX_ABORTED:
		LOG_ERR("UART TX aborted");
		k_sem_give(&uart_config->tx_busy_sem);
		break;

	case UART_RX_RDY:

		struct zbus_proxy_agent_msg *msg = (struct zbus_proxy_agent_msg *)evt->data.rx.buf;

		/* Synchronize uart message with rx buffer */
		if (msg->message_size == 0 ||
		    msg->message_size > CONFIG_ZBUS_MULTIDOMAIN_MESSAGE_SIZE) {
			LOG_ERR("Invalid message size: %zu", msg->message_size);

			/* Force uart reset to dump buffer with possible overflow */
			ret = uart_rx_disable(dev);
			if (ret < 0) {
				LOG_ERR("Failed to disable RX for reset: %d", ret);
			}
			return;
		}

		ret = zbus_proxy_agent_msg_recv_cb(msg);
		if (ret < 0) {
			LOG_ERR("Failed to process received message: %d", ret);
			return;
		}

		break;

	case UART_RX_BUF_REQUEST:
		ret = uart_rx_buf_rsp(dev, uart_config->async_rx_buf[uart_config->async_rx_buf_idx],
				      sizeof(uart_config->async_rx_buf[0]));
		if (ret < 0) {
			LOG_ERR("Failed to provide RX buffer: %d", ret);
		} else {
			uart_config->async_rx_buf_idx = (uart_config->async_rx_buf_idx + 1) %
							CONFIG_ZBUS_MULTIDOMAIN_UART_BUF_COUNT;
			LOG_DBG("Provided RX buffer %d", uart_config->async_rx_buf_idx);
		}
		break;

	case UART_RX_BUF_RELEASED:
		break;

	case UART_RX_DISABLED:
		LOG_WRN("UART RX disabled, re-enabling");
		ret = uart_rx_enable(dev, uart_config->async_rx_buf[uart_config->async_rx_buf_idx],
				     sizeof(uart_config->async_rx_buf[0]), SYS_FOREVER_US);
		if (ret < 0) {
			LOG_ERR("Failed to re-enable UART RX: %d", ret);
		}
		break;

	default:
		LOG_DBG("Unhandled UART event: %d", evt->type);
		break;
	}
}

int zbus_multidomain_uart_backend_init(void *config)
{
	int ret;

	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	if (!device_is_ready(uart_config->dev)) {
		LOG_ERR("Device %s is not ready", uart_config->dev->name);
		return -ENODEV;
	}

	ret = uart_callback_set(uart_config->dev, zbus_multidomain_uart_backend_cb, uart_config);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback: %d", ret);
		return ret;
	}

	ret = uart_rx_enable(uart_config->dev, uart_config->async_rx_buf[0],
			     sizeof(uart_config->async_rx_buf[0]), SYS_FOREVER_US);
	if (ret < 0) {
		LOG_ERR("Failed to enable UART RX: %d", ret);
		return ret;
	}

	ret = k_sem_init(&uart_config->tx_busy_sem, 1, 1);
	if (ret < 0) {
		LOG_ERR("Failed to initialize TX busy semaphore: %d", ret);
		return ret;
	}

	LOG_DBG("ZBUS Multidomain UART initialized for device %s", uart_config->dev->name);

	return 0;
}

int zbus_multidomain_uart_backend_send(void *config, struct zbus_proxy_agent_msg *msg)
{
	int ret;

	struct zbus_multidomain_uart_config *uart_config =
		(struct zbus_multidomain_uart_config *)config;

	ret = k_sem_take(&uart_config->tx_busy_sem, K_FOREVER);
	if (ret < 0) {
		LOG_ERR("Failed to take TX busy semaphore: %d", ret);
		return ret;
	}

	/* Create a copy of the message to hand to uart tx, as the original message may be modified
	 */
	static struct zbus_proxy_agent_msg tx_msg = {0};
	memcpy(&tx_msg, msg, sizeof(tx_msg));

	ret = uart_tx(uart_config->dev, (uint8_t *)&tx_msg, sizeof(tx_msg), SYS_FOREVER_US);
	if (ret < 0) {
		LOG_ERR("Failed to send message via UART: %d", ret);
		k_sem_give(&uart_config->tx_busy_sem);
		return ret;
	}

	LOG_DBG("Sent message of size %d via UART", tx_msg.message_size);

	return 0;
}

/* Define the UART backend API */
const struct zbus_proxy_agent_api zbus_multidomain_uart_api = {
	.backend_init = zbus_multidomain_uart_backend_init,
	.backend_send = zbus_multidomain_uart_backend_send,
};
