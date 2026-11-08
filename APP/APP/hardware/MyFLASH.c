#include "stm32f1xx_hal.h"                  // Device header

/**
  * 函    数：FLASH读取一个32位的字
  * 参    数：Address 要读取数据的字地址
  * 返 回 值：指定地址下的数据
  */
uint32_t MyFLASH_ReadWord(uint32_t Address)
{
	return *((__IO uint32_t *)(Address));	//使用指针访问指定地址下的数据并返回
}

/**
  * 函    数：FLASH读取一个16位的半字
  * 参    数：Address 要读取数据的半字地址
  * 返 回 值：指定地址下的数据
  */
uint16_t MyFLASH_ReadHalfWord(uint32_t Address)
{
	return *((__IO uint16_t *)(Address));	//使用指针访问指定地址下的数据并返回
}

/**
  * 函    数：FLASH读取一个8位的字节
  * 参    数：Address 要读取数据的字节地址
  * 返 回 值：指定地址下的数据
  */
uint8_t MyFLASH_ReadByte(uint32_t Address)
{
	return *((__IO uint8_t *)(Address));	//使用指针访问指定地址下的数据并返回
}

/**
  * 函    数：FLASH连续读取字
  * 参    数：Address 起始字地址
  * 参    数：Data 读取数据缓冲区
  * 参    数：WordCount 读取字数
  */
void MyFLASH_ReadBuffer(uint32_t Address, uint32_t* Data, uint16_t WordCount)
{
	uint16_t i;

	for (i = 0U; i < WordCount; i++)
	{
		Data[i] = MyFLASH_ReadWord(Address + ((uint32_t)i * 4U));
	}
}

/**
  * 函    数：FLASH全擦除
  * 参    数：无
  * 返 回 值：无
  * 说    明：调用此函数后，FLASH的所有页都会被擦除，包括程序文件本身，擦除后，程序将不复存在
  */
void MyFLASH_EraseAllPages(void)
{
	FLASH_EraseInitTypeDef EraseInitStruct = {0};
	uint32_t PageError = 0;

	HAL_FLASH_Unlock();										//解锁
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;	//全擦除
	HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);		//全擦除
	HAL_FLASH_Lock();										//加锁
}

/**
  * 函    数：FLASH页擦除
  * 参    数：PageAddress 要擦除页的页地址
  * 返 回 值：无
  */
void MyFLASH_ErasePage(uint32_t PageAddress)
{
	FLASH_EraseInitTypeDef EraseInitStruct = {0};
	uint32_t PageError = 0;

	HAL_FLASH_Unlock();										//解锁
	EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;		//页擦除
	EraseInitStruct.PageAddress = PageAddress;				//页地址
	EraseInitStruct.NbPages = 1;							//擦除1页
	HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);		//页擦除
	HAL_FLASH_Lock();										//加锁
}

/**
  * 函    数：FLASH编程字
  * 参    数：Address 要写入数据的字地址
  * 参    数：Data 要写入的32位数据
  * 返 回 值：无
  */
void MyFLASH_ProgramWord(uint32_t Address, uint32_t Data)
{
	HAL_FLASH_Unlock();															//解锁
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, (uint64_t)Data);		//编程字
	HAL_FLASH_Lock();															//加锁
}

/**
  * 函    数：FLASH编程半字
  * 参    数：Address 要写入数据的半字地址
  * 参    数：Data 要写入的16位数据
  * 返 回 值：无
  */
void MyFLASH_ProgramHalfWord(uint32_t Address, uint16_t Data)
{
	HAL_FLASH_Unlock();															//解锁
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, Address, (uint64_t)Data);	//编程半字
	HAL_FLASH_Lock();															//加锁
}

/**
  * 函    数：FLASH连续编程字
  * 参    数：Address 起始字地址
  * 参    数：Data 待写入的 32 位数据缓冲区
  * 参    数：WordCount 写入字数
  * 说    明：一次解锁后连续写入，供 OTA 页写入复用
  */
void MyFLASH_ProgramBuffer(uint32_t Address, const uint32_t* Data, uint16_t WordCount)
{
	uint16_t i;

	HAL_FLASH_Unlock();
	for (i = 0U; i < WordCount; i++)
	{
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address + ((uint32_t)i * 4U), (uint64_t)Data[i]);
	}
	HAL_FLASH_Lock();
}
