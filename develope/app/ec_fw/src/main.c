/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int main(void)
{
	printk("Hello World! %s, Rev: %s\n", CONFIG_BOARD, CONFIG_BOARD_REVISION);
	return 0;
}
