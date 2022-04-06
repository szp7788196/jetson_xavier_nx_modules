#ifndef __CSSC132_CTRL_H
#define __CSSC132_CTRL_H

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



#define DEVICE_NAME			                            			"cssc132_ctrl_"
#define DEVICE_NUMBER		                            			1

#define IOC_CSSC132_CTRL_MAGIC                           			'C'
#define IOC_CSSC132_CTRL_INIT                            			_IO(IOC_CSSC132_CTRL_MAGIC, 0)
#define IOC_CSSC132_CTRL_R_DEVICE_ID                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 1, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_VER                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 2, int)
#define IOC_CSSC132_CTRL_R_FIRMWARE_VER                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 3, int)
#define IOC_CSSC132_CTRL_R_CAM_CAP                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 4, int)
#define IOC_CSSC132_CTRL_R_PRODUCT_MODULE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 5, int)
#define IOC_CSSC132_CTRL_R_SUPPORT_FORMAT                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 6, int)
#define IOC_CSSC132_CTRL_R_CURRENT_FORMAT                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 7, int)
#define IOC_CSSC132_CTRL_W_CURRENT_FORMAT                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 8, int)
#define IOC_CSSC132_CTRL_R_ISP_CAP                       	 		_IOR(IOC_CSSC132_CTRL_MAGIC, 9, int)
#define IOC_CSSC132_CTRL_W_PARAMS_SAVE                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 10, int)
#define IOC_CSSC132_CTRL_R_POWER_HZ                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 11, int)
#define IOC_CSSC132_CTRL_W_POWER_HZ                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 12, int)
#define IOC_CSSC132_CTRL_W_SYS_RESET                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 13, int)
#define IOC_CSSC132_CTRL_R_I2C_ADDR                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 14, int)
#define IOC_CSSC132_CTRL_W_I2C_ADDR                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 15, int)
#define IOC_CSSC132_CTRL_R_STREAM_MODE                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 16, int)
#define IOC_CSSC132_CTRL_W_STREAM_MODE                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 17, int)
#define IOC_CSSC132_CTRL_R_DAY_NIGHT_MODE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 18, int)
#define IOC_CSSC132_CTRL_W_DAY_NIGHT_MODE                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 19, int)
#define IOC_CSSC132_CTRL_R_HUE                        				_IOR(IOC_CSSC132_CTRL_MAGIC, 20, int)
#define IOC_CSSC132_CTRL_W_HUE                        				_IOW(IOC_CSSC132_CTRL_MAGIC, 21, int)
#define IOC_CSSC132_CTRL_R_CONTRAST                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 22, int)
#define IOC_CSSC132_CTRL_W_CONTRAST                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 23, int)
#define IOC_CSSC132_CTRL_R_STATURATION                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 24, int)
#define IOC_CSSC132_CTRL_W_STATURATION                         		_IOW(IOC_CSSC132_CTRL_MAGIC, 25, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_STATE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 26, int)
#define IOC_CSSC132_CTRL_R_WB_STATE                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 27, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_FRAME_MODE                      _IOR(IOC_CSSC132_CTRL_MAGIC, 28, int)
#define IOC_CSSC132_CTRL_W_EXPOSURE_FRAME_MODE                      _IOW(IOC_CSSC132_CTRL_MAGIC, 29, int)
#define IOC_CSSC132_CTRL_R_SLOW_SHUTTER_GAIN                        _IOR(IOC_CSSC132_CTRL_MAGIC, 30, int)
#define IOC_CSSC132_CTRL_W_SLOW_SHUTTER_GAIN                        _IOW(IOC_CSSC132_CTRL_MAGIC, 31, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_MODE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 32, int)
#define IOC_CSSC132_CTRL_W_EXPOSURE_MODE                       		_IOW(IOC_CSSC132_CTRL_MAGIC, 33, int)
#define IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_TIME                     _IOR(IOC_CSSC132_CTRL_MAGIC, 34, int)
#define IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_TIME                     _IOW(IOC_CSSC132_CTRL_MAGIC, 35, int)
#define IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_AGAIN                    _IOR(IOC_CSSC132_CTRL_MAGIC, 36, int)
#define IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_AGAIN                    _IOW(IOC_CSSC132_CTRL_MAGIC, 37, int)
#define IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_DGAIN                    _IOR(IOC_CSSC132_CTRL_MAGIC, 38, int)
#define IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_DGAIN                    _IOW(IOC_CSSC132_CTRL_MAGIC, 39, int)
#define IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_TIME              _IOR(IOC_CSSC132_CTRL_MAGIC, 40, int)
#define IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_TIME              _IOW(IOC_CSSC132_CTRL_MAGIC, 41, int)
#define IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_AGAIN             _IOR(IOC_CSSC132_CTRL_MAGIC, 42, int)
#define IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_AGAIN             _IOW(IOC_CSSC132_CTRL_MAGIC, 43, int)
#define IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_DGAIN             _IOR(IOC_CSSC132_CTRL_MAGIC, 44, int)
#define IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_DGAIN				_IOW(IOC_CSSC132_CTRL_MAGIC, 45, int)
#define IOC_CSSC132_CTRL_R_AWB_MODE                      			_IOR(IOC_CSSC132_CTRL_MAGIC, 46, int)
#define IOC_CSSC132_CTRL_W_AWB_MODE                      			_IOW(IOC_CSSC132_CTRL_MAGIC, 47, int)
#define IOC_CSSC132_CTRL_R_MWB_COLOR_TEMPERATURE         			_IOR(IOC_CSSC132_CTRL_MAGIC, 48, int)
#define IOC_CSSC132_CTRL_W_MWB_COLOR_TEMPERATURE             		_IOW(IOC_CSSC132_CTRL_MAGIC, 49, int)
#define IOC_CSSC132_CTRL_R_MWB_GAIN                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 50, int)
#define IOC_CSSC132_CTRL_W_MWB_GAIN                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 51, int)
#define IOC_CSSC132_CTRL_R_IMAGE_DIRECTION                   		_IOR(IOC_CSSC132_CTRL_MAGIC, 52, int)
#define IOC_CSSC132_CTRL_W_IMAGE_DIRECTION                         	_IOW(IOC_CSSC132_CTRL_MAGIC, 53, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_TARGET_BRIGHTNESS               _IOR(IOC_CSSC132_CTRL_MAGIC, 54, int)
#define IOC_CSSC132_CTRL_W_EXPOSURE_TARGET_BRIGHTNESS               _IOW(IOC_CSSC132_CTRL_MAGIC, 55, int)
#define IOC_CSSC132_CTRL_R_AUTO_EXPOSURE_MAX_TIME                   _IOR(IOC_CSSC132_CTRL_MAGIC, 56, int)
#define IOC_CSSC132_CTRL_W_AUTO_EXPOSURE_MAX_TIME                   _IOW(IOC_CSSC132_CTRL_MAGIC, 57, int)
#define IOC_CSSC132_CTRL_R_AUTO_EXPOSURE_MAX_GAIN                   _IOR(IOC_CSSC132_CTRL_MAGIC, 58, int)
#define IOC_CSSC132_CTRL_W_AUTO_EXPOSURE_MAX_GAIN                   _IOW(IOC_CSSC132_CTRL_MAGIC, 59, int)
#define IOC_CSSC132_CTRL_W_SOFTWARE_TRIGGER_ONE                     _IOW(IOC_CSSC132_CTRL_MAGIC, 60, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_EDGE                    _IOR(IOC_CSSC132_CTRL_MAGIC, 61, int)
#define IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_EDGE                    _IOW(IOC_CSSC132_CTRL_MAGIC, 62, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_DELETE_BOUNCER_TIME		_IOR(IOC_CSSC132_CTRL_MAGIC, 63, int)
#define IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_DELETE_BOUNCER_TIME     _IOW(IOC_CSSC132_CTRL_MAGIC, 64, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_DELAY                   _IOR(IOC_CSSC132_CTRL_MAGIC, 65, int)
#define IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_DELAY                   _IOW(IOC_CSSC132_CTRL_MAGIC, 66, int)
#define IOC_CSSC132_CTRL_R_PICK_MODE                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 67, int)
#define IOC_CSSC132_CTRL_W_PICK_MODE                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 68, int)
#define IOC_CSSC132_CTRL_W_PICK_ONE                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 69, int)
#define IOC_CSSC132_CTRL_R_MIPI_STATUS                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 70, int)
#define IOC_CSSC132_CTRL_W_SYSTEM_REBOOT                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 71, int)
#define IOC_CSSC132_CTRL_W_LED_STROBE_ENABLE                        _IOW(IOC_CSSC132_CTRL_MAGIC, 72, int)
#define IOC_CSSC132_CTRL_R_YUV_SEQUENCE                       		_IOR(IOC_CSSC132_CTRL_MAGIC, 73, int)
#define IOC_CSSC132_CTRL_W_YUV_SEQUENCE                        	    _IOW(IOC_CSSC132_CTRL_MAGIC, 74, int)

#define IOC_CSSC132_CTRL_MAX_NUM                         			74


#define CSSC132_MODULE_NUM              0x0132
#define MAX_SUPPORT_FORMAT_NUM          10

/*************************************** FPGA ISP 寄存器地址 ************************************/
#define DEVICE_ID                       0x0000
#define HARDWARE_VER                    0x0001
#define I2C_EN                          0x0003
#define CAM_CAP_L                       0X0004
#define CAM_CAP_H                       0X0005
#define I2C_ADDR                        0x0006

#define STREAM_MODE                     0x000E
#define SLAVE_MODE                      0x000F

#define STROBE_IO_MODE                  0x0010
#define STROBE_SEL                      0x0011
#define STROBE_VALUE                    0x0012

#define TRIG_IO_MODE                    0x0014
#define TRIG_SEL                        0x0015
#define TRIG_VALUE                      0x0016

#define EXT_TRIG_EDGE                   0x0018
#define EXT_TRIG_DEBOUNCER_EN           0x0019
#define EXT_TRIG_DEBOUNCER_TIME_L       0x001a
#define EXT_TRIG_DEBOUNCER_TIME_M       0x001b
#define EXT_TRIG_DEBOUNCER_TIME_H       0x001c

#define SOFT_TRIG                       0x001d

#define TRIG_DLY_L                      0x001e
#define TRIG_DLY_M                      0x001f
#define TRIG_DLY_H                      0x0020
#define TRIG_DLY_E                      0x0021

#define PICK_MODE_EN                    0x0026
#define PICK_ONE                        0x0027

#define YUV_SEQUENCE                    0x0028

#define AUTO_TRIG_CNT_MAX_L             0x00C7
#define AUTO_TRIG_CNT_MAX_M             0X00C8
#define AUTO_TRIG_CNT_MAX_H             0X00C9
#define AUTO_TRIG_EN                    0x00CA

#define MIPI_CNT_L                      0x00CE
#define MIPI_CNT_H                      0x00CF
#define MIPI_STAT                       0x00D0

/*************************************** ARM ISP 寄存器地址 ************************************/
#define ARM_VER_L                       0x0100
#define ARM_VER_H                       0x0101
#define PRODUCT_ID_L                    0x0102
#define PRODUCT_ID_H                    0x0103
#define SYSTEM_RESET                    0x0104
#define PARAM_SAVE                      0x0105

#define VIDEO_FMT_CAP                   0x0106
#define VIDEO_FMT_NUM                   0x0107
#define FMT_CAP_WIDTH_L                 0x0108
#define FMT_CAP_WIDTH_H                 0x0109
#define FMT_CAP_HEIGHT_L                0x010A
#define FMT_CAP_HEIGHT_H                0x010B
#define FMT_CAP_FRAMRAT_L               0x010C
#define FMT_CAP_FRAMRAT_H               0x010D

#define FMT_WIDTH_L                     0x0180
#define FMT_WIDTH_H                     0x0181
#define FMT_HEIGHT_L                    0x0182
#define FMT_HEIGHT_H                    0x0183
#define FMT_FRAMRAT_L                   0x0184
#define FMT_FRAMRAT_H                   0x0185

#define IMAGE_DIR                       0x0186

#define SYSTEM_REBOOT                   0x0187

#define NEW_FMT_FRAMRAT_MODE            0x0188
#define NEW_FMT_FRAMRAT_L               0x0189
#define NEW_FMT_FRAMRAT_H               0x018A

#define I2C_W_EN                        0x0190
#define ISP_CAP_L                       0x0200
#define ISP_CAP_M                       0x0201
#define ISP_CAP_H                       0x0202
#define ISP_CAP_E                       0x0203

#define POWER_HZ                        0x0204

#define DAY_NIGHT_MODE                  0x0205

#define CSC_HUE                         0x0206
#define CSC_CONTT                       0x0207
#define CSC_SATU                        0x0208

#define EXP_FRM_MODE                    0x020F
#define AE_MODE                         0x0210
#define EXP_TIME_L                      0x0211
#define EXP_TIME_M                      0x0212
#define EXP_TIME_H                      0x0213
#define EXP_TIME_E                      0x0214

#define AGAIN_NOW_DEC                   0x0215
#define AGAIN_NOW_INTER                 0x0216
#define DGAIN_NOW_DEC                   0x0217
#define DGAIN_NOW_INTER                 0x0218

#define AE_SPEED                        0x0219
#define AE_TARGET                       0x021A
#define AE_MAX_TIME_L                   0x021B
#define AE_MAX_TIME_M                   0x021C
#define AE_MAX_TIME_H                   0x021D
#define AE_MAX_TIME_E                   0x021E
#define AE_MAX_GAIN_DEC                 0x021F
#define AE_MAX_GAIN_INTER               0x0220

#define ME_TIME_L                       0x0226
#define ME_TIME_M                       0x0227
#define ME_TIME_H                       0x0228
#define ME_TIME_E                       0x0229
#define ME_AGAIN_DEC                    0x022A
#define ME_AGAIN_INTER                  0x022B
#define ME_DGAIN_DEC                    0x022C
#define ME_DGAIN_INTER                  0x022D

#define AE_SLOW_GAIN_DEC                0x022E
#define AE_SLOW_GAIN_INTER              0x022F

#define AWB_MODE                        0x0230
#define WB_R_GAIN                       0x0231
#define WB_G_GAIN                       0x0232
#define WB_B_GAIN                       0x0233
#define WB_COLOR_TEMP_L                 0x0235
#define WB_COLOR_TEMP_H                 0x0236

#define MWB_COLOR_TEMP_L                0x023A
#define MWB_COLOR_TEMP_H                0x023B
#define MWB_R_GAIN                      0x023C
#define MWB_G_GAIN                      0x023D
#define MWB_B_GAIN                      0x023E

#define DME_TIME_L                      0x0240
#define DME_TIME_M                      0x0241
#define DME_TIME_H                      0x0242
#define DME_TIME_E                      0x0243
#define DME_AGAIN_DEC                   0x0244
#define DME_AGAIN_INTER                 0x0245
#define DME_DGAIN_DEC                   0x0246
#define DME_DGAIN_INTER                 0x0247

#define SNSOR_REG_FLG                   0x0700
#define SNSOR_REG_ADDR_L                0x0701
#define SNSOR_REG_ADDR_H                0x0702
#define SNSOR_REG_VAL                   0x0703


struct cssc132_ctrl_device
{
	struct i2c_client *client;	
	dev_t devid;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *of_node;
};

struct cssc132_format
{
	unsigned short widht;
    unsigned short height;
    unsigned short frame_rate;
};

struct cssc132_support_format
{
	unsigned char number;
    struct cssc132_format format[MAX_SUPPORT_FORMAT_NUM];
};

struct white_balance_state
{
	char rgain;
	char ggain;
	char bgain;
	int color_temp;
};

struct gain_disassemble
{
	unsigned char inter;
	unsigned char dec;
};

struct exposure_state
{
	unsigned int expo_time; 
	struct gain_disassemble again;
	struct gain_disassemble dgain;
};

struct hardware_trigger_delete_bouncer_time
{
	unsigned char enable;
    unsigned int time;
};

struct mipi_status {
	unsigned int count;
	unsigned char start;
};


#endif
