KERN_DIR := /home/szp/work/nvidia/Linux_for_Tegra/source/public/kernel/kernel-4.9/kernel_output

obj-m := gpio_led.o

all:
	make ARCH=arm64 CROSS_COMPILE=/home/szp/tools/gcc-linaro-7.3.1-2018.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu- -C $(KERN_DIR) M=`pwd` modules

clean:
	make -C $(KERN_DIR) M=`pwd` clean