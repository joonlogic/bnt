/********************************************************************
 *  FILE   : bnt_spi.c
 *  Author : joon
 *  Content : SPI driver interface with spidev
 ********************************************************************/

#include <stdio.h>
#include <unistd.h>
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



