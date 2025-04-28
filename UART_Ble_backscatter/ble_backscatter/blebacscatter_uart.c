
/**********************************************
* BeagleBone PRU Backscatter
* BLE advertisements backscatter
* Authors : Ambuj Varshney < ambuj_varshney@it.uu.se >
* (C) 2016 Uppsala Networked Objects (UNO)
************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <termios.h>
#include "blebackscatter.h"
#define BEAGLEBONE

#define MAX_LINE_LEN 128
//#define CSV_PATH "home/debian/Desktop/UART_Ble_backscatter/uart_log.csv" // Path to your CSV file

// Control variable
static unsigned char count=0;
// The structure to hold the MAC address of the BLE backscatter device
static uint8_t arrMacAddress[BLE_MAC_ADDRESS];
// Bluetooth packet instantiation variable 
struct blePacket sblePacketInstance;


#ifdef BEAGLEBONE
#include "pru.h"
// Pointer to PRU memory bank
void *pru0DataMem;
void *pru1DataMem;
// BeagleBone PRU pin toggle calculator macro
#define DELAY(ns) ((ns/5-6)/2)
// Freerunning time to calulate the start and end of the symbols for bluetooth
unsigned long  time,endtime;
#endif


/**** Convert byte to bit
 * Argument 1 : byte - Character to be converted to binary
 * Argument 2 : arr  - Binary representation of the character MSB first
**/


int open_uart(const char *port){
	int fd = open(port, O_RDONLY | O_NOCTTY);
	if (fd <0) {
		perror("Failed to open UART");
		return -1;
	}

	struct termios options;
	tcgetattr(fd, &options);
	cfsetispeed(&options, B9600);
	options.c_cflag |= (CLOCAL | CREAD);
	tcsetattr(fd, TCSANOW, &options);
	return fd;
}



void bytetobit(uint8_t byt,uint8_t arr[8])
{

  int i;

  for(i = 0; i < 8; i++) 
    arr[i] = (((byt << i) & 0x80)) >> 7;
      
}

#ifdef BEAGLEBONE
// Function to read a line from UART
int read_uart_line(int fd, char *buffer, int maxlen){
	int i = 0;
	char c;
	while (i < maxlen - 1){
	     int n = read(fd, &c, 1);
	     if (n > 0){
		if(c == '\n') break;
		buffer[i++] = c;
	     }
	}
	buffer[i] = '\0'; //Null terminate
	return i;
}
#endif


int main(int argc, char **argv) {

  unsigned int i,j;
  int prumemCnt=0;
  uint8_t barr[8];
  char payload[MAX_PAYLOAD] = {0}; //Empty buffer
  int bytes_read;


  #ifdef BEAGLEBONE
  int uart_fd;
  struct termios options;
  #endif

 //Open UART
 // int uart_fd = open("/dev/ttyS1", O_RDONLY | O_NOCTTY);
 // if (uart_fd < 0){
//	perror("Failed to open UART");
//	return -1;
//  }

  if(argc == 2)
  {
      strcpy(payload, argv[1]);
      printf("Payload is : %s\n", payload);

  }

 //int n = read(uart_fd, payload, MAX_PAYLOAD - 1);
// if (n > 0){
//    payload[n] = '\0'; //NULL-terminate the string
//    printf("Received payload: %s\n", payload);
//  } else {
//    printf("No data received from UART\n", payload);
//    return -1;
//  }

  //close(uart_fd);  

  // Variables used to keep track of rolling time and the last bit
  // used for different stages of BLE packet backscatter process.
  time = 0; 

#ifdef BEAGLEBONE
    // PRU setup and other configuration
   if(geteuid()) {
      fprintf(stderr, "%s must be run as root to use prussdrv\n", argv[0]);
      return -1;
   }

  // initialize the library, PRU and interrupt; launch our PRU program 
   if(pru_setup()) {
      pru_cleanup();
      return -1;
   }

   uint8_t *pruMem8 = (uint8_t *) pru0DataMem;
   int rtn;

//Open UART
  uart_fd = open("/dev/ttyS1", O_RDONLY | O_NOCTTY);
  if (uart_fd < 0){
      perror("Failed to open UART");
      return -1;
  }
  tcgetattr(fd, &options);
  cfsetispeed(&options, B9600);
  options.c_cflag |= (CLOCAL | CREAD);
  tcsetattr(fd, TCSANOW, &options);


#endif

  // Put the MAC address here
   arrMacAddress[0] = 0x10; arrMacAddress[1] = 0x11; 
   arrMacAddress[2] = 0x12; arrMacAddress[3] = 0x13; 
   arrMacAddress[4] = 0x14; arrMacAddress[5] = 0x15; 

  // Populate the BLE payload array and structures
  //generate_ble_adv_payload(&sblePacketInstance, payload, arrMacAddress,39);

  // If beaglebone is not defined, return immediately after printing the BLE payload

   while (1){
       memset(payload, 0, MAX_PAYLOAD);
       bytes_read = read(uart_fd, payload, MAX_PAYLOAD - 1);
       if (bytes_read > 0){
            payload[bytes_read] = '\0';
	    printf("Received payload: %s\n", payload);

	   generate_ble_adv_payload(&sblePacketInstance, payload, arrMacAddress,39);

   
#ifndef BEAGLEBONE

  printf("\n BLE payload is: ");
  printf("%x,", sblePacketInstance.u8preamble);
  printf("%x,", sblePacketInstance.access_address[0])


  return 0;
#endif

#ifdef DEBUG
  printf("BLE packet generated, length %d\n",sblePacketInstance.u8PayloadLen );
#endif

#ifdef BEAGLEBONE
    // Populate PRU memory ban to toggle now!!!
    bytetobit(sblePacketInstance.u8preamble, barr);
    POPULATE_PRU;

#ifdef DEBUG
    printf ("len %d\n",sblePacketInstance.u8PayloadLen, sblePacketInstance.u8PayloadLen );
#endif

#ifdef DEBUG
    printf("BLE: Preamble processed is : %x\n", sblePacketInstance.u8preamble);
#endif

    bytetobit(sblePacketInstance.access_address[0], barr);
    POPULATE_PRU;
    bytetobit(sblePacketInstance.access_address[1], barr);
    POPULATE_PRU;
    bytetobit(sblePacketInstance.access_address[2], barr);
    POPULATE_PRU;
    bytetobit(sblePacketInstance.access_address[3], barr);
    POPULATE_PRU;

#ifdef DEBUG
    printf("BLE: Access address processed is %x,%x,%x,%x\n", sblePacketInstance.access_address[0],\
            sblePacketInstance.access_address[1],sblePacketInstance.access_address[2],\
            sblePacketInstance.access_address[3]);
    printf("BLE Payload is:");
#endif

    for(j=0; j < sblePacketInstance.u8PayloadLen; j++)
    {

#ifdef DEBUG
      printf("%x,",sblePacketInstance.blePDU[j] );
#endif

      bytetobit(sblePacketInstance.blePDU[j], barr);
      POPULATE_PRU;

    }

    // Indicate that bits to be transmitted are done now.
    pruMem8[prumemCnt ] = 0; 


#ifdef DEBUG
    printf("\nBLE: Payload processed: %d\n", prumemCnt);
    printf("Endtime: %f\n", endtime);

#endif

    printf("Executing PRU program\n");

    for(i=0; i< 10; i++) //i<100 previous
    {

    if((rtn = prussdrv_exec_program(PRU_NUM, "fsk.bin")) < 0) {
      fprintf(stderr, "prussdrv_exec_program() failed\n");
      return rtn;
   }

   printf("Loop %d\n", i);
   //usleep(1000*300);
   usleep(1000*100);
  }
}
 //  printf("waiting for interrupt from PRU0...\n");

   //rtn = prussdrv_pru_wait_event(PRU_EVTOUT_0);

 //  printf("PRU program completed, event number %d\n", rtn);

   //return pru_cleanup();
   //Should never reach here
   close(uart_fd);
   return 0;

#endif


}
