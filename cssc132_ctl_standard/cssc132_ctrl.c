#include "cssc132_ctrl.h"

static DECLARE_BITMAP(cssc132_ctrl_minors, DEVICE_NUMBER);
static struct class *cssc132_ctrl_class;
static dev_t cssc132_ctrl_devid;

/*
*
* reg_mode 0:8bit寄存器地址；1:16bit寄存器地址
*/
static int cssc132_ctrl_write_reg(struct cssc132_ctrl_device *dev, unsigned short reg, unsigned char data)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg;
	unsigned char send_buf[3] = {0};
	int ret;

    send_buf[0] = (unsigned char)((reg >> 8) & 0x00FF);
    send_buf[1] = (unsigned char)((reg >> 0) & 0x00FF);
    send_buf[2] = data;

	msg.addr 	= 0x3B;		        // 相机从机地址
	msg.flags 	= 0;				// 标记为写数据
	msg.buf 	= send_buf;			// 要写入的数据缓冲区
	msg.len 	= 3;			    // 要写入的数据长度

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (1 != ret)
	{
		dev_err(&client->dev, "%s: error: reg=0x%x, data=0x%x\n",__func__, reg, data);
		return -EIO;
	}

	return 0;
}

/*
*
* reg_mode 0:8bit寄存器地址；1:16bit寄存器地址
*/
static int cssc132_ctrl_read_reg(struct cssc132_ctrl_device *dev, unsigned short reg, unsigned char *buf, unsigned short len)
{
	struct i2c_client *client = dev->client;
	struct i2c_msg msg[2];
    unsigned char reg_buf[2] = {0};
	int ret;

    reg_buf[0] = (unsigned char)((reg >> 8) & 0x00FF);
    reg_buf[1] = (unsigned char)((reg >> 0) & 0x00FF);

	/* msg[0]: 发送消息 */
	msg[0].addr 	= 0x3B;		        // 相机从机地址
	msg[0].flags 	= 0;				// 标记为写数据
	msg[0].buf 		= reg_buf;		    // 要写入的数据缓冲区
	msg[0].len 		= 2;			    // 要写入的数据长度

	/* msg[1]: 接收消息 */
	msg[1].addr 	= 0x3B;		        // 相机从机地址
	msg[1].flags 	= I2C_M_RD;			// 标记为读数据
	msg[1].buf 		= buf;				// 存放读数据的缓冲区
	msg[1].len 		= len;				// 读取的字节长度

	ret = i2c_transfer(client->adapter, msg, 2);
	if (2 != ret)
	{
		printk("%s: error: reg=0x%x, len=0x%x\n",__func__, reg, len);
		return -EIO;
	}

	return 0;
}

//读取device id
static int cssc132_ctrl_read_device_id(struct cssc132_ctrl_device *dev, unsigned char *dev_id)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_read_reg(dev,DEVICE_ID,dev_id,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read device_id failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取硬件版本号
static int cssc132_ctrl_read_hardware_version(struct cssc132_ctrl_device *dev, unsigned char *hd_ver)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_read_reg(dev,HARDWARE_VER,hd_ver,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_version failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取固件版本号
static int cssc132_ctrl_read_firmware_version(struct cssc132_ctrl_device *dev, unsigned short *fm_ver)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev,ARM_VER_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read firmware_version l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ARM_VER_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read firmware_version h failed\n", __func__);
        return -EIO;
    }

    *fm_ver = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
              ((((unsigned short)temp_l) << 0) & 0x00FF);

    return ret;
}

/*
* 读取相机能力集
* bit description
* 0   sync stream mode
* 1   trigger mode
* 2   pick mode 
*/
static int cssc132_ctrl_read_camera_capability(struct cssc132_ctrl_device *dev, unsigned short *cam_cap)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev,CAM_CAP_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read camera_capability l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,CAM_CAP_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read camera_capability h failed\n", __func__);
        return -EIO;
    }

    *cam_cap = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
               ((((unsigned short)temp_l) << 0) & 0x00FF);

    return ret;
}

//读取相机型号
static int cssc132_ctrl_read_product_module(struct cssc132_ctrl_device *dev, unsigned short *module)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev,PRODUCT_ID_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read product_module l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,PRODUCT_ID_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read product_module h failed\n", __func__);
        return -EIO;
    }

    *module = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
              ((((unsigned short)temp_l) << 0) & 0x00FF);

    if(*module != CSSC132_MODULE_NUM)
    {
        dev_err(&client->dev,"%s: warning: product_module mismatch\n", __func__);
    }

    return ret;
}

//读取相机支持的码流格式
static int cssc132_ctrl_read_support_format(struct cssc132_ctrl_device *dev, struct cssc132_support_format *format)
{
    int ret = 0;
    unsigned char i = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev,VIDEO_FMT_NUM,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read support format num failed\n", __func__);
        return -EIO;
    }

    format->number = temp_l;

    for(i = 0; i < format->number; i ++)
    {
        ret = cssc132_ctrl_read_reg(dev,FMT_CAP_WIDTH_L + i * 6,&temp_l,1);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: read %dst format widht l failed\n", __func__,i + 1);
            return -EIO;
        }

        ret = cssc132_ctrl_read_reg(dev,FMT_CAP_WIDTH_H + i * 6,&temp_h,1);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: read %dst format widht h failed\n", __func__,i + 1);
            return -EIO;
        }

        format->format[i].widht = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                                  ((((unsigned short)temp_l) << 0) & 0x00FF);

        ret = cssc132_ctrl_read_reg(dev,FMT_CAP_HEIGHT_L + i * 6,&temp_l,1);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: read %dst format height l failed\n", __func__,i + 1);
            return -EIO;
        }

        ret = cssc132_ctrl_read_reg(dev,FMT_CAP_HEIGHT_H + i * 6,&temp_h,1);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: read %dst format height h failed\n", __func__,i + 1);
            return -EIO;
        }

        format->format[i].height = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                                   ((((unsigned short)temp_l) << 0) & 0x00FF);

        ret = cssc132_ctrl_read_reg(dev,FMT_CAP_FRAMRAT_L + i * 6,&temp_l,1);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: read %dst format frame rate l failed\n", __func__,i + 1);
            return -EIO;
        }

        ret = cssc132_ctrl_read_reg(dev,FMT_CAP_FRAMRAT_H + i * 6,&temp_h,1);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: read %dst format frame rate h failed\n", __func__,i + 1);
            return -EIO;
        }

        format->format[i].frame_rate = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                                       ((((unsigned short)temp_l) << 0) & 0x00FF);
    }

    return ret;
}

//读取当前码流格式
static int cssc132_ctrl_read_current_format(struct cssc132_ctrl_device *dev, struct cssc132_format *format)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev,FMT_WIDTH_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read current format widht l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,FMT_WIDTH_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read current format widht h failed\n", __func__);
        return -EIO;
    }

    format->widht = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                    ((((unsigned short)temp_l) << 0) & 0x00FF);

    ret = cssc132_ctrl_read_reg(dev,FMT_HEIGHT_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read current format height l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,FMT_HEIGHT_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read current format height h failed\n", __func__);
        return -EIO;
    }

    format->height = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                     ((((unsigned short)temp_l) << 0) & 0x00FF);

    ret = cssc132_ctrl_read_reg(dev,FMT_FRAMRAT_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read current format frame rate l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,FMT_FRAMRAT_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read current format frame rate h failed\n", __func__);
        return -EIO;
    }

    format->frame_rate = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                         ((((unsigned short)temp_l) << 0) & 0x00FF);

    return ret;
}

//设置当前码流格式
static int cssc132_ctrl_write_current_format(struct cssc132_ctrl_device *dev, struct cssc132_format format)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(((format.widht == 1280 && format.height == 1080 && format.frame_rate <=  45) || 
        (format.widht == 1280 && format.height == 720  && format.frame_rate <=  60) || 
        (format.widht == 640  && format.height == 480  && format.frame_rate <= 120) || 
        (format.widht == 1080 && format.height == 1208 && format.frame_rate <=  45) || 
        (format.widht == 720  && format.height == 1280 && format.frame_rate <=  60) || 
        (format.widht == 480  && format.height == 640  && format.frame_rate <= 120)) && 
         format.frame_rate >= 2)
    {
        ret = cssc132_ctrl_write_reg(dev, FMT_WIDTH_L, (unsigned char)((format.widht >> 0) & 0x00FF));
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write format widht l failed\n", __func__);
            return -EIO;
        }

        ret = cssc132_ctrl_write_reg(dev, FMT_WIDTH_H, (unsigned char)((format.widht >> 8) & 0x00FF));
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write format widht h failed\n", __func__);
            return -EIO;
        }

        ret = cssc132_ctrl_write_reg(dev, FMT_HEIGHT_L, (unsigned char)((format.height >> 0) & 0x00FF));
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write format widht l failed\n", __func__);
            return -EIO;
        }

        ret = cssc132_ctrl_write_reg(dev, FMT_HEIGHT_H, (unsigned char)((format.height >> 8) & 0x00FF));
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write format height h failed\n", __func__);
            return -EIO;
        }

        ret = cssc132_ctrl_write_reg(dev, FMT_FRAMRAT_L, (unsigned char)((format.frame_rate >> 0) & 0x00FF));
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write format frame_rate l failed\n", __func__);
            return -EIO;
        }

        ret = cssc132_ctrl_write_reg(dev, FMT_FRAMRAT_H, (unsigned char)((format.frame_rate >> 8) & 0x00FF));
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write format frame_rate h failed\n", __func__);
            return -EIO;
        }
    }
    else
    {
        dev_err(&client->dev,"%s: error: input format error\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取相机型号
static int cssc132_ctrl_read_isp_capability(struct cssc132_ctrl_device *dev, unsigned int *isp_cap)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,ISP_CAP_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read isp_capability l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ISP_CAP_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read isp_capability m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ISP_CAP_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read isp_capability h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ISP_CAP_E,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read isp_capability e failed\n", __func__);
        return -EIO;
    }

    *isp_cap = ((((unsigned int)temp_e) << 24) & 0xFF000000) + 
               ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
               ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
               ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    return ret;
}

//保存配置信息到相机flash
static int cssc132_ctrl_write_params_save(struct cssc132_ctrl_device *dev)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_write_reg(dev, PARAM_SAVE, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write params_save failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取工频频率
static int cssc132_ctrl_read_power_hz(struct cssc132_ctrl_device *dev, unsigned char *power_hz)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev,POWER_HZ,&temp,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read power_hz failed\n", __func__);
        return -EIO;
    }

    *power_hz = temp;

    return ret;
}

//设置工频频率
static int cssc132_ctrl_write_power_hz(struct cssc132_ctrl_device *dev, unsigned char power_hz)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(power_hz != 50 && power_hz != 60)
    {
        dev_err(&client->dev,"%s: error: input power_hz = %d error,should be 50/60\n", __func__,power_hz);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, POWER_HZ, power_hz);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write power_hz failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//恢复出厂设置
static int cssc132_ctrl_write_sys_reset(struct cssc132_ctrl_device *dev)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_write_reg(dev, SYSTEM_RESET, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: system reset failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取相机IIC地址
static int cssc132_ctrl_read_i2c_addr(struct cssc132_ctrl_device *dev, unsigned char *addr)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, I2C_ADDR, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read i2c addr failed\n", __func__);
        return -EIO;
    }

    *addr = temp;

    return ret;
}

//设置相机IIC地址
static int cssc132_ctrl_write_i2c_addr(struct cssc132_ctrl_device *dev, unsigned char addr)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(addr < 0x03 || addr > 0x77)
    {
        dev_err(&client->dev,"%s: error: input addr = %d error,should be 0x03 to x077\n", __func__,addr);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, I2C_ADDR, addr);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write i2c addr failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取相机码流模式
/*
* value 	description
* 0 	    free running
* 1 	    sync mode:cssc132 not support
* 2 	    Hardware Trigger
* 3 	    Software Trigger
*/
static int cssc132_ctrl_read_stream_mode(struct cssc132_ctrl_device *dev, unsigned char *mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, STREAM_MODE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read stream_mode failed\n", __func__);
        return -EIO;
    }

    *mode = temp;

    return ret;
}

//设置相机码流模式
static int cssc132_ctrl_write_stream_mode(struct cssc132_ctrl_device *dev, unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode != 0 && mode != 2 && mode != 3)
    {
        dev_err(&client->dev,"%s: error: input stream_mode = %d error,should be 0/2/3\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, STREAM_MODE, mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write stream_mode failed\n", __func__);
        return -EIO;
    }

/*
    if(mode == 2)
    {
        ret = cssc132_ctrl_write_reg(dev, STROBE_IO_MODE, 0);
        if(ret != 0)
        {
            dev_err(&client->dev,"%s: error: write strobe io mode failed\n", __func__);
            return -EIO;
        }
    }
*/
    return ret;
}

//读取day night模式
/*
* value 	description
* 0x1 	Color Mode
* 0x2 	Black&White Mode
* 0x0 	Trigger Mode
* 0x3 	IR-CUT day, Image Black&White mode
* 0x4 	IR-CUT night, Image Color mode
*
* IR-CUT (Infrared cut-off filter) is a mechanical shutter design. 
* It is placed between the lens and the image sensor, and is controlled by a motor or an electromagnet.
* IR-CUT has two status: Block or Deliver the infrared.
*
* Color Mode:
*     Image is Color Mode and IR_CUT status Block infrared.
*
* Black&White Mode:
*     Image is Black&White Mode and IR_CUT status Deliver infrared.
*
* Trigger Mode:
*     Trigger pin : J4 pin1.
*     Trigger pin High(3.3~12V),Image is Black&White Mode and IR-CUT status Deliver infrared.
*     Trigger pin Low(GND),Image is Color Mode and IR-CUT status Bolck infrared.
*
* IR-CUT day, Image black&white mode:
*     Image is Black&White Mode and IR_CUT status Block infrared.
*
* IR-CUT night, Image color mode:
*     Image is Color Mode and IR_CUT status Deliver infrared. 
*/
static int cssc132_ctrl_read_day_night_mode(struct cssc132_ctrl_device *dev, unsigned char *mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, DAY_NIGHT_MODE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read day_night_mode failed\n", __func__);
        return -EIO;
    }

    *mode = temp;

    return ret;
}

static int cssc132_ctrl_write_day_night_mode(struct cssc132_ctrl_device *dev, unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode > 4)
    {
        dev_err(&client->dev,"%s: error: input day_night_mode = %d error,should be 0/1/2/3/4\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DAY_NIGHT_MODE, mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write day_night_mode failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取色调
//0-100
static int cssc132_ctrl_read_hue(struct cssc132_ctrl_device *dev, unsigned char *hue)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, CSC_HUE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hue failed\n", __func__);
        return -EIO;
    }

    *hue = temp;

    return ret;
}

//设置色调
//0-100
static int cssc132_ctrl_write_hue(struct cssc132_ctrl_device *dev, unsigned char hue)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(hue > 100)
    {
        dev_err(&client->dev,"%s: error: input hue = %d error,should be: 0 <= hue <= 100\n", __func__,hue);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, CSC_HUE, hue);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hue failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取对比度
//0-100
static int cssc132_ctrl_read_contrast(struct cssc132_ctrl_device *dev, unsigned char *con)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, CSC_CONTT, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read contrast failed\n", __func__);
        return -EIO;
    }

    *con = temp;

    return ret;
}

//设置对比度
//0-100
static int cssc132_ctrl_write_contrast(struct cssc132_ctrl_device *dev, unsigned char con)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(con > 100)
    {
        dev_err(&client->dev,"%s: error: input contrast = %d error,should be: 0 <= contrast <= 100\n", __func__,con);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, CSC_CONTT, con);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write contrast failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取饱和度
//0-100
static int cssc132_ctrl_read_saturation(struct cssc132_ctrl_device *dev, unsigned char *satu)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, CSC_SATU, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read saturation failed\n", __func__);
        return -EIO;
    }

    *satu = temp;

    return ret;
}

//设置饱和度
//0-100
static int cssc132_ctrl_write_saturation(struct cssc132_ctrl_device *dev, unsigned char satu)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(satu > 100)
    {
        dev_err(&client->dev,"%s: error: input saturation = %d error,should be: 0 <= saturation <= 100\n", __func__,satu);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, CSC_SATU, satu);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write saturation failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//读取当前曝光状态
static int cssc132_ctrl_read_exposure_state(struct cssc132_ctrl_device *dev, struct exposure_state *state)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,EXP_TIME_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read exposure time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,EXP_TIME_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read exposure time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,EXP_TIME_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read exposure time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,EXP_TIME_E,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read exposure time e failed\n", __func__);
        return -EIO;
    }

    state->expo_time = ((((unsigned int)temp_e) << 24) & 0xFF000000) + 
                       ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
                       ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
                       ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    ret = cssc132_ctrl_read_reg(dev,AGAIN_NOW_DEC,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read again dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,AGAIN_NOW_INTER,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read again inter failed\n", __func__);
        return -EIO;
    }

    state->again.dec = temp_l;
    state->again.inter = temp_h;

    ret = cssc132_ctrl_read_reg(dev,DGAIN_NOW_DEC,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read dgain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,DGAIN_NOW_INTER,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read dgain inter failed\n", __func__);
        return -EIO;
    }

    state->dgain.dec = temp_l;
    state->dgain.inter = temp_h;

    return ret;
}

//读取白平衡色温状态
static int cssc132_ctrl_read_wb_state(struct cssc132_ctrl_device *dev, struct white_balance_state *state)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, WB_R_GAIN, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read white balance red gain failed\n", __func__);
        return -EIO;
    }

    state->rgain = (char)temp_l;

    ret = cssc132_ctrl_read_reg(dev, WB_G_GAIN, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read white balance green gain failed\n", __func__);
        return -EIO;
    }

    state->ggain = (char)temp_l;

    ret = cssc132_ctrl_read_reg(dev, WB_B_GAIN, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read white balance blue gain failed\n", __func__);
        return -EIO;
    }

    state->bgain = (char)temp_l;

    ret = cssc132_ctrl_read_reg(dev, WB_COLOR_TEMP_L, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read white balance color temperature l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, WB_COLOR_TEMP_H, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read white balance color temperature h failed\n", __func__);
        return -EIO;
    }

    state->color_temp = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                        ((((unsigned short)temp_l) << 0) & 0x00FF);

    return ret;
}

/*
* value 	description
* 0 	慢快门模式
* 1 	固定帧率模式（默认）
* 自动曝光模式有效，慢快门模式通常用于低照度场景下进行自动降帧，
* 以减少画面噪声。需要配合aetime，slowshuttergain参数使用。 
*/
static int cssc132_ctrl_read_exposure_frame_mode(struct cssc132_ctrl_device *dev, unsigned char *mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, EXP_FRM_MODE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read exposure_frame_mode failed\n", __func__);
        return -EIO;
    }

    *mode = temp;

    return ret;
}

static int cssc132_ctrl_write_exposure_frame_mode(struct cssc132_ctrl_device *dev, unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode > 1)
    {
        dev_err(&client->dev,"%s: error: input exposure_frame_mode = %d error,should be 0/1\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, EXP_FRM_MODE, mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write exposure_frame_mode failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 慢快门模式下,开始降低帧率的增益阈值,单位dB，需expfrmmode为慢快门模式下生效。
* 在慢快门模式下，场景逐渐变暗，AE算法会首先提高曝光时间，达到aetime上限后，
* 再提高增益，达到slowshuttergain值后，开始继续提高曝光时间，此时帧率会降低。
*/
static int cssc132_ctrl_read_slow_shutter_gain(struct cssc132_ctrl_device *dev, struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, AE_SLOW_GAIN_DEC, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read slow_shutter_gain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, AE_SLOW_GAIN_INTER, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read slow_shutter_gain inter failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

static int cssc132_ctrl_write_slow_shutter_gain(struct cssc132_ctrl_device *dev, struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(gain.dec > 9)
    {
        dev_err(&client->dev,"%s: error: input slow_shutter_gain dec = %d error,should be less than 9\n", 
                __func__,gain.dec);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_SLOW_GAIN_DEC, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write slow_shutter_gain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_SLOW_GAIN_INTER, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write slow_shutter_gain inter failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取曝光模式
* 0: 自动曝光
* 1: 手动曝光
* 2: 直接手动曝光，跳过isp环节，直接写sensor寄存器。
*    CS-MIPI-SC132有效。此模式配合触发抓拍功能，配置参数的生效时间更快。 
*/
static int cssc132_ctrl_read_exposure_mode(struct cssc132_ctrl_device *dev, unsigned char *mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, AE_MODE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read exposure_mode failed\n", __func__);
        return -EIO;
    }

    *mode = temp;

    return ret;
}

//设置曝光模式
static int cssc132_ctrl_write_exposure_mode(struct cssc132_ctrl_device *dev, unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode > 2)
    {
        dev_err(&client->dev,"%s: error: input exposure_mode = %d error,should be 0/1/2\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, EXP_FRM_MODE, mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write exposure_mode failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取手动曝光时间
* 手动曝光时间，需expmode为手动模式下生效, us.
* range:[0-1000000/framerate]
* 如需超长曝光，需要调整framerate以配合此参数的生效。 
*/
static int cssc132_ctrl_read_manual_exposure_time(struct cssc132_ctrl_device *dev, unsigned int *time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,ME_TIME_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ME_TIME_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ME_TIME_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,ME_TIME_E,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_time e failed\n", __func__);
        return -EIO;
    }

    *time = ((((unsigned int)temp_e) << 24) & 0xFF000000) + 
            ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
            ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
            ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    return ret;
}

//设置手动曝光时间
static int cssc132_ctrl_write_manual_exposure_time(struct cssc132_ctrl_device *dev, unsigned int time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    temp_e = (unsigned char)((time >> 24) & 0x000000FF);
    temp_h = (unsigned char)((time >> 16) & 0x000000FF);
    temp_m = (unsigned char)((time >>  8) & 0x000000FF);
    temp_l = (unsigned char)((time >>  0) & 0x000000FF);

    ret = cssc132_ctrl_write_reg(dev, ME_TIME_L, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_TIME_M, temp_m);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_TIME_H, temp_h);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_TIME_E, temp_e);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_time e failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取手动模拟增益
* 单位dB，需expmode为手动模式下生效。
*/
static int cssc132_ctrl_read_manual_exposure_again(struct cssc132_ctrl_device *dev, struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, ME_AGAIN_DEC, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_again dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, ME_AGAIN_INTER, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_again inter failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

/*
* 设置手动模拟增益
* 单位dB，需expmode为手动模式下生效。
*/
static int cssc132_ctrl_write_manual_exposure_again(struct cssc132_ctrl_device *dev, struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(gain.dec > 9)
    {
        dev_err(&client->dev,"%s: error: input manual_exposure_again dec = %d error,should be less than 9\n", 
                __func__,gain.dec);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_AGAIN_DEC, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_again dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_AGAIN_INTER, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_again inter failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取手动数字增益
* 单位dB，需expmode为手动模式下生效。
*/
static int cssc132_ctrl_read_manual_exposure_dgain(struct cssc132_ctrl_device *dev, struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, ME_DGAIN_DEC, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_dgain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, ME_DGAIN_INTER, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual_exposure_dgain inter failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

/*
* 设置手动数字增益
* 单位dB，需expmode为手动模式下生效。
*/
static int cssc132_ctrl_write_manual_exposure_dgain(struct cssc132_ctrl_device *dev, struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(gain.dec > 9)
    {
        dev_err(&client->dev,"%s: error: input manual_exposure_dgain dec = %d error,should be less than 9\n", 
                __func__,gain.dec);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_DGAIN_DEC, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_dgain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, ME_DGAIN_INTER, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write manual_exposure_dgain inter failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取直接手动曝光时间
* 直接手动曝光时间，需expmode为直接手动模式下生效, us.
* range:[0-1000000/framerate]
* 如需超长曝光，需要调整framerate以配合此参数的生效。 
*/
static int cssc132_ctrl_read_direct_manual_exposure_time(struct cssc132_ctrl_device *dev, unsigned int *time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,DME_TIME_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,DME_TIME_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,DME_TIME_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,DME_TIME_E,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_time e failed\n", __func__);
        return -EIO;
    }

    *time = ((((unsigned int)temp_e) << 24) & 0xFF000000) + 
            ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
            ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
            ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    return ret;
}

//设置直接手动曝光时间
static int cssc132_ctrl_write_direct_manual_exposure_time(struct cssc132_ctrl_device *dev, unsigned int time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    temp_e = (unsigned char)((time >> 24) & 0x000000FF);
    temp_h = (unsigned char)((time >> 16) & 0x000000FF);
    temp_m = (unsigned char)((time >>  8) & 0x000000FF);
    temp_l = (unsigned char)((time >>  0) & 0x000000FF);

    ret = cssc132_ctrl_write_reg(dev, DME_TIME_L, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_TIME_M, temp_m);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_TIME_H, temp_h);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_TIME_E, temp_e);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_time e failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取直接手动模拟增益
* 单位dB，需expmode为直接手动模式下生效。
*/
static int cssc132_ctrl_read_direct_manual_exposure_again(struct cssc132_ctrl_device *dev, struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, DME_AGAIN_DEC, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_again dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, DME_AGAIN_INTER, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_again inter failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

/*
* 设置直接手动模拟增益
* 单位dB，需expmode为直接手动模式下生效。
*/
static int cssc132_ctrl_write_direct_manual_exposure_again(struct cssc132_ctrl_device *dev, struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(gain.dec > 9)
    {
        dev_err(&client->dev,"%s: error: input direct_manual_exposure_again dec = %d error,should be less than 9\n", 
                __func__,gain.dec);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_AGAIN_DEC, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_again dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_AGAIN_INTER, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_again inter failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取直接手动数字增益
* 单位dB，需expmode为直接手动模式下生效。
*/
static int cssc132_ctrl_read_direct_manual_exposure_dgain(struct cssc132_ctrl_device *dev, struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, DME_DGAIN_DEC, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_dgain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, DME_DGAIN_INTER, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read direct_manual_exposure_dgain inter failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

/*
* 设置直接手动数字增益
* 单位dB，需expmode为直接手动模式下生效。
*/
static int cssc132_ctrl_write_direct_manual_exposure_dgain(struct cssc132_ctrl_device *dev, struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(gain.dec > 9)
    {
        dev_err(&client->dev,"%s: error: input direct_manual_exposure_dgain dec = %d error,should be less than 9\n", 
                __func__,gain.dec);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_DGAIN_DEC, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_dgain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, DME_DGAIN_INTER, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write direct_manual_exposure_dgain inter failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

/*
* 读取自动白平衡模式
* 0: auto white balance
* 1: manual white balance, gain mode
* 2: manual white balance, color temperature mode 
*/
static int cssc132_ctrl_read_awb_mode(struct cssc132_ctrl_device *dev, unsigned char *mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, AWB_MODE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read awb_mode failed\n", __func__);
        return -EIO;
    }

    *mode = temp;

    return ret;
}

//设置自动白平衡模式
static int cssc132_ctrl_write_awb_mode(struct cssc132_ctrl_device *dev, unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode > 2)
    {
        dev_err(&client->dev,"%s: error: input awb_mode = %d error,should be 0/1/2\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AWB_MODE, mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write awb_mode failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 手动白平衡，色温模式
* color tempture,range [1500,15000] 
*/
static int cssc132_ctrl_read_mwb_color_temperature(struct cssc132_ctrl_device *dev,unsigned int *color_temp)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, MWB_COLOR_TEMP_L, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual white balance color temperature l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, MWB_COLOR_TEMP_H, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual white balance color temperature h failed\n", __func__);
        return -EIO;
    }

    *color_temp = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                  ((((unsigned short)temp_l) << 0) & 0x00FF);

    return ret;
}

//设置手动白平衡，色温模式
static int cssc132_ctrl_write_mwb_color_temperature(struct cssc132_ctrl_device *dev,unsigned int color_temp)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    if(color_temp < 1500 || color_temp > 15000)
    {
        dev_err(&client->dev,"%s: error: input mwb_color_temperature = %d error,should be 1500 <= color_temp <= 15000\n",
                __func__,color_temp);
        return -EIO;
    }

    temp_h = (unsigned char)((color_temp >> 8) & 0x00FF);
    temp_l = (unsigned char)((color_temp >> 0) & 0x00FF);

    ret = cssc132_ctrl_write_reg(dev, MWB_COLOR_TEMP_L, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write mwb_color_temperature l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, MWB_COLOR_TEMP_H, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write mwb_color_temperature h failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* manual white balance mode, read rgain and bgain manually, ggain is always 1.
* GAIN is 4-bit decimal precision, for example: 0x23 means 2 * 10 + 3 = 23 
*/
static int cssc132_ctrl_read_mwb_gain(struct cssc132_ctrl_device *dev,struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, MWB_R_GAIN, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual white balance red gain failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, MWB_B_GAIN, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read manual white balance blue gain failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

/*
* manual white balance mode, set rgain and bgain manually, ggain is always 1.
* GAIN is 4-bit decimal precision, for example: 0x23 means 2+3/16=2.19. 
*/
static int cssc132_ctrl_write_mwb_gain(struct cssc132_ctrl_device *dev,struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    
    ret = cssc132_ctrl_write_reg(dev, MWB_R_GAIN, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write mwb_rgain failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, MWB_B_GAIN, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write mwb_bgain failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取图像的方向
* value 	description
* 0 	normal
* 1 	左右翻转
* 2 	上下翻转
* 3 	上下+左右翻转 
*/
static int cssc132_ctrl_read_image_direction(struct cssc132_ctrl_device *dev,unsigned char *dir)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, IMAGE_DIR, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read image_direction failed\n", __func__);
        return -EIO;
    }

    *dir = temp;

    return ret;
}

//设置图像的方向
static int cssc132_ctrl_write_image_direction(struct cssc132_ctrl_device *dev,unsigned char dir)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(dir > 99)
    {
        dev_err(&client->dev,"%s: error: input image_direction = %d error,should be 0/1/2/3\n",
                __func__,dir);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, IMAGE_DIR, dir);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write image_direction failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取自动曝光模式下目标亮度,需expmode为自动模式下生效。默认值不同型号会略有不同。
* range:[0,255] 
*/
static int cssc132_ctrl_read_auto_exposure_target_brightness(struct cssc132_ctrl_device *dev,unsigned char *brightness)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, AE_TARGET, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_target_brightness failed\n", __func__);
        return -EIO;
    }

    *brightness = temp;

    return ret;
}

//设置自动曝光模式下目标亮度
static int cssc132_ctrl_write_auto_exposure_target_brightness(struct cssc132_ctrl_device *dev,unsigned char brightness)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_write_reg(dev, AE_TARGET, brightness);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_target_brightness failed\n", __func__);
        return -EIO;
    }

    return ret;
}
/*
* 读取AE最长曝光时间，单位us。需expmode为自动模式下生效。
* expfrmmode为固定帧率模式下：range:(100,1/framerate]，如帧率为30，则范围是(100,33333].
* expfrmmode为慢快门模式下:range:(100,0xFFFFFFFF)
* 特殊值：0xFFFFFFFF=1/framerate,自动根据camera工作模式调整。 
*/
static int cssc132_ctrl_read_auto_exposure_max_time(struct cssc132_ctrl_device *dev, unsigned int *time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,AE_MAX_TIME_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_max_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,AE_MAX_TIME_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_max_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,AE_MAX_TIME_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_max_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,AE_MAX_TIME_E,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_max_time e failed\n", __func__);
        return -EIO;
    }

    *time = ((((unsigned int)temp_e) << 24) & 0xFF000000) + 
            ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
            ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
            ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    return ret;
}

//设置AE最长曝光时间
static int cssc132_ctrl_write_auto_exposure_max_time(struct cssc132_ctrl_device *dev, unsigned int time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    temp_e = (unsigned char)((time >> 24) & 0x000000FF);
    temp_h = (unsigned char)((time >> 16) & 0x000000FF);
    temp_m = (unsigned char)((time >>  8) & 0x000000FF);
    temp_l = (unsigned char)((time >>  0) & 0x000000FF);

    ret = cssc132_ctrl_write_reg(dev, AE_MAX_TIME_L, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_max_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_MAX_TIME_M, temp_m);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_max_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_MAX_TIME_H, temp_h);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_max_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_MAX_TIME_E, temp_e);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_max_time e failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* 读取自动曝光模式下，最大系统总增益,单位dB，需expmode为自动模式下生效。
* agc组成部分为Again+Dgain+ISPGain。
* X is the integer part and Y is the decimal part, for example X is 2 and Y is 3 means 2.3dB.
* X range: depending on sensor.
* Y range: [0-9]. 
*/
static int cssc132_ctrl_read_auto_exposure_max_gain(struct cssc132_ctrl_device *dev, struct gain_disassemble *gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;

    ret = cssc132_ctrl_read_reg(dev, AE_MAX_GAIN_DEC, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_max_gain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, AE_MAX_GAIN_INTER, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read auto_exposure_max_gain inter failed\n", __func__);
        return -EIO;
    }

    gain->dec = temp_l;
    gain->inter = temp_h;

    return ret;
}

//设置自动曝光模式下，最大系统总增益,单位dB，需expmode为自动模式下生效。
static int cssc132_ctrl_write_auto_exposure_max_gain(struct cssc132_ctrl_device *dev, struct gain_disassemble gain)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(gain.dec > 9)
    {
        dev_err(&client->dev,"%s: error: input auto_exposure_max_gain dec = %d error,should be less than 9\n", 
                __func__,gain.dec);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_MAX_GAIN_DEC, gain.dec);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_max_gain dec failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, AE_MAX_GAIN_INTER, gain.inter);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write auto_exposure_max_gain inter failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

/*
* Software Trigger模式下，通过写寄存器，触发一次曝光和图像。 
* note: 仅CS-MIPI-SC132支持。 
*/
static int cssc132_ctrl_write_software_trigger_one(struct cssc132_ctrl_device *dev)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_write_reg(dev, SOFT_TRIG, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write software_trigger_one failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

/*
* value 	description
* 0 	上升沿触发
* 1 	下降沿触发 
*/
static int cssc132_ctrl_read_hardware_trigger_edge(struct cssc132_ctrl_device *dev, unsigned char *edge)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, EXT_TRIG_EDGE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_edge failed\n", __func__);
        return -EIO;
    }

    *edge = temp;

    return ret;
}

/*
* value 	description
* 0 	上升沿触发
* 1 	下降沿触发 
*/
static int cssc132_ctrl_write_hardware_trigger_edge(struct cssc132_ctrl_device *dev,unsigned char edge)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(edge > 1)
    {
        dev_err(&client->dev,"%s: error: input hardware_trigger_edge = %d error,should be 0/1\n", __func__,edge);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, EXT_TRIG_EDGE, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_edge failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

/*
* Hardware Trigger模式下，触发信号的去毛刺属性。凡小于value(us)的触发信号将被忽略。
* note: 仅CS-MIPI-SC132支持。 
*/
static int cssc132_ctrl_read_hardware_trigger_delete_bouncer_time(struct cssc132_ctrl_device *dev, 
                                                                 struct hardware_trigger_delete_bouncer_time *hd_trig_del_bou_time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,EXT_TRIG_DEBOUNCER_TIME_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delete_bouncer_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,EXT_TRIG_DEBOUNCER_TIME_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delete_bouncer_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,EXT_TRIG_DEBOUNCER_TIME_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delete_bouncer_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,EXT_TRIG_DEBOUNCER_EN,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delete_bouncer_time enable failed\n", __func__);
        return -EIO;
    }

    hd_trig_del_bou_time->time = ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
                                 ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
                                 ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    hd_trig_del_bou_time->enable = temp_e;

    return ret;
}

/*
* Hardware Trigger模式下，触发信号的去毛刺属性。凡小于value(us)的触发信号将被忽略。
* note: 仅CS-MIPI-SC132支持。 
*/
static int cssc132_ctrl_write_hardware_trigger_delete_bouncer_time(struct cssc132_ctrl_device *dev, 
                                                                  struct hardware_trigger_delete_bouncer_time hd_trig_del_bou_time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;

    if(hd_trig_del_bou_time.enable > 1)
    {
        dev_err(&client->dev,"%s: error: input hardware_trigger_delete_bouncer_time enable = %d error,should be 0/1\n", 
                __func__,hd_trig_del_bou_time.enable);
        return -EIO;
    }

    if(hd_trig_del_bou_time.time > 0x00FFFFFF)
    {
        dev_err(&client->dev,"%s: error: input hardware_trigger_delete_bouncer_time = %d error,should less than 0xFFFFFF\n", 
                __func__,hd_trig_del_bou_time.time);
        return -EIO;
    }

    temp_h = (unsigned char)((hd_trig_del_bou_time.time >> 16) & 0x000000FF);
    temp_m = (unsigned char)((hd_trig_del_bou_time.time >>  8) & 0x000000FF);
    temp_l = (unsigned char)((hd_trig_del_bou_time.time >>  0) & 0x000000FF);

    ret = cssc132_ctrl_write_reg(dev, EXT_TRIG_DEBOUNCER_TIME_L, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delete_bouncer_time l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, EXT_TRIG_DEBOUNCER_TIME_M, temp_m);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delete_bouncer_time m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, EXT_TRIG_DEBOUNCER_TIME_H, temp_h);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delete_bouncer_time h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, EXT_TRIG_DEBOUNCER_EN, hd_trig_del_bou_time.enable);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delete_bouncer_time enable failed\n", __func__);
        return -EIO;
    }

    return ret;
}

/*
* Hardware Trigger模式下，触发延迟，单位为us. 
* note: 仅CS-MIPI-SC132支持。 
*/
static int cssc132_ctrl_read_hardware_trigger_delay(struct cssc132_ctrl_device *dev, unsigned int *time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    ret = cssc132_ctrl_read_reg(dev,TRIG_DLY_L,&temp_l,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delay l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,TRIG_DLY_M,&temp_m,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delay m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,TRIG_DLY_H,&temp_h,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delay h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev,TRIG_DLY_E,&temp_e,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read hardware_trigger_delay e failed\n", __func__);
        return -EIO;
    }

    *time = ((((unsigned int)temp_e) << 24) & 0xFF000000) + 
            ((((unsigned int)temp_h) << 16) & 0x00FF0000) + 
            ((((unsigned int)temp_m) <<  8) & 0x0000FF00) + 
            ((((unsigned int)temp_l) <<  0) & 0x000000FF);

    return ret;
}

/*
* Hardware Trigger模式下，触发延迟，单位为us. 
* note: 仅CS-MIPI-SC132支持。 
*/
static int cssc132_ctrl_write_hardware_trigger_delay(struct cssc132_ctrl_device *dev, unsigned int time)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_m = 0;
    unsigned char temp_h = 0;
    unsigned char temp_e = 0;

    temp_e = (unsigned char)((time >> 24) & 0x000000FF);
    temp_h = (unsigned char)((time >> 16) & 0x000000FF);
    temp_m = (unsigned char)((time >>  8) & 0x000000FF);
    temp_l = (unsigned char)((time >>  0) & 0x000000FF);

    ret = cssc132_ctrl_write_reg(dev, TRIG_DLY_L, temp_l);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delay l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, TRIG_DLY_M, temp_m);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delay m failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, TRIG_DLY_H, temp_h);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delay h failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, TRIG_DLY_E, temp_e);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write hardware_trigger_delay e failed\n", __func__);
        return -EIO;
    }

    return ret;
}

//pickmode是一个特殊的功能，一旦开启，sensor正常工作，但是模组将不输出图像，只有收到pickone指令，输出一张。 
static int cssc132_ctrl_read_pick_mode(struct cssc132_ctrl_device *dev, unsigned char *mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, PICK_MODE_EN, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read pick_mode failed\n", __func__);
        return -EIO;
    }

    *mode = temp;

    return ret;
}

//pickmode是一个特殊的功能，一旦开启，sensor正常工作，但是模组将不输出图像，只有收到pickone指令，输出一张。 
static int cssc132_ctrl_write_pick_mode(struct cssc132_ctrl_device *dev,unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode > 1)
    {
        dev_err(&client->dev,"%s: error: input pick_mode = %d error,should be 0/1\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, PICK_MODE_EN, mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write pick_mode failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

//在使能pickmode情况下，输出一张图像。 
static int cssc132_ctrl_write_pick_one(struct cssc132_ctrl_device *dev)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    ret = cssc132_ctrl_write_reg(dev, PICK_ONE, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write pick_one failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

static int cssc132_ctrl_read_mipi_status(struct cssc132_ctrl_device *dev, struct mipi_status *status)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp_l = 0;
    unsigned char temp_h = 0;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, MIPI_CNT_L, &temp_l, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read mipi_status count l failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_read_reg(dev, MIPI_CNT_H, &temp_h, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read mipi_status count h failed\n", __func__);
        return -EIO;
    }

    status->count = ((((unsigned short)temp_h) << 8) & 0xFF00) + 
                    ((((unsigned short)temp_l) << 0) & 0x00FF);

    ret = cssc132_ctrl_read_reg(dev, MIPI_STAT, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read mipi_status count l h failed\n", __func__);
        return -EIO;
    }

    status->start = temp;

    return ret;
}

/*
* value 	description
* 1 	重启程序
* 2 	完全重启(6-8秒)
*/
static int cssc132_ctrl_write_system_reboot(struct cssc132_ctrl_device *dev,unsigned char mode)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(mode != 1 && mode != 2)
    {
        dev_err(&client->dev,"%s: error: input system_reboot mode = %d error,should be 1/2\n", __func__,mode);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, SYSTEM_REBOOT, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write pick_one failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

/*
* 是否使能灯光同步信号。
* 如果使能，则在Strobe IO引脚在传感器曝光时输出高电平。 
*/
static int cssc132_ctrl_write_led_strobe_enable(struct cssc132_ctrl_device *dev,unsigned char enable)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char io_mode = 0;
    unsigned char sel = 0;

    if(enable > 1)
    {
        dev_err(&client->dev,"%s: error: input led_strobe_enable = %d error,should be 0/1\n", __func__,enable);
        return -EIO;
    }

    if(enable == 1)
    {
        io_mode = 1;
        sel = 3;
    }

    ret = cssc132_ctrl_write_reg(dev, STROBE_IO_MODE, io_mode);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write led_strobe io mode failed\n", __func__);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, STROBE_SEL, sel);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write led_strobe sel failed\n", __func__);
        return -EIO;
    }

    return ret; 
}

/*
* 读取摄像头的yuv顺序 
* 0     VYUY
* 1     YUYV
*/
static int cssc132_ctrl_read_yuv_sequence(struct cssc132_ctrl_device *dev, unsigned char *seq)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    unsigned char temp = 0;

    ret = cssc132_ctrl_read_reg(dev, YUV_SEQUENCE, &temp, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: read yuv_sequence failed\n", __func__);
        return -EIO;
    }

    *seq = temp;

    return ret;
}

/*
* 设置摄像头的yuv顺序 
* 0     VYUY
* 1     YUYV
*/
static int cssc132_ctrl_write_yuv_sequence(struct cssc132_ctrl_device *dev,unsigned char seq)
{
    int ret = 0;
    struct i2c_client *client = dev->client;

    if(seq > 1)
    {
        dev_err(&client->dev,"%s: error: input yuv_sequence = %d error,should be 0/1\n", __func__,seq);
        return -EIO;
    }

    ret = cssc132_ctrl_write_reg(dev, YUV_SEQUENCE, seq);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: write yuv_sequence failed\n", __func__);
        return -EIO;
    }

    return ret;
}

static int cssc132_ctrl_open(struct inode *inode, struct file *filp)
{
    struct cssc132_ctrl_device *dev = container_of(inode->i_cdev, struct cssc132_ctrl_device, cdev); 

	filp->private_data = dev;

	return 0;
}

static long cssc132_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
    struct cssc132_ctrl_device *dev = filp->private_data;
	struct i2c_client *client = dev->client;
	void __user *argp = (void __user *)arg;
    struct cssc132_support_format support_format;
    struct cssc132_format format;
    struct white_balance_state wb_state;
    struct hardware_trigger_delete_bouncer_time hd_trig_del_bou_time;
    struct mipi_status mipi_status_;
    struct exposure_state exposure_state_;
    struct gain_disassemble gain;
    unsigned char temp_u8 = 0;
    unsigned short temp_u16 = 0;
    unsigned int temp_u32 = 0;
 
	//检查设备类型
    if (_IOC_TYPE(cmd) != IOC_CSSC132_CTRL_MAGIC) 
	{
        dev_err(&client->dev,"%s: error: command type [%c] error\n", __func__, _IOC_TYPE(cmd));
        return -EPERM; 
    }

	//检查序数 
    if (_IOC_NR(cmd) > IOC_CSSC132_CTRL_MAX_NUM) 
	{ 
        dev_err(&client->dev,"%s: error: command numer [%d] exceeded\n", __func__, _IOC_NR(cmd));
        return -EPERM;
    }

    dev_err(&client->dev,"%s: error: ioctl read device_id failed\n", __func__);

	switch(cmd)
	{
		case IOC_CSSC132_CTRL_R_DEVICE_ID:
             ret = cssc132_ctrl_read_device_id(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read device_id failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy device_id to user failed\n", __func__);
				return -EIO;
			} 
		break;

         case IOC_CSSC132_CTRL_R_HARDWARE_VER:
            ret = cssc132_ctrl_read_hardware_version(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read hardware_version failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_version to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_FIRMWARE_VER:
            ret = cssc132_ctrl_read_firmware_version(dev, &temp_u16);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read firmware_version failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u16, sizeof(unsigned short));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy firmware_version to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_CAM_CAP:
            ret = cssc132_ctrl_read_camera_capability(dev, &temp_u16);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read camera_capability failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u16, sizeof(unsigned short));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy camera_capability to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_PRODUCT_MODULE:
            ret = cssc132_ctrl_read_product_module(dev, &temp_u16);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read product_module failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u16, sizeof(unsigned short));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy product_module to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_SUPPORT_FORMAT:
            ret = cssc132_ctrl_read_support_format(dev, &support_format);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read support_format failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &support_format, sizeof(struct cssc132_support_format));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy support_format to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_CURRENT_FORMAT:
            ret = cssc132_ctrl_read_current_format(dev, &format);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read current_format failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &format, sizeof(struct cssc132_format));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy current_format to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_ISP_CAP:
            ret = cssc132_ctrl_read_isp_capability(dev, &temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read isp_capability failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u32, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy isp_capability to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_POWER_HZ:
            ret = cssc132_ctrl_read_power_hz(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read power_hz failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy power_hz to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_I2C_ADDR:
            ret = cssc132_ctrl_read_i2c_addr(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read i2c_addr failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy i2c_addr to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_STREAM_MODE:
            ret = cssc132_ctrl_read_stream_mode(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read stream_mode failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy stream_mode to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_DAY_NIGHT_MODE:
            ret = cssc132_ctrl_read_day_night_mode(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read day_night_mode failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy day_night_mode to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_HUE:
            ret = cssc132_ctrl_read_hue(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read hue failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hue to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_CONTRAST:
            ret = cssc132_ctrl_read_contrast(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read contrast failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy contrast to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_STATURATION:
            ret = cssc132_ctrl_read_saturation(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read saturation failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy saturation to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_EXPOSURE_STATE:
            ret = cssc132_ctrl_read_exposure_state(dev, &exposure_state_);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read exposure_state failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &exposure_state_, sizeof(struct exposure_state));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy exposure_state to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_WB_STATE:
            ret = cssc132_ctrl_read_wb_state(dev, &wb_state);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read wb_state failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &wb_state, sizeof(struct white_balance_state));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy wb_state to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_EXPOSURE_FRAME_MODE:
            ret = cssc132_ctrl_read_exposure_frame_mode(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read exposure_frame_mode failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy exposure_frame_mode to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_SLOW_SHUTTER_GAIN:
            ret = cssc132_ctrl_read_slow_shutter_gain(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read slow_shutter_gain failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy slow_shutter_gain to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_EXPOSURE_MODE:
            ret = cssc132_ctrl_read_exposure_mode(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read exposure_mode failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy exposure_mode to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_TIME:
            ret = cssc132_ctrl_read_manual_exposure_time(dev, &temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read manual_exposure_time failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u32, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy manual_exposure_time to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_AGAIN:
            ret = cssc132_ctrl_read_manual_exposure_again(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read manual_exposure_again failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy manual_exposure_again to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_DGAIN:
            ret = cssc132_ctrl_read_manual_exposure_dgain(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read manual_exposure_dgain failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy manual_exposure_dgain to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_TIME:
            ret = cssc132_ctrl_read_direct_manual_exposure_time(dev, &temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read direct_manual_exposure_time failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u32, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy direct_manual_exposure_time to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_AGAIN:
            ret = cssc132_ctrl_read_direct_manual_exposure_again(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read direct_manual_exposure_again failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy direct_manual_exposure_again to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_DGAIN:
            ret = cssc132_ctrl_read_direct_manual_exposure_dgain(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read direct_manual_exposure_dgain failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy direct_manual_exposure_dgain to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_AWB_MODE:
            ret = cssc132_ctrl_read_awb_mode(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read awb_mode failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy awb_mode to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_MWB_COLOR_TEMPERATURE:
            ret = cssc132_ctrl_read_mwb_color_temperature(dev, &temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read mwb_color_temperature failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u32, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy mwb_color_temperature to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_MWB_GAIN:
            ret = cssc132_ctrl_read_mwb_gain(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read mwb_gain failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy mwb_gain to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_IMAGE_DIRECTION:
            ret = cssc132_ctrl_read_image_direction(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read image_direction failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy image_direction to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_EXPOSURE_TARGET_BRIGHTNESS:
            ret = cssc132_ctrl_read_auto_exposure_target_brightness(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read auto_exposure_target_brightness failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy auto_exposure_target_brightness to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_AUTO_EXPOSURE_MAX_TIME:
            ret = cssc132_ctrl_read_auto_exposure_max_time(dev, &temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read auto_exposure_max_time failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u32, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy auto_exposure_max_time to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_AUTO_EXPOSURE_MAX_GAIN:
            ret = cssc132_ctrl_read_auto_exposure_max_gain(dev, &gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read auto_exposure_max_gain failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &gain, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy auto_exposure_max_gain to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_EDGE:
            ret = cssc132_ctrl_read_hardware_trigger_edge(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read hardware_trigger_edge failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_trigger_edge to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_DELETE_BOUNCER_TIME:
            ret = cssc132_ctrl_read_hardware_trigger_delete_bouncer_time(dev, &hd_trig_del_bou_time);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read hardware_trigger_delete_bouncer_time failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &hd_trig_del_bou_time, sizeof(struct hardware_trigger_delete_bouncer_time));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_trigger_delete_bouncer_time to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_DELAY:
            ret = cssc132_ctrl_read_hardware_trigger_delay(dev, &temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read hardware_trigger_delay failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u32, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_trigger_delay to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_PICK_MODE:
            ret = cssc132_ctrl_read_pick_mode(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read pick_mode failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy pick_mode to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_MIPI_STATUS:
            ret = cssc132_ctrl_read_mipi_status(dev, &mipi_status_);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read mipi_status failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &mipi_status_, sizeof(struct mipi_status));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy mipi_status to user failed\n", __func__);
				return -EIO;
			}
		break;

        case IOC_CSSC132_CTRL_R_YUV_SEQUENCE:
            ret = cssc132_ctrl_read_yuv_sequence(dev, &temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl read yuv_sequence failed\n", __func__);
				return -EPERM;
            }

            ret = copy_to_user(argp, &temp_u8, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy yuv_sequence to user failed\n", __func__);
				return -EIO;
			}
        break;

        case IOC_CSSC132_CTRL_W_CURRENT_FORMAT:
            ret = copy_from_user(&format, argp, sizeof(struct cssc132_format));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy current_format from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_current_format(dev, format);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write current_format failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_PARAMS_SAVE:
            ret = cssc132_ctrl_write_params_save(dev);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write params_save failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_POWER_HZ:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy power_hz from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_power_hz(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write power_hz failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_SYS_RESET:
            ret = cssc132_ctrl_write_sys_reset(dev);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write sys_reset failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_I2C_ADDR:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy i2c_addr from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_i2c_addr(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write i2c_addr failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_STREAM_MODE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy stream_mode from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_stream_mode(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write stream_mode failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_DAY_NIGHT_MODE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy day_night_mode from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_day_night_mode(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write day_night_mode failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_HUE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hue from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_hue(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write hue failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_CONTRAST:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy contrast from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_contrast(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write contrast failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_STATURATION:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy saturation from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_saturation(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write saturation failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_EXPOSURE_FRAME_MODE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy exposure_frame_mode from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_exposure_frame_mode(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write exposure_frame_mode failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_SLOW_SHUTTER_GAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy slow_shutter_gain from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_slow_shutter_gain(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write slow_shutter_gain failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_EXPOSURE_MODE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy exposure_mode from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_exposure_mode(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write exposure_mode failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_TIME:
            ret = copy_from_user(&temp_u32, argp, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy manual_exposure_time from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_manual_exposure_time(dev, temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write manual_exposure_time failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_AGAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy manual_exposure_again from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_manual_exposure_again(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write manual_exposure_again failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_DGAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy manual_exposure_dgain from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_manual_exposure_dgain(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write manual_exposure_dgain failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_TIME:
            ret = copy_from_user(&temp_u32, argp, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy direct_manual_exposure_time from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_direct_manual_exposure_time(dev, temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write direct_manual_exposure_time failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_AGAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy direct_manual_exposure_again from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_direct_manual_exposure_again(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write direct_manual_exposure_again failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_DGAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy direct_manual_exposure_dgain from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_direct_manual_exposure_dgain(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write direct_manual_exposure_dgain failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_AWB_MODE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy awb_mode from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_awb_mode(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write awb_mode failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_MWB_COLOR_TEMPERATURE:
            ret = copy_from_user(&temp_u32, argp, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy mwb_color_temperature from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_mwb_color_temperature(dev, temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write mwb_color_temperature failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_MWB_GAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy mwb_gain from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_mwb_gain(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write mwb_gain failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_IMAGE_DIRECTION:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy image_direction from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_image_direction(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write image_direction failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_EXPOSURE_TARGET_BRIGHTNESS:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy auto_exposure_target_brightness from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_auto_exposure_target_brightness(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write auto_exposure_target_brightness failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_AUTO_EXPOSURE_MAX_TIME:
            ret = copy_from_user(&temp_u32, argp, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy auto_exposure_max_time from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_auto_exposure_max_time(dev, temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write auto_exposure_max_time failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_AUTO_EXPOSURE_MAX_GAIN:
            ret = copy_from_user(&gain, argp, sizeof(struct gain_disassemble));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy auto_exposure_max_gain from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_auto_exposure_max_gain(dev, gain);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write auto_exposure_max_gain failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_SOFTWARE_TRIGGER_ONE:
            ret = cssc132_ctrl_write_software_trigger_one(dev);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write software_trigger_one failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_EDGE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_trigger_edge from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_hardware_trigger_edge(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write hardware_trigger_edge failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_DELETE_BOUNCER_TIME:
            ret = copy_from_user(&hd_trig_del_bou_time, argp, sizeof(struct hardware_trigger_delete_bouncer_time));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_trigger_delete_bouncer_time from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_hardware_trigger_delete_bouncer_time(dev, hd_trig_del_bou_time);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write hardware_trigger_delete_bouncer_time failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_DELAY:
            ret = copy_from_user(&temp_u32, argp, sizeof(unsigned int));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy hardware_trigger_delay from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_hardware_trigger_delay(dev, temp_u32);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write hardware_trigger_delay failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_PICK_MODE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy pick_mode from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_pick_mode(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write pick_mode failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_PICK_ONE:
            ret = cssc132_ctrl_write_pick_one(dev);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write pick_one failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_SYSTEM_REBOOT:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy system_reboot from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_system_reboot(dev,temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write system_reboot failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_LED_STROBE_ENABLE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy led_strobe_enable from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_led_strobe_enable(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write led_strobe_enable failed\n", __func__);
				return -EPERM;
            }
		break;

        case IOC_CSSC132_CTRL_W_YUV_SEQUENCE:
            ret = copy_from_user(&temp_u8, argp, sizeof(unsigned char));
			if(ret)
			{
				dev_err(&client->dev,"%s: error: ioctl copy yuv_sequence from user failed\n", __func__);
				return -EIO;
			}

            ret = cssc132_ctrl_write_yuv_sequence(dev, temp_u8);
            if(ret != 0)
            {
                dev_err(&client->dev,"%s: error: ioctl write yuv_sequence failed\n", __func__);
				return -EPERM;
            }
        break;

		default:
            return -EINVAL;
	}

	return ret;
}

static const struct file_operations cssc132_ctrl_dev_ops = {
	.owner 			= THIS_MODULE,
	.open 			= cssc132_ctrl_open,
	.unlocked_ioctl = cssc132_ctrl_ioctl,
};

static int cssc132_ctrl_mode_init(struct cssc132_ctrl_device *dev)
{
    int ret = 0;
    struct i2c_client *client = dev->client;
    struct cssc132_format format;
    struct gain_disassemble gain;

    format.widht = 1280;
    format.height = 720;
    format.frame_rate = 60;

    gain.inter = 115;
    gain.dec = 3;

    ret = cssc132_ctrl_write_current_format(dev, format);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default format(w:1280 h:720 fm:60) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_power_hz(dev, 50);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default power frequency(50 Hz) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_stream_mode(dev, 0);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default stream mode(free running) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_day_night_mode(dev, 0);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default day night mode(Trigger Mode) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_hue(dev, 50);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default hue(50) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_contrast(dev, 50);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default contrast(50) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_saturation(dev, 50);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default saturation(50) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_exposure_mode(dev, 0);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default exposure mode(auto) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_auto_exposure_target_brightness(dev,56);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default auto exposure target brightness(56) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_auto_exposure_max_time(dev, 0xFFFFFFFF);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default auto exposure max time(auto) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_auto_exposure_max_gain(dev, gain);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default auto exposure max gain(115.3) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_awb_mode(dev, 0);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default auto white balance mode(auto white balance) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_image_direction(dev,0);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default image dirction(normal) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_led_strobe_enable(dev,1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default led strobe(enable) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_exposure_frame_mode(dev, 1);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default exposure frame mode(fixed frame rate) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_yuv_sequence(dev, 0);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: set default yuv sequence(UYVY) failed\n", __func__);
    }

    ret = cssc132_ctrl_write_params_save(dev);
    if(ret != 0)
    {
        dev_err(&client->dev,"%s: error: ioctl write params_save failed\n", __func__);
        return -EPERM;
    }

    return ret;
}

static int cssc132_ctrl_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret = 0;
    static unsigned char probe_cnt = 0;
    struct cssc132_ctrl_device *cssc132_ctrl_dev = NULL;
    
	dev_info(&client->dev, "%s: hello: %dst cssc132_ctrl device probing\n",__func__,probe_cnt ++);

    cssc132_ctrl_dev = (struct cssc132_ctrl_device *)kzalloc(sizeof(struct cssc132_ctrl_device),GFP_KERNEL);
    if(cssc132_ctrl_dev == NULL)
    {
        dev_err(&client->dev, "%s: error: Failed to malloc %dst cssc132_ctrl_device\n",__func__,probe_cnt);
        return -EINVAL;
    }

	cssc132_ctrl_dev->client = client;

	dev_set_drvdata(&client->dev, cssc132_ctrl_dev);

    cssc132_ctrl_dev->devno = find_first_zero_bit(cssc132_ctrl_minors, DEVICE_NUMBER);
	if(cssc132_ctrl_dev->devno >= DEVICE_NUMBER)
    {
		dev_err(&client->dev, "%s: error: find device num failed.\n",__func__);
		goto error0;
	}

	cdev_init(&cssc132_ctrl_dev->cdev, &cssc132_ctrl_dev_ops);
	cssc132_ctrl_dev->cdev.owner = THIS_MODULE;
	cssc132_ctrl_dev->cdev.ops = &cssc132_ctrl_dev_ops;

	ret = cdev_add(&cssc132_ctrl_dev->cdev, MKDEV(MAJOR(cssc132_ctrl_devid), cssc132_ctrl_dev->devno), 1);
	if(ret)
	{
		dev_err(&client->dev, "%s: error: cdev add failed.\n",__func__);
		goto error0;
	}

	cssc132_ctrl_dev->device = device_create(cssc132_ctrl_class, 
                                             &client->dev,
                                             MKDEV(MAJOR(cssc132_ctrl_devid), cssc132_ctrl_dev->devno), 
                                             NULL,
                                             "%s%d", 
                                             DEVICE_NAME, 
                                             cssc132_ctrl_dev->devno);
	if (IS_ERR(cssc132_ctrl_dev->device)) 
	{
		ret = PTR_ERR(cssc132_ctrl_dev->device);
        dev_err(&client->dev, "%s: error: device create failed.ret = %d\n",__func__,ret);
		goto error1;
	}

    ret = cssc132_ctrl_mode_init(cssc132_ctrl_dev);
	if (ret)
	{
		dev_err(&client->dev, "%s: error: Failed to init cssc132 sensor to default mode\n",__func__);
		goto error2;
	}

    set_bit(cssc132_ctrl_dev->devno, cssc132_ctrl_minors);

	return 0;

error2:
    device_destroy(cssc132_ctrl_class, MKDEV(MAJOR(cssc132_ctrl_devid), cssc132_ctrl_dev->devno));

error1:
	cdev_del(&cssc132_ctrl_dev->cdev);

error0:
    if (cssc132_ctrl_dev->devno < DEVICE_NUMBER)
    {
        clear_bit(cssc132_ctrl_dev->devno, cssc132_ctrl_minors);
    }
    dev_set_drvdata(&client->dev, NULL);
	kfree(cssc132_ctrl_dev);	

	return -EINVAL;
}

static int cssc132_ctrl_remove(struct i2c_client *client)
{
    static unsigned char remove_cnt = 0;
    struct cssc132_ctrl_device *dev = i2c_get_clientdata(client);

    dev_info(&client->dev, "%s: bye: %dst cssc132_ctrl device removing\n",__func__,remove_cnt ++);

	dev_set_drvdata(&client->dev, NULL);
	
    /* 注销设备 */
	device_destroy(cssc132_ctrl_class, MKDEV(MAJOR(cssc132_ctrl_devid), dev->devno));

	/* 删除cdev */
	cdev_del(&dev->cdev);

    clear_bit(dev->devno, cssc132_ctrl_minors);

    kfree(dev);

	return 0;
}

static const struct of_device_id cssc132_ctrl_of_match[] = {
	{ .compatible = "eyestar,cssc132-ctrl" },
	{ /* Sentinel */ }
};

const struct i2c_device_id cssc132_ctrl_id[] = {    
    { "eyestar,cssc132-ctrl", 0 },
    { }
};

static struct i2c_driver cssc132_ctrl_driver = {
	.driver = {
		.name			= DEVICE_NAME,
		.owner 			= THIS_MODULE,
		.of_match_table	= cssc132_ctrl_of_match,
	},
	.probe		= cssc132_ctrl_probe,		// probe函数
	.remove		= cssc132_ctrl_remove,		// remove函数
	.id_table 	= cssc132_ctrl_id,
};

static int __init cssc132_ctrl_init(void)
{
	int ret = 0;

    //以下顺序不可颠倒
    //第一步 创建类
    cssc132_ctrl_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(cssc132_ctrl_class)) 
	{
		printk(KERN_ERR "%s: error: class create failed.\n",__func__);
		return PTR_ERR(cssc132_ctrl_class);
	}

    //第二步 申请设备号
	ret = alloc_chrdev_region(&cssc132_ctrl_devid, 0, DEVICE_NUMBER, DEVICE_NAME);
	if (ret)
	{
        class_destroy(cssc132_ctrl_class);
		printk(KERN_ERR "%s: error: Failed to alloc_chrdev_region\n",__func__);
		return ret;
	}

    //第三步
    ret = i2c_add_driver(&cssc132_ctrl_driver);
    if(ret != 0)
    {
        printk(KERN_ERR "Failed to register cssc132_ctrl I2C driver: %d\n",ret);
        return ret;
    }

	return ret;
}

static void __exit cssc132_ctrl_exit(void)
{
	i2c_del_driver(&cssc132_ctrl_driver);
    unregister_chrdev_region(cssc132_ctrl_devid, DEVICE_NUMBER);
	class_destroy(cssc132_ctrl_class);
}

module_init(cssc132_ctrl_init);
module_exit(cssc132_ctrl_exit);

//module_i2c_driver(cssc132_ctrl_driver);

MODULE_AUTHOR("EyeStar");
MODULE_DESCRIPTION("cssc132 camera i2c control driver");
MODULE_LICENSE("GPL");
