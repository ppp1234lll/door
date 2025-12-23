#include "tcp_client.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "includes.h"
#include "key.h"
#include "app.h"
#include "appconfig.h"
#include "com.h"
#include "led.h"
#include "gsm.h"
#include "eth.h"
#include "malloc.h"
#include "lwip/tcp_impl.h"
#include "gsm.h"
#include "update.h"

struct netconn *tcp_clientconn = NULL;	// TCP CLIENT网络连接结构体
uint16_t tcp_client_flag;				// TCP客户端数据发送标志位
uint8_t *tcp_client_send_buff;
struct netbuf *sg_recvbuf = NULL;

// TCP客户端任务
#define TCPCLIENT_PRIO		11
// 任务堆栈大小
#define TCPCLIENT_STK_SIZE	300
// 任务堆栈
OS_STK TCPCLIENT_TASK_STK[TCPCLIENT_STK_SIZE];

//tcp客户端任务函数
static void tcp_client_thread(void *arg)
{
	OS_CPU_SR cpu_sr;
	struct pbuf *q;
	err_t err,recv_err;
	
	static ip_addr_t server_ipaddr,loca_ipaddr;
	static u16_t 		 server_port,loca_port;
	uint8_t tcp_connect_cnt = 0;
	
	LWIP_UNUSED_ARG(arg);

	while (1) 
	{
		if(lwipdev.tcp_status == LWIP_TCP_INIT_CONNECT) // TCP连接中
		{
			/* 获取IP地址 */
			server_port = lwipdev.remoteport;
			IP4_ADDR(&server_ipaddr, lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
			/* 尝试连接TCP */
			tcp_clientconn = netconn_new(NETCONN_TCP);  						// 创建一个TCP链接
			if(tcp_clientconn == NULL)
			{
				eth_set_network_reset();
				break;
			}
			err = netconn_connect(tcp_clientconn,&server_ipaddr,server_port);	// 连接服务器
			if(err != ERR_OK)  													// 连接失败
			{
				netconn_delete(tcp_clientconn); 								// 返回值不等于ERR_OK,删除tcp_clientconn连接
				tcp_clientconn = NULL;

				tcp_connect_cnt++;
				if(tcp_connect_cnt > LWIP_TCP_CONNECT_NUM)
				{
					/* 一定次数过后还未成功连接上服务器 */
					lwipdev.tcp_status = 0;
					tcp_connect_cnt = 0;
					/* 关闭TCP连接 */
					#ifdef WIRED_PRIORITY_CONNECTION
					/* 开启GPRS */
					gsm_set_tcp_cmd(1);
					#endif

					#if 1 // add by unclexu, 当被扫描断网时,重启lwip,不如重启系统
						//lwipdev.init = 0;
						//lwipdev.reset = 1;
						System_SoftReset(); // 重启系统也很快,还能消除未知的隐患
					#endif
					break;
				}
			}
			else																// 连接成功
			{
				lwipdev.tcp_status = LWIP_TCP_CONNECT;							// 服务器连接成功
				
				#ifdef WIRED_PRIORITY_CONNECTION
				gsm_set_tcp_cmd(0);
				#endif
				app_set_com_interface_selection_function(0);
				led_control_function(LD_LAN,LD_FLICKER);
//				led_outside_control_function(LD_LAN_LED,LD_FLICKER);
				
				tcp_clientconn->recv_timeout = 10;
				netconn_getaddr(tcp_clientconn,&loca_ipaddr,&loca_port,1); 		// 获取本地IP主机IP地址和端口号

				app_send_once_heart_infor();  									// 发送一次心跳
			}
		}
		else if(lwipdev.tcp_status == LWIP_TCP_CONNECT) // TCP连接成功
		{
			if((tcp_client_flag & LWIP_SEND_DATA) == LWIP_SEND_DATA) // 有数据要发送
			{
				err = netconn_write( tcp_clientconn ,tcp_client_send_buff,(tcp_client_flag & 0x3fff),NETCONN_COPY); 
				if(err != ERR_OK)
				{
					/* 发送失败 */
					app_set_send_result_function(SR_SEND_ERROR);
				}
				tcp_client_flag &= ~LWIP_SEND_DATA;
			}
				
			if((recv_err = netconn_recv(tcp_clientconn,&sg_recvbuf)) == ERR_OK)  //接收到数据
			{	
				OS_ENTER_CRITICAL(); //关中断
//				memset(tcp_client_recvbuf,0,TCP_CLIENT_RX_BUFSIZE);  //数据接收缓冲区清零
				for(q=sg_recvbuf->p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
				{
					//判断要拷贝到TCP_CLIENT_RX_BUFSIZE中的数据是否大于TCP_CLIENT_RX_BUFSIZE的剩余空间，如果大于
					//的话就只拷贝TCP_CLIENT_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数
					if(q->len > 0) {
						com_stroage_cache_data(q->payload,q->len);
					}					
				}
				OS_EXIT_CRITICAL();  //开中断				
				
				netbuf_delete(sg_recvbuf);
				sg_recvbuf = NULL;
			}else if(recv_err == ERR_CLSD)  //关闭连接
			{
//				led_control_function(LD_LAN,LD_OFF);
				netconn_close(tcp_clientconn);
				netconn_delete(tcp_clientconn);
				lwipdev.tcp_status = LWIP_TCP_NO_CONNECT;		// 服务器断开
				tcp_connect_cnt = 0;
//				break;
			}
		}
		
		/* 停止tcp */
		if(lwipdev.tcp_reset == 1)					  // 重启tcp连接
		{
//			lwipdev.domename = 0; 					  // 重新获取IP
			lwipdev.tcp_reset = 0;
			tcp_client_stop_function();
		}
		
		OSTimeDlyHMSM(0,0,0,10);
	}
	
}

//创建TCP客户端线程
//返回值:0 TCP客户端创建成功
//		其他 TCP客户端创建失败
INT8U tcp_client_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(tcp_client_thread,(void*)0,(OS_STK*)&TCPCLIENT_TASK_STK[TCPCLIENT_STK_SIZE-1],TCPCLIENT_PRIO); //创建TCP客户端线程
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}

/************************************************************
*
* Function name	: tcp_client_start_function
* Description	: tcp客户端启动函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void tcp_client_start_function(void)
{
	/* 创建TCP客户端 */
	lwipdev.tcp_reset = 0;
	tcp_client_init();
}

/************************************************************
*
* Function name	: tcp_client_stop_function
* Description	: tcp客户端停止函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void tcp_client_stop_function(void)
{
	lwipdev.tcp_reset = 0;
	
	OS_CPU_SR cpu_sr;
	
	led_control_function(LD_LAN,LD_ON);
	
	/* 关闭tcp服务器 */
	if(lwipdev.tcp_status == LWIP_TCP_CONNECT)
	{
		netconn_close(tcp_clientconn);
	}
	netconn_delete(tcp_clientconn);
	tcp_clientconn = NULL;
	
	lwipdev.tcp_status = LWIP_TCP_NO_CONNECT;
	
	OS_ENTER_CRITICAL();		// 关中断
	OSTaskDel(TCPCLIENT_PRIO);	// 删除TCP任务
	OS_EXIT_CRITICAL();			// 开中断
}


/************************************************************
*
* Function name	: tcp_send_data_immediately
* Description	: 立刻发送tcp数据
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t tcp_send_data_immediately(uint8_t *str, uint16_t len)
{
	err_t err;
	
	if(lwipdev.tcp_status != LWIP_TCP_CONNECT) {
		return 0;
	}	
	
	err = netconn_write(tcp_clientconn ,(const void *)str,len,NETCONN_COPY);
	if(err != ERR_OK)
	{
		 return -1;
	}
	return 0;
}

/************************************************************
*
* Function name	: tcp_set_send_buff
* Description	: tcp发送数据
* Parameter		: 
* Return		: 
*	
************************************************************/
void tcp_set_send_buff(uint8_t *buff, uint16_t len)
{
	tcp_client_send_buff = buff;
	tcp_client_flag      = len + LWIP_SEND_DATA;
}

/************************************************************
*
* Function name	: tcp_get_send_status
* Description	: tcp发送状态
* Parameter		: 
* Return		: 
*	
************************************************************/
uint8_t tcp_get_send_status(void)
{
	return tcp_client_flag;
}


