#include "gpio_led.h"

struct gpio_led_device *gpio_led_dev = NULL;



static int gpio_led_dev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = gpio_led_dev;

	return 0;
}

static long gpio_led_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
	struct device *dev = &gpio_led_dev->pdev->dev;
    void __user *argp = (void __user *)arg;
    unsigned char state = 0;

	/* 检查设备类型 */
    if (_IOC_TYPE(cmd) != IOC_LED_MAGIC) 
	{
        dev_err(dev,"%s: error: command type [%c] error\n", __func__, _IOC_TYPE(cmd));
        return -ENOTTY; 
    }

	/* 检查序数 */
    if (_IOC_NR(cmd) > IOC_LED_MAX_NUM) 
	{ 
        dev_err(dev,"%s: error: command numer [%d] exceeded\n", __func__, _IOC_NR(cmd));
        return -ENOTTY;
    }

	switch(cmd)
	{
		case IOC_W_LED1_ON:
            gpio_set_value(gpio_led_dev->gpio_led1,1);
		break;

        case IOC_W_LED1_OFF:
            gpio_set_value(gpio_led_dev->gpio_led1,0);
		break;

        case IOC_W_LED2_ON:
            gpio_set_value(gpio_led_dev->gpio_led2,1);
		break;

        case IOC_W_LED2_OFF:
            gpio_set_value(gpio_led_dev->gpio_led2,0);
		break;

        case IOC_W_LED3_ON:
            gpio_set_value(gpio_led_dev->gpio_led3,1);
		break;

        case IOC_W_LED3_OFF:
            gpio_set_value(gpio_led_dev->gpio_led3,0);
		break;

        case IOC_W_LED4_ON:
            gpio_set_value(gpio_led_dev->gpio_led4,1);
		break;

        case IOC_W_LED4_OFF:
            gpio_set_value(gpio_led_dev->gpio_led4,0);
		break;

        case IOC_W_LED_ALL_ON:
            gpio_set_value(gpio_led_dev->gpio_led1,1);
            gpio_set_value(gpio_led_dev->gpio_led2,1);
            gpio_set_value(gpio_led_dev->gpio_led3,1);
            gpio_set_value(gpio_led_dev->gpio_led4,1);
		break;

        case IOC_W_LED_ALL_OFF:
            gpio_set_value(gpio_led_dev->gpio_led1,0);
            gpio_set_value(gpio_led_dev->gpio_led2,0);
            gpio_set_value(gpio_led_dev->gpio_led3,0);
            gpio_set_value(gpio_led_dev->gpio_led4,0);
		break;

        case IOC_W_LED_ALL_STATE:
            ret = copy_from_user(&state, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(dev,"%s: error: ioctl read led state failed\n", __func__);
				return -EFAULT;
			}

            if(state > 0x0F)
            {
                dev_err(dev,"%s: error: ioctl read state value error,state = %02X\n", __func__,state);
				return -EFAULT;
            }

            if(state & 0x01)
            {
                gpio_set_value(gpio_led_dev->gpio_led1,1);
            }
            else
            {
                gpio_set_value(gpio_led_dev->gpio_led1,0);
            }
            if(state & 0x02)
            {
                gpio_set_value(gpio_led_dev->gpio_led2,1);
            }
            else
            {
                gpio_set_value(gpio_led_dev->gpio_led2,0);
            }
            if(state & 0x04)
            {
                gpio_set_value(gpio_led_dev->gpio_led3,1);
            }
            else
            {
                gpio_set_value(gpio_led_dev->gpio_led3,0);
            }
            if(state & 0x08)
            {
                gpio_set_value(gpio_led_dev->gpio_led4,1);
            }
            else
            {
                gpio_set_value(gpio_led_dev->gpio_led4,0);
            }
        break;

		default:
        break;
	}

	return 0;
}

static struct file_operations gpio_led_dev_fops = {
	.owner  	    = THIS_MODULE,
	.open   	    = gpio_led_dev_open,
	.unlocked_ioctl = gpio_led_dev_ioctl,
};

static int led_gpio_request(struct platform_device *pdev,struct gpio_led_device *gpio_led_dev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;

	gpio_led_dev->gpio_led1 = of_get_named_gpio(dev->of_node, "gpio-led1", 0);
	if(!gpio_is_valid(gpio_led_dev->gpio_led1)) 
	{
		dev_err(dev, "get gpio_led1 failed\n");
		return -EINVAL;
	}

    gpio_led_dev->gpio_led2 = of_get_named_gpio(dev->of_node, "gpio-led2", 0);
	if(!gpio_is_valid(gpio_led_dev->gpio_led2)) 
	{
		dev_err(dev, "get gpio_led2 failed\n");
		goto error0;
	}

    gpio_led_dev->gpio_led3 = of_get_named_gpio(dev->of_node, "gpio-led3", 0);
	if(!gpio_is_valid(gpio_led_dev->gpio_led3)) 
	{
		dev_err(dev, "get gpio_led3 failed\n");
		goto error1;
	}

    gpio_led_dev->gpio_led4 = of_get_named_gpio(dev->of_node, "gpio-led4", 0);
	if(!gpio_is_valid(gpio_led_dev->gpio_led4)) 
	{
		dev_err(dev, "get gpio_led4 failed\n");
		goto error2;
	}

	ret = gpio_request(gpio_led_dev->gpio_led1, "gpio_led1");
	if (ret)
	{
		dev_err(dev, "request gpio_led1 failed\n");
		return -EINVAL;
	}

    ret = gpio_request(gpio_led_dev->gpio_led2, "gpio_led2");
	if (ret)
	{
		dev_err(dev, "request gpio_led2 failed\n");
		return -EINVAL;
	}

    ret = gpio_request(gpio_led_dev->gpio_led3, "gpio_led3");
	if (ret)
	{
		dev_err(dev, "request gpio_led3 failed\n");
		return -EINVAL;
	}

    ret = gpio_request(gpio_led_dev->gpio_led4, "gpio_led4");
	if (ret)
	{
		dev_err(dev, "request gpio_led4 failed\n");
		return -EINVAL;
	}

	gpio_direction_output(gpio_led_dev->gpio_led1,0);
    gpio_direction_output(gpio_led_dev->gpio_led2,0);
    gpio_direction_output(gpio_led_dev->gpio_led3,0);
    gpio_direction_output(gpio_led_dev->gpio_led4,0);

	return ret;

error2:
    gpio_free(gpio_led_dev->gpio_led3);

error1:
    gpio_free(gpio_led_dev->gpio_led2);

error0:
    gpio_free(gpio_led_dev->gpio_led1);

    return -EINVAL;
}

static int gpio_led_probe(struct platform_device *pdev)
{
    int ret = 0;
	struct device *dev = &pdev->dev;

	printk("hello: gpio led device probing\n");

	gpio_led_dev = (struct gpio_led_device *) kmalloc(sizeof(struct gpio_led_device), GFP_KERNEL);
	if (!gpio_led_dev) 
    {
		dev_err(dev, "Cound not allocate gpio_led_dev device\n");
		return -ENOMEM;
	}

    memset(gpio_led_dev,0,sizeof(struct gpio_led_device));

    gpio_led_dev->pdev = pdev;

	dev_set_drvdata(dev, gpio_led_dev);

    ret = led_gpio_request(pdev,gpio_led_dev);
    if(ret)
	{
		dev_err(dev, "request gpio failed.\n");
		goto error0;
	}

	ret = alloc_chrdev_region(&gpio_led_dev->devid, 0, DEVICE_NUMBER, DEVICE_NAME);
	if(ret)
	{
		dev_err(dev, "alloc chrdev region failed.\n");
		goto error0;
	}

	gpio_led_dev->cdev.owner = THIS_MODULE;
	cdev_init(&gpio_led_dev->cdev, &gpio_led_dev_fops);

	ret = cdev_add(&gpio_led_dev->cdev, gpio_led_dev->devid, DEVICE_NUMBER);
	if(ret)
	{
		dev_err(dev, "cdev add failed.\n");
		goto error1;
	}

	gpio_led_dev->class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(gpio_led_dev->class)) 
	{
		dev_err(dev, "class create failed.\n");
		ret = PTR_ERR(gpio_led_dev->class);
		goto error2;
	}

    gpio_led_dev->device = device_create(gpio_led_dev->class, &pdev->dev,gpio_led_dev->devid, NULL, DEVICE_NAME);
	if (IS_ERR(gpio_led_dev->device)) 
	{
		dev_err(dev, "device create failed.\n");
		ret = PTR_ERR(gpio_led_dev->device);
		goto error3;
	}

	return 0;

error3:
	class_destroy(gpio_led_dev->class);
error2:
    cdev_del(&gpio_led_dev->cdev);
error1:
	unregister_chrdev_region(gpio_led_dev->devid, DEVICE_NUMBER);
error0:
	dev_set_drvdata(dev, NULL);

	return -EINVAL;
}

static int gpio_led_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct gpio_led_device *gpio_led_dev = dev_get_drvdata(dev);

	dev_set_drvdata(dev, NULL);
	
	device_destroy(gpio_led_dev->class, gpio_led_dev->devid);

	class_destroy(gpio_led_dev->class);

	cdev_del(&gpio_led_dev->cdev);

	unregister_chrdev_region(gpio_led_dev->devid, 1);

	gpio_free(gpio_led_dev->gpio_led1);
    gpio_free(gpio_led_dev->gpio_led2);
    gpio_free(gpio_led_dev->gpio_led3);
    gpio_free(gpio_led_dev->gpio_led4);

	return 0;
}

static struct of_device_id gpio_led_of_match[] = {
	{ .compatible = "eyestar,gpio-led", },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, gpio_led_of_match);

static struct platform_driver gpio_led_driver = {
	.driver = {
		.name			= DEVICE_NAME,
		.owner 			= THIS_MODULE,
		.of_match_table	= gpio_led_of_match,
	},
	.probe		= gpio_led_probe,
	.remove		= gpio_led_remove,
};

module_platform_driver(gpio_led_driver);

MODULE_AUTHOR("EyeStar");
MODULE_DESCRIPTION("Quectel ec20 power driver");
MODULE_LICENSE("GPL v2");
