#ifndef _LWIP_COMM_H
#define _LWIP_COMM_H 
#include "sys.h"
#include "bsp.h"
//#include "lan8720.h" 
   

#define LWIP_MAX_DHCP_TRIES		4   //DHCP服务器最大重试次数
   
/* TCP客户端相关参数 */
#define LWIP_TCP_NO_CONNECT   (0) // TCP还未连接
#define LWIP_TCP_INIT_CONNECT (1) // TCP初始化中
#define LWIP_TCP_CONNECT	  (2) // TCP连接成功

#define LWIP_TCP_CONNECT_NUM  (5) // TCP连接尝试次数

/* UDP客户端相关参数 */
#define LWIP_UDP_NO_CONNECT   0 // UDP还未连接
#define LWIP_UDP_INIT_CONNECT 1 // UDP初始化中
#define LWIP_UDP_CONNECT	  	2 // UDP连接成功
//lwip控制结构体
typedef struct  
{
	uint8_t init;		   // 网络初始化：0：还未初始化 1：初始化过了
	uint8_t netif_state;   // 网口状态:0未创建网卡 1已经创建了网卡
	uint8_t tcp_status;	   // 网络状态：0：tcp还未连接 1：tcp连接成功
	uint8_t udp_status;    // 网络状态：0：udp还未连接 1：udp连接成功
	uint8_t udp_multicast_status;	 // 网络状态：0：udp还未连接 1：udp连接成功
	uint8_t iporname;	   // ip或则域名 0-直接使用IP 1-需要通过域名获取ip
	uint8_t domename;	   // 域名获取状态
	
	uint8_t mac[6];        // MAC地址
	uint8_t remoteip[4];   // 远端主机IP地址 
	uint8_t ip[4];         // 本机IP地址
	uint8_t netmask[4];    // 子网掩码
	uint8_t gateway[4];    // 默认网关的IP地址
	uint32_t remoteport;   // 远端主机端口
	uint8_t dns[4];		   // dns
	
	vu8 dhcpstatus;		   // dhcp状态 
						   // 0,未获取DHCP地址;
						   // 1,进入DHCP获取状态
						   // 2,成功获取DHCP地址
						   // 0XFF,获取失败.
	
	uint8_t reset;		   // 网络复位：检测到1对网口进行复位
	uint8_t tcp_reset;	   // tcp复位: 检测到1对tcp客户端进行复位
	uint8_t udp_reset;	   // udp复位: 检测到1对udp客户端进行复位
	uint8_t udp_onvif_flag;	// onvif主动搜索标志位，防止ping模式下开始onvif任务，随后被关闭
	uint8_t udp_multicast_reset;
	#ifdef WIRELESS_PRIORITY_CONNECTION
	uint8_t tcp_cmd;	   // 网络连接命令
	#endif
}__lwip_dev;
extern __lwip_dev lwipdev; // lwip控制结构体


void lwip_pkt_handle(void);
void lwip_comm_default_ip_set(__lwip_dev *lwipx);
void lwip_updata_remote_network_infor(__lwip_dev *lwipx);
u8 lwip_comm_mem_malloc(void);
void lwip_comm_mem_free(void);
u8 lwip_comm_init(void);
void lwip_comm_dhcp_creat(void);
void lwip_comm_dhcp_delete(void);
void lwip_comm_destroy(void);
void lwip_comm_delete_next_timeout(void);

int8_t lwip_start_function(void);
void lwip_stop_function(void);
void lwip_get_remote_ip(uint8_t *ip);

uint8_t *lwip_get_mac_addr(void);

#endif













