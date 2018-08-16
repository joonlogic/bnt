/********************************************************************
 * bnt_spi.c : SPI driver interface with spidev
 *             GPIO IRQ interface with sysfs
 *
 * Copyright (c) 2018  TheFrons, Inc.
 * Copyright (c) 2018  Joon Kim <joonlogic@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include "bnt_def.h"
#include "bnt_ext.h"

int
do_open( 
		int bus,  
		int cs
)
{
	char buf[32]={0,};
	int fd=0;

	snprintf(buf, sizeof(buf), "/dev/spidev%d.%d", bus, cs);
	fd = open(buf, O_RDWR);
	if(fd < 0) 
		printf("%s: SPI %s open error. fd %d\n", __func__, buf, fd);

	return fd;
} 

int 
do_read(
		int fd, 
		void* buf,
		int len
	   )
{
	int ret = 0;
	BNT_CHECK_NULL(buf, -1);

	ret = read(fd, buf, len);
	BNT_CHECK_TRUE(ret == len, ret);

#ifdef DEBUG
	hexdump(buf, len, 16, "READ");
#endif

	return ret;
}

int 
do_write(
		int fd, 
		void* buf,
		int len
	   )
{
	int ret = 0;
	BNT_CHECK_NULL(buf, -1);

	ret = write(fd, buf, len);
	BNT_CHECK_TRUE(ret == len, ret);

#ifdef DEBUG
	hexdump(buf, len, 16, "WRITE");
#endif

	return ret;
}

int 
do_spi_tx_rx(
		int fd,
		unsigned char* txbuf,
		unsigned char* rxbuf,
		int txlen,
		int rxlen
		)
{
	int ret = 0;
	struct spi_ioc_transfer xfer[2] = {0,};
	BNT_CHECK_NULL(txbuf && rxbuf, -1);

	xfer[0].tx_buf = (unsigned long)txbuf;
	xfer[0].len = txlen;

	xfer[1].rx_buf = (unsigned long)rxbuf;
	xfer[1].len = rxlen;

	ret = ioctl(fd, SPI_IOC_MESSAGE(2), xfer);
	BNT_CHECK_TRUE(ret >= 0, ret);

#ifdef DEBUG
	hexdump(txbuf, txlen, 16, "(1) WRITE");
	hexdump(rxbuf, rxlen, 16, "(2) READ ");
#endif
	return ret;
}

//////////////////
//// GPIO 
/////////////////

#define STR_GPIO_SYS            "/sys/class/gpio"
#define STR_GPIO_EXPORT         "/export"
#define STR_GPIO_EXPORT_VAL     "%d"
#define STR_GPIO_UNEXPORT       "/unexport"
#define STR_GPIO_NUMBER         "/gpio%d"
#define STR_GPIO_DIR            "/direction"
#define STR_GPIO_DIR_READ       "in"
#define STR_GPIO_EDGE           "/edge"
#define STR_GPIO_EDGE_RISING    "rising"
#define STR_GPIO_ACT_LOW        "/active_low"
#define STR_GPIO_ACT_LOW_FALSE  "0"
#define STR_GPIO_VALUE          "/value"

/* 
 * RETURN : fd when open "/sys/class/gpio/gpio(n)/value"
 */
int
do_config_gpio_irq(
		int gpiopin,
		int disable
		)
{
	char strgpio[64]={0,};
	char strexp[64]={0,};
	char strgpiopin[8]={0,};
	struct stat sb={0,};

	int fd = 0;
	int ret = 0;

	//check unexport
	if(disable) {
		sprintf(strexp, STR_GPIO_SYS STR_GPIO_UNEXPORT);
		fd = open(strexp, O_WRONLY | O_SYNC);
		BNT_CHECK_TRUE(fd >= 0, -1);

		sprintf(strgpiopin, STR_GPIO_EXPORT_VAL, gpiopin);
		ret = write(fd, strgpiopin, (size_t)strlen(strgpiopin));
		if(ret != (ssize_t)strlen(strgpiopin)) {
			fprintf(stderr, "%s: failed to write %s to %s. (err %d)\n", 
					__func__, strgpiopin, strexp, ret<0 ? errno:ret);
			close(fd);
			return -1;
		}
		close(fd);
		return 0;
	}

	//check already existence
	sprintf(strgpio, STR_GPIO_SYS STR_GPIO_NUMBER, gpiopin);
	if(stat(strgpio, &sb) == 0) { //already exist
		BNT_CHECK_TRUE(S_ISDIR(sb.st_mode), -1);
	}
	else {
		//enable gpio
		sprintf(strexp, STR_GPIO_SYS STR_GPIO_EXPORT);
		fd = open(strexp, O_WRONLY | O_SYNC);
		BNT_CHECK_TRUE(fd >= 0, -1);
		
		sprintf(strgpiopin, STR_GPIO_EXPORT_VAL, gpiopin);
		ret = write(fd, strgpiopin, (size_t)strlen(strgpiopin));
		if(ret != (ssize_t)strlen(strgpiopin)) {
			fprintf(stderr, "%s: failed to write %s to %s. (err %d)\n", 
					__func__, strgpiopin, strexp, ret<0 ? errno:ret);
			close(fd);
			return -1;
		}
		close(fd);
		usleep(50000);
	}

	//direction
	char strdir[64]={0,};
	sprintf(strdir, STR_GPIO_SYS STR_GPIO_NUMBER STR_GPIO_DIR, gpiopin);
	fd = open(strdir, O_WRONLY | O_SYNC);
	BNT_CHECK_TRUE(fd >= 0, -1);

	ret = write(fd, STR_GPIO_DIR_READ, (size_t)strlen(STR_GPIO_DIR_READ));
	if(ret != (ssize_t)strlen(STR_GPIO_DIR_READ)) {
		fprintf(stderr, "%s: failed to write %s to %s. (err %d)\n", 
				__func__, STR_GPIO_DIR_READ, strdir, ret<0 ? errno:ret);
		close(fd);
		return -1;
	}
	close(fd);

	//edge
	char stredge[64]={0,};
	sprintf(stredge, STR_GPIO_SYS STR_GPIO_NUMBER STR_GPIO_EDGE, gpiopin);
	fd = open(stredge, O_WRONLY | O_SYNC);
	BNT_CHECK_TRUE(fd >= 0, -1);

	ret = write(fd, STR_GPIO_EDGE_RISING, (size_t)strlen(STR_GPIO_EDGE_RISING));
	if(ret != (ssize_t)strlen(STR_GPIO_EDGE_RISING)) {
		fprintf(stderr, "%s: failed to write %s to %s. (err %d)\n", 
				__func__, STR_GPIO_EDGE_RISING, stredge, ret<0 ? errno:ret);
		close(fd);
		return -1;
	}
	close(fd);

	//active_low
	char stract[64]={0,};
	sprintf(stract, STR_GPIO_SYS STR_GPIO_NUMBER STR_GPIO_ACT_LOW, gpiopin);
	fd = open(stract, O_WRONLY | O_SYNC);
	BNT_CHECK_TRUE(fd >= 0, -1);

	ret = write(fd, STR_GPIO_ACT_LOW_FALSE, (size_t)strlen(STR_GPIO_ACT_LOW_FALSE));
	if(ret != (ssize_t)strlen(STR_GPIO_ACT_LOW_FALSE)) {
		fprintf(stderr, "%s: failed to write %s to %s. (err %d)\n", 
				__func__, STR_GPIO_ACT_LOW_FALSE, stract, ret<0 ? errno:ret);
		close(fd);
		return -1;
	}
	close(fd);


	//value
	char strval[64]={0,};
	sprintf(strval, STR_GPIO_SYS STR_GPIO_NUMBER STR_GPIO_VALUE, gpiopin);
	fd = open(strval, O_RDONLY);
	BNT_CHECK_TRUE(fd >= 0, -1);

	return fd; 
}

