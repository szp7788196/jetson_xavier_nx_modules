#include "mpu9250.h"


static struct mpu9250_device mpu9250_Dev;			//设备驱动结构体
static struct mpu9250_sample_data sampleData;		//采集数据结构体
static int GyroRange = 0;							//陀螺仪范围
static int AccelRange = 3;							//加速度计范围
static int sampleRate = 125;						//采样频率
static int readMode = 0;							//采集模式
static int intervalTime = 0;						//采集时间间隔
//static int memoryPutIndex = 0;						//内存采集index
//static int memoryGutIndex = 0;						//内存读取index
struct task_struct *read_task;
static struct completion comp;
atomic_t have_new_data;


static int mpu9250_auto_read_func(void *data)
{
	int ret = 0;
	unsigned char int_status = 0;
	struct i2c_client *client = mpu9250_Dev.client;

    while(!kthread_should_stop())
    {
        wait_for_completion(&comp);

		if(readMode == 1)
		{
			ret = mpu9250_read_reg(&mpu9250_Dev, MPU_INT_STA_REG, &int_status, 1);		//读取mpu9250中断状态寄存器
			if(ret)
			{
				dev_err(&client->dev, "%s: error: mpu9250 get MPU_INT_STA_REG failed 1\n",__func__);
			}
		}

		/* 获取互斥体,可以被信号打断 */
		ret = mutex_lock_interruptible(&mpu9250_Dev.data_lock);
		if(ret)
		{
			dev_err(&client->dev, "%s: error: mutex_lock failed 1\n",__func__);
			return -ERESTARTSYS;
		}

		ret = mpu9250_get_accel_temp_gyro_data();			//读取加速度、温度、陀螺仪数据
		if(ret)
		{
			dev_err(&client->dev, "%s: error: mpu9250 get accel_temp_gyro_data failed 1\n",__func__);
		}

		ret = mpu9250_get_magnetometer_data();				//读取地磁数据
		if(ret)
		{
			dev_err(&client->dev, "%s: error: mpu9250 get magnetometer failed 1\n",__func__);
		}

		memcpy(mpu9250_Dev.virt_addr,&sampleData,sizeof(struct mpu9250_sample_data));

		mutex_unlock(&mpu9250_Dev.data_lock);

		atomic_set(&have_new_data, 1);
		wake_up_interruptible(&mpu9250_Dev.r_wait);
    }

    return 0;
}

static irqreturn_t mpu9250_device_irq(int irq, void *mpu9250_device)
{
	complete(&comp);

	return IRQ_HANDLED;
}

static void mpu9250_timer_function(unsigned long arg)
{
	mod_timer(&mpu9250_Dev.timer, jiffies + msecs_to_jiffies(intervalTime));

	complete(&comp);
}

static int mpu9250_write_reg(struct mpu9250_device *dev, u8 reg, u8 data)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg;
	unsigned char send_buf[2] = {0};
	int ret;

	send_buf[0] = reg;				// 寄存器首地址
	send_buf[1] = data;				// 将要写入的数据存放到数组send_buf后面

	msg.addr 	= client->addr;		// mpu9250从机地址
	msg.flags 	= 0;				// 标记为写数据
	msg.buf 	= send_buf;			// 要写入的数据缓冲区
	msg.len 	= 1 + 1;			// 要写入的数据长度

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (1 != ret)
	{
		dev_err(&client->dev, "%s: error: reg=0x%x, data=0x%x\n",__func__, reg, data);
		return -EIO;
	}

	return 0;
}

static int mpu9250_read_reg(struct mpu9250_device *dev, u8 reg, u8 *buf, u8 len)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg[2];
	int ret;

	/* msg[0]: 发送消息 */
	msg[0].addr 	= client->addr;		// mpu9250从机地址
	msg[0].flags 	= 0;				// 标记为写数据
	msg[0].buf 		= &reg;				// 要写入的数据缓冲区
	msg[0].len 		= 1;				// 要写入的数据长度

	/* msg[1]: 接收消息 */
	msg[1].addr 	= client->addr;		// mpu9250从机地址
	msg[1].flags 	= I2C_M_RD;			// 标记为读数据
	msg[1].buf 		= buf;				// 存放读数据的缓冲区
	msg[1].len 		= len;				// 读取的字节长度

	ret = i2c_transfer(client->adapter, msg, 2);
	if (2 != ret)
	{
		dev_err(&client->dev, "%s: error: reg=0x%x, len=0x%x\n",__func__, reg, len);
		return -EIO;
	}

	return 0;
}

static int mpu9250_ak8963_write_reg(struct mpu9250_device *dev, u8 reg, u8 data)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg;
	u8 send_buf[2] = {0};
	int ret;

	send_buf[0] = reg;				// 寄存器首地址
	send_buf[1] = data;				// 将要写入的数据存放到数组send_buf后面

	msg.addr 	= 0x0C;				// mpu9250从机地址
	msg.flags 	= 0;				// 标记为写数据
	msg.buf 	= send_buf;			// 要写入的数据缓冲区
	msg.len 	= 1 + 1;			// 要写入的数据长度

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (1 != ret)
	{
		dev_err(&client->dev, "%s: error: reg=0x%x, data=0x%x\n",__func__, reg, data);
		return -EIO;
	}

	return 0;
}

static int mpu9250_ak8963_read_reg(struct mpu9250_device *dev, u8 reg, u8 *buf, u8 len)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg[2];
	int ret;

	/* msg[0]: 发送消息 */
	msg[0].addr 	= 0x0C;				// ak8963从机地址
	msg[0].flags 	= 0;				// 标记为写数据
	msg[0].buf 		= &reg;				// 要写入的数据缓冲区
	msg[0].len 		= 1;				// 要写入的数据长度

	/* msg[1]: 接收消息 */
	msg[1].addr 	= 0x0C;				// ak8963从机地址
	msg[1].flags 	= I2C_M_RD;			// 标记为读数据
	msg[1].buf 		= buf;				// 存放读数据的缓冲区
	msg[1].len 		= len;				// 读取的字节长度

	ret = i2c_transfer(client->adapter, msg, 2);
	if (2 != ret)
	{
		dev_err(&client->dev, "%s: error: reg=0x%x, len=0x%x\n",__func__, reg, len);
		return -EIO;
	}

	return 0;
}

//读取加速度计 温度 陀螺仪数据
static int mpu9250_get_accel_temp_gyro_data(void)
{
	int ret = 0;
	struct i2c_client *client = mpu9250_Dev.client;
	unsigned char buf[14] = {0};

	ret = mpu9250_read_reg(&mpu9250_Dev, MPU_ACCEL_XOUTH_REG, buf, 14);
	if(ret)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_GYRO_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	sampleData.accel_x 		= (short)(((((unsigned short)buf[0]) << 8) & 0xFF00) + (unsigned short)buf[1]);
	sampleData.accel_y 		= (short)(((((unsigned short)buf[2]) << 8) & 0xFF00) + (unsigned short)buf[3]);
	sampleData.accel_z 		= (short)(((((unsigned short)buf[4]) << 8) & 0xFF00) + (unsigned short)buf[5]);

	sampleData.temperature	= (short)(((((unsigned short)buf[6]) << 8) & 0xFF00) + (unsigned short)buf[7]);
//	sampleData.temperature 	= (float)(sampleData.temperature - 21) / 333.87f + 21.0f;

	sampleData.gyro_x 		= (short)(((((unsigned short)buf[8]) << 8) & 0xFF00) + (unsigned short)buf[9]);
	sampleData.gyro_y 		= (short)(((((unsigned short)buf[10]) << 8) & 0xFF00) + (unsigned short)buf[11]);
	sampleData.gyro_z 		= (short)(((((unsigned short)buf[12]) << 8) & 0xFF00) + (unsigned short)buf[13]);

	return ret;
}

//读取磁力计数据
static int mpu9250_get_magnetometer_data(void)
{
	int ret = 0;
	struct i2c_client *client = mpu9250_Dev.client;
	unsigned char buf[6] = {0};

	ret = mpu9250_ak8963_read_reg(&mpu9250_Dev, AK_HXL_REG, buf, 6);
	if(ret)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_GYRO_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	sampleData.magne_x 	= (short)(((((unsigned short)buf[1]) << 8) & 0xFF00) + (unsigned short)buf[0]);
	sampleData.magne_y 	= (short)(((((unsigned short)buf[3]) << 8) & 0xFF00) + (unsigned short)buf[2]);
	sampleData.magne_z 	= (short)(((((unsigned short)buf[5]) << 8) & 0xFF00) + (unsigned short)buf[4]);

	//当ak8963循环读取模式或外部触发读取模式时，数据读取结束后必须读取AK_STATUS2_REG寄存器，否则数据不会更新
	ret = mpu9250_ak8963_read_reg(&mpu9250_Dev, AK_STATUS2_REG, buf, 1);
	if(ret)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_GYRO_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	return ret;
}

static int mpu9250_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &mpu9250_Dev;

	return 0;
}

static ssize_t mpu9250_read(struct file *filp, char __user *buf,size_t cnt, loff_t *off)
{
	int ret = 0;
	struct i2c_client *client = mpu9250_Dev.client;

	if(cnt != sizeof(struct mpu9250_sample_data))
	{
		dev_err(&client->dev, "%s: error: read length error\n",__func__);
		return -EFAULT;
	}

	wait_event_interruptible(mpu9250_Dev.r_wait, atomic_read(&have_new_data));

	ret = mutex_lock_interruptible(&mpu9250_Dev.data_lock);
	if(ret)
	{
		dev_err(&client->dev, "%s: error: mutex_lock failed 3\n",__func__);
		return -ERESTARTSYS;
	}

	if(readMode == 0)
	{
		mpu9250_get_accel_temp_gyro_data();
		mpu9250_get_magnetometer_data();
	}

	ret = copy_to_user(buf, &sampleData, cnt);
	mutex_unlock(&mpu9250_Dev.data_lock);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: copy data to user buffer failed\n",__func__);
		return -EFAULT;
	}

	atomic_set(&have_new_data, 0);

	return ret;
}

static long mpu9250_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct i2c_client *client = mpu9250_Dev.client;
	void __user *argp = (void __user *)arg;
	struct mpu9250_config mpu9250_cfg;

	/* 检查设备类型 */
    if (_IOC_TYPE(cmd) != IOC_MAGIC) 
	{
        dev_err(&client->dev,"%s: error: command type [%c] error\n", __func__, _IOC_TYPE(cmd));
        return -EPERM; 
    }

	/* 检查序数 */
    if (_IOC_NR(cmd) > IOC_MAX_NUM) 
	{ 
        dev_err(&client->dev,"%s: error: command numer [%d] exceeded\n", __func__, _IOC_NR(cmd));
        return -EPERM;
    }

	switch(cmd)
	{
		case IOC_R_DATA:
			ret = mutex_lock_interruptible(&mpu9250_Dev.data_lock);
			if(ret)
			{
				dev_err(&client->dev, "%s: error: mutex_lock failed 4\n",__func__);
				return -ERESTARTSYS;
			}
			ret = copy_to_user(argp, &sampleData, sizeof(struct mpu9250_sample_data));
			mutex_unlock(&mpu9250_Dev.data_lock);
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl read sensor data failed\n", __func__);
				return -EIO;
			}
		break;

		case IOC_W_CONFIG:
			ret = copy_from_user(&mpu9250_cfg, argp, sizeof(struct mpu9250_config));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl read config data failed\n", __func__);
				return -EIO;
			}

			if(mpu9250_cfg.gyro_range  < 0 || mpu9250_cfg.gyro_range  > 3 || 
			   mpu9250_cfg.accel_range < 0 || mpu9250_cfg.accel_range > 3 ||
			   mpu9250_cfg.sample_rate < 4 || mpu9250_cfg.sample_rate > 1000 ||
			   mpu9250_cfg.read_mode   < 0 || mpu9250_cfg.read_mode   > 2)
			{
				dev_err(&client->dev,"%s: error: ioctl config data invalid\n", __func__);
				return -EPERM;
			}

			ret = mpu9250_sensor_init(mpu9250_cfg.gyro_range,mpu9250_cfg.accel_range,mpu9250_cfg.sample_rate,mpu9250_cfg.read_mode);
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl config mpu9250 failed\n", __func__);
				return -EPERM;
			}
		break;
	}

	return ret;
}

static int mpu9250_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct i2c_client *client = mpu9250_Dev.client;
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	unsigned long pfn_start = (virt_to_phys(mpu9250_Dev.virt_addr) >> PAGE_SHIFT) + vma->vm_pgoff;
	unsigned long virt_start = (unsigned long)mpu9250_Dev.virt_addr + offset;
	unsigned long size = vma->vm_end - vma->vm_start;
	int ret = 0;

	dev_info(&client->dev,"phy: 0x%lx, offset: 0x%lx, size: 0x%lx\n", pfn_start << PAGE_SHIFT, offset, size);

	ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
	if (ret)
	{
		dev_err(&client->dev,"%s: error: remap_pfn_range failed at [0x%lx  0x%lx]\n",__func__, vma->vm_start, vma->vm_end);
	}
	else
	{
		dev_info(&client->dev,"%s: map 0x%lx to 0x%lx, size: 0x%lx\n", __func__, virt_start,vma->vm_start, size);
	}

	return ret;
}

 /*
  * @description     : poll函数，用于处理非阻塞访问
  * @param - filp    : 要打开的设备文件(文件描述符)
  * @param - wait    : 等待列表(poll_table)
  * @return          : 设备或者资源状态，
  */
unsigned int mpu9250_poll(struct file *filp, struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &mpu9250_Dev.r_wait, wait);	/* 将等待队列头添加到poll_table中 */
	
	if(atomic_read(&have_new_data))
	{
		mask = POLLIN | POLLRDNORM;			/* 返回PLLIN */
	}
	
	return mask;
}

static int mpu9250_release(struct inode *inode, struct file *filp)
{
	/* 释放互斥锁 */
	mutex_unlock(&mpu9250_Dev.data_lock);

	return 0;
}

static const struct file_operations mpu9250_dev_ops = {
	.owner 			= THIS_MODULE,
	.open 			= mpu9250_open,
	.read 			= mpu9250_read,
	.unlocked_ioctl = mpu9250_ioctl,
	.mmap			= mpu9250_mmap,
	.poll 			= mpu9250_poll,
	.release 		= mpu9250_release,
};

/*
*	gyro_range	:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
*	accel_range	:0,±2g;1,±4g;2,±8g;3,±16g
*	sample_rate	:4~1000(Hz)
*	read_mode	:0,被动读取;1,中断读取;2,定时器读取; 1和2支持mmap
*/
static int mpu9250_sensor_init(int gyro_range,int accel_range,int sample_rate,int read_mode)
{
	int ret = 0;
	struct i2c_client *client = mpu9250_Dev.client;
	unsigned char dev_id = 0;
	int _gyro_range = 0;
	int _accel_range = 0;
	int _sample_rate = 0;
	int temp = 0;

	memset(&sampleData,0,sizeof(struct mpu9250_sample_data));

	if(gyro_range >= 0 && gyro_range <= 3)
	{
		_gyro_range = gyro_range;
	}
	else
	{
		_gyro_range = 3;
		dev_err(&client->dev, "%s: error: mpu9250 set gyro_range error,auto default to 3(±2000dps)\n",__func__);
	}

	GyroRange = _gyro_range;

	if(accel_range >= 0 && accel_range <= 3)
	{
		_accel_range = accel_range;
	}
	else
	{
		_accel_range = 0;
		dev_err(&client->dev, "%s: error: mpu9250 set accel_range error,auto default to 0(±2g)\n",__func__);
	}

	AccelRange = _accel_range;

	if(sample_rate >= 4 && sample_rate <= 1000)
	{
		_sample_rate = sample_rate;
	}
	else
	{
		_sample_rate = 125;
		dev_err(&client->dev, "%s: error: mpu9250 set sample_rate error,auto default to 125Hz\n",__func__);
	}

	sampleRate = _sample_rate;

	if(read_mode >= 0 && read_mode <= 2)
	{
		readMode = read_mode;
	}
	else
	{
		readMode = 0;
		dev_err(&client->dev, "%s: error: mpu9250 set read_mode error,auto default to 0(passive mode)\n",__func__);
	}

	ret = mpu9250_read_reg(&mpu9250_Dev, MPU_DEVICE_ID_REG, &dev_id, 1);
	if(ret != 0 || dev_id != MPU9250_ID)
	{
		dev_err(&client->dev, "%s: error: read mpu9250 sensor id failed\n",__func__);
		return -EINVAL;
	}

	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_PWR_MGMT1_REG, 0x80);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_PWR_MGMT1_REG failed 1\n",__func__);
		return -EINVAL;
	}

	mdelay(100);

	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_PWR_MGMT1_REG, 0x00);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_PWR_MGMT1_REG failed 2\n",__func__);
		return -EINVAL;
	}

	//配置磁力计读取方式为IIC直接读取
	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_INTBP_CFG_REG, 0x02);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_INTBP_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_GYRO_CFG_REG, _gyro_range << 3);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_GYRO_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_ACCEL_CFG_REG, _accel_range << 3);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_ACCEL_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	temp = 1000 / sample_rate - 1;
	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_SAMPLE_RATE_REG, (unsigned char)temp);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_ACCEL_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	if(sample_rate / 2 >= 188)
	{
		temp=1;
	}
	else if(sample_rate / 2 >= 98)
	{
		temp=2;
	}
	else if(sample_rate / 2 >= 42)
	{
		temp=3;
	}
	else if(sample_rate / 2 >= 20)
	{
		temp=4;
	}
	else if(sample_rate / 2 >= 10)
	{
		temp=5;
	}
	else
	{
		temp=6; 
	}
	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_CFG_REG, (unsigned char)temp);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_CFG_REG failed\n",__func__);
		return -EINVAL;
	}

	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_PWR_MGMT1_REG, 0x01);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_PWR_MGMT1_REG failed\n",__func__);
		return -EINVAL;
	}

	ret = mpu9250_write_reg(&mpu9250_Dev, MPU_PWR_MGMT2_REG, 0x00);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set MPU_PWR_MGMT2_REG failed\n",__func__);
		return -EINVAL;
	}

	//复位ak8963
	ret = mpu9250_ak8963_write_reg(&mpu9250_Dev, AK_CNTL1_REG, 0x10);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set AK_CNTL1_REG failed 1\n",__func__);
		return -EINVAL;
	}

	mdelay(100);

	//设置ak8963为连续采集模式 数据宽度16bits
	ret = mpu9250_ak8963_write_reg(&mpu9250_Dev, AK_CNTL1_REG, 0x12);
	if(ret != 0)
	{
		dev_err(&client->dev, "%s: error: mpu9250 set AK_CNTL1_REG failed 1\n",__func__);
		return -EINVAL;
	}

	ret = mpu9250_read_reg(&mpu9250_Dev, MPU_INT_STA_REG, (unsigned char *)&temp, 1);	//清除中断状态寄存器
	if(ret)
	{
		dev_err(&client->dev, "%s: error: mpu9250 get MPU_INT_STA_REG failed 2\n",__func__);
	}

	switch(read_mode)
	{
		case 0:																	//被动读取
			default_mode:
			ret = mpu9250_write_reg(&mpu9250_Dev, MPU_INT_EN_REG, 0x00);		//关闭中断
			if(ret != 0)
			{
				dev_err(&client->dev, "%s: error: mpu9250 close interrupt failed\n",__func__);
				return -EINVAL;
			}

			if(intervalTime)
			{
				intervalTime = 0;
				del_timer_sync(&mpu9250_Dev.timer);							//删除定时器
			}
		break;

		case 1:																//中断读取
			if(intervalTime)
			{
				intervalTime = 0;
				del_timer_sync(&mpu9250_Dev.timer);							//删除定时器
			}

			ret = mpu9250_write_reg(&mpu9250_Dev, MPU_INT_EN_REG, 0x01);	//打开中断
			if(ret != 0)
			{
				dev_err(&client->dev, "%s: error: mpu9250 open interrupt failed\n",__func__);
				return -EINVAL;
			}
		break;

		case 2:																//定时器读取
			ret = mpu9250_write_reg(&mpu9250_Dev, MPU_INT_EN_REG, 0x00);	//关闭中断
			if(ret != 0)
			{
				dev_err(&client->dev, "%s: error: mpu9250 close interrupt failed\n",__func__);
				return -EINVAL;
			}

			if(intervalTime == 0)
			{
				intervalTime = 1000 / sample_rate;
				init_timer(&mpu9250_Dev.timer);
				mpu9250_Dev.timer.function = mpu9250_timer_function;
				mpu9250_Dev.timer.expires = jiffies + intervalTime;
				add_timer(&mpu9250_Dev.timer);
			}
		break;

		default:
			goto default_mode;
		break;
	}

	mdelay(500);

	return ret;
}

static int mpu9250_interrupt_init(struct i2c_client *client)
{
	int ret = 0;
	unsigned long irq_flags;

	/* 1.获取设备节点 */
	mpu9250_Dev.of_node = of_find_node_by_path("/i2c@3160000/mpu9250@68");
	if(NULL == mpu9250_Dev.of_node) 
	{
		dev_err(&client->dev, "%s: error: find device node /i2c@3160000/mpu9250@68 failed\n",__func__);
		return -EINVAL;
	}

	/* 获取设备树中的key-gpio属性，得到按键的GPIO编号 */
	mpu9250_Dev.irq_gpio = of_get_named_gpio(mpu9250_Dev.of_node, "interrupt-gpio", 0);
	if(!gpio_is_valid(mpu9250_Dev.irq_gpio)) 
	{
		dev_err(&client->dev, "%s: error: Failed to get interrupt-gpio\n",__func__);
		return -EINVAL;
	}

	/* 获取GPIO对应的中断号 */
	mpu9250_Dev.irq_num = gpio_to_irq(mpu9250_Dev.irq_gpio);
	if (!mpu9250_Dev.irq_num)
	{
		dev_err(&client->dev, "%s: error: Failed to get irq num\n",__func__);
		return -EINVAL;
	}

	/* 申请使用GPIO */
	ret = gpio_request(mpu9250_Dev.irq_gpio, "irq gpio");
	if (ret)
	{
		dev_err(&client->dev, "%s: error: Failed to request irq gpio\n",__func__);
		return -EINVAL;
	}

	/* 将GPIO设置为输入模式 */
	gpio_direction_input(mpu9250_Dev.irq_gpio);

	/* 获取设备树中指定的中断触发类型 */
	irq_flags = irq_get_trigger_type(mpu9250_Dev.irq_num);
	if (IRQF_TRIGGER_NONE == irq_flags)
	{
		dev_info(&client->dev, "%s: warning: Failed to get trigger type, default to IRQF_TRIGGER_FALLING\n",__func__);
		irq_flags = IRQF_TRIGGER_FALLING;
	}
	
	/* 申请中断 */
	ret = request_irq(mpu9250_Dev.irq_num, mpu9250_device_irq, irq_flags, "TEGRA194_MAIN_GPIO_PORT_N_01", &mpu9250_Dev);
	if (ret) 
	{
		dev_err(&client->dev, "%s: error: Failed to request irq\n",__func__);
		gpio_free(mpu9250_Dev.irq_gpio);
		return -EINVAL;
	}
		
	return ret;
}

static int mpu9250_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret = 0;

	dev_info(&client->dev, "%s: hello: mpu9250 device probing\n",__func__);

	memset(&mpu9250_Dev,0,sizeof(struct mpu9250_device));

	mpu9250_Dev.client = client;

	dev_set_drvdata(&client->dev, &mpu9250_Dev);

	mpu9250_Dev.virt_addr = (unsigned char *)kmalloc(sizeof(struct mpu9250_sample_data) * SAMPLE_DATA_BLOCK_NUM,GFP_KERNEL);
	if(mpu9250_Dev.virt_addr == NULL)
	{
		dev_err(&client->dev, "%s: error: Failed to alloc memory\n",__func__);
		goto error0;
	}

	ret = mpu9250_sensor_init(3,0,125,0);
	if (ret)
	{
		dev_err(&client->dev, "%s: error: Failed to init mpu9250 sensor\n",__func__);
		goto error0;
	}

	ret =mpu9250_interrupt_init(client);
	if (ret)
	{
		dev_err(&client->dev, "%s: error: Failed to init interrupt\n",__func__);
		goto error0;
	}

	/* 申请设备号 */
	ret = alloc_chrdev_region(&mpu9250_Dev.devid, 0, 1, DEVICE_NAME);
	if (ret)
	{
		dev_err(&client->dev, "%s: error: Failed to alloc_chrdev_region\n",__func__);
		return ret;
	}

	mpu9250_Dev.cdev.owner = THIS_MODULE;
	cdev_init(&mpu9250_Dev.cdev, &mpu9250_dev_ops);

	ret = cdev_add(&mpu9250_Dev.cdev, mpu9250_Dev.devid, DEVICE_NUMBER);
	if(ret)
	{
		dev_err(&client->dev, "%s: error: cdev add failed.\n",__func__);
		goto error1;
	}

	mpu9250_Dev.class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(mpu9250_Dev.class)) 
	{
		dev_err(&client->dev, "%s: error: class create failed.\n",__func__);
		ret = PTR_ERR(mpu9250_Dev.class);
		goto error2;
	}

	mpu9250_Dev.device = device_create(mpu9250_Dev.class, &client->dev,mpu9250_Dev.devid, NULL, DEVICE_NAME);
	if (IS_ERR(mpu9250_Dev.device)) 
	{
		dev_err(&client->dev, "%s: error: device create failed.\n",__func__);
		ret = PTR_ERR(mpu9250_Dev.device);
		goto error3;
	}

	/* 初始化互斥体 */
	mutex_init(&mpu9250_Dev.data_lock);
	init_waitqueue_head(&mpu9250_Dev.r_wait);
	atomic_set(&have_new_data, 0);

	init_completion(&comp);

	read_task = kthread_run(mpu9250_auto_read_func, NULL, "mythread");
    if(IS_ERR(read_task))
    {
		dev_err(&client->dev, "%s: error: read_task create failed.\n",__func__);

		ret = mpu9250_sensor_init(GyroRange,AccelRange,sampleRate,0);
		if (ret)
		{
			dev_err(&client->dev, "%s: error: Failed to init mpu9250 sensor\n",__func__);
			goto error3;
		}
    }

	return 0;
		
error3:
	class_destroy(mpu9250_Dev.class);
error2:
	cdev_del(&mpu9250_Dev.cdev);
error1:
	unregister_chrdev_region(mpu9250_Dev.devid, DEVICE_NUMBER);
error0:
	dev_set_drvdata(&client->dev, NULL);

	return -EINVAL;
}

static int mpu9250_remove(struct i2c_client *client)
{
	int ret = 0;
	struct mpu9250_device *mpu9250_dev = i2c_get_clientdata(client);

	if(readMode != 0)
	{
		ret = mpu9250_sensor_init(3,0,125,0);
		if (ret)
		{
			dev_err(&client->dev, "%s: error: Failed to init mpu9250 sensor\n",__func__);
		}
	}

	if(!IS_ERR(read_task))
    {  
		complete(&comp);
		
        ret = kthread_stop(read_task);

        printk(KERN_INFO "read_task thread function has stopped ,return %d\n", ret);
    }
	
	dev_set_drvdata(&client->dev, NULL);

	kfree(mpu9250_dev->virt_addr);

	if(intervalTime != 0)
	{
		/* 删除定时器 */
		del_timer_sync(&mpu9250_dev->timer);
	}
	
	/* 注销设备 */
	device_destroy(mpu9250_dev->class, mpu9250_dev->devid);

	/* 注销类 */
	class_destroy(mpu9250_dev->class);

	/* 删除cdev */
	cdev_del(&mpu9250_dev->cdev);

	/* 注销设备号 */
	unregister_chrdev_region(mpu9250_dev->devid, 1);

	free_irq(mpu9250_dev->irq_num, mpu9250_dev);
	gpio_free(mpu9250_dev->irq_gpio);

	return 0;
}

static const struct of_device_id mpu9250_of_match[] = {
	{ .compatible = "eyestar,mpu9250" },
	{ /* Sentinel */ }
};

const struct i2c_device_id mpu9250_id[] = {    
    { "eyestar,mpu9250", 0 },
    { }
};

static struct i2c_driver mpu9250_driver = {
	.driver = {
		.name			= DEVICE_NAME,
		.owner 			= THIS_MODULE,
		.of_match_table	= mpu9250_of_match,
	},
	.probe		= mpu9250_probe,		// probe函数
	.remove		= mpu9250_remove,		// remove函数
	.id_table 	= mpu9250_id,
};

module_i2c_driver(mpu9250_driver);

MODULE_AUTHOR("EyeStar");
MODULE_DESCRIPTION("Inven Sense MPU9250 driver");
MODULE_LICENSE("GPL");