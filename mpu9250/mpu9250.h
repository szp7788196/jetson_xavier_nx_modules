#ifndef __MPU9250_H
#define __MPU9250_H

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
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/ide.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>


#define MPU9250_ID                      0x71

#define MPU_SELF_TEST_X_GYRO_REG        0x00
#define MPU_SELF_TEST_Y_GYRO_REG        0x00
#define MPU_SELF_TEST_Z_GYRO_REG        0x00

#define MPU_SELF_TESTX_ACCEL_REG		0x0D
#define MPU_SELF_TESTY_ACCEL_REG		0x0E
#define MPU_SELF_TESTZ_ACCEL_REG		0x0F
#define MPU_SELF_TESTA_REG		        0x10
#define MPU_SAMPLE_RATE_REG		        0x19        //陀螺仪采样率典型值为0X07 1000/(1+7)=125HZ
#define MPU_CFG_REG				        0x1A        //低通滤波器  典型值0x06 5hz
#define MPU_GYRO_CFG_REG		        0x1B        //陀螺仪测量范围 0X18 正负2000度
#define MPU_ACCEL_CFG_REG		        0x1C        //加速度计测量范围 0X18 正负16g
#define MPU_ACCEL_CFG2_REG              0x1D        //加速度计低通滤波器 0x06 5hz        
#define MPU_MOTION_DET_REG		        0x1F
#define MPU_FIFO_EN_REG			        0x23
#define MPU_I2CMST_CTRL_REG		        0x24
#define MPU_I2CSLV0_ADDR_REG	        0x25
#define MPU_I2CSLV0_REG			        0x26
#define MPU_I2CSLV0_CTRL_REG	        0x27
#define MPU_I2CSLV1_ADDR_REG	        0x28
#define MPU_I2CSLV1_REG			        0x29
#define MPU_I2CSLV1_CTRL_REG	        0x2A
#define MPU_I2CSLV2_ADDR_REG	        0x2B
#define MPU_I2CSLV2_REG			        0x2C
#define MPU_I2CSLV2_CTRL_REG	        0x2D
#define MPU_I2CSLV3_ADDR_REG	        0x2E
#define MPU_I2CSLV3_REG			        0x2F
#define MPU_I2CSLV3_CTRL_REG	        0x30
#define MPU_I2CSLV4_ADDR_REG	        0x31
#define MPU_I2CSLV4_REG			        0x32
#define MPU_I2CSLV4_DO_REG		        0x33
#define MPU_I2CSLV4_CTRL_REG	        0x34
#define MPU_I2CSLV4_DI_REG		        0x35
 
#define MPU_I2CMST_STA_REG		        0x36
#define MPU_INTBP_CFG_REG		        0x37        //中断配置寄存器 bit1=1 接通内部磁力计IIC
#define MPU_INT_EN_REG			        0x38
#define MPU_INT_STA_REG			        0x3A
 
#define MPU_ACCEL_XOUTH_REG		        0x3B        //加速度计输出数据
#define MPU_ACCEL_XOUTL_REG		        0x3C
#define MPU_ACCEL_YOUTH_REG		        0x3D
#define MPU_ACCEL_YOUTL_REG		        0x3E
#define MPU_ACCEL_ZOUTH_REG		        0x3F
#define MPU_ACCEL_ZOUTL_REG		        0x40
 
#define MPU_TEMP_OUTH_REG		        0x41        //温度计输出数据
#define MPU_TEMP_OUTL_REG		        0x42
 
#define MPU_GYRO_XOUTH_REG		        0x43        //陀螺仪输出数据
#define MPU_GYRO_XOUTL_REG		        0x44
#define MPU_GYRO_YOUTH_REG		        0x45
#define MPU_GYRO_YOUTL_REG		        0x46
#define MPU_GYRO_ZOUTH_REG		        0x47
#define MPU_GYRO_ZOUTL_REG		        0x48
 
#define MPU_I2CSLV0_DO_REG		        0x63
#define MPU_I2CSLV1_DO_REG		        0x64
#define MPU_I2CSLV2_DO_REG		        0x65
#define MPU_I2CSLV3_DO_REG		        0x66
 
#define MPU_I2CMST_DELAY_REG	        0x67
#define MPU_SIGPATH_RST_REG		        0x68
#define MPU_MDETECT_CTRL_REG	        0x69
#define MPU_USER_CTRL_REG		        0x6A        //用户配置当为0X10时使用SPI模式
#define MPU_PWR_MGMT1_REG		        0x6B        //电源管理1 典型值为0x00
#define MPU_PWR_MGMT2_REG		        0x6C        //电源管理2 典型值为0X00
#define MPU_FIFO_CNTH_REG		        0x72
#define MPU_FIFO_CNTL_REG		        0x73
#define MPU_FIFO_RW_REG			        0x74
#define MPU_DEVICE_ID_REG		        0x75        //器件ID MPU9250默认ID为0X71
#define MPU_XA_OFFSET_H                 0x77
#define MPU_XA_OFFSET_L                 0x78
#define MPU_YA_OFFSET_H                 0x7A
#define MPU_YA_OFFSET_L                 0x7B
#define MPU_ZA_OFFSET_H                 0x7D
#define MPU_ZA_OFFSET_L                 0x7E

#define AK_DEVICE_ID_REG                0x00
#define AK_INFO_REG                     0x01
#define AK_STATUS1_REG                  0x02
#define AK_HXL_REG                      0x03
#define AK_HXH_REG                      0x04
#define AK_HYL_REG                      0x05
#define AK_HYH_REG                      0x06
#define AK_HZL_REG                      0x07
#define AK_HZH_REG                      0x08
#define AK_STATUS2_REG                  0x09
#define AK_CNTL1_REG                    0x0A
#define AK_CNTL2_REG                    0x0B
#define AK_ASTC_REG                     0x0C
#define AK_TS1_REG                      0x0D
#define AK_TS2_REG                      0x0E
#define AK_I2CDIS_REG                   0x0F
#define AK_ASAX_REG                     0x10
#define AK_ASAY_REG                     0x11
#define AK_ASAZ_REG                     0x12


#define DEVICE_NAME			            "mpu9250"
#define DEVICE_NUMBER		            1

#define SAMPLE_DATA_BLOCK_NUM           16

/* 定义设备类型 */
#define IOC_MAGIC  'M'
/* 初始化设备 */
#define IOC_INIT            _IO(IOC_MAGIC, 0)
/* 读寄存器 */
#define IOC_R_DATA          _IOR(IOC_MAGIC, 1, int)
/* 写寄存器 */
#define IOC_W_CONFIG        _IOW(IOC_MAGIC, 2, int)
#define IOC_MAX_NUM  3


struct mpu9250_device
{
	struct i2c_client *client;	
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *of_node;
	int irq_gpio;
	int irq_num;
    unsigned char  *phys_addr;
	unsigned char  *virt_addr;
    struct timer_list timer;
    struct mutex data_lock;		    /* 互斥体 */
    wait_queue_head_t r_wait;	    /* 读等待队列头 */

};

struct mpu9250_sample_data
{
	short accel_x;
    short accel_y;
    short accel_z;
    short temperature;
    short gyro_x;
    short gyro_y;
    short gyro_z;
    short magne_x;
    short magne_y;
    short magne_z;

};

struct mpu9250_config
{
    int gyro_range;         //:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
    int accel_range;        //0,±2g;1,±4g;2,±8g;3,±16g
    int sample_rate;        //4~1000(Hz)
    int read_mode;          //0,被动读取;1,中断读取;2,定时器读取;3,mmap读取;
};







static int mpu9250_write_reg(struct mpu9250_device *dev, u8 reg, u8 data);
static int mpu9250_read_reg(struct mpu9250_device *dev, u8 reg, u8 *buf, u8 len);
static int mpu9250_ak8963_write_reg(struct mpu9250_device *dev, u8 reg, u8 data);
static int mpu9250_ak8963_read_reg(struct mpu9250_device *dev, u8 reg, u8 *buf, u8 len);
static int mpu9250_get_accel_temp_gyro_data(void);
static int mpu9250_get_magnetometer_data(void);
static int mpu9250_sensor_init(int gyro_range,int accel_range,int sample_rate,int read_mode);

#endif
