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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "ioparport.h"

using namespace std;

IOParport::IOParport(const char *device_name) : IOBase()
{
  struct termios newtio;
  char c;
  fd = open (device_name, O_RDWR);
  
  if (fd == -1) {
    //perror ("open");
    error=true;
    return;
  }
  
  tcgetattr(fd, &oldtio);

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0;
  newtio.c_cc[VMIN] = 1;

  tcflush(fd, TCIFLUSH);
  tcsetattr(fd, TCSANOW, &newtio);

  fprintf(stderr, "Waiting for Arduino to load the sketch...");
  //read(fd, &c, 1);
  sleep(3);
  fprintf(stderr, " done\n");

  error=false;
}

bool IOParport::txrx(bool tms, bool tdi)
{
  unsigned char ret;
  unsigned char data=4; // rx mode
  if(tdi)data|=2;
  if(tms)data|=1;
  write(fd, &data, 1);
  read(fd, &ret, 1);
  return (ret & 1) == 1;
}

void IOParport::tx(bool tms, bool tdi)
{
  unsigned char data=0;
  if(tdi)data|=2;
  if(tms)data|=1;
  write(fd, &data, 1);
}
 
IOParport::~IOParport()
{
  usleep(0);
  tcsetattr(fd, TCSANOW, &oldtio);
  close (fd);

}
