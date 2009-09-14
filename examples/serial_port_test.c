/**
* @file serial port I/O, a hardware queueing example.
* @author Karl Hiramoto <karl@hiramoto.org>
* @brief simple example showing example queueing commands on serial port.
*
* @NOTE this example is meant to be run with a loopback cable.
*  Cross RX and TX.  Pins 2 and 3 on a DB 9 connector.
* 
*
* Example code is Distributed under Dual BSD / LGPL licence
* Copyright 2009 Karl Hiramoto
*/
#define _GNU_SOURCE

#include <stdlib.h>             /* Standard lib */
#include <stdio.h>              /* Standard input/output definitions */
#include <string.h>             /* String function definitions */


#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)

#pragma once

#include "windows.h"            // big fat windows lib

#else // on a unix/linux system
#include <sys/time.h>           // for timeing, timeouts, pselect
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>             /* UNIX standard function definitions */
#include <fcntl.h>              /* File control definitions */
#include <errno.h>              /* Error number definitions */
#include <termios.h>            /* POSIX terminal control definitions */

#include <sys/select.h>
#endif
#include "workqueue.h"

#define NUM_COMMANDS 5
static const char *port_commands[NUM_COMMANDS+1] = {
	"command 0",
	"command 1",
	"command 2",
	"command 3",
	"command 4",
	"command 5",
};

struct prg_ctx
{
	struct workqueue_ctx *ctx;
	int counter;
	int fd;
};


int write_serial_data(struct prg_ctx *prg_ctx, const char *buffer, unsigned int size)
{

	char TempStr[16];
	int ret;
#ifdef WIN32
	DWORD Bytes_Written = 0;
	ret = WriteFile (prg_ctx->fd, (void *) buffer, size, &Bytes_Written, NULL);
	if (ret < 1)
	{
		perror ("Error Writing to Port WriteBuffer()");
		Error_String = " Error Writing to Port WriteBuffer ";
		return false;
	}
#else
	ret = write (prg_ctx->fd, buffer, size);	// send all at once

	if (ret < 1)
	{
		snprintf (TempStr, 14, "Writing errno=%d", errno);
		perror(TempStr);
		return -errno;

	}

	if (tcdrain (prg_ctx->fd) < 0)
	{
		perror("tcdrain");
		return -errno;	// error writing.  errno set
	}
#endif

	return 0;
}


int read_serial_data (struct prg_ctx *prg_ctx,char *Buffer, int Buff_Size)
{

	int select_retval = 0;
	int ret = 0;
#ifdef WIN32
	// debug me.
	COMSTAT Win_ComStat;
	DWORD Win_BytesRead = 0;
	DWORD Win_ErrorMask = 0;
	ClearCommError (prg_ctx->fd, &Win_ErrorMask, &Win_ComStat);
	if (Win_ComStat.cbInQue && (!ReadFile (prg_ctx->fd, (void *) Buffer, 2048, &Win_BytesRead, NULL) || Win_BytesRead == 0))
	{
		//lastErr=E_READ_FAILED;
		*Read_Size = -1;
	}
	else
	{
		*Read_Size = ((int) Win_BytesRead);
	}
	if (*Read_Size < 0)
	{
		Error_String += "Error in SerialClass::Read";
		return false;
	}
	else
	{			// no error
//                      Data=Buffer;
		return true;
	}

#else // POSIX linux code

	struct timeval tv;
	fd_set read_fileset;


	// setup timeout
	tv.tv_sec = 1;		//wait 1 second max
	tv.tv_usec = 0;

	FD_ZERO (&read_fileset);
	FD_SET (prg_ctx->fd, &read_fileset);

	select_retval = select (prg_ctx->fd + 1, &read_fileset, NULL, NULL, &tv);

	if (select_retval < 0)
	{
		perror ("ERROR select()");
		return -1;
	}
	else if (select_retval == 0)
	{			// no data to read
		return -ENODATA;
	}
	else if (FD_ISSET(prg_ctx->fd, &read_fileset))
	{
		ret = read (prg_ctx->fd, Buffer, Buff_Size);
	}

	if (ret < 0)
	{
		perror ("read error");
		return -errno;
	}

	return ret;
#endif

}

int config_serial_port(struct prg_ctx *prg_ctx,int baud, unsigned char data_bits, char parity, unsigned char stop_bits)
{
// 	const short int Data_Bits = 8;
// 	const short int Stop_Bits = 1;
#ifdef WIN32
	unsigned long confSize;

	/*configure port settings */
	if (!GetCommConfig (fd, &Win_CommConfig, &confSize))
	{
		char TempStr[128];
		sprintf_s (TempStr, "Error Calling GetCommConfig in ConfigurePortDefault Error=%d", GetLastError ());
		Error_String = TempStr;
		return false;
	}
	if (!GetCommState (fd, &(Win_CommConfig.dcb)))
	{
		char TempStr[128];
		sprintf_s (TempStr, "Error Calling GetCommState in ConfigurePortDefault Error=%d", GetLastError ());
		Error_String = TempStr;
		return false;
	}
	Win_CommConfig.dcb.fBinary = TRUE;	// Windows does not support nonbinary mode transfers, so this member must be TRUE.
	Win_CommConfig.dcb.fInX = FALSE;	// XON/XOFF  disabled
	Win_CommConfig.dcb.fOutX = FALSE;	// XON/XOFF disabled
	Win_CommConfig.dcb.fAbortOnError = FALSE;
	Win_CommConfig.dcb.fNull = FALSE;

	if (!SetBaudRate (baud))
	{
		return false;
	}

	if (!SetDataBits (Data_Bits))
		return false;

	if (!SetStopBits (Stop_Bits))
		return false;

	if (!SetParity (parity))
		return false;

	if (!SetFlowControl (FLOW_OFF))
		return false;

	//setTimeout(0,10);
	if (SetCommConfig (fd, &Win_CommConfig, sizeof (Win_CommConfig)))
	{
		return 0;
	}
	else
	{
		char TempStr[128];
		sprintf_s (TempStr, " in ConfigurePortDefault SetCommConfig Error: %d ", GetLastError ());
		Error_String += TempStr;
		return false;
	}

#else // POSIX linux
	struct termios options;
	if (tcgetattr (prg_ctx->fd, &options) < 0)	// get the opts
	{
		perror("tcgetattr");
		return -errno;
	}


	if (baud == 1200)
	{
		cfsetispeed (&options, B1200);
		cfsetospeed (&options, B1200);
	}

	else if (baud == 2400)
	{
		cfsetispeed (&options, B2400);
		cfsetospeed (&options, B2400);
	}
	else if (baud == 4800)
	{
		cfsetispeed (&options, B4800);
		cfsetospeed (&options, B4800);
	}
	else if (baud == 9600)
	{
		cfsetispeed (&options, B9600);
		cfsetospeed (&options, B9600);
	}
	else if (baud == 19200)
	{
		cfsetispeed (&options, B19200);
		cfsetospeed (&options, B19200);
	}
	else if (baud == 57600)
	{
		cfsetispeed (&options, B57600);
		cfsetospeed (&options, B57600);
	}
	else
		return -EINVAL;

	// set to no parity 8N1

	if (parity == 'N')
		options.c_cflag &= ~PARENB;	// no parity clear flag
	else
	{
		options.c_cflag |= PARENB;	// set parity flag

		if (parity == 'E')
			options.c_cflag &= ~PARODD;	// clear odd bit
		else if (parity == 'O')
			options.c_cflag |= PARODD;	// set odd bit
		else		//unknown opt
			return -EINVAL;
	}

	if (stop_bits == 1)
		options.c_cflag &= ~CSTOPB;	// clear flag. one stop bit
	else if (stop_bits == 2)
		options.c_cflag |= CSTOPB;	// set flag. 2 stop bits
	else
		return -EINVAL;	//unknown opt


	options.c_cflag &= ~CSIZE;	// clear data bits mask

	if (data_bits == 8)
		options.c_cflag |= CS8;
	else if (data_bits == 7)
		options.c_cflag |= CS7;
	else if (data_bits == 6)
		options.c_cflag |= CS6;
	else
		return -EINVAL;	//unknown opt;


	//  options.c_oflag     &= ~OPOST;

	// disable flow control
//    options.c_cflag &= ~CNEW_RTSCTS;
	/*
	 * Enable the receiver and set local mode...
	 */

	options.c_cflag |= (CLOCAL | CREAD);

	//   options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG); // clear these flags. canonical input, echo, echo errase
	options.c_lflag = 0;	// disable 

	// set raw output.  don't do any CR LF translations
	options.c_oflag &= ~OPOST;	// clear post process flag.  (raw)
	options.c_oflag &= ~OLCUC;	// clear 
	options.c_oflag &= ~ONLCR;
	options.c_oflag &= ~OCRNL;

	options.c_iflag &= ~(IXON | IXOFF | IXANY);	// turn off flow control
	options.c_iflag &= ~(ICRNL | INLCR | IGNCR | IUCLC);	// turn off Ignore CR, Map CR to NL, Map NL to CR


	options.c_cc[VMIN] = 0;	// minimum number of chars to read()
	options.c_cc[VTIME] = 10;	// time to wait for first char. in 10'ths of seconds


	options.c_cflag &= ~CRTSCTS;	// disable flow control 

	//   cfmakeraw( &options); // set in raw mode


	// Set the new options for the port...
	if (tcsetattr (prg_ctx->fd, TCSANOW, &options) < 0)
	{
		perror("tcsetattr");
		return -errno;
	}

	return 0;
#endif

}

int open_port (struct prg_ctx *prg_ctx, const char *dev_name)
{

#ifdef WIN32
	prg_ctx->fd = CreateFileA (dev_name.c_str (), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (fd == INVALID_HANDLE_VALUE)
	{			// Could not open the port.
		printf ("CreateFile failed with error %d.\n", GetLastError ());
#else
/*POSIX */
	prg_ctx->fd = open (dev_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (prg_ctx->fd == -1)
	{			// Could not open the port.

#endif
		char err_str[128];
		snprintf (err_str, 127, "open_port: Unable to open %s", dev_name);
		perror (err_str);
		return -errno;
	}
	else
	{
		//  fcntl(fd, F_SETFL, 0);  //restore blocking read

#ifdef WIN32
		// FIX ME  add non blocking flags for windows
#else
// 		fcntl (prg_ctx->fd, F_SETFL, O_NDELAY);// no delay on read.  no block
#endif

	}
	return 0;
}

void do_cmd(struct prg_ctx *prg, unsigned int i)
{
	int ret = 0;
	unsigned int len = 0;
	char buff[256];
	if (i < NUM_COMMANDS) {
		printf("sending cmd='%s'\n", port_commands[i]);
		len = strlen(port_commands[i]) +1;
		ret = write_serial_data(prg, port_commands[i], len);
		if (ret < 0) {
			fprintf(stderr, "error writing cmd %d\n", i);
			return;
		}
		usleep(12345);
		ret = read_serial_data (prg, buff, 255);

		if (ret < 0) {
			printf("error reading cmd %d : %d=%s \n",
				 i, ret, strerror_r(-ret, buff, 255) );
			return;
		}
		printf("Read cmd %d ='%s'\n",i, buff);
	} else {
		fprintf(stderr, "Invalid command");
	}

}

static void callback_func0(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	do_cmd(prg, 0);
}

static void callback_func1(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	do_cmd(prg, 1);
}
static void callback_func2(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	do_cmd(prg, 2);
}


static void callback_func3(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	do_cmd(prg, 3);
}
static void callback_func4(void *data)
{
	struct prg_ctx *prg = (struct prg_ctx *) data;
	do_cmd(prg, 4);
}


/**
 * 
 * @param argc 
 * @param argv[] 
 * @return 
 */
int main(int argc, char *argv[]) {
	struct prg_ctx prg = { .counter = 0};
	int ret;
	int i;
	printf("starting argc=%d\n", argc);
	if (argc < 2) {
		printf("usage %s <Serial Port>\n", argv[0]);
		return -1;
	}

	prg.ctx = workqueue_init(32, 1);
	

	ret = open_port (&prg, argv[1]);

	ret = config_serial_port(&prg, 9600, 8, 'N', 1);

	ret = workqueue_add_work(prg.ctx, 5, 0,
		callback_func0, &prg);

	if (ret >= 0) {
		printf("Added job %d \n", ret);
	} else {
		printf("Error adding job err=%d\n", ret);
	}

	ret = workqueue_add_work(prg.ctx, 5, 0,
		callback_func1, &prg);

	if (ret >= 0) {
		printf("Added job %d \n", ret);
	} else {
		printf("Error adding job err=%d\n", ret);
	}

	ret = workqueue_add_work(prg.ctx, 1, 0,
		callback_func2, &prg);

	if (ret >= 0) {
		printf("Added job %d \n", ret);
	} else {
		printf("Error adding job err=%d\n", ret);
	}

	ret = workqueue_add_work(prg.ctx, 1, 0,
		callback_func3, &prg);

	if (ret >= 0) {
		printf("Added job %d \n", ret);
	} else {
		printf("Error adding job err=%d\n", ret);
	}

	ret = workqueue_add_work(prg.ctx, 1, 0,
		callback_func4, &prg);

	if (ret >= 0) {
		printf("Added job %d \n", ret);
	} else {
		printf("Error adding job err=%d\n", ret);
	}

	workqueue_show_status(prg.ctx, stdout);


	workqueue_show_status(prg.ctx, stdout);



	for (i = 20; i && (ret = workqueue_get_queue_len(prg.ctx)); i--) {
	  	printf("waiting for %d jobs \n", ret);
		sleep(1);
	}
	sleep(2);
	workqueue_destroy(prg.ctx);

	return 0;
}