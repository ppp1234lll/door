#include "update.h"
#include "iap.h"
#include "bootload.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "gsm.h"
#include "eth.h"
#include "malloc.h"
#include "iwdg.h"
#include "http_update.h"

//static update_param_t *sg_updateparam_t;
struct netconn *tcp_update = NULL;
//struct netbuf 	 *recvbuf = NULL;


int8_t update_lwip_task_function(void)
{
	ip_addr_t server_ipaddr;
	uint16_t  server_port;
	int8_t    ret = 0;
	update_param_t *updateparam = NULL;
	////
	
	/* 初始化参数 */
	updateparam = update_get_infor_data_function();
	server_port = updateparam->port;
	IP4_ADDR(&server_ipaddr, updateparam->ip[0], updateparam->ip[1], updateparam->ip[2], updateparam->ip[3]);

	sg_http_update_param.section_len = (UPDATE_CHUNK_SIZE - 2);
	sg_http_update_param.http_response_recv_size = 0;
	////

	// 1: 获得info.txt信息
	ret = http_update_get_info_txt_by_lwip(&server_ipaddr, server_port);
	if( (ret < 0) || (ret == 2) )
	{
		if(ret < 0){ printf("\n获得info.txt信息,失败! ret: %d\n", ret); }
		else{ printf("\n版本是最新版本,无需更新!\n"); }
		goto UPDATE_END;
	}

	// 2: 获得crc_bin文件大小
	ret = http_update_get_crc_bin_file_size_by_lwip();
	if(ret < 0)
	{
		printf("\n获得crc_bin文件大小,失败! ret: %d\n", ret);
		goto UPDATE_END;
	}
	
	// 3: 获得crc_bin文件数据
	ret = http_update_get_crc_bin_file_data_by_lwip();
	if(ret < 0)
	{
		printf("\n获得crc_bin文件内容,失败! ret: %d\n", ret);
		goto UPDATE_END;
	}

	// 重启系统
	printf("\n升级完成,重启设备...\n");
	http_update_success_reboot();

	ret = 0;

	// 更新结束
UPDATE_END:
	http_update_close_connect_by_lwip(); // 先关闭连接
	updateparam->error	 = 0;
	if(ret < 0){ updateparam->success = 0; }
	else{ updateparam->success = 1; }
	updateparam->end = 1;
	updateparam->mode = UPDATE_MODE_NULL; // 更新结束

	led_control_function(LD_LAN, LD_OFF);
	
	if(ret < 0){ return(-1); }
	return(0);
}
//////////////////////

