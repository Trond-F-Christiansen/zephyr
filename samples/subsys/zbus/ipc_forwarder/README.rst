.. zephyr:code-sample:: zbus-ipc-forwarder
   :name: zbus ipc forwarder
   :relevant-api: zbus_apis

    Zbus forwarder between two cores using IPC.

Overview
********
This sample demonstrates how to use the Zbus IPC forwarder to forward zbus messages on a zbus channel between two cores on a multicore system.

Requirements
************

The sample supports the following development kits:
* nRF5340 DK (nrf5340dk_nrf5340_cpuapp)
* nRF54H20 DK (nrf54h20dk_nrf54h20_cpuapp)

Building and Running
********************
The sample logs messages over the serial console. It can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/ipc_forwarder
   :board: nrf5340dk/nrf5340/cpuapp
   :goals: build flash

alternatively you can build and flash the sample with the ``nordic-log-stm`` snippet, which will log messages over the same interface for both cores.

Testing
=======
The sample logs messages over the serial console, and it can be tested as follows:

1. Connects the nRF5340 DK to a host computer using a USB cable.
#. Reset the kit.
#. Observe the console output for both cores:

  * For the application core, the output is similar to the following one:

    .. code-block:: console

        *** Booting nRF Connect SDK v3.0.1-9eb5615da66b ***
        *** Using Zephyr OS v4.0.99-77f865b8f8d0 ***
        [00:00:00.274,376] <inf> zbus: nrf54h20dk/nrf54h20/cpuapp started
        [00:00:00.274,421] <inf> zbus: Received message from domain 'nrf54h20/cpuapp': cnt=0, str='Hello from CPUAPP'
        [00:00:00.274,889] <inf> zbus: Received message from domain 'nrf54h20/cpurad': cnt=0, str='Hello from CPUAPP'
        [00:00:05.274,592] <inf> zbus: Received message from domain 'nrf54h20/cpuapp': cnt=1, str='Hello from CPUAPP'
        [00:00:05.274,921] <inf> zbus: Received message from domain 'nrf54h20/cpurad': cnt=1, str='Hello from CPUAPP'
        [00:00:10.274,736] <inf> zbus: Received message from domain 'nrf54h20/cpuapp': cnt=2, str='Hello from CPUAPP'
        [00:00:10.275,160] <inf> zbus: Received message from domain 'nrf54h20/cpurad': cnt=2, str='Hello from CPUAPP'
        [00:00:15.274,866] <inf> zbus: Received message from domain 'nrf54h20/cpuapp': cnt=3, str='Hello from CPUAPP'
        [00:00:15.275,293] <inf> zbus: Received message from domain 'nrf54h20/cpurad': cnt=3, str='Hello from CPUAPP'
        [00:00:20.274,994] <inf> zbus: Received message from domain 'nrf54h20/cpuapp': cnt=4, str='Hello from CPUAPP'
        [00:00:20.275,419] <inf> zbus: Received message from domain 'nrf54h20/cpurad': cnt=4, str='Hello from CPUAPP'
        [00:00:25.275,122] <inf> zbus: Received message from domain 'nrf54h20/cpuapp': cnt=5, str='Hello from CPUAPP'
        [00:00:25.275,550] <inf> zbus: Received message from domain 'nrf54h20/cpurad': cnt=5, str='Hello from CPUAPP'

    Where the listener receives messages both from its own domain and the echoed messages from the radio core.

  * For the radio core, the output is similar to the following one:

    .. code-block:: console

        [00:00:00.274,370] <inf> zbus: nrf54h20dk/nrf54h20/cpurad started
        [00:00:00.274,403] <inf> zbus: Echo thread started for channel: multicore_test_remote
        [00:00:00.274,728] <inf> zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=0, str='Hello from CPUAPP'
        [00:00:05.274,759] <inf> zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=1, str='Hello from CPUAPP'
        [00:00:10.274,954] <inf> zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=2, str='Hello from CPUAPP'
        [00:00:15.275,078] <inf> zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=3, str='Hello from CPUAPP'
        [00:00:20.275,208] <inf> zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=4, str='Hello from CPUAPP'
        [00:00:25.275,336] <inf> zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=5, str='Hello from CPUAPP'

  * If the sample was compiled with the ``nordic-log-stm`` snippet, both cores will logg messages over the same interface, similar to the following:

    .. code-block:: console

        *** Booting nRF Connect SDK v3.0.1-9eb5615da66b ***
        *** Using Zephyr OS v4.0.99-77f865b8f8d0 ***
        [00:00:00.255,254] <inf> app/zbus: nrf54h20dk@0.9.0/nrf54h20/cpuapp started
        [00:00:00.255,294] <inf> app/zbus: Received message from domain 'nrf54h20/cpuapp': cnt=0, str='Hello from CPUAPP'
        [00:00:00.255,318] <inf> rad/zbus: nrf54h20dk@0.9.0/nrf54h20/cpurad started
        [00:00:00.255,339] <inf> rad/zbus: Echo thread started for channel: multicore_test_remote
        [00:00:00.255,568] <inf> rad/zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=0, str='Hello from CPUAPP'
        [00:00:00.255,678] <inf> app/zbus: Received message from domain 'nrf54h20/cpurad': cnt=0, str='Hello from CPUAPP'
        [00:00:05.228,228] <inf> app/zbus: Received message from domain 'nrf54h20/cpuapp': cnt=1, str='Hello from CPUAPP'
        [00:00:05.228,408] <inf> rad/zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=1, str='Hello from CPUAPP'
        [00:00:05.228,520] <inf> app/zbus: Received message from domain 'nrf54h20/cpurad': cnt=1, str='Hello from CPUAPP'
        [00:00:10.202,724] <inf> app/zbus: Received message from domain 'nrf54h20/cpuapp': cnt=2, str='Hello from CPUAPP'
        [00:00:10.202,900] <inf> rad/zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=2, str='Hello from CPUAPP'
        [00:00:10.203,014] <inf> app/zbus: Received message from domain 'nrf54h20/cpurad': cnt=2, str='Hello from CPUAPP'
        [00:00:15.173,336] <inf> app/zbus: Received message from domain 'nrf54h20/cpuapp': cnt=3, str='Hello from CPUAPP'
        [00:00:15.173,526] <inf> rad/zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=3, str='Hello from CPUAPP'
        [00:00:15.173,643] <inf> app/zbus: Received message from domain 'nrf54h20/cpurad': cnt=3, str='Hello from CPUAPP'
        [00:00:20.143,680] <inf> app/zbus: Received message from domain 'nrf54h20/cpuapp': cnt=4, str='Hello from CPUAPP'
        [00:00:20.143,856] <inf> rad/zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=4, str='Hello from CPUAPP'
        [00:00:20.143,968] <inf> app/zbus: Received message from domain 'nrf54h20/cpurad': cnt=4, str='Hello from CPUAPP'
        [00:00:25.113,985] <inf> app/zbus: Received message from domain 'nrf54h20/cpuapp': cnt=5, str='Hello from CPUAPP'
        [00:00:25.114,161] <inf> rad/zbus: Echoing message from domain 'nrf54h20/cpuapp': cnt=5, str='Hello from CPUAPP'
        [00:00:25.114,273] <inf> app/zbus: Received message from domain 'nrf54h20/cpurad': cnt=5, str='Hello from CPUAPP'
