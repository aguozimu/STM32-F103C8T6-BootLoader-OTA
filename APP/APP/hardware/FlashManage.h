#ifndef __FLASHMANAGE_H
#define __FLASHMANAGE_H


#include "stm32f1xx_hal.h" 
#define BackupPageADD1 0x8007C00 
#define BackupPageADD2 0x800BC00 
#define BackupPageADD3 0x800FC00 

#define OTA_FRAME_LEN 16U

/* 本 APP 的 MID（程序 ID）：16 位有效，0x0000 保留为无效值 */
#define APP_MID 0x0002U

/* 运行区与 OTA 页布局：15 个程序页 + 1 个控制页，每页 1 KB */
#define APP_RUN_BASE          0x08004000U
#define OTA_PAGE_SIZE         0x400U
#define OTA_PROGRAM_PAGE_COUNT 15U
#define OTA_PAGE_COUNT        16U

/* 当前运行程序状态；Data_area=0xFF 表示无待写页 */
#define OTA_RUN_STATUS_RUNNING 0x06U
#define OTA_DATA_AREA_COMPLETE 0xFFU

typedef struct {
	uint32_t Page_ADD;
	uint32_t Page_CRC;
}Page_CRC;


typedef struct {
	uint16_t ID;
	uint8_t Run_status;
	uint8_t Data_area;
	Page_CRC Page[OTA_PAGE_COUNT]; /* Page[0]~[14] 程序页，Page[15] 控制页 */
	
}Data_Backup;

void FlashManage_HCC  (uint8_t* RXData,uint8_t* TXData);
void FlashManage_FirstOnly(void);
void FlashManage_BackupWritePage(uint32_t backup_page, const Data_Backup* Data);
void TXD_Send(uint8_t* TXD);
















#endif
