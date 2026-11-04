#include "Mydef.h"

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





void FlashManage_BackupWritePage(Data_Backup Data){
	
	uint32_t BackupPageADD;
	
	if(Data.Data_area==1)BackupPageADD=BackupPageADD1;else BackupPageADD=BackupPageADD2;
	
	MyFLASH_ErasePage(BackupPageADD);
	/*版本号 */
	MyFLASH_ProgramHalfWord(BackupPageADD,Data.ID);
	
    /*运行状态（低字节） + 代码区域（高字节） */
    MyFLASH_ProgramHalfWord(BackupPageADD+0x02U,(uint16_t)((uint16_t)Data.Run_status |((uint16_t)Data.Data_area << 8)));
	/*第一页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x04U,Data.Page1.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x08U,Data.Page1.Page_CRC);
	/*第二页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x0CU,Data.Page2.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x10U,Data.Page2.Page_CRC);	
	/*第三页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x14U,Data.Page3.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x18U,Data.Page3.Page_CRC);	
	/*第四页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x1CU,Data.Page4.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x20U,Data.Page4.Page_CRC);	
	/*第五页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x24U,Data.Page5.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x28U,Data.Page5.Page_CRC);
	/*第六页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x2CU,Data.Page6.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x30U,Data.Page6.Page_CRC);
	/*第七页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x34U,Data.Page7.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x38U,Data.Page7.Page_CRC);
	/*第八页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x3CU,Data.Page8.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x40U,Data.Page8.Page_CRC);
	/*第九页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x44U,Data.Page9.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x48U,Data.Page9.Page_CRC);	
	/*第十页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x4CU,Data.Page10.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x50U,Data.Page10.Page_CRC);
	/*第十一页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x54U,Data.Page11.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x58U,Data.Page11.Page_CRC);	
	/*第十二页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x5CU,Data.Page12.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x60U,Data.Page12.Page_CRC);
	/*第十三页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x64U,Data.Page13.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x68U,Data.Page13.Page_CRC);		
	/*第十四页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x6CU,Data.Page14.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x70U,Data.Page14.Page_CRC);
	/*第十五页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x74U,Data.Page15.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x78U,Data.Page15.Page_CRC);
	
	//	/*第十六页CRC*//*数据备份区域*/
	MyFLASH_ProgramWord(BackupPageADD+0x7CU,Data.Page16.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x80U,Data.Page16.Page_CRC);
		
}

void TXD_Send(uint8_t* TXD){



}
	
	

void FlashManage_HCC  (uint8_t* RXData,uint8_t* TXData){
	
	/*RX为接收到的数据，TX为发送出去的数据（不用考虑数据缓存区大小）*/
	
while(1){
	uint8_t Data_Head[1];/*帧头*/
	uint8_t Data_type[6];/*帧类型*/
	uint8_t Data_num[1];/*帧编号*/
	uint8_t Data_Dat[4];/*帧数据*/
	uint8_t Data_CRC[4];/*帧CRC(数据段)* /

/*			1bye/6bye/1bye/4bye/4bye 				 */
/*Data为	帧头/类型/编号/数据段/CRC(数据段)			*/


/*数据处理*/
Data_Head[0]=RXData[0];
Data_num[0]=RXData[7];

for (int i = 0; i < 6; i++)
{
	Data_type[i]=RXData[i+1];
}
for (int i = 0; i < 4; i++)
{
	Data_Dat[i]=RXData[i+8];
}
for (int i = 0; i < 4; i++)
{
	Data_CRC[i]=RXData[i+12];
}




/*状态机*/
if(Data_Head[0]==0xA5){
/*开始通讯*/
	if(strcmp("SetDat",(const char*)Data_type)==0){
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

	
	}else if (strcmp("GetCRC",(const char*)Data_type)==0)
	{
		/*上位机获取每页CRC校验*/
		/*每页编号0~255*/
		/*0xA5/GetCRC/当前编号/(uint32)0x00/CRC(数据段)*/

		/*发送每页CRC*/
/*		0xA5/SetCRC/当前编号/CRC校验值/CRC(数据段)    		*/



		/*发送结束帧(最后单独发送一次)*/
/*		0xA5/SetEnd/0x00/当前状态/CRC(数据段)    		*/

	}else if (strcmp("GetMID",(const char*)Data_type)==0)
	{
		/*上位机获取当前运行固件ID*/
		/*0xA5/GetMID/0x00/(uint32)0x00/CRC(数据段)*/
		
		/*发送当前状态*/
		/*0xA5/GetMID/0x00/(uint32)ID/CRC(数据段)*/

		/*发送结束帧(最后单独发送一次)*/
		/*0xA5/SetEnd/0x00/当前状态/CRC(数据段)*/

	}else if (strcmp("GetSta",(const char*)Data_type)==0){
		/*上位机获取当前烧录状态*/
		/*0xA5/GetSta/0x00/(uint32)0x00/CRC(数据段)*/



		/*发送当前状态*/
		/*0xA5/SetSta/0x00/(uint32)Start/CRC(数据段)*/	


		/*发送结束帧(最后单独发送一次)*/
		/*0xA5/SetEnd/0x00/当前状态/CRC(数据段)*/




	}else if (strcmp("SetEro",(const char*)Data_type)==0){
		/*上位机发送错误指令*/
		/*0xA5/SetEro/0x00/(uint32)0x00/CRC(数据段)*/

		/*错误处理(先不写)*/

		/*发送结束帧(最后单独发送一次)*/
		/*0xA5/SetEnd/0x00/当前状态/CRC(数据段)*/

	}else if (strcmp("SetEnd",(const char*)Data_type)==0){
		/*上位机发送结束指令*/
		/*0xA5/SetEnd/0x00/(uint32)0x00/CRC(数据段)*/

		/*发送结束帧(最后单独发送一次)*/
		/*0xA5/SetEnd/0x00/当前状态/CRC(数据段)*/

	}
	
	
	
	





	
	

	
}	
	


};




}




















