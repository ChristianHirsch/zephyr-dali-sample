DALI blinky sample
##################

Overview
********

This sample show how to blink lights connected to DALI drivers. The sample acts as a control device and broadcasts the control commands to swith off and on the LEDs to the Control Gear.

The following devices have been tested and are working at:

- STM32F405 (`Adafruit Feather STM32F405 Express`_) wired to a `DALI interface circuit`_/board.

Requirements
************

It is necessary to have a circuit or board that connects the DALI bus to the Rx and Tx pins of the microcontroller. This is a sample `DALI interface circuit`_.

Building and Running
********************

.. code-block:: console

   west build -b adafruit_feather_stm32f405
   west flash --runner jlink

Known Problems
**************

The timing for the half bits for the DALI bus was fine tuned to fit the DALI specifications. The DALI frames are transmitted at a baud rate of 1200 bps. This results in a bit period of 833 us. Because Manchester encoding is used for the transmission, the half bit period is 416 us. However, the half bit period in this sample is set to 284 us to fit 416 us.

.. _Adafruit Feather STM32F405 Express: https://www.adafruit.com/product/4382
.. _DALI interface circuit: https://www.mouser.at/applications/lighting-digitally-addressable/
