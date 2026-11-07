#include "Mydef.h"

/* OTA 停等确认：每帧等待 500 ms，超时后最多重发 5 次 */
#define OTA_ACK_TIMEOUT_MS 500U
#define OTA_ACK_RETRY_MAX 5U

/*flash内存管理*/


/*运行时进行程序刷写，并且双备份*/
/*0x8000000~0x8003FFF*//*bootloader区域*/
/*0x8004000~0x8007BFF*//*一号程序区域*/
/*0x8007C00~0x8007FFF*//*一号程序数据备份区域*/
/*0x8008000–0x800BBFF*//*二号程序区域*/
/*0x800BC00–0x800BFFF*//*二号程序数据备份区域*/
/*0x800C000–0x800FBFF*//*三号程序区域*/
/*0x800FC00–x0800FFFF*//*三号程序数据备份区域*/
/*每个区域宽度0x4000==16KB*/


/*1KB一页*/

/*数据备份区域*/
/*0x8007C00~0x8007FFF*//*一号程序数据备份*//*size=1024，一页*/
/*0x800BC00–0x800BFFF*//*二号程序数据备份*//*size=1024，一页*/
/*0x800FC00–x0800FFFF*//*三号程序数据备份*//*size=1024，一页*/
/*.....*/

/*单个数据备份区*/
/*版本号*//*2字节*/
/*运行状态*//*1字节*/
/*代码区域*//*1字节*/
/*程序区域第1页首地址*//*4字节*/
/*程序区域第1页CRC*//*4字节*/
/*程序区域第2页首地址*//*4字节*/
/*程序区域第2页CRC*//*4字节*/
/*程序区域第3页首地址*//*4字节*/
/*程序区域第3页CRC*//*4字节*/
/*................*/
/*程序区域第16页首地址*//*4字节*/
/*程序区域第16页CRC*//*4字节*/


/* 计算一个 1 KB Flash 页的 STM32F1 硬件 CRC，输入为 256 个小端 uint32_t 字 */
static uint32_t FlashManage_FlashPageCRC(uint32_t page_address){
	return HAL_CRC_Calculate(&hcrc, (uint32_t*)page_address, OTA_PAGE_SIZE / sizeof(uint32_t));
}

/*
 * 开机只执行一次：刷新一号运行区元数据。
 * 写入 APP_MID、RUNNING 状态、15 个程序页 CRC 和控制页 CRC，
 * 供 GetMID、GetCRC、自动选槽及后续 OTA 流程直接读取。
 */
void FlashManage_FirstOnly(void){
	Data_Backup data = {0};
	uint8_t page;
	uint32_t control_page_crc;

	data.ID = APP_MID;
	data.Run_status = OTA_RUN_STATUS_RUNNING;
	data.Data_area = OTA_DATA_AREA_COMPLETE;

	/* Page1~Page15 为程序页，Page16 为控制页；地址均为槽内相对地址 */
	for (page = 0U; page < OTA_PAGE_COUNT; page++){
		data.Page[page].Page_ADD = (uint32_t)page * OTA_PAGE_SIZE;
		data.Page[page].Page_CRC = (page < OTA_PROGRAM_PAGE_COUNT)
			? FlashManage_FlashPageCRC(APP_RUN_BASE + ((uint32_t)page * OTA_PAGE_SIZE))
			: 0U;
	}

	/* 先以 Page16.CRC=0 写入整份元数据，再计算并回填控制页自身 CRC */
	FlashManage_BackupWritePage(BackupPageADD1, &data);
	control_page_crc = FlashManage_FlashPageCRC(BackupPageADD1);
	MyFLASH_ProgramWord(BackupPageADD1 + 0x80U, control_page_crc);
}

/* 将一份元数据写入指定备份页；backup_page 可为 BackupPageADD1/2/3 */
void FlashManage_BackupWritePage(uint32_t backup_page, const Data_Backup* Data){
	uint8_t page;

	MyFLASH_ErasePage(backup_page);
	/* 偏移 0x00：程序 ID；偏移 0x02：Run_status + Data_area */
	MyFLASH_ProgramHalfWord(backup_page, Data->ID);
	MyFLASH_ProgramHalfWord(backup_page + 0x02U,
		(uint16_t)((uint16_t)Data->Run_status | ((uint16_t)Data->Data_area << 8U)));

	/* 每页 8 字节：偏移 0x04 起为 Address/CRC，循环写入 16 页 */
	for (page = 0U; page < OTA_PAGE_COUNT; page++){
		MyFLASH_ProgramWord(backup_page + 0x04U + ((uint32_t)page * 8U), Data->Page[page].Page_ADD);
		MyFLASH_ProgramWord(backup_page + 0x08U + ((uint32_t)page * 8U), Data->Page[page].Page_CRC);
	}
}

void TXD_Send(uint8_t* TXD){
	/* OTA 协议帧固定 16 字节，统一经 UART2 TX（PA2）发送给上位机 */
	Uart_Send(&huart2, TXD, OTA_FRAME_LEN);
}

/* 按协议小端序读取 32 位数据 */
static uint32_t FlashManage_ReadU32(const uint8_t* data){
	return ((uint32_t)data[0]) |
	       ((uint32_t)data[1] << 8U) |
	       ((uint32_t)data[2] << 16U) |
	       ((uint32_t)data[3] << 24U);
}

/* 按协议小端序写入 32 位数据 */
static void FlashManage_WriteU32(uint8_t* data, uint32_t value){
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8U);
	data[2] = (uint8_t)(value >> 16U);
	data[3] = (uint8_t)(value >> 24U);
}

/* STM32F1 硬件 CRC：将帧前 12 字节作为 3 个小端 uint32_t 计算 */
static uint32_t FlashManage_FrameCRC(const uint8_t* frame){
	uint32_t words[3];

	words[0] = FlashManage_ReadU32(&frame[0]);
	words[1] = FlashManage_ReadU32(&frame[4]);
	words[2] = FlashManage_ReadU32(&frame[8]);
	return HAL_CRC_Calculate(&hcrc, words, 3U);
}

/* 构造 16 字节帧：A5 | Type(6) | Number | Data(4) | CRC32(4)，返回帧 CRC */
static uint32_t FlashManage_BuildFrame(uint8_t* frame, const char* type, uint8_t number, uint32_t data){
	uint32_t crc;

	frame[0] = 0xA5U;
	memcpy(&frame[1], type, 6U);
	frame[7] = number;
	FlashManage_WriteU32(&frame[8], data);
	crc = FlashManage_FrameCRC(frame);
	FlashManage_WriteU32(&frame[12], crc);
	return crc;
}

/* 构造并发送一帧，不等待确认 */
static void FlashManage_SendFrame(const char* type, uint8_t number, uint32_t data){
	uint8_t frame[OTA_FRAME_LEN];

	(void)FlashManage_BuildFrame(frame, type, number, data);
	TXD_Send(frame);
}

/* 比较帧尾 CRC 与硬件计算结果 */
static uint8_t FlashManage_FrameIsValid(const uint8_t* frame){
	return (FlashManage_ReadU32(&frame[12]) == FlashManage_FrameCRC(frame)) ? 1U : 0U;
}

/* AckCRC：回显请求帧的 Number 和 CRC 原值 */
static void FlashManage_SendAck(const uint8_t* request){
	FlashManage_SendFrame("AckCRC", request[7], FlashManage_ReadU32(&request[12]));
}

/* 等待匹配的 AckCRC，Number 和 Data 中的 CRC 都必须一致 */
static uint8_t FlashManage_WaitAck(uint8_t number, uint32_t crc){
	uint8_t frame[OTA_FRAME_LEN];
	uint32_t start_tick = HAL_GetTick();

	while ((HAL_GetTick() - start_tick) < OTA_ACK_TIMEOUT_MS){
		if ((Uart2_Read(frame, OTA_FRAME_LEN) == OTA_FRAME_LEN) &&
		    (memcmp(&frame[1], "AckCRC", 6U) == 0) &&
		    (frame[7] == number) &&
		    (FlashManage_ReadU32(&frame[8]) == crc) &&
		    FlashManage_FrameIsValid(frame)){
			return 1U;
		}
	}
	return 0U;
}

/* 发送业务帧并停等确认，超时后重发；返回 1 表示确认成功 */
static uint8_t FlashManage_SendAndWaitAck(const char* type, uint8_t number, uint32_t data){
	uint8_t frame[OTA_FRAME_LEN];
	uint32_t crc = FlashManage_BuildFrame(frame, type, number, data);
	uint8_t retry;

	for (retry = 0U; retry <= OTA_ACK_RETRY_MAX; retry++){
		TXD_Send(frame);
		if (FlashManage_WaitAck(number, crc)){
			return 1U;
		}
	}
	return 0U;
}

/* 通过 MyFLASH 读取控制页中页 n 的 CRC，偏移为 0x08 + n*8 */
static uint32_t FlashManage_ReadPageCRC(uint32_t control_page, uint8_t page){
	return MyFLASH_ReadWord(control_page + 0x08U + ((uint32_t)page * 8U));
}

/* 通过 MyFLASH 读取控制页运行状态，Run_status 位于控制页偏移 0x02 */
static uint8_t FlashManage_ReadRunStatus(uint32_t control_page){
	return MyFLASH_ReadByte(control_page + 0x02U);
}

/* 通过 MyFLASH 读取控制页 Data_area，编码与状态记录中的 Page 相同 */
static uint8_t FlashManage_ReadDataArea(uint32_t control_page){
	return MyFLASH_ReadByte(control_page + 0x03U);
}

/* 状态记录按小端组成：[Run_status][Page][Block][Error] */
static uint32_t FlashManage_MakeStatus(uint8_t run_status, uint8_t page, uint8_t block, uint8_t error){
	return ((uint32_t)run_status) |
	       ((uint32_t)page << 8U) |
	       ((uint32_t)block << 16U) |
	       ((uint32_t)error << 24U);
}

/* 通用状态帧：Run_status 取当前控制页，Page 由调用者指定，Block 不适用填 0xFF */
static uint8_t FlashManage_SendStatusFrame(const char* type, uint8_t page, uint8_t error){
	return FlashManage_SendAndWaitAck(type, 0U,
		FlashManage_MakeStatus(FlashManage_ReadRunStatus(BackupPageADD1), page, 0xFFU, error));
}

/* 查询事务结束：Page 不适用时使用 0xFF，附加指定错误码 */
static void FlashManage_SendQueryEnd(uint8_t error){
	(void)FlashManage_SendStatusFrame("SetEnd", 0xFFU, error);
}

void FlashManage_HCC  (uint8_t* RXData,uint8_t* TXData){

	/*RX为接收到的数据，TX为发送出去的数据（不用考虑数据缓存区大小）*/

while(1){
/* 固定 16 字节帧，各命令直接按偏移从 RXData 读取字段 */
if(RXData[0] == 0xA5U){
/*开始通讯*/
	if(memcmp(&RXData[1], "SetDat", 6) == 0){

		/*上位机数据传输*/
		/*地址偏移（查看空闲区域）*/
		/*每页编号0~255*/	
/*		0xA5/SetDat/当前编号/DATA/CRC(数据段)    				*/
		

		/*CRC校验*/

		/*存入缓存数组*/

		/*回发CRC校验值*/

/*		帧头/类型/编号/数据段/CRC校验值/CRC(数据段)					*/		
/*		0xA5/AckCRC/当前编号/CRC校验值/CRC(数据段)    				*/

		/*发送结束帧(最后单独发送一次)*/
/*		0xA5/SetEnd/0x00/当前状态/CRC(数据段)    		*/

	
	}else if (memcmp(&RXData[1], "GetCRC", 6) == 0)
	{
		uint8_t page;

		/* 丢弃 CRC 错误帧；只有校验正确才响应业务请求 */
		if (!FlashManage_FrameIsValid(RXData)){
			return;
		}

		/* GetCRC 事务：先确认请求，再逐页返回运行区控制页中的 16 页 CRC */
		FlashManage_SendAck(RXData);
		/* Page 0~14 为程序页，Page 15 为控制页本身 */
		for (page = 0U; page < OTA_PAGE_COUNT; page++){
			/* 每帧 SetCRC 都必须收到上位机 AckCRC 后才能继续 */
			if (!FlashManage_SendAndWaitAck("SetCRC", page, FlashManage_ReadPageCRC(BackupPageADD1, page))){
				/* 确认超时：按协议以 TIMEOUT(0x0E) 结束查询事务 */
				FlashManage_SendQueryEnd(0x0EU);
				return;
			}
		}

		/* 查询完成 */
		FlashManage_SendQueryEnd(0x00U);
		return;
	}else if (memcmp(&RXData[1], "GetMID", 6) == 0)
	{
		/* 丢弃 CRC 错误帧；只有校验正确才响应业务请求 */
		if (!FlashManage_FrameIsValid(RXData)){
			return;
		}

		/* GetMID 事务：确认请求后返回本 APP 的 MID，并等待上位机确认 */
		FlashManage_SendAck(RXData);
		if (!FlashManage_SendAndWaitAck("SetMID", 0U, APP_MID)){
			FlashManage_SendQueryEnd(0x0EU);
			return;
		}
		FlashManage_SendQueryEnd(0x00U);
		return;

	}else if (memcmp(&RXData[1], "GetSta", 6) == 0){
		uint8_t page;

		/* 丢弃 CRC 错误帧；只有校验正确才响应业务请求 */
		if (!FlashManage_FrameIsValid(RXData)){
			return;
		}

		/* GetSta 事务：确认请求后返回当前状态，最后发送 SetEnd */
		FlashManage_SendAck(RXData);
		page = FlashManage_ReadDataArea(BackupPageADD1);
		if (!FlashManage_SendStatusFrame("SetSta", page, 0x00U)){
			(void)FlashManage_SendStatusFrame("SetEnd", page, 0x0EU);
			return;
		}
		(void)FlashManage_SendStatusFrame("SetEnd", page, 0x00U);
		return;
	}else if (memcmp(&RXData[1], "SetEro", 6) == 0){
		/*上位机发送错误指令*/
		/*0xA5/SetEro/0x00/(uint32)0x00/CRC(数据段)*/

		/*错误处理(先不写)*/

		/*发送结束帧(最后单独发送一次)*/
		/*0xA5/SetEnd/0x00/当前状态/CRC(数据段)*/

	}else if (memcmp(&RXData[1], "SetEnd", 6) == 0){
		/*上位机发送结束指令*/
		/*0xA5/SetEnd/0x00/(uint32)0x00/CRC(数据段)*/

		/*发送结束帧(最后单独发送一次)*/
		/*0xA5/SetEnd/0x00/当前状态/CRC(数据段)*/

	}
	
	
	
	





	
	

	
}	
	


};




}




















