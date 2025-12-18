#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/init.h>

#define MODULE_NAME "SSNESJ" // nome para debug

MODULE_LICENSE("GPL");                                     // Tipo de licença -- afeta o comportamento do tempo de execução
MODULE_AUTHOR("Arthur Silva Matias");                                 // Autor -- visivel quando usado o modinfo
MODULE_DESCRIPTION("Driver do joystick");  // Descrição do módulo -- visível no modinfo
MODULE_VERSION("0.1");                                     // Versão do módulo

static struct input_dev *joystick_dev;

int __init joystick_init(void) {
	int error;


	printk(KERN_INFO "Bem-vindo à criação do módulo");
	printk(KERN_INFO MODULE_NAME);
	joystick_dev = input_allocate_device();
  	if (!joystick_dev) {
                printk(KERN_ERR "button.c: Not enough memory\n");
                error = -ENOMEM;
		goto no_memory;
        }
	set_bit(EV_KEY, joystick_dev->evbit);
	set_bit(BTN_A, joystick_dev->keybit);
	set_bit(BTN_B, joystick_dev->keybit);
	set_bit(BTN_X, joystick_dev->keybit);
	set_bit(BTN_Y, joystick_dev->keybit);
	joystick_dev->name = "Titan JoyStick";
	error = input_register_device(joystick_dev);
	if (error) {
		printk(KERN_ERR "button.c: Failed to register device\n");
                goto err_free_dev;
	}
	return 0;

err_free_dev:
	input_free_device(joystick_dev);	
no_memory:
	return error;
}

void __exit joystick_exit(void) {
	printk(KERN_INFO "O novo módulo foi removido ...\n");
        input_unregister_device(joystick_dev);
}

module_init(joystick_init);
module_exit(joystick_exit);
