#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>

#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/zbus_multidomain_uart.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#define UART_RX_TIMEOUT 10000

struct test_data {
	int count;
	char message[32];
};

/* Outgoing ZBUS channel, connected to UART1 */
ZBUS_CHAN_DEFINE(uart1_channel, struct test_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

/* Incoming ZBUS channel, connected to UART2 */
ZBUS_CHAN_DEFINE(uart2_channel, struct test_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

static void uart_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct test_data *data = zbus_chan_const_msg(chan);
	LOG_INF("Received message on channel %s from domain %s: message='%s' (count=%d)",
		chan->name, zbus_chan_msg_domain(chan), data->message, data->count);
}

ZBUS_LISTENER_DEFINE(uart_forwarder_listener, uart_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(uart2_channel, uart_forwarder_listener, 0);

/* Add UART forwarders for both channels */
ZBUS_MULTIDOMAIN_ADD_FORWARDER_UART(uart1_forwarder, uart1_channel, uart1);
ZBUS_MULTIDOMAIN_ADD_FORWARDER_UART(uart2_forwarder, uart2_channel, uart2);

int main(void)
{

	int ret;

	LOG_INF("%s started", CONFIG_BOARD_TARGET);

	struct test_data data = {
		.count = 0,
	};
	snprintf(data.message, sizeof(data.message), "Hello from %s", CONFIG_BOARD_QUALIFIERS);

	/* Periodically publish messages to the ZBUS channel */
	while (1) {
		ret = zbus_chan_pub(&uart1_channel, &data, K_MSEC(100));
		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", uart1_channel.name, ret);
		} else {
			LOG_INF("Published on channel %s: count=%d, message='%s'",
				uart1_channel.name, data.count, data.message);
		}
		data.count++;
		k_sleep(K_SECONDS(5));
	}
	return 0;
}
