#ifndef ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_
#define ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_

#include <zephyr/zbus/zbus.h>
#include <zephyr/zbus/zbus_multidomain.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zbus API
 * @defgroup zbus_apis Zbus APIs
 * @since 3.3.0
 * @version 1.0.0
 * @ingroup os_services
 * @{
 */

/** @cond INTERNAL_HIDDEN */
/* Hidden internal structures and functions for the multidomain ZBUS UART forwarder */

/**
 * @brief Configuration structure for uart specific configurations.
 *
 * This structure holds the configuration parameters for the UART device
 * used in the multidomain ZBUS forwarding.
 *
 */
struct zbus_multidomain_uart_config {
	/** UART device instance. */
	const struct device *uart_dev;

	/** Asynchronous RX buffers for receiving messages. */
	uint8_t async_rx_buffer[CONFIG_ZBUS_MULTIDOMAIN_UART_RX_BUFFER_COUNT]
			       [ZBUS_MULTIDOMAIN_WRAPPED_DOMAIN_MSG_SIZE];

	/** Index for the next RX buffer to use. */
	volatile uint8_t async_rx_buffer_idx;
};

/**
 * @brief Configuration structure for UART forwarder.
 *
 * This structure holds the configuration parameters for the UART forwarder,
 * including the channel to forward messages from, the channel observer,
 * the UART configuration.
 *
 */
struct zbus_multidomain_uart_forwarder_config {
	/** Channel to forward messages from. */
	const struct zbus_channel *chan;

	/** Channel observer for the UART forwarder. Should be msg subscriber. */
	const struct zbus_observer *uart_forwarder_sub;

	/** UART configuration for the forwarder. */
	struct zbus_multidomain_uart_config *uart_config;
};

/**
 * @brief UART event callback function.
 *
 * This function is called when a UART event occurs.
 * It handles the necessary RX events.
 *
 * - UART_RX_RDY: The event indicates that data has been received and is ready to be processed.
 *      It retrieves the received data, unwraps it from the generic domain message structure,
 *      and publishes it to the ZBUS channel with the domain information.
 *      note: The domain had "_uart" suffix appended to it to differentiate in case of uart
 *      between two devices with the same domain name.
 *
 * - UART_RX_BUF_REQUEST: The event indicates that the UART driver is requesting a new RX buffer.
 *     It provides the next RX buffer in the circular buffer for receiving more data.
 *
 * - UART_RX_DISABLED: The event indicates that the UART RX has been disabled.
 *    It re-enables the RX to continue receiving data.
 *
 * @param dev The UART device instance that generated the event.
 * @param evt The UART event that occurred.
 * @param config Pointer to the forwarder configuration structure allowing single callback
 *                     to be used for multiple forwarders.
 */
void zbus_multidomain_uart_cb(const struct device *dev, struct uart_event *evt, void *config);

/**
 * @brief Thread function for the UART forwarder.
 *
 * This function initializes the UART forwarder, sets up the UART device,
 * and waits for messages from the ZBUS channel. When a message is received,
 * it wraps the message in a generic domain message structure and sends it via UART.
 *
 * @param config Pointer to the UART forwarder configuration structure.
 * @param arg1 Unused argument.
 * @param arg2 Unused argument.
 */
void zbus_multidomain_uart_forwarder_thread(void *config, void *arg1, void *arg2);

/**
 * @brief Macro to set up configuration structures for UART forwarder.
 *
 * This macro initializes the UART configuration structure and the UART forwarder configuration
 * structure with the provided parameters.
 *
 * @param _name Name of the UART forwarder
 * @param _chan ZBUS channel to forward messages from
 * @param _zbus_subscriber ZBUS subscriber for the UART forwarder
 * @param _uart_dev UART device instance to use for forwarding
 */
#define _ZBUS_MULTIDOMAIN_UART_CONF_SETUP(_name, _chan, _zbus_subscriber, _uart_dev)               \
	static struct zbus_multidomain_uart_config _name##_uart_config = {                         \
		.uart_dev = _uart_dev,                                                             \
		.async_rx_buffer = {{0}},                                                          \
		.async_rx_buffer_idx = 0,                                                          \
	};                                                                                         \
	struct zbus_multidomain_uart_forwarder_config _name##_uart_forwarder_config = {            \
		.chan = &_chan,                                                                    \
		.uart_forwarder_sub = &_zbus_subscriber,                                           \
		.uart_config = &_name##_uart_config,                                               \
	};

/** @endcond */

/**
 * @brief Macro to add a UART forwarder to a ZBUS channel.
 *
 * This macro sets up a UART forwarder that listens to a ZBUS channel and forwards messages
 * to the specified UART device. It defines the necessary configurations, adds a message subscriber
 * to the ZBUS channel, and starts a thread for the UART forwarder.
 *
 * @param _name Name of the UART forwarder
 * @param _zbus_chan ZBUS channel to forward messages from
 * @param _uart_dt_nodelabel Device tree label for the UART instance
 */
#define ZBUS_MULTIDOMAIN_ADD_FORWARDER_UART(_name, _zbus_chan, _uart_dt_nodelabel)                 \
	ZBUS_MSG_SUBSCRIBER_DEFINE(_name##_forwarder);                                             \
	ZBUS_CHAN_ADD_OBS(_zbus_chan, _name##_forwarder, 1);                                       \
	_ZBUS_MULTIDOMAIN_UART_CONF_SETUP(_name, _zbus_chan, _name##_forwarder,                    \
					  DEVICE_DT_GET(DT_NODELABEL(_uart_dt_nodelabel)));        \
	K_THREAD_DEFINE(_name##_forwarder_thread_id,                                               \
			CONFIG_ZBUS_MULTIDOMAIN_UART_FORWARDER_THREAD_STACK_SIZE,                  \
			zbus_multidomain_uart_forwarder_thread, &_name##_uart_forwarder_config,    \
			NULL, NULL, CONFIG_ZBUS_MULTIDOMAIN_UART_FORWARDER_THREAD_PRIORITY, 0, 0);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_ */
