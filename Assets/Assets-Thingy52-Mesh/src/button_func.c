#include "button_func.h"
#include "power/reboot.h"

static const struct device *button;
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev,
	struct gpio_callback *cb,
	uint32_t pins)
{
	printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
	sys_reboot(SYS_REBOOT_COLD);
}

void button_init(void)
{
	int ret;

	button = device_get_binding(SW0_GPIO_LABEL);
	if (button == NULL) {
		printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
		return;
	}

	ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
			ret,
			SW0_GPIO_LABEL,
			SW0_GPIO_PIN);
		return;
	}

	ret = gpio_pin_interrupt_configure(button,
		SW0_GPIO_PIN,
		GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error %d: failed to configure interrupt on %s pin %d\n",
			ret,
			SW0_GPIO_LABEL,
			SW0_GPIO_PIN);
		return;
	}

	gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button, &button_cb_data);

}