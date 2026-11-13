#include "Mydef.h"

/* SPI 操作最长等待 2 ms，避免 HAL_MAX_DELAY 造成阻塞 */
#define FREQ2_4G_SPI_TIMEOUT_MS 2U
#define FREQ2_4G_TX_TIMEOUT_MS 100U

uint16_t FREQ2_4G_SendFREQ=2400;
uint16_t FREQ2_4G_ReadFREQ=2400;


uint64_t FREQ2_4G_SendARR=0xC2C2C2C2C2;
uint64_t FREQ2_4G_ReadARR=0xA5A5A5A5A5;
uint64_t FREQ2_4G_ReadARR_1=0xC2C2C2C2C2;

void FREQ2_4G_SendInit (void){
////////ACK+ACK包回应模式
uint8_t TX;
uint8_t RX;
	HAL_GPIO_WritePin(SPI_CE_GPIO_Port, SPI_CE_Pin, GPIO_PIN_RESET);
	//CE 拉低后直接配置

	
	//设置地址长度
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_SETUP_AW);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x03;						//5个地址
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	//设置动态负载
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_FEATURE);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x06;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);	
	
	//打开0通道动态负载
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_DYNPD);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x01;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);		
	
	//设置频率
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RF_CH);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=FREQ2_4G_SendFREQ-2400;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	//设置发送地址
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_TX_ADDR);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	for (int i = 0; i < 5; i++)
	{
    TX = (uint8_t)((FREQ2_4G_SendARR >> (8 * i)) & 0xFF);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	}
	SPI_CSN(1);
	
	//设置接收0通道地址
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RX_ADDR_P0);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	for (int i = 0; i < 5; i++)
	{
    TX = (uint8_t)((FREQ2_4G_SendARR >> (8 * i)) & 0xFF);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	}
	SPI_CSN(1);
	
	//设置射频设置
	
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RF_SETUP);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x0F; 					//2Mbps，7dBm
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
		//自动重发配置
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_SETUP_RETR);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0xff; 					
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	//设置模式
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_CONFIG);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x0E; 					//配置为发射模式，CRC，开机
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	HAL_GPIO_WritePin(SPI_CE_GPIO_Port,SPI_CE_Pin,1);
	
	
	
}


void FREQ2_4G_ReadInit (void)
{

////////ACK+ACK包回应模式
uint8_t TX;
uint8_t RX;
	 HAL_GPIO_WritePin(SPI_CE_GPIO_Port, SPI_CE_Pin, GPIO_PIN_RESET);
	//CE 拉低后直接配置


	//使能接收通道0、通道1
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_EN_RXADDR);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x03;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	// 选择射频信道
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RF_CH);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=FREQ2_4G_ReadFREQ-2400;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	//设置地址长度
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_SETUP_AW);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x03;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);	
	
		//设置通道0接收地址
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RX_ADDR_P0);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	for (int i = 0; i < 5; i++)
	{
    TX = (uint8_t)((FREQ2_4G_ReadARR >> (8 * i)) & 0xFF);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	}
	SPI_CSN(1);
	
		//设置通道1接收地址	
	
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RX_ADDR_P1);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	for (int i = 0; i < 5; i++)
	{
    TX = (uint8_t)((FREQ2_4G_ReadARR_1 >> (8 * i)) & 0xFF);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	}
	SPI_CSN(1);

	
	
	
	//开启通道0、1动态负载
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_DYNPD);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x03;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);	

	//配置特征寄存器
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_FEATURE);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x06;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);	
	
	//设置射频设置
	
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_RF_SETUP);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x0F; 					//配置为发射模式，CRC，开机
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	//自动重发配置
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_SETUP_RETR);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0xff; 					
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	//启动RX
	SPI_CSN(0);
	TX=SI24R1_WriteRegister(SI24R1_CONFIG);	
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x0F;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);	
	
	
	
	
  HAL_GPIO_WritePin(SPI_CE_GPIO_Port, SPI_CE_Pin, GPIO_PIN_SET);


}





static void FREQ2_4G_ClearStatus(uint8_t flags)
{
	uint8_t TX = SI24R1_WriteRegister(SI24R1_STATUS);
	uint8_t RX;

	SPI_CSN(0);
	HAL_SPI_TransmitReceive(&hspi1, &TX, &RX, 1, FREQ2_4G_SPI_TIMEOUT_MS);
	TX = flags;
	HAL_SPI_TransmitReceive(&hspi1, &TX, &RX, 1, FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
}

static void FREQ2_4G_FlushTx(void)
{
	uint8_t TX = SI24R1_FLUSH_TX;
	uint8_t RX;

	SPI_CSN(0);
	HAL_SPI_TransmitReceive(&hspi1, &TX, &RX, 1, FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
}

uint8_t FREQ2_4G_SendBag(Send_Data *DATA)
{
	uint8_t TX = SI24R1_W_TX_PAYLOAD;
	uint8_t RX;
	uint8_t RX_NULL[32] = {0};
	uint8_t STATUS = 0U;
	uint32_t start_tick;

	if ((DATA == NULL) || (DATA->TXData == NULL) ||
	    (DATA->TXLen == 0U) || (DATA->TXLen > 32U))
	{
		return HAL_ERROR;
	}

	DATA->RXLen = 0U;
	DATA->STATUS = 0U;
	HAL_GPIO_WritePin(SPI_CE_GPIO_Port, SPI_CE_Pin, GPIO_PIN_RESET);
	FREQ2_4G_FlushTx();
	FREQ2_4G_ClearStatus(0x70U);

	/* 写入一包数据，再给 CE 一个有效高电平 */
	SPI_CSN(0);
	if (HAL_SPI_TransmitReceive(&hspi1, &TX, &RX, 1, FREQ2_4G_SPI_TIMEOUT_MS) != HAL_OK)
	{
		SPI_CSN(1);
		return HAL_TIMEOUT;
	}
	if (HAL_SPI_TransmitReceive(&hspi1, DATA->TXData, RX_NULL, DATA->TXLen,
	                            FREQ2_4G_SPI_TIMEOUT_MS) != HAL_OK)
	{
		SPI_CSN(1);
		return HAL_TIMEOUT;
	}
	SPI_CSN(1);
	HAL_GPIO_WritePin(SPI_CE_GPIO_Port, SPI_CE_Pin, GPIO_PIN_SET);

	/* 等待 ACK：TX_DS 表示发送成功并收到自动应答 */
	start_tick = HAL_GetTick();
	do
	{
		TX = SI24R1_NOP;
		SPI_CSN(0);
		HAL_SPI_TransmitReceive(&hspi1, &TX, &STATUS, 1, FREQ2_4G_SPI_TIMEOUT_MS);
		SPI_CSN(1);

		if ((STATUS & 0x20U) != 0U)
		{
			DATA->STATUS = STATUS;
			FREQ2_4G_ClearStatus(0x70U);
			return HAL_OK;
		}
		if ((STATUS & 0x10U) != 0U)
		{
			DATA->STATUS = STATUS;
			FREQ2_4G_ClearStatus(0x70U);
			FREQ2_4G_FlushTx();
			return HAL_ERROR;
		}
		HAL_Delay(1);
	} while ((uint32_t)(HAL_GetTick() - start_tick) < FREQ2_4G_TX_TIMEOUT_MS);

	DATA->STATUS = STATUS;
	FREQ2_4G_ClearStatus(0x70U);
	FREQ2_4G_FlushTx();
	return HAL_TIMEOUT;
}

uint8_t FREQ2_4G_ReadBag (Read_Data *DATA){


	uint8_t TX=SI24R1_R_RX_PL_WID;
	uint8_t RX=0;
	uint8_t RX_Len=0;
	uint8_t STATUS=0;

	//有数据再运行
	uint8_t cmd = SI24R1_ReadRegister(SI24R1_FIFO_STATUS);
    uint8_t fifoStatus = 0U;

    SPI_CSN(0);
    HAL_SPI_TransmitReceive(&hspi1, &cmd, &RX, 1, FREQ2_4G_SPI_TIMEOUT_MS);
    cmd = 0x00U;
    HAL_SPI_TransmitReceive(&hspi1, &cmd, &fifoStatus, 1, FREQ2_4G_SPI_TIMEOUT_MS);
    SPI_CSN(1);

    if ((fifoStatus & 0x01U) != 0U)
    {
        DATA->RXLen = 0U;
        DATA->STATUS = 0x0EU;
        return HAL_OK;
    }
	
	
	//获取收到字节数
	SPI_CSN(0);
	HAL_SPI_TransmitReceive(&hspi1,&TX,&STATUS,1,FREQ2_4G_SPI_TIMEOUT_MS);
	TX=0x00;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX_Len,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	if((STATUS&0x0E)==0x08){//未查明偶发问题（可能为芯片或电路设计问题）/*只要是读STATUS就会偶发出错0x48*/
	SPI_CSN(0);	
	TX=SI24R1_NOP;
	HAL_SPI_TransmitReceive(&hspi1,&TX,&STATUS,1,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	};
	
	
	

if(RX_Len){
	uint8_t *RX_Data=DATA->RXData;
	uint8_t TX_NULL[RX_Len];
	
	//读取字节
	TX=SI24R1_R_RX_PAYLOAD;
	SPI_CSN(0);	
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	HAL_SPI_TransmitReceive(&hspi1,TX_NULL,RX_Data,RX_Len,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);	

	
	//写ACK BAG
	uint8_t TX_len=DATA->TXLen;

	
	if(TX_len)
	{
	uint8_t* TX_Data=DATA->TXData;
	uint8_t RX_NULL[TX_len];
	TX=SI24R1_W_ACK_PAYLOAD_BASE;
	SPI_CSN(0);		
	HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
	HAL_SPI_TransmitReceive(&hspi1,TX_Data,RX_NULL,TX_len,FREQ2_4G_SPI_TIMEOUT_MS);
	SPI_CSN(1);
	
	};
	
};
	
	
	DATA->RXLen=RX_Len;
	DATA->STATUS=STATUS;
	
	
	return HAL_OK;
	

}

uint8_t FREQ2_4G_SendClc(SendClc_Data *Data)
{
	Send_Data DATA = {0};
	uint8_t status;

	if ((Data == NULL) || (Data->TXData == NULL) ||
	    (Data->TXLen == 0U) || (Data->TXLen > 32U))
	{
		return HAL_ERROR;
	}

	DATA.TXData = Data->TXData;
	DATA.TXLen = Data->TXLen;
	status = FREQ2_4G_SendBag(&DATA);
	Data->RXLen = 0U;
	return status;
}






uint8_t FREQ2_4G_ReadClc(ReadClc_Data *Data){


uint8_t RX_BUFF[100];
 Read_Data DATA;


DATA.RXData=RX_BUFF;
DATA.TXData=Data->TXData ;
DATA.TXLen=Data->TXLen;



FREQ2_4G_ReadBag(&DATA);

	if((DATA.RXLen>0U)&&(DATA.RXLen<=32U)){
	if((DATA.STATUS&0x0E)==0x00){//P0接到数据
	
	Data->RX_PIN=0;
		
	for(int i=0;i<DATA.RXLen;i++){
	
		*((Data->RXData_P0)+i)=*(RX_BUFF+i);
	
	};
	
	
	}else if((DATA.STATUS&0x0E)==0x02){//P1接到数据
	
	Data->RX_PIN=1;
	
		for(int i=0;i<DATA.RXLen;i++){
	
		*((Data->RXData_P1)+i)=*(RX_BUFF+i);
	
	};

	}else if((DATA.STATUS&0x0E)==0x04){//P2接到数据
	
	Data->RX_PIN=2;
	
		for(int i=0;i<DATA.RXLen;i++){
	
		*((Data->RXData_P2)+i)=*(RX_BUFF+i);
	
	};
	
	}else if((DATA.STATUS&0x0E)==0x06){//P3接到数据
	
	Data->RX_PIN=3;
		for(int i=0;i<DATA.RXLen;i++){
	
		*((Data->RXData_P3)+i)=*(RX_BUFF+i);
	
	};
	
	
	}else if((DATA.STATUS&0x0E)==0x08){//P4接到数据
	
	Data->RX_PIN=4;
	
		for(int i=0;i<DATA.RXLen;i++){
	
		*((Data->RXData_P4)+i)=*(RX_BUFF+i);
	
	};


	
		
		

	
	
	
	}else if((DATA.STATUS&0x0E)==0x0A){////P5接到数据
	
	Data->RX_PIN=5;
	
		for(int i=0;i<DATA.RXLen;i++){
	
		*((Data->RXData_P5)+i)=*(RX_BUFF+i);
	
	};
	
	};
};
	
	Data->RXLen=DATA.RXLen;
	
	
if((DATA.STATUS&0x0E)==0x0E||(DATA.STATUS&0x0E)==0x0C){//未收到包或错误处理

return HAL_ERROR;
	
}else{
return HAL_OK;
}


}

void FREQ2_4G_GetSignal(uint8_t* VAL){//信号强度
	uint8_t TX;
	uint8_t RX;
//	HAL_GPIO_WritePin(SPI_CE_GPIO_Port,SPI_CE_Pin,GPIO_PIN_RESET);
SPI_CSN(0);
TX=SI24R1_ReadRegister(SI24R1_RSSI);
HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
TX=0x00;
HAL_SPI_TransmitReceive(&hspi1,&TX,&RX,1,FREQ2_4G_SPI_TIMEOUT_MS);
SPI_CSN(1);

if(RX){*VAL=1;}	else{*VAL=0;};
//	HAL_GPIO_WritePin(SPI_CE_GPIO_Port,SPI_CE_Pin,GPIO_PIN_SET);
	
}
