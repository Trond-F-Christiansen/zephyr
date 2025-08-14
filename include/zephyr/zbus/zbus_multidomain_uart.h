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

	/** Semaphore to signal when the UART TX is busy. */
	struct k_sem *tx_busy_sem;
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
	static K_SEM_DEFINE(_name##_uart_tx_busy_sem, 1, 1);                                      \
	static struct zbus_multidomain_uart_config _name##_uart_config = {                         \
		.uart_dev = _uart_dev,                                                             \
		.async_rx_buffer = {{0}},                                                          \
		.async_rx_buffer_idx = 0,                                                          \
		.tx_busy_sem = &_name##_uart_tx_busy_sem,                                          \
	};

/** @endcond */

/**
 * @brief Macro to setup a UART ZBUS forwarder.
 * 
 * This macro sets up the backend for a UART ZBUS forwarder. It sets up a message subscriber,
 * initializes the UART backend, and creates a thread to handle incoming messages.
 *
 * @param _name Name of the UART forwarder
 * @param _uart_dt_nodelabel Device tree label for the UART instance
 * 
 * @note This macro sets up a thread
 * @note The macro does not add the message subscriber to the channel, 
 * use @ref ZBUS_MULTIDOMAIN_FORWARDER_UART_ADD_CHANNEL
 */
#define ZBUS_MULTIDOMAIN_DEFINE_FORWARDER_UART(_name, _uart_dt_nodelabel) \
	ZBUS_MSG_SUBSCRIBER_DEFINE(_name##_forwarder);                                             \
	_ZBUS_MULTIDOMAIN_UART_CONF_SETUP(_name, _zbus_chan, _name##_forwarder,                    \
					  DEVICE_DT_GET(DT_NODELABEL(_uart_dt_nodelabel)));        \
	K_THREAD_DEFINE(_name##_forwarder_thread_id,                                               \
			CONFIG_ZBUS_MULTIDOMAIN_UART_FORWARDER_THREAD_STACK_SIZE,                  \
			zbus_multidomain_uart_forwarder_thread, &_name##_uart_config,    \
			&_name##_forwarder, NULL, CONFIG_ZBUS_MULTIDOMAIN_UART_FORWARDER_THREAD_PRIORITY, 0, 0);

/**
 * @brief Macro to add a channel to a UART forwarder.
 * 
 * This macro adds a ZBUS channel to a UART forwarder, allowing the forwarder to listen for messages
 * on the specified channel and forward them via UART.
 * 
 * @param _name Name of the UART forwarder
 * @param _zbus_chan ZBUS channel to forward messages from
 * 
 * @note This macro does not create the forwarder, use @ref ZBUS_MULTIDOMAIN_DEFINE_FORWARDER_UART
 *       to create the forwarder first.
 * @note The forwarder must be created before adding channels to it.
 */
#define ZBUS_MULTIDOMAIN_FORWARDER_UART_ADD_CHANNEL(_name, _zbus_chan) \
	ZBUS_CHAN_ADD_OBS(_zbus_chan, _name##_forwarder, 0);  



/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZBUS_MULTIDOMAIN_UART_H_ */
