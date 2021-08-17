#ifndef __BUTTON_FUNC_H__	
#define __BUTTON_FUNC_H__		
#include <zephyr.h>
#include <device.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>

#define SW0_NODE	DT_ALIAS(sw0)
#define SW0_GPIO_LABEL	DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN	DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS	(GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))
void button_init(void);
#endif // !__BUTTON_FUNC_H__	

