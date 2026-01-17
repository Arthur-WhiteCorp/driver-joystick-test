#include <asm/io.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
#define RETURN_INT
#endif

//	This driver follows the Linux kernel platform device model.
//	used to integrate peripherals on many system-on-chip processors,
//	https://docs.kernel.org/driver-api/driver-model/platform.html

static struct input_dev *joystick_input_dev; //  Input device file
struct gpio_desc *data, *clk, *latch;			 // gpios

static int create_input_device(const struct device *dev) {
	int status;
	joystick_input_dev = input_allocate_device();
	if (!joystick_input_dev) {
		dev_err(dev, "failed to create input device for joystick\n");
		status = -ENOMEM;
		return status;
	}
	set_bit(EV_KEY, joystick_input_dev->evbit);
	set_bit(BTN_A, joystick_input_dev->keybit);
	set_bit(BTN_B, joystick_input_dev->keybit);
	set_bit(BTN_SELECT, joystick_input_dev->keybit);
	set_bit(BTN_START, joystick_input_dev->keybit);
	set_bit(BTN_DPAD_UP, joystick_input_dev->keybit);
	set_bit(BTN_DPAD_DOWN, joystick_input_dev->keybit);
	set_bit(BTN_DPAD_LEFT, joystick_input_dev->keybit);
	set_bit(BTN_DPAD_RIGHT, joystick_input_dev->keybit);
	set_bit(BTN_X, joystick_input_dev->keybit);
	set_bit(BTN_Y, joystick_input_dev->keybit);
	set_bit(BTN_THUMBL, joystick_input_dev->keybit);
	joystick_input_dev->name = dev->driver->name;
	status = input_register_device(joystick_input_dev);
	if (status) {
		dev_err(dev, "failed to register input device for joystick\n");
		goto err_free_dev;
	}
	return status;

err_free_dev:
	input_free_device(joystick_input_dev);
	return status;
}

static int device_tree_parse(struct device *dev) {
	int status;
	u32 latch_gpio[3];
	u32 clk_gpio[3];
	u32 data_gpio[3];
	const char *message;

	if (!device_property_present(dev, "latch-gpios")) {
		dev_err(dev, "latch-gpios - property is not present!\n");
		return -1;
	}
	if (!device_property_present(dev, "clk-gpios")) {
		dev_err(dev, "clk-gpios - property is not present!\n");
		return -1;
	}
	if (!device_property_present(dev, "data-gpios")) {
		dev_err(dev, "data-gpios - property is not present!\n");
		return -1;
	}
	if (!device_property_present(dev, "message")) {
		dev_err(dev, "message - property is not present!\n");
		return -1;
	}

	status = device_property_read_u32_array(dev, "latch-gpios", latch_gpio,3); // returns 0 if sucessfull
	if (status) {
		dev_err(dev, "latch-gpios - error reading property! \n");
		return status;
	}
	status = device_property_read_u32_array(dev, "clk-gpios", clk_gpio, 3);
	if (status) {
		dev_err(dev, "clk-gpios - error reading property! \n");
		return status;
	}
	status = device_property_read_u32_array(dev, "data-gpios", data_gpio, 3);
	if (status) {
		dev_err(dev, "data-gpios - error reading property! \n");
		return status;
	}
	status = device_property_read_string(dev, "message", &message);
	if (status) {
		dev_err(dev, "message - error reading property! \n");
		return status;
	}

	dev_info(dev, "%u,%u,%u,%s\n", latch_gpio[1], clk_gpio[1], data_gpio[1],message);

	return status;
}

static int joystick_probe(struct platform_device *device) {
	int status;
	pr_info("funcao de probe do joystick foi chamada!\n");
	struct device *dev = &(device->dev);

	status = device_tree_parse(dev); // returns 0 if sucessfull
	if (status) {
		return status;
	}

	data = devm_gpiod_get_index(dev, "data", 0, GPIOD_IN);
	if (IS_ERR(data)) {
		dev_err(dev,"failed to get gpio data\n");
		return -1;
	}

	latch = devm_gpiod_get_index(dev, "latch", 0, GPIOD_OUT_LOW); // TODO
	if (IS_ERR(latch)) {
		dev_err(dev,"failed to get gpio latch\n");
		return -1;
	}
	
	clk = devm_gpiod_get_index(dev, "clk", 0, GPIOD_OUT_LOW); // TODO
	if (IS_ERR(clk)) {
		dev_err(dev,"failed to get gpio clk\n");
		return -1;
	}
	


	status = create_input_device(dev);
	return status;
}

#ifndef RETURN_INT
static void joystick_remove(struct platform_device *device) {
	input_unregister_device(joystick_input_dev);
}
#else
static int joystick_remove(struct platform_device *device) {
	input_unregister_device(joystick_input_dev);
	return 0;
}
#endif

/*Naming compatible devices*/
static const struct of_device_id my_dev_ids[] = {
		{.compatible = "equipe03,esp32_joystick"}, {} /*End of list*/
};

MODULE_DEVICE_TABLE(of, my_dev_ids);


static struct platform_driver joystick_driver = {
		.probe = joystick_probe,
		.remove = joystick_remove,
		.driver = {.name = "PLATFORM DRIVER FOR JOYSTICK",
				 .of_match_table = my_dev_ids}};

int __init joystick_init(void) {
	pr_info("Inicializando o modulo!\n");
	return platform_driver_register(&joystick_driver);
}

void __exit joystick_exit(void) {
	pr_info("Iniciando remocao do modulo...\n");
	platform_driver_unregister(&joystick_driver);
}

module_init(joystick_init);
module_exit(joystick_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arthur Silva Matias");
MODULE_DESCRIPTION("Driver do joystick");
MODULE_VERSION("0.1");
