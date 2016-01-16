/*
 * RxEibd.h
 *
 *  Created on: 30 окт. 2014 г.
 *      Author: nuh
 */

#ifndef RXEIBD_H_
#define RXEIBD_H_
//#include "ledcontrol.h"

struct listGroupAddr{
	char groupAddr[10]; //  1/1/1
	int value;			//	1
	char mpdip[16];		//192.168.123.39
	int  mpdport;		//6600
	char mpdcmd[128];	//	play
	int hGA;			//  group address in hex 039A
};

int mpdControl(char *mpdcmd, char *mpdip, int mpdport); //Send command to mpd




#endif /* RXEIBD_H_ */
