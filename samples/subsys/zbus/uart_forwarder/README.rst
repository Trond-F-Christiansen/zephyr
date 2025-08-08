.. zephyr:code-sample:: zbus-uart-forwarder
   :name: zbus uart forwarder
   :relevant-api: zbus_apis

    Zbus forwarder between two UART channels.

Overview
********
This sample demonstrates how to use the Zbus UART forwarder to forward zbus messages on a zbus channel between two UART devices.
In this sample, both UART devices are on the same mcu, but there are no restrictions on the UART devices being on different devices.

Requirements
************

The sample supports the following development kits:
* nRF5340 DK (nrf5340dk_nrf5340_cpuapp)
* nRF7002 DK (nrf7002dk_nrf5340_cpuapp)

Building and Running
********************
The sample logs messages over the serial console. It can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/uart_forwarder
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash

The sample also requires the following pins to be shorted:

  .. list-table::
     :widths: auto
     :header-rows: 1

     * - Development kit
       - nRF5340 DK
       - nRF7002 DK
     * - UART pins
       - P1.01 (UART1 TX) to P1.06 (UART2 RX)
       - P1.01 (UART1 TX) to P1.06 (UART2 RX)


Testing
=======
The sample logs messages over the serial console, and it can be tested as follows:

1. Connects the nRF5340 DK to a host computer using a USB cable.
#. Reset the kit.
#. Observe the console output for both cores:

  * For the application core, the output is similar to the following one:

    .. code-block:: console

        *** Booting Zephyr OS build v4.2.0-1142-ge7c42e6d0b8f ***
        I: nrf5340dk/nrf5340/cpuapp started
        I: Published on channel uart1_channel: count=0, message='Hello from nrf5340/cpuapp'
        I: Published on channel uart1_channel: count=1, message='Hello from nrf5340/cpuapp'
        I: Received message on channel uart2_channel from domain nrf5340/cpuapp_uart: message='Hello from nrf5340/cpuapp' (count=1)
        I: Published on channel uart1_channel: count=2, message='Hello from nrf5340/cpuapp'
        I: Received message on channel uart2_channel from domain nrf5340/cpuapp_uart: message='Hello from nrf5340/cpuapp' (count=2)
        I: Published on channel uart1_channel: count=3, message='Hello from nrf5340/cpuapp'
        I: Received message on channel uart2_channel from domain nrf5340/cpuapp_uart: message='Hello from nrf5340/cpuapp' (count=3)
        I: Published on channel uart1_channel: count=4, message='Hello from nrf5340/cpuapp'
        I: Received message on channel uart2_channel from domain nrf5340/cpuapp_uart: message='Hello from nrf5340/cpuapp' (count=4)

