#include "linux/dev_printk.h"
#include "linux/irq.h"
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
#include <linux/version.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/pwm.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 2, 0)
#define RETURN_INT
#endif

#define NES_BITS 11

//	This driver follows the Linux kernel platform device model.
//	used to integrate peripherals on many system-on-chip processors,
//	https://docs.kernel.org/driver-api/driver-model/platform.html

// static int poll_interval_ms = 2; // ~500 Hz
struct task_struct *thread;
static struct input_dev *joystick_input_dev; //  Input device file
struct gpio_desc *data, *clk;        // gpios
struct pwm_device *pwm;
struct pwm_state state;

// Ordem (bit0→bit10): A, B, Select, Start, Up, Down, Left, Right, C, D, Push
static const unsigned int nes_keycodes[NES_BITS] = {
    BTN_A,         BTN_B,         BTN_SELECT,     BTN_START, BTN_DPAD_UP,
    BTN_DPAD_DOWN, BTN_DPAD_LEFT, BTN_DPAD_RIGHT, BTN_X,     BTN_Y, // C, D
    BTN_THUMBL // Push-button
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
  u32 latch_pwm[3];
  u32 clk_gpio[3];
  u32 data_gpio[3];
  const char *message;

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
  if (!device_property_present(dev, "pwms")) {
    dev_err(dev, "pwms - property is not present!\n");
    return -1;
  }
  if (!device_property_present(dev, "pwm-names")) {
    dev_err(dev, "pwm-names - property is not present!\n");
    return -1;
  }




  status = device_property_read_u32_array(dev, "pwms", latch_pwm, 4); // returns 0 if sucessfull
  if (status) {
    dev_err(dev, "pwms - error reading property! \n");
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

  dev_info(dev, "%u,%u,%u,%s\n", latch_pwm[1], clk_gpio[1], data_gpio[1],
           message);

  return status;
}

/*
// Lê NES_BITS (11) bits, ativo-em-0 (0 = pressionado no fio DATA)
static u16 nesjoy_read_bits(void) {
  int i;
  u16 bits = 0;

  // Pulso de LATCH: alto para carregar o shift register no "controle"
  //gpiod_set_value_cansleep(latch, 1);
  udelay(12);
  //gpiod_set_value_cansleep(latch, 0);
  udelay(6);

  // Primeiro bit já disponível, depois avançar com clock
  for (i = 0; i < NES_BITS; i++) {
    int v = gpiod_get_value_cansleep(data);
    if (v < 0)
      v = 1; // em caso de erro, trata como solto
    // 0 = pressed no fio, armazenamos pressed = 1
    bits |= ((v == 0) ? 1 : 0) << i;

    // Pulso de clock para próximo bit
    gpiod_set_value_cansleep(clk, 1);
    udelay(6);
    gpiod_set_value_cansleep(clk, 0);
    udelay(6);
  }
  return bits;
}

static int nesjoy_thread_fn(void *device) {

  while (!kthread_should_stop()) {
    u16 state = nesjoy_read_bits();

    dev_info(device, " joy thread running! %d\n", state);
    // Reportar botões
    for (int i = 0; i < NES_BITS; i++) {
      uint pressed = (state >> i) & 0x1;
      dev_info(device, " Button %d: %d\n", i, pressed);
      input_report_key(joystick_input_dev, nes_keycodes[i], pressed);
    }
    input_sync(joystick_input_dev);

    if (poll_interval_ms < 1)
      poll_interval_ms = 1;
    msleep(poll_interval_ms);
  }
  return 0;
}
*/
static int joystick_probe(struct platform_device *device) {
  int status;
  pr_info("funcao de probe do joystick foi chamada!\n");
  struct device *dev = &(device->dev);

  status = device_tree_parse(dev); // returns 0 if sucessfull
  if (status) {
    return status;
  }

  pwm = devm_pwm_get(dev, "latch");
  if (IS_ERR(data)) {
    dev_err(dev, "failed to get pwm latch\n");
    return -1;
  }

  data = devm_gpiod_get_index(dev, "data", 0, GPIOD_IN);
  if (IS_ERR(data)) {
    dev_err(dev, "failed to get gpio data\n");
    return -1;
  }

  clk = devm_gpiod_get_index(dev, "clk", 0, GPIOD_OUT_LOW); // TODO
  if (IS_ERR(clk)) {
    dev_err(dev, "failed to get gpio clk\n");
    return -1;
  }

  status = create_input_device(dev);
  pwm_config(pwm, 2500000, 5000000);  // 2.5ms HIGH, 2.5ms LOW
  pwm_enable(pwm);  // START CONTINUOUS CLOCK
  /*
  thread = kthread_run(nesjoy_thread_fn, dev, "joy rasp");
  if (IS_ERR(thread)) {
    int err = PTR_ERR(thread);
    thread = NULL;
    return err;
  }
  */
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
