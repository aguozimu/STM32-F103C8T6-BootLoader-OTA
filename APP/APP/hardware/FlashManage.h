#ifndef __FLASHMANAGE_H
#define __FLASHMANAGE_H


#include "stm32f1xx_hal.h" 
#define BackupPageADD1 0x8007C00 
#define BackupPageADD2 0x800BC00 
#define BackupPageADD3 0x800FC00 




typedef struct {
	uint32_t Page_ADD;
	uint32_t Page_CRC;
}Page_CRC;


typedef struct {
	uint16_t ID;
	uint8_t Run_status;
	uint8_t Data_area;
	Page_CRC Page1;
	Page_CRC Page2;
	Page_CRC Page3;	
	Page_CRC Page4;	
	Page_CRC Page5;	
	Page_CRC Page6;
	Page_CRC Page7;
	Page_CRC Page8;
	Page_CRC Page9;	
	Page_CRC Page10;	
	Page_CRC Page11;
	Page_CRC Page12;	
	Page_CRC Page13;	
	Page_CRC Page14;	
	Page_CRC Page15;	
	Page_CRC Page16;
	
}Data_Backup;

void FlashManage_BackupWritePage(Data_Backup Data);
















#endif
