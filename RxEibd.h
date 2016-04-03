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
	char group_addr[10]; //  1/1/1
	int value;			//	1
	char send_to_ip[16];		//192.168.123.39
	int  send_to_port;		//6600
	char cmd_string[128];	//	play
	int group_addr_in_hex;			//  group address in hex 039A
};

int mpdControl(char *cmd_string, char *send_to_ip, int send_to_port, int verbose_switcher); //Send command to mpd
void convert_str_to_myListGA(char* parsed_cfg_str, listGroupAddr *myListGA, int index_gaddr);




#endif /* RXEIBD_H_ */
