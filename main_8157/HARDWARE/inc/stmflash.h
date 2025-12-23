#ifndef __STMFLASH_H
#define __STMFLASH_H

#include "sys.h"  
#include "appconfig.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////
//用户根据自己的需要设置
#define STM32_FLASH_SIZE 256 	 		//所选STM32的FLASH容量大小(单位为K)
#define STM32_FLASH_WREN 1        //使能FLASH写入(0，不是能;1，使能)
//////////////////////////////////////////////////////////////////////////////////////////////////////

//FLASH起始地址
#define STM32_FLASH_BASE 0x08000000 	//STM32 FLASH的起始地址

// 0x0803F000
#define STM_SAVE_SIZE	 4096
#define STORE_ADDR	   (STM32_FLASH_BASE + (STM32_FLASH_SIZE * 1024) - STM_SAVE_SIZE)

// 0x0803E800
#define DEVICE_ID_ADDR   (STM32_FLASH_BASE + (STM32_FLASH_SIZE * 1024) - 5120)		// 设备ID 5k
#define DEVICE_MAC_ADDR  (STM32_FLASH_BASE + (STM32_FLASH_SIZE * 1024) - 4608)		// 设备MAC 5K-512

// 0x0803E000
#define GSM_ERROR_ADDR	 (STM32_FLASH_BASE + (STM32_FLASH_SIZE * 1024) - 8192)  // 



//FLASH解锁键值
 
u16 STMFLASH_ReadHalfWord(u32 faddr);		  //读出半字  
void STMFLASH_WriteLenByte(u32 WriteAddr,u32 DataToWrite,u16 Len);	//指定地址开始写入指定长度的数据
u32 STMFLASH_ReadLenByte(u32 ReadAddr,u16 Len);						//指定地址开始读取指定长度数据
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite);		//从指定地址开始写入指定长度的数据
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead);   		//从指定地址开始读出指定长度的数据
void STMFLASH_Write_SAVE(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)	;
//测试写入
void Test_Write(u32 WriteAddr,u16 WriteData);			
void STMFLASH_ErasePage(u32 WriteAddr);


#endif



