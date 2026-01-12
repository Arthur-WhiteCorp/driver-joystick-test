#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/property.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Arthur Silva Matias");
MODULE_DESCRIPTION("Driver do joystick");
MODULE_VERSION("0.1");

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,2,0)
#define RETURN_INT
#endif

//  This driver follows the Linux kernel platform device model.
//  used to integrate peripherals on many system-on-chip processors,
//  https://docs.kernel.org/driver-api/driver-model/platform.html


/*Naming compatible devices*/
static const struct of_device_id my_dev_ids[] = {
	{.compatible = "equipe03,esp32_joystick"},
	{}  /*End of list*/
	};

MODULE_DEVICE_TABLE(of, my_dev_ids);

static struct input_dev *joystick_input_dev; //  Input device file




static int create_input_device(const struct device* dev){
	int error;
	joystick_input_dev = input_allocate_device();
	if (!joystick_input_dev) {
		dev_err(dev,"failed to create input device for joystick\n");
		error = -ENOMEM;
		goto no_memory;
        }
	set_bit(EV_KEY, joystick_input_dev->evbit);
	set_bit(BTN_A, joystick_input_dev->keybit);
	set_bit(BTN_B, joystick_input_dev->keybit);
	set_bit(BTN_X, joystick_input_dev->keybit);
	set_bit(BTN_Y, joystick_input_dev->keybit);

	joystick_input_dev->name = dev->driver->name;
	error = input_register_device(joystick_input_dev);
	if (error) {
		dev_err(dev,"failed to register input device for joystick\n");
                goto err_free_dev;
	}
	return 0;

err_free_dev:
	input_free_device(joystick_input_dev);	
no_memory:
	return error;
}


static int joystick_probe(struct platform_device* device){
	pr_info("funcao de probe do joystick foi chamada!\n");
	int status;
	u32 latch_gpio[3]; 
        u32 clk_gpio[3];
        u32 data_gpio[3];
	const char* message;
	struct device *dev = &(device->dev);
	
	if (!device_property_present(dev,"latch_gpio")){
		dev_err(dev,"latch_gpio - property is not present!\n");
		return -1;
	}
	if (!device_property_present(dev,"clk_gpio")){
		dev_err(dev,"clk_gpio - property is not present!\n");
		return -1;
	}
	if (!device_property_present(dev,"data_gpio")){
		dev_err(dev,"data_gpio - property is not present!\n");
		return -1;
	}
	if (!device_property_present(dev,"message")){
		dev_err(dev,"message - property is not present!\n");
		return -1;
	}
	
	status = device_property_read_u32_array(dev,"latch_gpio",latch_gpio,3); // returns 0 if sucessfull
	if (status){
		dev_err(dev,"latch_gpio - error reading property! \n");
		return status;
	}
	status = device_property_read_u32_array(dev,"clk_gpio",clk_gpio,3);
	if (status){
		dev_err(dev,"clk_gpio - error reading property! \n");
		return status;
	}
	status = device_property_read_u32_array(dev,"data_gpio",data_gpio,3);
	if (status){
		dev_err(dev,"data_gpio - error reading property! \n");
		return status;
	}
	status = device_property_read_string(dev,"message",&message);
	if (status){
		dev_err(dev,"message - error reading property! \n");
		return status;
	}
 
	dev_info(dev,"%u,%u,%u,%s\n",latch_gpio[1],clk_gpio[1],data_gpio[1],message);


	status = create_input_device(dev);
	return status;
}





#ifndef RETURN_INT
static void joystick_remove(struct platform_device* device){
        input_unregister_device(joystick_dev);
}
#else
static int joystick_remove(struct platform_device* device){
        input_unregister_device(joystick_input_dev);
	return 0;
}
#endif


static struct platform_driver joystick_driver = {
	.probe = joystick_probe,
	.remove = joystick_remove,
	.driver = {
		.name = "PLATFORM DRIVER FOR JOYSTICK",
		.of_match_table = my_dev_ids
	}
};

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
