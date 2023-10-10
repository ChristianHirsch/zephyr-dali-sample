/*
 * Copyright (c) 2023 RSA FG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dali.h"

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dali, LOG_LEVEL_DBG);

#define STOP_BITS_PERIOD K_USEC(5500)

static const struct gpio_dt_spec dali_tx = GPIO_DT_SPEC_GET(DT_NODELABEL(dali_tx), gpios);
static const struct gpio_dt_spec dali_rx = GPIO_DT_SPEC_GET(DT_NODELABEL(dali_rx), gpios);

#define HALF_BIT_PERIOD K_USEC(284)
#define QUARTER_BIT_PERIOD K_USEC(284 / 2)

#define DALI_WAIT K_MSEC(50)

#define FADE_ITERATIONS 20

static uint8_t addr;
static uint8_t command;

static uint16_t rx_buf;
static uint16_t rx_buf_rdy = 0;

static struct gpio_callback dali_cb_data;

static K_THREAD_STACK_DEFINE(dali_loop_worker_stack_area, 1024);
static struct k_thread dali_loop_worker_data;
static k_tid_t dali_loop_worker_tid = 0;

static void dali_write_bit_0(void)
{
        if (!gpio_is_ready_dt(&dali_tx)) {
		LOG_ERR("device not ready");
                return;
        }
        
	gpio_pin_configure_dt(&dali_tx, GPIO_OUTPUT_ACTIVE);
	k_sleep(HALF_BIT_PERIOD);
	gpio_pin_configure_dt(&dali_tx, GPIO_OUTPUT_INACTIVE);
	k_sleep(HALF_BIT_PERIOD);
}

static void dali_write_bit_1(void)
{
        if (!gpio_is_ready_dt(&dali_tx)) {
		LOG_ERR("device not ready");
                return;
        }
        
	gpio_pin_configure_dt(&dali_tx, GPIO_OUTPUT_INACTIVE);
	k_sleep(HALF_BIT_PERIOD);
	gpio_pin_configure_dt(&dali_tx, GPIO_OUTPUT_ACTIVE);
	k_sleep(HALF_BIT_PERIOD);
}

static void dali_write_idle(void)
{
	gpio_pin_configure_dt(&dali_tx, GPIO_OUTPUT_ACTIVE);
	k_sleep(STOP_BITS_PERIOD);
}

static void dali_write_byte(uint8_t data)
{
	uint8_t i = 0x80;

	for (i = 0x80; i > 0x00; i >>= 1)
	{
		if (data & i)
	       	{
			dali_write_bit_1();
		}
		else
		{
			dali_write_bit_0();
		}
	}
}

void dali_query(void)
{
	dali_send_data(BROADCAST_C, QUERY_STATUS);
}

void dali_on(void)
{
	dali_send_data(BROADCAST_C, ON_C);
}

void dali_off(void)
{
	dali_send_data(BROADCAST_C, OFF_C);
}

static void dali_loop_worker(void *p1, void *p2, void *p3)
{
	gpio_remove_callback(dali_rx.port, &dali_cb_data);
	dali_write_bit_1();
	dali_write_byte(*(uint8_t *)p1);
	dali_write_byte(*(uint8_t *)p2);
	dali_write_idle();
	gpio_add_callback(dali_rx.port, &dali_cb_data);

	dali_loop_worker_tid = 0;

        return;
}

void dali_send_data(uint8_t _addr, uint8_t _command)
{
	addr = _addr;
	command = _command;

	if (dali_loop_worker_tid > 0)
	{
		return;
	}

	dali_loop_worker_tid = k_thread_create(&dali_loop_worker_data,
                        dali_loop_worker_stack_area,
                        K_THREAD_STACK_SIZEOF(dali_loop_worker_stack_area),
                        dali_loop_worker,
                        &addr, &command, NULL,
                        -CONFIG_NUM_COOP_PRIORITIES, 0, K_NO_WAIT);
}

static void dali_receive(const struct device *dev, struct gpio_callback *cb,
		    uint32_t pins)
{
	static uint32_t last = 0;
	uint32_t now = k_cycle_get_32();
	uint32_t diff = now - last;

	static uint8_t received_half_bits = 0;
	static uint8_t last_bit = 1;

	if (diff > 200000)
       	{
		rx_buf = 0x00;
		rx_buf_rdy = 0;
		received_half_bits = 0;
		last_bit = 1;
	}
       	else if (diff > 105000)
	{
		rx_buf <<= 1;
		// long: bit shift
		last_bit = !last_bit;
		rx_buf |= last_bit;

		received_half_bits+=2;
		LOG_INF("long");
	}
	else
	{
		received_half_bits++;
		if (received_half_bits % 2 == 1)
		{
			rx_buf <<= 1;
			rx_buf |= last_bit;
		}
		// short: same bit
		LOG_INF("short");
	}

	if (received_half_bits >= 17)
	{
		rx_buf_rdy = 1;
		LOG_INF("received data: %hu", rx_buf & 0x00ff);
	}

	last = now;
}

int dali_receive_byte()
{
	int i;
	for (i = 0; i < 500; i++)
	{
		if (rx_buf_rdy == 1) {
			break;
		}
		k_sleep(K_MSEC(1));
	}

	if (i >= 500) {
		return -1;
	}

	rx_buf_rdy = 0;
	return rx_buf;
}

static int dali_initialize(void)
{
	int ret;

	if (!gpio_is_ready_dt(&dali_rx)) {
		LOG_ERR("Error: dali device %s is not ready\n",
		       dali_rx.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&dali_rx, GPIO_INPUT | GPIO_ACTIVE_LOW);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d\n",
		       ret, dali_rx.port->name, dali_rx.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&dali_rx,
					      GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d\n",
			ret, dali_rx.port->name, dali_rx.pin);
		return 0;
	}

	gpio_init_callback(&dali_cb_data, dali_receive, BIT(dali_rx.pin));
	gpio_add_callback(dali_rx.port, &dali_cb_data);

	LOG_INF("successfully initialized");
	return 0;
}

SYS_INIT(dali_initialize, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
