#include "iap.h"
#include "w25qxx.h"
#include "stmflash.h"

# if 0

#if (UPDATE_INFOR_SAVEMODE == 0)
/************************************************************
*
* Function name	: save_stroage_update_file_infor_function
* Description	: 存储更新信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_stroage_update_file_infor_function(save_param_t param)
{
	STMFLASH_Write(STORE_ADDR, (u16 *)&param, sizeof(save_param_t) / 2);
}

/************************************************************
*
* Function name	: save_read_update_file_infor_function
* Description	: 读取更新信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void save_read_update_file_infor_function(save_param_t *param)
{
	STMFLASH_Read(STORE_ADDR,(uint16_t*)param,sizeof(save_param_t) / 2);
}

#endif

#endif
