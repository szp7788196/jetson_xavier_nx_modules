#include "ec20_power.h"

struct ec20_power_device *ec20_power_dev = NULL;
static atomic_t ec20PowerStatus = ATOMIC_INIT(STATUS_POWER_OFF);
static int forceSetEc20PowerStatus = SET_FORCE_POWER_ON;
static struct task_struct *operate_task;
static struct completion comp;

void ec20_reset(void)
{
	gpio_set_value(ec20_power_dev->reset_gpio, 1);

	mdelay(600);

	gpio_set_value(ec20_power_dev->reset_gpio, 0);
}

void ec20_pwr_on(void)
{
	mdelay(100);

	gpio_set_value(ec20_power_dev->pwr_key_gpio, 1);

	mdelay(600);

	gpio_set_value(ec20_power_dev->pwr_key_gpio, 0);

	mdelay(2500);
}

void ec20_pwr_off(void)
{
	gpio_set_value(ec20_power_dev->pwr_key_gpio, 0);

	mdelay(50);

	gpio_set_value(ec20_power_dev->pwr_key_gpio, 1);

	mdelay(800);

	gpio_set_value(ec20_power_dev->pwr_key_gpio, 0);
}

void ec20_force_pwr_on(void)
{
	ec20_reset();

	mdelay(1000);

	ec20_pwr_on();
}

static int ec20_power_dev_force_operate_func(void *data)
{
    while(!kthread_should_stop())
    {
        wait_for_completion(&comp);

        switch(forceSetEc20PowerStatus)
        {
            case (int)SET_POWER_OFF:
                ec20_pwr_off();
                atomic_set(&ec20PowerStatus, STATUS_POWER_OFF);
            break;

            case (int)SET_POWER_ON:
                ec20_pwr_on();
                atomic_set(&ec20PowerStatus, STATUS_POWER_ON);
            break;

            case (int)SET_FORCE_POWER_ON:
                ec20_force_pwr_on();
                atomic_set(&ec20PowerStatus, STATUS_POWER_ON);
            break;

            case (int)SET_RESET:
                ec20_reset();
                atomic_set(&ec20PowerStatus, STATUS_POWER_ON);
            break;

            default:
            break;
        }
    }

    return 0;
}

static int ec20_power_dev_open(struct inode *inode, struct file *filp)
{
	filp->private_data = ec20_power_dev;

	return 0;
}

static ssize_t ec20_power_dev_read(struct file *filp, char __user *buf,size_t cnt, loff_t *off)
{
	int ret = 0;
    struct device *dev = &ec20_power_dev->pdev->dev;
	int ec20_power_status = STATUS_POWER_OFF;

    if(cnt != sizeof(int))
    {
        dev_err(dev, "read cnt error\n");
        return -EFAULT;
    }

    ec20_power_status = atomic_read(&ec20PowerStatus);

	ret = copy_to_user(buf, &ec20_power_status, cnt);
	if(ret != 0)
	{
		dev_err(dev, "Failed to copy data to user buffer\n");
		return -EFAULT;
	}

	return ret;
}

static ssize_t ec20_power_dev_write(struct file *filp, const char __user *buf,size_t cnt, loff_t *offt)
{
	int ret = 0;
    struct device *dev = &ec20_power_dev->pdev->dev;
	int force_set_ec20_power_status = SET_POWER_OFF;

    if(cnt != sizeof(int))
    {
        dev_err(dev, "write cnt error\n");
        return -EFAULT;
    }

	ret = copy_from_user(&force_set_ec20_power_status, buf, cnt);
	if(ret < 0)
	{
		dev_err(dev, "Failed to copy data from user buffer\n");
		return -EFAULT;
	}

    if(force_set_ec20_power_status > SET_RESET)
    {
        dev_err(dev, "write ec20 power status error,should be 0 or 1 or 2\n");
        return -EFAULT;
    }

    forceSetEc20PowerStatus = force_set_ec20_power_status;

    complete(&comp);

	return 0;
}

static long ec20_power_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct device *dev = &ec20_power_dev->pdev->dev;
	void __user *argp = (void __user *)arg;
    int ec20_power_status = STATUS_POWER_OFF;
    int force_set_ec20_power_status = SET_POWER_OFF;

	/* 检查设备类型 */
    if (_IOC_TYPE(cmd) != IOC_MAGIC) 
	{
        dev_err(dev,"%s: error: command type [%c] error\n", __func__, _IOC_TYPE(cmd));
        return -ENOTTY; 
    }

	/* 检查序数 */
    if (_IOC_NR(cmd) > IOC_MAX_NUM) 
	{ 
        dev_err(dev,"%s: error: command numer [%d] exceeded\n", __func__, _IOC_NR(cmd));
        return -ENOTTY;
    }

	switch(cmd)
	{
		case IOC_R_STATUS:
            ec20_power_status = atomic_read(&ec20PowerStatus);

			ret = copy_to_user(argp, &ec20_power_status, sizeof(int));
			
			if(ret)
			{
				dev_err(dev,"%s: error: ioctl read sensor data failed\n", __func__);
				return -EFAULT;
			}
		break;

		case IOC_W_STATUS:
			ret = copy_from_user(&force_set_ec20_power_status, argp, sizeof(int));
			if(ret)
			{
				dev_err(dev,"%s: error: ioctl read config data failed\n", __func__);
				return -EFAULT;
			}

            if(force_set_ec20_power_status > SET_RESET)
            {
                dev_err(dev, "ioctl write ec20 power status error,should be 0 or 1 or 2\n");
                return -EFAULT;
            }

            forceSetEc20PowerStatus = force_set_ec20_power_status;

            complete(&comp);
		break;
	}

	return ret;
}

static struct file_operations ec20_power_dev_fops = {
	.owner  	    = THIS_MODULE,
	.open   	    = ec20_power_dev_open,
    .read   	    = ec20_power_dev_read,
	.write  	    = ec20_power_dev_write,
	.unlocked_ioctl = ec20_power_dev_ioctl,
};

static int ec20_gpio_request(struct platform_device *pdev,struct ec20_power_device *ec20_power_dev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;

	ec20_power_dev->pwr_en_gpio = of_get_named_gpio(dev->of_node, "pwr-en-gpio", 0);
	if(!gpio_is_valid(ec20_power_dev->pwr_en_gpio)) 
	{
		dev_err(dev, "get pwr-en-gpio failed\n");
		return -EINVAL;
	}

    ec20_power_dev->pwr_key_gpio = of_get_named_gpio(dev->of_node, "pwr-key-gpio", 0);
	if(!gpio_is_valid(ec20_power_dev->pwr_key_gpio)) 
	{
		dev_err(dev, "get pwr-key-gpio failed\n");
		return -EINVAL;
	}

    ec20_power_dev->reset_gpio = of_get_named_gpio(dev->of_node, "reset-gpio", 0);
	if(!gpio_is_valid(ec20_power_dev->reset_gpio)) 
	{
		dev_err(dev, "get reset-gpio failed\n");
		return -EINVAL;
	}

	ret = gpio_request(ec20_power_dev->pwr_en_gpio, "pwr-en-gpio");
	if (ret)
	{
		dev_err(dev, "request pwr-en-gpio failed\n");
		return -EINVAL;
	}

    ret = gpio_request(ec20_power_dev->pwr_key_gpio, "pwr-key-gpio");
	if (ret)
	{
		dev_err(dev, "request pwr-key-gpio failed\n");
		goto error0;
	}

    ret = gpio_request(ec20_power_dev->reset_gpio, "reset-gpio");
	if (ret)
	{
		dev_err(dev, "request reset-gpio failed\n");
		goto error1;
	}

	gpio_direction_output(ec20_power_dev->pwr_en_gpio,0);
    gpio_direction_output(ec20_power_dev->pwr_key_gpio,0);
    gpio_direction_output(ec20_power_dev->reset_gpio,0);

	return ret;

error1:
    gpio_free(ec20_power_dev->pwr_key_gpio);

error0:
    gpio_free(ec20_power_dev->pwr_en_gpio);

    return -EINVAL;
}

static int ec20_power_probe(struct platform_device *pdev)
{
    int ret = 0;
	struct device *dev = &pdev->dev;

	printk("hello: ec20 power device probing\n");

	ec20_power_dev = (struct ec20_power_device *) kmalloc(sizeof(struct ec20_power_device), GFP_KERNEL);
	if (!ec20_power_dev) 
    {
		dev_err(dev, "Cound not allocate ec20_power_dev device\n");
		return -ENOMEM;
	}

    memset(ec20_power_dev,0,sizeof(struct ec20_power_device));

    ec20_power_dev->pdev = pdev;

	dev_set_drvdata(dev, ec20_power_dev);

    ret = ec20_gpio_request(pdev,ec20_power_dev);
    if(ret)
	{
		dev_err(dev, "request gpio failed.\n");
		goto error0;
	}

	ret = alloc_chrdev_region(&ec20_power_dev->devid, 0, DEVICE_NUMBER, DEVICE_NAME);
	if(ret)
	{
		dev_err(dev, "alloc chrdev region failed.\n");
		goto error0;
	}

	ec20_power_dev->cdev.owner = THIS_MODULE;
	cdev_init(&ec20_power_dev->cdev, &ec20_power_dev_fops);

	ret = cdev_add(&ec20_power_dev->cdev, ec20_power_dev->devid, DEVICE_NUMBER);
	if(ret)
	{
		dev_err(dev, "cdev add failed.\n");
		goto error1;
	}

	ec20_power_dev->class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(ec20_power_dev->class)) 
	{
		dev_err(dev, "class create failed.\n");
		ret = PTR_ERR(ec20_power_dev->class);
		goto error2;
	}

    ec20_power_dev->device = device_create(ec20_power_dev->class, &pdev->dev,ec20_power_dev->devid, NULL, DEVICE_NAME);
	if (IS_ERR(ec20_power_dev->device)) 
	{
		dev_err(dev, "device create failed.\n");
		ret = PTR_ERR(ec20_power_dev->device);
		goto error3;
	}

    init_completion(&comp);

    operate_task = kthread_run(ec20_power_dev_force_operate_func, NULL, "mythread");
    if(IS_ERR(operate_task))
    {
        dev_err(dev, "operate_task create failed.\n");
        goto error3;
    }

	complete(&comp);

	return 0;

error3:
	class_destroy(ec20_power_dev->class);
error2:
    cdev_del(&ec20_power_dev->cdev);
error1:
	unregister_chrdev_region(ec20_power_dev->devid, DEVICE_NUMBER);
error0:
	dev_set_drvdata(dev, NULL);

	return -EINVAL;
}

static int ec20_power_remove(struct platform_device *pdev)
{
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct ec20_power_device *ec20_power_dev_ = dev_get_drvdata(dev);

	if(!IS_ERR(operate_task))
    {
		forceSetEc20PowerStatus = SET_DEFAULT;

		complete(&comp);

        ret = kthread_stop(operate_task);

        printk(KERN_INFO "operate_task thread function has stopped ,return %d\n", ret);  
    }

	dev_set_drvdata(dev, NULL);
	
	device_destroy(ec20_power_dev_->class, ec20_power_dev_->devid);

	class_destroy(ec20_power_dev_->class);

	cdev_del(&ec20_power_dev_->cdev);

	unregister_chrdev_region(ec20_power_dev_->devid, 1);

	gpio_free(ec20_power_dev_->pwr_en_gpio);
    gpio_free(ec20_power_dev_->pwr_key_gpio);
    gpio_free(ec20_power_dev_->reset_gpio);

	return 0;
}

static struct of_device_id ec20_power_of_match[] = {
	{ .compatible = "eyestar,ec20-power", },
	{ /* Sentinel */ },
};

MODULE_DEVICE_TABLE(of, ec20_power_of_match);

static struct platform_driver ec20_power_driver = {
	.driver = {
		.name			= DEVICE_NAME,
		.owner 			= THIS_MODULE,
		.of_match_table	= ec20_power_of_match,
	},
	.probe		= ec20_power_probe,
	.remove		= ec20_power_remove,
};

module_platform_driver(ec20_power_driver);

/*
static int __init ec20_power_device_init(void)
{
	printk("ec20_power_device:Hello.\n");
	return platform_driver_register(&ec20_power_driver);
}

static void __exit ec20_power_device_exit(void)
{
	platform_driver_unregister(&ec20_power_driver);
	printk("ec20_power_device:Goodbye.\n");
}

module_init(ec20_power_device_init);
module_exit(ec20_power_device_exit);
*/

MODULE_AUTHOR("EyeStar:szp");
MODULE_DESCRIPTION("Quectel ec20 power driver");
MODULE_LICENSE("GPL v2");
