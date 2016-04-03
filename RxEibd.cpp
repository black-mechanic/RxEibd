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

typedef unsigned char uchar;
#define LENGTH_OF_LIST_OF_GADDR	32	// Length of List of Group Address
#define LENGTH_OF_CMD_STR	200
#define CMD_STRING_DELIMITER	"~"

int main (int argc, char *argv[])
{
  int len;
  EIBConnection *eibcon;
  eibaddr_t eibdest;
  eibaddr_t eibsrc;
  uchar eibbuf[200];
  int tmpeibval;

  FILE *fh_cfg_file = NULL; // Файл с соответствиями KNX телеграммы <-> IPкоманды
  listGroupAddr myListGA[LENGTH_OF_LIST_OF_GADDR];
  eibaddr_t testga;
  int index_gaddr;  // index for group address array
  char *parsed_cfg_str; // Будет равна LENGTH_OF_CMD_STR
  char mcom[] = "next\n";
  int size_of_list_gaddr;	// real length of list of group address - counting due to read

  if (argc != 3)
    die ("usage: %s Url(ip:localhost:6720) file(with list of cmd)", argv[0]);
//Prepare KNX connection
  eibcon = EIBSocketURL (argv[1]);
  if (!eibcon)
    die ("Open failed");
  if (EIBOpen_GroupSocket (eibcon, 0) == -1)
    die ("Connect failed");
//Fill array from file
  if((fh_cfg_file = fopen(argv[2], "r")) == NULL)
	  die ("Error Open file with list of group address");
// Читаем командные строки из конфигурационного файла и заполняем массив структур myListGA
  index_gaddr = 0;
  parsed_cfg_str = (char*)malloc(LENGTH_OF_CMD_STR * sizeof(char));
  while(fgets(parsed_cfg_str, LENGTH_OF_CMD_STR, fh_cfg_file) != NULL){
	  // Здесь парсим строку
	  convert_str_to_myListGA(parsed_cfg_str, myListGA, index_gaddr);
	  index_gaddr++;
	  if(index_gaddr == LENGTH_OF_LIST_OF_GADDR)
		  break;
  }
  free(parsed_cfg_str);

  size_of_list_gaddr=index_gaddr;	// real number of monitoring group address
  for(index_gaddr=0; index_gaddr != size_of_list_gaddr; index_gaddr++)
	  printf("Result N:%d -> %s - %d - %s - %d - %s - %X\n", index_gaddr, myListGA[index_gaddr].group_addr, myListGA[index_gaddr].value, myListGA[index_gaddr].send_to_ip, myListGA[index_gaddr].send_to_port, myListGA[index_gaddr].cmd_string, myListGA[index_gaddr].group_addr_in_hex);
  fclose(fh_cfg_file);

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
    	  for(index_gaddr=0; index_gaddr != size_of_list_gaddr; index_gaddr++){
    		  if((myListGA[index_gaddr].group_addr_in_hex == eibdest) && (myListGA[index_gaddr].value == tmpeibval)){
    			  printf("\n Caught command => %s \n", myListGA[index_gaddr].cmd_string);
    			  mpdControl(myListGA[index_gaddr].cmd_string, myListGA[index_gaddr].send_to_ip, myListGA[index_gaddr].send_to_port);
    		  }
    	  }
    	  fflush (stdout);
      }
  }
  EIBClose (eibcon);
  return 0;
}

int mpdControl(char *cmd_string, char *send_to_ip, int send_to_port)
{
	const char *lfeed = "\n";
	char tmpcmd[128]="";
	char namecmd[64]="";
	char * fptr; char * nptr; //Pointer to certain position in string (first? next)
	if(cmd_string[0]=='#'){ 	  // Special command start with symbol #
		switch(cmd_string[1]){
		case 'v':
			printf("case v\n");
			strcat(tmpcmd, "volume");
			tmpcmd[6]=' ';
			tmpcmd[7]=cmd_string[2]; // + or -
			tmpcmd[8]=cmd_string[3]; // real value of step of volume, for example 5
			tmpcmd[9]='\0';		 // End of string
			break;				 // Ready volume +5 or volume -5
		case 'p':
			printf("case p\n");
			strcat(tmpcmd, "clear");	//clear
			strcat(tmpcmd, lfeed);		//clear<LF>
			strcat(tmpcmd, "load ");	//clear<LF>load <-- with space
			fptr = strchr(cmd_string, '"');
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
		strcpy(tmpcmd, cmd_string);	 // if command does not include special symbol #
	}
	//strcat(tmpcmd, lfeed);
//	"play\n" "next\n" "currentsong\n" "previous\n"
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
    char buf[1024];
	int sock;
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock < 0)
    {
        perror("socket");
        return -1;
     }
    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    	printf("setsockopt failed\n");
    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    	printf("setsockopt failed\n");

    addr.sin_family = AF_INET;
    addr.sin_port = htons(send_to_port); // или любой другой порт...
    //addr.sin_addr.s_addr = htonl(0xc0a87b26); //26 or 32
    inet_pton(AF_INET, send_to_ip, &(addr.sin_addr));

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        return -2;
        //exit(2);
    }
    printf("\n Prepare to send to IP -> %s | to Port -> %d | command -> %s \n", send_to_ip, send_to_port, tmpcmd);
    recv(sock, buf, 1023, 0);
    send(sock, tmpcmd, (strlen(tmpcmd)), 0);
    recv(sock, buf, 1023, 0);
    close(sock);
    return 0;
}

void convert_str_to_myListGA(char* parsed_cfg_str, listGroupAddr *myListGA, int index_gaddr)
{
	const char s[3] = CMD_STRING_DELIMITER; // Разделитель параметров в строке
	char *token; // Выделяемая подстрока

	token = strtok(parsed_cfg_str, s); // Первый токен - строка
	strcpy(myListGA[index_gaddr].group_addr, token);
	token = strtok(NULL, s);
	myListGA[index_gaddr].value = atoi(token);
	token = strtok(NULL, s);
	strcpy(myListGA[index_gaddr].send_to_ip, token);
	token = strtok(NULL, s);
	myListGA[index_gaddr].send_to_port = atoi(token);
	token = strtok(NULL, s);
	strcpy(myListGA[index_gaddr].cmd_string, token);
	myListGA[index_gaddr].group_addr_in_hex = readgaddr((myListGA[index_gaddr].group_addr));
}
