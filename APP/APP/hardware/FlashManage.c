#include "Mydef.h"

/*flash内存管理*/



/*0x8000000~0x8003FFF*//*bootloader区域*/
/*0x8004000~0x8007FFF*//*一号程序区域*/
/*0x8008000~0x800BFFF*//*二号程序区域*/
/*0x800C000~0x800FFFF*//*数据备份区域*/
/*每个区域宽度0x4000==16KB*/
/*1KB一页*/

/*数据备份区域*/
/*0x800C000~0x800C3FF*//*一号程序数据备份*//*size=1024，一页*/
/*0x800C400~0x800C7FF*//*二号程序数据备份*//*size=1024，一页*/
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





void Backup_WritePage(Data_Backup Data){
	
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
	/*第十六页CRC*/
	MyFLASH_ProgramWord(BackupPageADD+0x7CU,Data.Page16.Page_ADD);
	MyFLASH_ProgramWord(BackupPageADD+0x80U,Data.Page16.Page_CRC);
		
	

}
























