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

ZBUS_CHAN_DEFINE(extra_channel, struct test_data, NULL, NULL, ZBUS_OBSERVERS_EMPTY,
		 ZBUS_MSG_INIT(0));

static void uart_forwarder_listener_cb(const struct zbus_channel *chan)
{
	const struct test_data *data = zbus_chan_const_msg(chan);
	LOG_INF("Received message on channel %s from domain %s: message='%s' (count=%d)",
		chan->name, zbus_chan_msg_domain(chan), data->message, data->count);
}

ZBUS_LISTENER_DEFINE(uart_forwarder_listener, uart_forwarder_listener_cb);
ZBUS_CHAN_ADD_OBS(uart1_channel, uart_forwarder_listener, 0);
ZBUS_CHAN_ADD_OBS(extra_channel, uart_forwarder_listener, 0);

/* Setup forwarders for both channels */
ZBUS_MULTIDOMAIN_DEFINE_FORWARDER_UART(uart1_forwarder, uart1);

/* Normally this would be on another device, but for testing we send it to ourself */
ZBUS_MULTIDOMAIN_DEFINE_FORWARDER_UART(uart2_forwarder, uart2);	

/* Add forwarder observers to the channels */
ZBUS_MULTIDOMAIN_FORWARDER_UART_ADD_CHANNEL(uart1_forwarder, uart1_channel);
ZBUS_MULTIDOMAIN_FORWARDER_UART_ADD_CHANNEL(uart1_forwarder, extra_channel);

void generate_test_string(char *buffer, size_t size)
{
	static int counter = 0;
	int len = snprintf(buffer, size, "Test message %d from %s :", counter++, CONFIG_BOARD_QUALIFIERS);
	
	/* Generate a random string of characters */
	for (int i = len; i < size - 1; i++) {
		buffer[i] = 'A' + (i % 26); // Fill with A-Z characters
	}
	buffer[size - 1] = '\0'; // Null-terminate the string
}

int main(void)
{

	int ret;

	LOG_INF("%s started", CONFIG_BOARD_TARGET);

	struct test_data data = {
		.count = 0,
	};
	snprintf(data.message, sizeof(data.message), "Hello from %s", CONFIG_BOARD_QUALIFIERS);
	// generate_test_string(data.message, sizeof(data.message));

	/* Periodically publish messages to the ZBUS channel */
	while (1) {
		ret = zbus_chan_pub(&uart1_channel, &data, K_MSEC(100));
		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", uart1_channel.name, ret);
		} else {
			LOG_INF("Published on channel %s: count=%d, message='%s'",
				uart1_channel.name, data.count, data.message);
		}
		k_sleep(K_MSEC(10)); // Small delay between messages
		ret = zbus_chan_pub(&extra_channel, &data, K_MSEC(100));
		if (ret < 0) {
			LOG_ERR("Failed to publish on channel %s: %d", extra_channel.name, ret);
		} else {
			LOG_INF("Published on channel %s: count=%d, message='%s'",
				extra_channel.name, data.count, data.message);
		}

		data.count++;
		k_sleep(K_SECONDS(5));
	}
	return 0;
}
