/*
    EIB Gate program - catch group telegrams and send appropriate command to ip socket
    write by Nuh 01.01.2015
*/
#include "common.h"
#include "eibclient.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "RxEibd.h"
#include <stdio.h>
#include <string.h>
#include "ledcontrol.h"

typedef unsigned char uchar;
#define LLGA	32	// Length of List of Group Address

int main (int argc, char *argv[])
{
  int len;
  EIBConnection *eibcon;
  eibaddr_t eibdest;
  eibaddr_t eibsrc;
  uchar eibbuf[200];
  int tmpeibval;

  FILE *ListGroupAddr = NULL;
  listGroupAddr myListGA[LLGA];
  eibaddr_t testga;
  int iga = 0;  // index for group address array
  char mcom[] = "next\n";
  int figa;	// real length of list of group address - counting due to read

  if (argc != 3)
    die ("usage: %s Url(ip:localhost:6720) file(with list of cmd)", argv[0]);
//Prepare KNX connection
  eibcon = EIBSocketURL (argv[1]);
  if (!eibcon)
    die ("Open failed");
  if (EIBOpen_GroupSocket (eibcon, 0) == -1)
    die ("Connect failed");
//Fill array from file
  if((ListGroupAddr = fopen(argv[2], "r")) == NULL)
	  die ("Error Open file with list of group address");
  //while(fscanf (ListGroupAddr, "%s%d%s%d%*c%[^\n]", myListGA[iga].groupAddr, &myListGA[iga].value, myListGA[iga].mpdip, &myListGA[iga].mpdport, myListGA[iga].mpdcmd) != EOF){
  while(fscanf (ListGroupAddr, "%s%d%s%d%s", myListGA[iga].groupAddr, &myListGA[iga].value, myListGA[iga].mpdip, &myListGA[iga].mpdport, myListGA[iga].mpdcmd) != EOF){
	  myListGA[iga].hGA = readgaddr((myListGA[iga].groupAddr));
	  iga++;
	  if(iga == LLGA)
		  break;
  }
  figa=iga;	// real number of monitoring group address
  for(iga=0; iga != figa; iga++)
	  printf("Result N:%d -> %s - %d - %s - %d - %s - %X\n", iga, myListGA[iga].groupAddr, myListGA[iga].value, myListGA[iga].mpdip, myListGA[iga].mpdport, myListGA[iga].mpdcmd, myListGA[iga].hGA);
  fclose(ListGroupAddr);

  while (1){
	  len = EIBGetGroup_Src (eibcon, sizeof (eibbuf), eibbuf, &eibsrc, &eibdest);
      if (len == -1)
    	  die ("Read failed");
      if (len < 2)
    	  die ("Invalid Packet");
      if ((eibbuf[0] & 0x3) || (eibbuf[1] & 0xC0) == 0xC0){
    	  printf ("Unknown APDU from ");
    	  printIndividual (eibsrc);
    	  printf (" to ");
    	  printGroup (eibdest);
    	  printf (": ");
    	  printHex (len, eibbuf);
    	  printf ("\n");
      }
      else{
    	  switch (eibbuf[1] & 0xC0){
    	  case 0x00:
    		  printf ("Read");
    		  break;
    	  case 0x40:
    		  printf ("Response");
    		  break;
    	  case 0x80:
    		  printf ("Write");
    		  break;
    	  }
    	  printf (" from ");
    	  printIndividual (eibsrc);
    	  printf (" to ");
    	  printGroup (eibdest);
    	  if (eibbuf[1] & 0xC0){
    		  printf (": ");
    		  if (len == 2){
    			  printf ("%02X", eibbuf[1] & 0x3F);
    		  }
    		  else
    			  printHex (len - 2, eibbuf + 2);
    	  }
    	  printf(" Destination in HEX is ->  %X ", eibdest);
    	  tmpeibval = eibbuf[1] & 0x3F;
    	  for(iga=0; iga != figa; iga++){
    		  if((myListGA[iga].hGA == eibdest) && (myListGA[iga].value == tmpeibval)){
    			  printf("\n Caught command => %s \n", myListGA[iga].mpdcmd);
    			  mpdControl(myListGA[iga].mpdcmd, myListGA[iga].mpdip, myListGA[iga].mpdport);
    		  }
    	  }
    	  fflush (stdout);
      }
  }
  EIBClose (eibcon);
  return 0;
}

int mpdControl(char *mpdcmd, char *mpdip, int mpdport)
{
	const char *lfeed = "\n";
	char tmpcmd[128]="";
	char namecmd[64]="";
	char * fptr; char * nptr; //Pointer to certain position in string (first? next)
	if(mpdcmd[0]=='#'){ 	  // Special command start with symbol #
		switch(mpdcmd[1]){
		case 'v':
			printf("case v\n");
			strcat(tmpcmd, "volume");
			tmpcmd[6]=' ';
			tmpcmd[7]=mpdcmd[2]; // + or -
			tmpcmd[8]=mpdcmd[3]; // real value of step of volume, for example 5
			tmpcmd[9]='\0';		 // End of string
			break;				 // Ready volume +5 or volume -5
		case 'p':
			printf("case p\n");
			strcat(tmpcmd, "clear");	//clear
			strcat(tmpcmd, lfeed);		//clear<LF>
			strcat(tmpcmd, "load ");	//clear<LF>load <-- with space
			fptr = strchr(mpdcmd, '"');
			nptr = strchr(fptr+1, '"');
			strncpy(namecmd, fptr, nptr-fptr+1); // extract name like "Radio"
			namecmd[(nptr-fptr+1)]='\0';
			strcat(tmpcmd, namecmd);	//clear<LF>load "Radio"
			strcat(tmpcmd, lfeed);		//clear<LF>load "Radio"<LF>
			strcat(tmpcmd, "play");		//clear<LF>load "Radio"<LF>play
			break;
		default:
			printf("case default\n");
			strcpy(tmpcmd, "\r");
			break;
		}
	}else{
		printf("case else\n");
		strcpy(tmpcmd, mpdcmd);	 // if command does not include special symbol #
	}
	strcat(tmpcmd, lfeed);
//	"play\n" "next\n" "currentsong\n" "previous\n"
	char buf[1024];
	int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(mpdport); // или любой другой порт...
    //addr.sin_addr.s_addr = htonl(0xc0a87b26); //26 or 32
    inet_pton(AF_INET, mpdip, &(addr.sin_addr));

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }
    printf("\n Prepare to send to IP -> %s | to Port -> %d | command -> %s \n", mpdip, mpdport, tmpcmd);
    recv(sock, buf, 1023, 0);
    send(sock, tmpcmd, (strlen(tmpcmd)), 0);
    recv(sock, buf, 1023, 0);
    close(sock);
    return 0;
}
