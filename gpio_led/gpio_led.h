#ifndef __GPIO_LED_H
#define __GPIO_LED_H

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/bcd.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/ioctl.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/ide.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>



#define DEVICE_NAME			            "gpio_led"
#define DEVICE_NUMBER		            1


#define STATUS_POWER_OFF                0
#define STATUS_POWER_ON                 1


#define IOC_LED_MAGIC                   'L'
#define IOC_LED_INIT                    _IO(IOC_LED_MAGIC, 0)
#define IOC_W_LED1_ON                   _IOW(IOC_LED_MAGIC, 1, int)
#define IOC_W_LED1_OFF                  _IOW(IOC_LED_MAGIC, 2, int)
#define IOC_W_LED2_ON                   _IOW(IOC_LED_MAGIC, 3, int)
#define IOC_W_LED2_OFF                  _IOW(IOC_LED_MAGIC, 4, int)
#define IOC_W_LED3_ON                   _IOW(IOC_LED_MAGIC, 5, int)
#define IOC_W_LED3_OFF                  _IOW(IOC_LED_MAGIC, 6, int)
#define IOC_W_LED4_ON                   _IOW(IOC_LED_MAGIC, 7, int)
#define IOC_W_LED4_OFF                  _IOW(IOC_LED_MAGIC, 8, int)
#define IOC_W_LED_ALL_ON                _IOW(IOC_LED_MAGIC, 9, int)
#define IOC_W_LED_ALL_OFF               _IOW(IOC_LED_MAGIC, 10, int)
#define IOC_W_LED_ALL_STATE             _IOW(IOC_LED_MAGIC, 11, int)
#define IOC_LED_MAX_NUM                 11



struct gpio_led_device {
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *of_node;
    struct platform_device *pdev;
	int gpio_led1;
    int gpio_led2;
    int gpio_led3;
    int gpio_led4;

};


#endif
