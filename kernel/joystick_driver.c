#include "linux/dev_printk.h"
#include "linux/irq.h"
#include "linux/input-event-codes.h"
#include "linux/stddef.h"
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/driver.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/pwm.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
#define RETURN_INT
#endif

#define NES_BITS 11

static struct task_struct *thread;
static struct input_dev *joystick_input_dev; //  Input device file
static struct gpio_desc *data, *latch;        // gpios
static struct pwm_device *pwm_clk;    // clock using pwm
static struct pwm_state state = {.period = 5000000ull, .duty_cycle = 2500000ull, .polarity = PWM_POLARITY_NORMAL, .enabled = false, .usage_power = false};

// Ordem (bit0→bit10): A, B, Select, Start, Up, Down, Left, Right, C, D, Push
static const unsigned int nes_keycodes[NES_BITS] = {
    BTN_A, BTN_B, BTN_SELECT, BTN_START,
    /* Up, Down, Left, Right will be handled as hat axes */
    0, 0, 0, 0,              // placeholders for Up, Down, Left, Right
    BTN_X, BTN_Y, BTN_THUMBL // C, D, Push-button
};

static int create_input_device(const struct device *dev) {
  int status;
  joystick_input_dev = input_allocate_device();
  if (!joystick_input_dev) {
    dev_err(dev, "failed to create input device for joystick\n");
    status = -ENOMEM;
    return status;
  }
  set_bit(EV_KEY, joystick_input_dev->evbit);
  set_bit(EV_ABS, joystick_input_dev->evbit);
  set_bit(BTN_A, joystick_input_dev->keybit);
  set_bit(BTN_B, joystick_input_dev->keybit);
  set_bit(BTN_SELECT, joystick_input_dev->keybit);
  set_bit(BTN_START, joystick_input_dev->keybit);
  set_bit(BTN_X, joystick_input_dev->keybit);
  set_bit(BTN_Y, joystick_input_dev->keybit);
  set_bit(BTN_THUMBL, joystick_input_dev->keybit);

  // input_set_abs_params(joystick_input_dev, ABS_HAT0X, -1, 1, 0, 0);
  // input_set_abs_params(joystick_input_dev, ABS_HAT0Y, -1, 1, 0, 0);

  // Support for ABS_X and ABS_Y
  input_set_abs_params(joystick_input_dev, ABS_X, -1, 1, 0, 0);
  input_set_abs_params(joystick_input_dev, ABS_Y, -1, 1, 0, 0);

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
  u32 clk_pwm[4];
  u32 latch_gpio[3];
  u32 data_gpio[3];
  const char *message;
  const char *pwm_name;
  if (!device_property_present(dev, "latch-gpios")) {
    dev_err(dev, "latch-gpios - property is not present!\n");
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
  if (!device_property_present(dev, "pwms")) {
    dev_err(dev, "pwm - property is not present!\n");
    return -1;
  }
  if (!device_property_present(dev, "pwm-names")) {
    dev_err(dev, "pwm-names - property is not present!\n");
    return -1;
  }



  status = device_property_read_u32_array(dev, "pwms", clk_pwm, 4); // returns 0 if sucessfull
  if (status) {
    dev_err(dev, "pwm - error reading property! \n");
    return status;
  }
  status = device_property_read_u32_array(dev, "latch-gpios", latch_gpio, 3);
  if (status) {
    dev_err(dev, "latch-gpios - error reading property! \n");
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
  status = device_property_read_string(dev, "pwm-names", &pwm_name);
  if (status) {
    dev_err(dev, "pwm-names - error reading property! \n");
    return status;
  }
  dev_info(dev, "%u,%u,%u,%u,%s,%s\n", clk_pwm[1], latch_gpio[1], data_gpio[1],clk_pwm[2],pwm_name,message);

  return status;
}


// Lê NES_BITS (11) bits, ativo-em-0 (0 = pressionado no fio DATA)


static u16 nesjoy_read_bits(void) {
  int i;
  u16 bits = 0;

  // Pulso de LATCH: alto para carregar o shift register no "controle"
  gpiod_set_value_cansleep(latch, 1);
  mdelay(1);
  gpiod_set_value_cansleep(latch, 0);
  mdelay(1);
  
  pwm_enable(pwm_clk);
  // Primeiro bit já disponível, depois avançar com clock
  for (i = 0; i < NES_BITS; i++) {
    int v = gpiod_get_value_cansleep(data);
    if (v < 0){
      v = 1; // em caso de erro, trata como solto
    }
    // 0 = pressed no fio, armazenamos pressed = 1
    bits |= ((v == 0) ? 1 : 0) << i;

    mdelay(5);
  }
  pwm_disable(pwm_clk);
  return bits;
}

static int nesjoy_thread_fn(void *device) {

  while (!kthread_should_stop()) {
    int left, right, up, down, hat_y, hat_x;
    u16 state = nesjoy_read_bits();

    dev_info(device, "thread running! %d\n", state);
    // Reportar botões
    // Report buttons (A, B, Select, Start, C, D, Push)
    input_report_key(joystick_input_dev, BTN_A, (state >> 0) & 0x1);
    input_report_key(joystick_input_dev, BTN_B, (state >> 1) & 0x1);
    input_report_key(joystick_input_dev, BTN_SELECT, (state >> 2) & 0x1);
    input_report_key(joystick_input_dev, BTN_START, (state >> 3) & 0x1);
    input_report_key(joystick_input_dev, BTN_X, (state >> 8) & 0x1);
    input_report_key(joystick_input_dev, BTN_Y, (state >> 9) & 0x1);
    input_report_key(joystick_input_dev, BTN_THUMBL, (state >> 10) & 0x1);

    // Report D-Pad as hat axes
    left = (state >> 6) & 0x1;
    right = (state >> 7) & 0x1;
    up = (state >> 4) & 0x1;
    down = (state >> 5) & 0x1;
    hat_x = right - left; // -1 = left, 1 = right, 0 = neutral
    hat_y =
        up -
	down; 

    input_report_abs(joystick_input_dev, ABS_X,
                     hat_x); // Lower 8 bits for X
    input_report_abs(joystick_input_dev, ABS_Y,
                     hat_y); // Upper 8 bits for Y
    input_sync(joystick_input_dev);
  }
  return 0;
}

static int joystick_probe(struct platform_device *device) {
  int status;
  struct device *dev; 
  pr_info("funcao de probe do joystick foi chamada!\n");
  dev = &(device->dev);

  status = device_tree_parse(dev); // returns 0 if sucessfull
  if (status) {
    return status;
  }

  pwm_clk = devm_pwm_get(dev, "clk");
  if (IS_ERR(pwm_clk)) {
    dev_err(dev, "failed to get pwm clock\n");
    return -1;
  }

  data = devm_gpiod_get_index(dev, "data", 0, GPIOD_IN);
  if (IS_ERR(data)) {
    dev_err(dev, "failed to get gpio data\n");
    return -1;
  }

  latch = devm_gpiod_get_index(dev, "latch", 0, GPIOD_OUT_LOW); // TODO
  if (IS_ERR(latch)) {
    dev_err(dev, "failed to get gpio lacth\n");
    return -1;
  }
  
  status = create_input_device(dev);
  if (status) {
    return status;
  }
  status = pwm_apply_state(pwm_clk, &state);
  if (status) {
    dev_err(dev, "failed to apply initial pwm state");
    return status;
  }

  
  thread = kthread_run(nesjoy_thread_fn, dev, "joy rasp");
  if (IS_ERR(thread)) {
    int err = PTR_ERR(thread);
    thread = NULL;
    return err;
  }
  
  return status;
}

#ifndef RETURN_INT
static void joystick_remove(struct platform_device *device) {
  if (thread) {
    kthread_stop(thread);
    thread = NULL;
  }
  if (joystick_input_dev) {
    input_unregister_device(joystick_input_dev);
    input_free_device(joystick_input_dev);
    joystick_input_dev = NULL;
  }
}
#else
static int joystick_remove(struct platform_device *device) {
 
  if (thread) {
    kthread_stop(thread);
    thread = NULL;
  }
  
  if (joystick_input_dev) {
    input_unregister_device(joystick_input_dev);
    input_free_device(joystick_input_dev);
    joystick_input_dev = NULL;
  }
  return 0;
}
#endif

//	This driver follows the Linux kernel platform device model.
//	used to integrate peripherals on many system-on-chip processors,
//	https://docs.kernel.org/driver-api/driver-model/platform.html

/*Naming compatible devices*/
static const struct of_device_id my_dev_ids[] = {
    {.compatible = "TCC,Kronos"}, {} /*End of list*/
};



MODULE_DEVICE_TABLE(of, my_dev_ids);

static struct platform_driver joystick_driver = {
    .probe = joystick_probe,
    .remove = joystick_remove,
    .driver = {.name = "KRONOS",
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
