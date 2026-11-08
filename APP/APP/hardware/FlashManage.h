#ifndef __FLASHMANAGE_H
#define __FLASHMANAGE_H


#include "stm32f1xx_hal.h" 
#define BackupPageADD1 0x8007C00 
#define BackupPageADD2 0x800BC00 
#define BackupPageADD3 0x800FC00 

#define OTA_FRAME_LEN 16U

/* 本 APP 的 MID（程序 ID）：16 位有效，0x0000 保留为无效值 */
#define APP_MID 0x0003U

/* 运行区与 OTA 页布局：15 个程序页 + 1 个控制页，每页 1 KB */
#define APP_RUN_BASE           0x08004000U
#define APP_SLOT2_BASE         0x08008000U
#define APP_SLOT3_BASE         0x0800C000U
#define OTA_PAGE_SIZE          0x400U
#define OTA_PROGRAM_PAGE_COUNT 15U
#define OTA_PAGE_COUNT         16U
#define OTA_BLOCK_COUNT        256U
#define OTA_PAGE_WORD_COUNT    256U
#define OTA_CONTROL_PAGE_OFFSET 0x3C00U

/* OTA 运行状态 */
#define OTA_RUN_STATUS_EMPTY       0x00U
#define OTA_RUN_STATUS_FLASHABLE   0x01U
#define OTA_RUN_STATUS_RECEIVING   0x02U
#define OTA_RUN_STATUS_PAUSED      0x03U
#define OTA_RUN_STATUS_VERIFYING   0x04U
#define OTA_RUN_STATUS_VALID_BACKUP 0x05U
#define OTA_RUN_STATUS_RUNNING     0x06U
#define OTA_RUN_STATUS_ERROR       0x07U
#define OTA_DATA_AREA_COMPLETE     0xFFU

/* OTA 错误码 */
#define OTA_ERROR_NONE                 0x00U
#define OTA_ERROR_INVALID_STATE        0x04U
#define OTA_ERROR_SEQUENCE             0x05U
#define OTA_ERROR_NO_WRITABLE_SLOT     0x06U
#define OTA_ERROR_PAGE_CRC_MISMATCH    0x09U
#define OTA_ERROR_CONTROL_PAGE_INVALID 0x0AU
#define OTA_ERROR_FIRMWARE_VERIFY_FAIL 0x0BU
#define OTA_ERROR_TIMEOUT              0x0EU

/* 页地址与页硬件 CRC 元数据 */
typedef struct {
	uint32_t Page_ADD; /* 页地址：目标槽内为绝对地址，上位机控制页中为相对地址 */
	uint32_t Page_CRC; /* 整页 1 KB 的 STM32F1 硬件 CRC */
} Page_CRC;

/* 槽控制页记录：位于每个 16 KB 槽的最后 1 KB */
typedef struct {
	uint16_t ID;          /* 程序 ID（16 位序列号，0x0000 无效） */
	uint8_t Run_status;   /* 槽运行状态：EMPTY/RECEIVING/PAUSED/VALID_BACKUP 等 */
	uint8_t Data_area;    /* 当前/最后处理的页；0xFF 表示完成或空闲 */
	Page_CRC Page[OTA_PAGE_COUNT]; /* Page[0]~[14] 程序页，Page[15] 控制页 */
} Data_Backup;

/* SetDat 接收阶段 */
typedef enum {
	FLASHMANAGE_OTA_IDLE = 0,  /* 无进行中的 SetDat 会话 */
	FLASHMANAGE_OTA_CONTROL,   /* 正在接收 1 KB 控制页 */
	FLASHMANAGE_OTA_PAGE,      /* 正在接收当前待写程序页 */
	FLASHMANAGE_OTA_WAIT_END   /* 所有待写页已处理，等待 SetEnd */
} FlashManage_OtaStage;

/* 目标槽描述 */
typedef struct {
	uint32_t control_page; /* 目标槽控制页绝对地址 */
	uint32_t data_base;    /* 目标槽 15 KB 程序数据起始地址 */
} FlashManage_Slot;

/* SetDat 跨帧状态：控制页接收完成后用于逐页写入和更新元数据 */
typedef struct {
	FlashManage_OtaStage stage; /* 当前接收阶段 */
	Data_Backup metadata;       /* 目标槽控制页元数据缓存 */
	uint32_t control_page;      /* 目标槽控制页绝对地址 */
	uint32_t data_base;         /* 目标槽程序数据起始地址 */
	uint16_t pending_pages;     /* 待写程序页位掩码，bit0~bit14 对应 Page0~Page14 */
	uint16_t next_block;        /* 当前 1 KB 页期望接收的下一个 4 字节块号 */
	uint8_t current_page;       /* 当前正在接收/写入的页号 */
	uint8_t last_block;         /* 上一有效 SetDat 块号，用于重复帧判断 */
	uint32_t last_frame_crc;    /* 上一有效 SetDat 帧 CRC，用于重复帧判断 */
} FlashManage_OtaState;

/* OTA 帧分发入口：处理 16 字节完整协议帧 */
void FlashManage_HCC(uint8_t* RXData, uint8_t* TXData);

/* SetDat 接收入口：维护控制页、程序页、ACK、选槽和 Flash 写入流程 */
void FlashManage_HandleSetDat(uint8_t* RXData);

/* 开机一次性初始化：写入一号运行区元数据并初始化首次烧录的二、三号空槽 */
void FlashManage_FirstOnly(void);

/* 将一份控制页元数据写入指定备份页 */
void FlashManage_BackupWritePage(uint32_t backup_page, const Data_Backup* Data);

/* 通过 UART2 发送固定 16 字节 OTA 协议帧 */
void TXD_Send(uint8_t* TXD);
















#endif
