#ifndef __EC20_POWER_H
#define __EC20_POWER_H

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
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>



#define DEVICE_NAME			            "ec20_power"
#define DEVICE_NUMBER		            1


#define STATUS_POWER_OFF                0
#define STATUS_POWER_ON                 1

#define SET_POWER_OFF                   0
#define SET_POWER_ON                    1
#define SET_FORCE_POWER_ON              2
#define SET_RESET                       3
#define SET_DEFAULT                     255

#define IOC_MAGIC  'E'
#define IOC_INIT                        _IO(IOC_MAGIC, 0)
#define IOC_R_STATUS                    _IOW(IOC_MAGIC, 1, int)
#define IOC_W_STATUS                    _IOR(IOC_MAGIC, 2, int)
#define IOC_MAX_NUM  3



struct ec20_power_device {
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *of_node;
    struct platform_device *pdev;
	int pwr_en_gpio;
    int pwr_key_gpio;
    int reset_gpio;

};


void ec20_reset(void);
void ec20_pwr_on(void);
void ec20_pwr_off(void);
void ec20_force_pwr_on(void);


#endif
