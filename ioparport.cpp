/* JTAG GNU/Linux parport device io

Copyright (C) 2004 Andrew Rogers

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */



#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "ioparport.h"

using namespace std;

const char* IOParport::usbErrorMessage(int errCode)
{
	static char buffer[80];
	
	switch(errCode){
		case USBOPEN_ERR_ACCESS:      return "Access to device denied";
		case USBOPEN_ERR_NOTFOUND:    return "The specified device was not found";
		case USBOPEN_ERR_IO:          return "Communication error with device";
		default:
			sprintf(buffer, "Unknown USB error %d", errCode);
			return buffer;
	}
	return NULL;    /* not reached */
}

usbDevice_t* IOParport::openDevice(void)
{
	usbDevice_t     *dev = NULL;
	unsigned char   rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
	char            vendorName[] = {USB_CFG_VENDOR_NAME, 0}, productName[] = {USB_CFG_DEVICE_NAME, 0};
	int             vid = rawVid[0] + 256 * rawVid[1];
	int             pid = rawPid[0] + 256 * rawPid[1];
	int             err;
	
	if((err = usbhidOpenDevice(&dev, vid, vendorName, pid, productName, 0)) != 0){
		fprintf(stderr, "error finding %s: %s\n", productName, usbErrorMessage(err));
		return NULL;
	}
	return dev;
}

IOParport::IOParport(const char *device_name) : IOBase()
{
	if((dev = openDevice()) == NULL) {
		error=true;
		return;
	}
	
	memset(txonlybuf, 0, sizeof(txonlybuf));
	txonlybufpos = 1;
	
	error=false;
}

bool IOParport::txrx(bool tms, bool tdi)
{
	int err;
	char buffer[2];    /* room for dummy report ID */
	if (txonlybufpos != 1) {
		buftx(tms, tdi, 4);
		flushtob();
	} else {
		buffer[0] = 0;
		buffer[1] = 4;
		if (tdi) buffer[1] |= 1;
		if (tms) buffer[1] |= 2;
		if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0) {   /* add a dummy report ID */
			fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));
			error = true;
		}
	}
	int len = sizeof(buffer);
	if((err = usbhidGetReport(dev, 0, buffer, &len)) != 0){
		fprintf(stderr, "error reading data: %s\n", usbErrorMessage(err));
		error = true;
	}
	return (buffer[1] & 1) == 1;
}

#define TOB txonlybuf[txonlybufpos]

void IOParport::buftx(bool tms, bool tdi, char startval) {
	if (TOB & 8) {
		TOB <<= 4;
		TOB |= startval;
		if (tdi) TOB |= 1;
		if (tms) TOB |= 2;
		if (++txonlybufpos == sizeof(txonlybuf)) flushtob();
	} else {
		TOB = startval | 8;
		if (tdi) TOB |= 1;
		if (tms) TOB |= 2;
	}
}

void IOParport::tx(bool tms, bool tdi)
{
	buftx(tms, tdi, 0);
}

void IOParport::flushtob() {
	int err;
	if (txonlybufpos == 1) return;
	if((err = usbhidSetReport(dev, txonlybuf, txonlybufpos)) != 0) {  /* add a dummy report ID */
		fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));
		error = true;
	}
	memset(txonlybuf, 0, sizeof(txonlybuf));
	txonlybufpos = 1;
}

IOParport::~IOParport()
{
	flushtob();
	usbhidCloseDevice(dev);
}
