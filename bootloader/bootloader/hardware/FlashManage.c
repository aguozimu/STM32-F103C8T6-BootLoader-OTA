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


/* 计算一个 1 KB 页缓冲区的 STM32F1 硬件 CRC */
static uint32_t FlashManage_BufferCRC(const uint32_t* data){
	return HAL_CRC_Calculate(&hcrc, (uint32_t*)data, OTA_PAGE_WORD_COUNT);
}

/* 计算一个 1 KB Flash 页的硬件 CRC */
static uint32_t FlashManage_FlashPageCRC(uint32_t page_address){
	return FlashManage_BufferCRC((const uint32_t*)page_address);
}

/* 首次烧录检测：控制页仍为 0xFF 时，将该槽元数据整体初始化为 EMPTY(0x00) */
static void FlashManage_InitEmptyBackup(uint32_t backup_page){
	Data_Backup empty = {0};
	empty.Run_status = OTA_RUN_STATUS_EMPTY;

	if (MyFLASH_ReadByte(backup_page + 0x02U) == 0xFFU){
		FlashManage_BackupWritePage(backup_page, &empty);
	}
}

/*
 * 开机只执行一次：刷新一号运行区元数据，并初始化首次烧录的二、三号空槽。
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

	/* 第一次烧录程序时，二、三号槽元数据仍为 0xFF，写入全 0 的 EMPTY 元数据 */
	FlashManage_InitEmptyBackup(BackupPageADD2);
	FlashManage_InitEmptyBackup(BackupPageADD3);
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
		HAL_IWDG_Refresh(&hiwdg);
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

/* SetDat 状态类型由 FlashManage.h 提供，实例仅在本文件内部使用 */
static const FlashManage_Slot FlashManage_Slots[2] = {
	{BackupPageADD2, APP_SLOT2_BASE},
	{BackupPageADD3, APP_SLOT3_BASE}
};

/* 1 KB 接收页缓冲；words 保证写 Flash 时的 32 位对齐 */
static union {
	uint32_t words[OTA_PAGE_WORD_COUNT];
	uint8_t bytes[OTA_PAGE_SIZE];
} FlashManage_PageBuffer;

static FlashManage_OtaState FlashManage_Ota;

/* 16 位程序 ID 回绕比较：a 比 b 新时返回 1 */
static uint8_t FlashManage_IsNewerProgramID(uint16_t a, uint16_t b){
	uint16_t delta = (uint16_t)(a - b);
	return ((delta != 0U) && (delta < 0x8000U)) ? 1U : 0U;
}

/* 根据二、三号槽 Run_status、PAUSED 续传 ID 和有效备份 ID 选择目标槽 */
static uint8_t FlashManage_SelectTargetSlot(uint16_t program_id){
	uint8_t status2 = FlashManage_ReadRunStatus(BackupPageADD2);
	uint8_t status3 = FlashManage_ReadRunStatus(BackupPageADD3);
	uint16_t id2 = MyFLASH_ReadHalfWord(BackupPageADD2);
	uint16_t id3 = MyFLASH_ReadHalfWord(BackupPageADD3);

	if ((status2 == OTA_RUN_STATUS_PAUSED) && (id2 == program_id)){
		return 0U;
	}
	if ((status3 == OTA_RUN_STATUS_PAUSED) && (id3 == program_id)){
		return 1U;
	}

	/* 非有效备份的槽可优先作为目标，避免覆盖唯一 VALID_BACKUP */
	if ((status2 != OTA_RUN_STATUS_VALID_BACKUP) && (status2 != OTA_RUN_STATUS_RUNNING)){
		return 0U;
	}
	if ((status3 != OTA_RUN_STATUS_VALID_BACKUP) && (status3 != OTA_RUN_STATUS_RUNNING)){
		return 1U;
	}

	if ((status2 == OTA_RUN_STATUS_VALID_BACKUP) && (status3 == OTA_RUN_STATUS_VALID_BACKUP)){
		return FlashManage_IsNewerProgramID(id2, id3) ? 1U : 0U;
	}
	if (status2 == OTA_RUN_STATUS_VALID_BACKUP){
		return 1U;
	}
	if (status3 == OTA_RUN_STATUS_VALID_BACKUP){
		return 0U;
	}
	return 0xFFU;
}

/* 返回页掩码中编号最小的待写页 */
static uint8_t FlashManage_FirstPendingPage(uint16_t pending_pages){
	uint8_t page;

	for (page = 0U; page < OTA_PROGRAM_PAGE_COUNT; page++){
		if ((pending_pages & (uint16_t)(1U << page)) != 0U){
			return page;
		}
	}
	return OTA_DATA_AREA_COMPLETE;
}

/* 发送指定目标状态下的 SetEro 状态帧 */
static void FlashManage_SendOtaError(uint8_t run_status, uint8_t page, uint8_t block, uint8_t error){
	(void)FlashManage_SendAndWaitAck("SetEro", 0U,
		FlashManage_MakeStatus(run_status, page, block, error));
}

/* 更新并重写目标槽控制页，同时重算 Page16 自身 CRC */
static void FlashManage_WriteControlPage(uint32_t control_page, Data_Backup* data, uint8_t run_status, uint8_t data_area){
	data->Run_status = run_status;
	data->Data_area = data_area;
	data->Page[15].Page_ADD = control_page;
	data->Page[15].Page_CRC = 0U;

	FlashManage_BackupWritePage(control_page, data);
	data->Page[15].Page_CRC = FlashManage_FlashPageCRC(control_page);
	MyFLASH_ProgramWord(control_page + 0x80U, data->Page[15].Page_CRC);
}

/* 校验上位机控制页 ID、相对地址和 Page16 CRC */
static uint8_t FlashManage_ControlPageIsValid(const Data_Backup* data){
	uint8_t page;
	uint32_t control_crc = data->Page[15].Page_CRC;

	if (data->ID == 0U){
		return 0U;
	}
	for (page = 0U; page < OTA_PAGE_COUNT; page++){
		if (data->Page[page].Page_ADD != ((uint32_t)page * OTA_PAGE_SIZE)){
			return 0U;
		}
	}

	FlashManage_WriteU32(&FlashManage_PageBuffer.bytes[0x80], 0U);
	return (FlashManage_BufferCRC(FlashManage_PageBuffer.words) == control_crc) ? 1U : 0U;
}
/* 将当前页缓冲写入指定 Flash 页，并回读校验预期 CRC */
static uint8_t FlashManage_WritePageFromBuffer(uint32_t page_address, uint32_t expected_crc){
	MyFLASH_ErasePage(page_address);
	MyFLASH_ProgramBuffer(page_address, FlashManage_PageBuffer.words, OTA_PAGE_WORD_COUNT);
	return (FlashManage_FlashPageCRC(page_address) == expected_crc) ? 1U : 0U;
}

/* 从任意源页复制 1 KB 到目标页；源页和目标页 CRC 都必须匹配预期值 */
static uint8_t FlashManage_CopyPage(uint32_t source_address, uint32_t target_address, uint32_t expected_crc){
	MyFLASH_ReadBuffer(source_address, FlashManage_PageBuffer.words, OTA_PAGE_WORD_COUNT);
	if (FlashManage_BufferCRC(FlashManage_PageBuffer.words) != expected_crc){
		return 0U;
	}
	return FlashManage_WritePageFromBuffer(target_address, expected_crc);
}

/* 将控制页中 CRC 非 0 的页从指定源槽复制到目标槽，返回失败页号或 0xFF */
static uint8_t FlashManage_CopyNonZeroPages(uint32_t source_base){
	uint8_t page;

	for (page = 0U; page < OTA_PROGRAM_PAGE_COUNT; page++){
		uint32_t expected_crc = FlashManage_Ota.metadata.Page[page].Page_CRC;
		uint32_t source_address = source_base + ((uint32_t)page * OTA_PAGE_SIZE);
		uint32_t target_address = FlashManage_Ota.data_base + ((uint32_t)page * OTA_PAGE_SIZE);

		if (expected_crc == 0U){
			continue;
		}
		if (!FlashManage_CopyPage(source_address, target_address, expected_crc)){
			return page;
		}
		HAL_IWDG_Refresh(&hiwdg);
	}
	return OTA_DATA_AREA_COMPLETE;
}

/* 控制页接收完成：校验、选槽、转换为绝对地址并写入 RECEIVING 元数据 */
static void FlashManage_CompleteControlPage(void){
	Data_Backup data;
	uint8_t slot;
	uint8_t page;
	uint8_t copy_page;
	uint16_t pending_pages = 0U;

	memcpy(&data, FlashManage_PageBuffer.bytes, sizeof(data));
	if (!FlashManage_ControlPageIsValid(&data)){
		FlashManage_Ota.stage = FLASHMANAGE_OTA_IDLE;
		FlashManage_SendOtaError(OTA_RUN_STATUS_PAUSED, 0xFFU, 0xFFU, OTA_ERROR_CONTROL_PAGE_INVALID);
		return;
	}

	slot = FlashManage_SelectTargetSlot(data.ID);
	if (slot == 0xFFU){
		FlashManage_Ota.stage = FLASHMANAGE_OTA_IDLE;
		FlashManage_SendOtaError(OTA_RUN_STATUS_PAUSED, 0xFFU, 0xFFU, OTA_ERROR_NO_WRITABLE_SLOT);
		return;
	}

	FlashManage_Ota.control_page = FlashManage_Slots[slot].control_page;
	FlashManage_Ota.data_base = FlashManage_Slots[slot].data_base;
	FlashManage_Ota.metadata = data;

	for (page = 0U; page < OTA_PROGRAM_PAGE_COUNT; page++){
		FlashManage_Ota.metadata.Page[page].Page_ADD =
			FlashManage_Ota.data_base + ((uint32_t)page * OTA_PAGE_SIZE);
		if (FlashManage_Ota.metadata.Page[page].Page_CRC == 0U){
			pending_pages |= (uint16_t)(1U << page);
		}
	}
	FlashManage_Ota.metadata.Page[15].Page_ADD =
		FlashManage_Ota.data_base + OTA_CONTROL_PAGE_OFFSET;

	/* CRC 非 0 页先从运行区转移到目标槽，全部成功后才写控制页进入下一步 */
	copy_page = FlashManage_CopyNonZeroPages(APP_RUN_BASE);
	if (copy_page != OTA_DATA_AREA_COMPLETE){
		FlashManage_Ota.metadata.Page[copy_page].Page_CRC = 0U;
		FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
			OTA_RUN_STATUS_PAUSED, copy_page);
		FlashManage_Ota.stage = FLASHMANAGE_OTA_WAIT_END;
		FlashManage_SendOtaError(OTA_RUN_STATUS_PAUSED, copy_page, 0xFFU, OTA_ERROR_PAGE_CRC_MISMATCH);
		return;
	}

	FlashManage_Ota.pending_pages = pending_pages;
	FlashManage_Ota.current_page = FlashManage_FirstPendingPage(pending_pages);
	FlashManage_Ota.next_block = 0U;
	FlashManage_Ota.stage = (FlashManage_Ota.current_page == OTA_DATA_AREA_COMPLETE)
		? FLASHMANAGE_OTA_WAIT_END : FLASHMANAGE_OTA_PAGE;

	FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
		(FlashManage_Ota.stage == FLASHMANAGE_OTA_WAIT_END)
			? OTA_RUN_STATUS_VERIFYING : OTA_RUN_STATUS_RECEIVING,
		FlashManage_Ota.current_page);
}

/* 程序页收满：擦除、写入、回读 CRC，更新控制页并推进到下一页 */
static void FlashManage_CompleteProgramPage(void){
	uint8_t page = FlashManage_Ota.current_page;
	uint32_t data_address = FlashManage_Ota.data_base + ((uint32_t)page * OTA_PAGE_SIZE);
	uint32_t page_crc = FlashManage_BufferCRC(FlashManage_PageBuffer.words);
	uint8_t next_page;
	uint8_t run_status;
	uint8_t data_area;

	if (!FlashManage_WritePageFromBuffer(data_address, page_crc)){
		FlashManage_Ota.metadata.Page[page].Page_CRC = 0U;
		FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
			OTA_RUN_STATUS_PAUSED, page);
		FlashManage_Ota.stage = FLASHMANAGE_OTA_WAIT_END;
		FlashManage_SendOtaError(OTA_RUN_STATUS_PAUSED, page, 0xFFU, OTA_ERROR_PAGE_CRC_MISMATCH);
		return;
	}

	FlashManage_Ota.metadata.Page[page].Page_CRC = page_crc;
	FlashManage_Ota.pending_pages &= (uint16_t)~(uint16_t)(1U << page);
	next_page = FlashManage_FirstPendingPage(FlashManage_Ota.pending_pages);
	run_status = (next_page == OTA_DATA_AREA_COMPLETE)
		? OTA_RUN_STATUS_VERIFYING : OTA_RUN_STATUS_RECEIVING;
	data_area = (next_page == OTA_DATA_AREA_COMPLETE) ? OTA_DATA_AREA_COMPLETE : page;

	FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
		run_status, data_area);
	FlashManage_Ota.current_page = next_page;
	FlashManage_Ota.next_block = 0U;
	FlashManage_Ota.stage = (next_page == OTA_DATA_AREA_COMPLETE)
		? FLASHMANAGE_OTA_WAIT_END : FLASHMANAGE_OTA_PAGE;
}

/* SetDat 接收入口：处理首帧、停等 ACK、重复帧和块序号 */
void FlashManage_HandleSetDat(uint8_t* RXData){
	uint8_t block = RXData[7];
	uint32_t frame_crc = FlashManage_ReadU32(&RXData[12]);

	if (!FlashManage_FrameIsValid(RXData)){
		return;
	}

	if (FlashManage_Ota.stage == FLASHMANAGE_OTA_IDLE){
		FlashManage_SendAck(RXData);
		if (block != 0U){
			FlashManage_SendOtaError(OTA_RUN_STATUS_PAUSED, 0xFFU, block, OTA_ERROR_SEQUENCE);
			return;
		}
		FlashManage_Ota.stage = FLASHMANAGE_OTA_CONTROL;
		FlashManage_Ota.pending_pages = 0U;
		FlashManage_Ota.next_block = 0U;
		FlashManage_Ota.current_page = 0xFFU;
		FlashManage_Ota.last_block = 0xFFU;
		FlashManage_Ota.last_frame_crc = 0U;
	}

	/* 重复帧只重发 AckCRC，不重复采样或写 Flash */
	if ((block == FlashManage_Ota.last_block) &&
	    (frame_crc == FlashManage_Ota.last_frame_crc)){
		FlashManage_SendAck(RXData);
		return;
	}

	if (FlashManage_Ota.stage == FLASHMANAGE_OTA_WAIT_END){
		FlashManage_SendAck(RXData);
		FlashManage_SendOtaError(OTA_RUN_STATUS_VERIFYING, 0xFFU, block, OTA_ERROR_INVALID_STATE);
		return;
	}

	FlashManage_SendAck(RXData);
	if (block != FlashManage_Ota.next_block){
		FlashManage_SendOtaError(
			(FlashManage_Ota.stage == FLASHMANAGE_OTA_CONTROL)
				? OTA_RUN_STATUS_PAUSED : FlashManage_Ota.metadata.Run_status,
			FlashManage_Ota.current_page, block, OTA_ERROR_SEQUENCE);
		return;
	}

	memcpy(&FlashManage_PageBuffer.bytes[(uint32_t)block * 4U], &RXData[8], 4U);
	FlashManage_Ota.next_block = (uint16_t)block + 1U;
	FlashManage_Ota.last_block = block;
	FlashManage_Ota.last_frame_crc = frame_crc;

	if (FlashManage_Ota.next_block == OTA_BLOCK_COUNT){
		if (FlashManage_Ota.stage == FLASHMANAGE_OTA_CONTROL){
			FlashManage_CompleteControlPage();
		}else{
			FlashManage_CompleteProgramPage();
		}
	}
}

/* 发送指定状态记录的业务帧，并等待上位机确认 */
static uint8_t FlashManage_SendStatusData(const char* type, uint8_t run_status, uint8_t page, uint8_t block, uint8_t error){
	return FlashManage_SendAndWaitAck(type, 0U,
		FlashManage_MakeStatus(run_status, page, block, error));
}

/* 当前运行区查询状态帧：Block 不适用填 0xFF */
static uint8_t FlashManage_SendStatusFrame(const char* type, uint8_t page, uint8_t error){
	return FlashManage_SendStatusData(type, FlashManage_ReadRunStatus(BackupPageADD1), page, 0xFFU, error);
}

/* 查询事务结束：Page 不适用时使用 0xFF，附加指定错误码 */
static void FlashManage_SendQueryEnd(uint8_t error){
	(void)FlashManage_SendStatusFrame("SetEnd", 0xFFU, error);
}

/* 最终验证目标槽 15 个程序页和 Page16 控制页 CRC */
static uint8_t FlashManage_VerifyTargetImage(uint8_t* failed_page){
	uint8_t page;

	for (page = 0U; page < OTA_PROGRAM_PAGE_COUNT; page++){
		uint32_t address = FlashManage_Ota.data_base + ((uint32_t)page * OTA_PAGE_SIZE);
		if (FlashManage_FlashPageCRC(address) != FlashManage_Ota.metadata.Page[page].Page_CRC){
			*failed_page = page;
			return 0U;
		}
	}

	MyFLASH_ReadBuffer(FlashManage_Ota.control_page, FlashManage_PageBuffer.words, OTA_PAGE_WORD_COUNT);
	FlashManage_WriteU32(&FlashManage_PageBuffer.bytes[0x80], 0U);
	if (FlashManage_BufferCRC(FlashManage_PageBuffer.words) != FlashManage_Ota.metadata.Page[15].Page_CRC){
		*failed_page = 0xFFU;
		return 0U;
	}
	return 1U;
}

/* 成功收尾：SetMID + 16 帧 SetCRC + SetSta + SetEnd */
static uint8_t FlashManage_SendFinalSuccess(void){
	uint8_t page;

	if (!FlashManage_SendAndWaitAck("SetMID", 0U, FlashManage_Ota.metadata.ID)){
		return 0U;
	}
	for (page = 0U; page < OTA_PAGE_COUNT; page++){
		if (!FlashManage_SendAndWaitAck("SetCRC", page, FlashManage_Ota.metadata.Page[page].Page_CRC)){
			return 0U;
		}
	}
	if (!FlashManage_SendStatusData("SetSta", OTA_RUN_STATUS_VALID_BACKUP, 0xFFU, 0xFFU, OTA_ERROR_NONE)){
		return 0U;
	}
	return FlashManage_SendStatusData("SetEnd", OTA_RUN_STATUS_VALID_BACKUP, 0xFFU, 0xFFU, OTA_ERROR_NONE);
}

/* 失败收尾：SetEro + SetSta + SetEnd */
static void FlashManage_SendFinalFailure(uint8_t page, uint8_t error){
	FlashManage_SendOtaError(OTA_RUN_STATUS_ERROR, page, 0xFFU, error);
	(void)FlashManage_SendStatusData("SetSta", OTA_RUN_STATUS_ERROR, 0xFFU, 0xFFU, error);
	(void)FlashManage_SendStatusData("SetEnd", OTA_RUN_STATUS_ERROR, 0xFFU, 0xFFU, error);
}

/* SetEnd 收尾：最终校验成功后上报目标信息，失败则进入 ERROR 状态 */
static void FlashManage_HandleSetEnd(uint8_t* RXData){
	uint8_t failed_page = 0xFFU;

	if (!FlashManage_FrameIsValid(RXData)){
		return;
	}
	FlashManage_SendAck(RXData);

	if (FlashManage_Ota.stage != FLASHMANAGE_OTA_WAIT_END){
		FlashManage_SendOtaError(
			(FlashManage_Ota.stage == FLASHMANAGE_OTA_CONTROL)
				? OTA_RUN_STATUS_PAUSED : FlashManage_Ota.metadata.Run_status,
			FlashManage_Ota.current_page, 0xFFU, OTA_ERROR_INVALID_STATE);
		return;
	}

	if (!FlashManage_VerifyTargetImage(&failed_page)){
		if (failed_page < OTA_PROGRAM_PAGE_COUNT){
			FlashManage_Ota.metadata.Page[failed_page].Page_CRC = 0U;
		}
		FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
			OTA_RUN_STATUS_ERROR, failed_page);
		FlashManage_SendFinalFailure(failed_page, OTA_ERROR_FIRMWARE_VERIFY_FAIL);
		FlashManage_Ota.stage = FLASHMANAGE_OTA_IDLE;
		return;
	}

	FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
		OTA_RUN_STATUS_VALID_BACKUP, OTA_DATA_AREA_COMPLETE);
	if (!FlashManage_SendFinalSuccess()){
		FlashManage_WriteControlPage(FlashManage_Ota.control_page, &FlashManage_Ota.metadata,
			OTA_RUN_STATUS_ERROR, OTA_DATA_AREA_COMPLETE);
		FlashManage_SendFinalFailure(0xFFU, OTA_ERROR_TIMEOUT);
	}
	FlashManage_Ota.stage = FLASHMANAGE_OTA_IDLE;
}

/*检查状态是否有VALID_BACKUP*/
uint8_t FlashManage_CheckValidBackup(void){
	uint8_t index;
	uint8_t found = 0xFFU;
	uint16_t found_id = 0U;
	uint16_t id;

	for (index = 0U; index < 2U; index++){
		if (MyFLASH_ReadByte(FlashManage_Slots[index].control_page + 0x02U) !=
		    OTA_RUN_STATUS_VALID_BACKUP){
			continue;
		}

		id = MyFLASH_ReadHalfWord(FlashManage_Slots[index].control_page);
		if ((found == 0xFFU) || FlashManage_IsNewerProgramID(id, found_id)){
			found = index;
			found_id = id;
		}
	}
	return found;
}

/*如果有，拷贝，改状态，改APP_add*/
void FlashManage_CopyValidBackup(uint8_t backup_slot){
	Data_Backup source;
	Data_Backup target;
	const FlashManage_Slot* slot = &FlashManage_Slots[backup_slot];
	uint8_t page;

	MyFLASH_ReadBuffer(slot->control_page, (uint32_t*)&source, sizeof(source) / 4U);

	for (page = 0U; page < OTA_PROGRAM_PAGE_COUNT; page++){
		FlashManage_CopyPage(
			slot->data_base + ((uint32_t)page * OTA_PAGE_SIZE),
			APP_RUN_BASE + ((uint32_t)page * OTA_PAGE_SIZE),
			source.Page[page].Page_CRC);
		HAL_IWDG_Refresh(&hiwdg);
	}

	target = source;
	for (page = 0U; page < OTA_PROGRAM_PAGE_COUNT; page++){
		target.Page[page].Page_ADD = APP_RUN_BASE + ((uint32_t)page * OTA_PAGE_SIZE);
	}
	FlashManage_WriteControlPage(BackupPageADD1, &target,
		OTA_RUN_STATUS_RUN_PENDING_CONFIRM, OTA_DATA_AREA_COMPLETE);
	FlashManage_WriteControlPage(slot->control_page, &source,
		OTA_RUN_STATUS_RUN_PENDING_CONFIRM, OTA_DATA_AREA_COMPLETE);
}void FlashManage_HCC  (uint8_t* RXData,uint8_t* TXData){

	/*RX为接收到的数据，TX为发送出去的数据（不用考虑数据缓存区大小）*/

while(1){
/* 固定 16 字节帧，各命令直接按偏移从 RXData 读取字段 */
if(RXData[0] == 0xA5U){
/*开始通讯*/
	if(memcmp(&RXData[1], "SetDat", 6) == 0){
		FlashManage_HandleSetDat(RXData);
		return;
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
		FlashManage_HandleSetEnd(RXData);
		return;
	}
	
	
	
	





	
	

	
}	
	


};




}




















