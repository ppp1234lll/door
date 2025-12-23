#include "onvif.h"
#include "lwip_comm.h"
#include "led.h"
#include "app.h"
#include "det.h"
#include "start.h"
#include "malloc.h"
#include "string.h"
#include "lwip/opt.h"
#include <lwip/sockets.h>
 
//TCP客户端任务
#define ONVIF_PRIO		  12
#define ONVIF_STK_SIZE	320//任务堆栈大小
OS_STK ONVIF_TASK_STK[ONVIF_STK_SIZE];//任务堆栈

#define ONVIF_SEARCH_NUM		1  //	ONVIF搜索次数
#define ONVIF_RECV_NUM		 10  //	ONVIF接收数据包次数
#define ONVIF_IPC_NUM 		 10  // 摄像机数量
#define ONVIF_SEARCH_TIME  10  // 60S

char dahua_protocol_buf[105] = { /* Packet 23 */
0x20, 0x00, 0x00, 0x00, 0x44, 0x48, 0x49, 0x50,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x49, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x7b, 0x20, 0x22, 0x6d, 0x65, 0x74, 0x68, 0x6f,0x64, 0x22, 0x20, 0x3a, 0x20, 0x22, 0x44, 0x48, 
0x44, 0x69, 0x73, 0x63, 0x6f, 0x76, 0x65, 0x72,0x2e, 0x73, 0x65, 0x61, 0x72, 0x63, 0x68, 0x22, 
0x2c, 0x20, 0x22, 0x70, 0x61, 0x72, 0x61, 0x6d,0x73, 0x22, 0x20, 0x3a, 0x20, 0x7b, 0x20, 0x22, 
0x6d, 0x61, 0x63, 0x22, 0x20, 0x3a, 0x20, 0x22,0x22, 0x2c, 0x20, 0x22, 0x75, 0x6e, 0x69, 0x22, 
0x20, 0x3a, 0x20, 0x31, 0x20, 0x7d, 0x20, 0x7d,0x0a };

char dahua_protocol_buf1[32] = { /* Packet 23 */
0xa3,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

uint16_t dahua_port_buf[4] = { DAHUA_MULTICAST_PROT1,DAHUA_MULTICAST_PROT2,\
															 DAHUA_MULTICAST_PROT3,DAHUA_MULTICAST_PROT4};

IPC_Info_t ipcInfo;  // IPC摄像头参数

int onvif_sock = -1;

//udp任务函数
static void onvif_thread(void *arg)
{
	int ret = 0;
//	struct sockaddr_in local_addr;
	struct ip_mreq mreq_onvif,mreq_dahua; 			// 多播地址结构体
	struct timeval tv_out;
	struct local_ip_t  *local = app_get_local_network_function();
	char ip_param[20] = {0};
	
	LWIP_UNUSED_ARG(arg);
	
	if(ONVIF_SEARCH_DEBUG) printf("create onvif \r\n");
	
	while (1) 
	{
		if(lwipdev.udp_status == LWIP_UDP_INIT_CONNECT) // UDP连接中
		{				
			sprintf(ip_param,"%d.%d.%d.%d",local->ip[0],local->ip[1],local->ip[2],local->ip[3]);
			onvif_sock = socket(AF_INET, SOCK_DGRAM, 0);  // 创建socket
			if(onvif_sock < 0) 
			{
				if(ONVIF_SEARCH_DEBUG) printf("sock error \r\n");
				lwipdev.udp_reset = 1;
			}	
			else
			{
//				local_addr.sin_family = AF_INET;
//				local_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr(ip_param); /*<! 待与 socket 绑定的本地网络接口 IP */   
//				local_addr.sin_port = htons(LOCAL_MULTICAST_PROT); /*<! 待与 socket 绑定的本地端口号 */  				
//				ret = bind(onvif_sock, (struct sockaddr*)&local_addr, sizeof(local_addr));		// 将 Socket 与本地某网络接口绑定 
//				if(ret != 0 )
//				{	
//					if(ONVIF_SEARCH_DEBUG) printf("bind error \r\n");
//				}
//				else
				{
//					if(ONVIF_SEARCH_DEBUG) printf("onvif bind success\r\n");
					
					// 加入海康、ONVIF组播地址
					mreq_onvif.imr_multiaddr.s_addr=inet_addr(MULTICAST_ADDR); /*<! 多播组 IP 地址设置 */
					mreq_onvif.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(ip_param); /*<! 待加入多播组的 IP 地址 */   
					// 添加多播组成员（该语句之前，socket 只与 某单播IP地址相关联 执行该语句后 将与多播地址相关联）
					ret = setsockopt(onvif_sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq_onvif,sizeof(mreq_onvif));
					if(ret != 0 )
					{	
						if(ONVIF_SEARCH_DEBUG) printf("MULTICAST_ADDR error：%d\r\n",ret);
					}					
					// 加入大华组播地址
					mreq_dahua.imr_multiaddr.s_addr=inet_addr(DAHUA_MULTICAST_ADDR1); /*<! 多播组 IP 地址设置 */
					mreq_dahua.imr_interface.s_addr = htonl(INADDR_ANY);//inet_addr(ip_param); /*<! 待加入多播组的 IP 地址 */   
					ret = setsockopt(onvif_sock,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq_dahua,sizeof(mreq_dahua));
					if(ret != 0 )
					{	
						if(ONVIF_SEARCH_DEBUG) printf("DAHUA_MULTICAST_ADDR error：%d\r\n",ret);
					}				
						
					tv_out.tv_sec = 10;
					tv_out.tv_usec = 0;
					setsockopt(onvif_sock, SOL_SOCKET, SO_RCVTIMEO, &tv_out, sizeof(tv_out));			//recv延时时间设置

					int value = 1; //1 开启端口复用 0关闭
					setsockopt(onvif_sock, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value));
					
					lwipdev.udp_status = LWIP_UDP_CONNECT;
					ipcInfo.onvif_times = app_get_onvif_time();;
					ipcInfo.search_flag |= ONVIF_INIT|ONVIF_START;
					if(ONVIF_SEARCH_DEBUG)  printf("onvif successed...\n");
				}
			}
		}
		else if(lwipdev.udp_status == LWIP_UDP_CONNECT) // 连接成功
		{		
			if((ipcInfo.search_flag & ONVIF_INIT)== ONVIF_INIT )  // 初始化参数
			{
				if(ONVIF_SEARCH_DEBUG)  printf("onvif_init...\n");
				ipcInfo.search_flag &=~ONVIF_INIT;
				ipcInfo.ipc_protocol_status = HIKVISION_P;   //先使用海康协议搜索
				ipcInfo.ipc_num = 0;		
				memset(ipcInfo.ip,0,sizeof(uint8_t)*4*IPC_NUM_MAX);				
			}
			if((ipcInfo.search_flag & ONVIF_START)== ONVIF_START )  // 开始检测
			{
				ONVIF_Search_API(onvif_sock);
			}
			else if((ipcInfo.search_flag & ONVIF_END)== ONVIF_END ) // 检测结束
			{
				ipcInfo.search_flag &=~ONVIF_END;
				if(ONVIF_SEARCH_DEBUG)  printf("onvif_search_end...\n");
				ONVIF_IPC_NET_Detection_API();  // 判断网络
			}
		}			
		if(lwipdev.udp_reset == 1)					  // 重启tcp连接
		{
			if(ONVIF_SEARCH_DEBUG) printf("onvif_CLOSE \r\n");
			lwipdev.udp_reset = 0;
			ipcInfo.onvif_times = 0;
			if(onvif_sock>=0) close(onvif_sock);	
			onvif_udp_stop();
		}
		OSTimeDlyHMSM(0,0,0,50);  //延时5s
	}
}
/************************************************************
*
* Function name	: ONVIF_Search_API
* Description	: 发送探测消息
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
int ONVIF_Search_API(int sockfd)
{
	int ret=0;
	
	switch(ipcInfo.ipc_protocol_status)
	{
		case HIKVISION_P:
			if(ONVIF_SEARCH_DEBUG) printf("HIKVISION_P_start...\n");
			ret = ONVIF_IPC_Bind_API(sockfd,HIKVISION_MULTICAST_PROT); // 连接端口
			if(ret ==0)
			{
				for(uint8_t j=0;j<ONVIF_SEARCH_NUM;j++)   // 循环搜索，
				{
					HIKVISION_IPC_Search_API(sockfd,MULTICAST_ADDR,HIKVISION_MULTICAST_PROT);
					OSTimeDlyHMSM(0,0,0,100); // unclexu add
					for(uint8_t i = 0 ; i < ONVIF_RECV_NUM; i++){ ONVIF_IPC_Recv_API(sockfd,HIKVISION_P); }
					OSTimeDlyHMSM(0,0,0,100);  //延时5s	
				}							
				ipcInfo.ipc_protocol_status = DAHUA_P;
			}
			break;
		
		case DAHUA_P:
			if(ONVIF_SEARCH_DEBUG) printf("DAHUA_P_start...\n");

			for(uint8_t k = 0 ; k < 4; k++)
			{
				ret = ONVIF_IPC_Bind_API(sockfd,dahua_port_buf[k]); // 连接端口
				if(ret ==0)
				{		
					for(uint8_t j=0;j<ONVIF_SEARCH_NUM;j++)   // 循环搜索，
					{
						if(dahua_port_buf[k] == DAHUA_MULTICAST_PROT1){ DAHUA_IPC_Search_API(sockfd,DAHUA_MULTICAST_ADDR2,dahua_port_buf[k]); }
						else{ DAHUA_IPC_Search_API(sockfd,DAHUA_MULTICAST_ADDR1,dahua_port_buf[k]); }
						OSTimeDlyHMSM(0,0,0,100); // unclexu add
						for(uint8_t i = 0 ; i < ONVIF_RECV_NUM; i++){ ONVIF_IPC_Recv_API(sockfd,DAHUA_P); }
						OSTimeDlyHMSM(0,0,0,100);  //延时5s
					}			
				}		
			}				
			ipcInfo.ipc_protocol_status = UNW_P;	
			break;

		case UNW_P:
			if(ONVIF_SEARCH_DEBUG) printf("UNW_P_start...\n");	
			ret = ONVIF_IPC_Bind_API(sockfd,UNW_MULTICAST_PROT); // 连接端口
			if(ret ==0)
			{		
				for(uint8_t j=0;j<ONVIF_SEARCH_NUM;j++)   // 循环搜索，
				{
					ONVIF_IPC_Search_API(sockfd,UNW_MULTICAST_ADDR,UNW_MULTICAST_PROT);
					OSTimeDlyHMSM(0,0,0,100); // unclexu add
					for(uint8_t i = 0 ; i < ONVIF_RECV_NUM; i++){ ONVIF_IPC_Recv_API(sockfd,UNW_P); }
					OSTimeDlyHMSM(0,0,0,100);  //延时5s
				}
			}				
			ipcInfo.ipc_protocol_status = ONVIF_P;	
			break;
			
		case ONVIF_P:
			if(ONVIF_SEARCH_DEBUG) printf("ONVIF_P_start...\n");
			ret = ONVIF_IPC_Bind_API(sockfd,MULTICAST_PROT); // 连接端口
			if(ret ==0)
			{	
				for(uint8_t j=0;j<ONVIF_SEARCH_NUM;j++)   // 循环搜索，
				{
					ONVIF_IPC_Search_API(sockfd,MULTICAST_ADDR,MULTICAST_PROT);
					OSTimeDlyHMSM(0,0,0,100); // unclexu add
					for(uint8_t i = 0 ; i < ONVIF_RECV_NUM; i++){ ONVIF_IPC_Recv_API(sockfd,ONVIF_P); }
					OSTimeDlyHMSM(0,0,0,100);  //延时5s
				}
			}
			ipcInfo.ipc_protocol_status = IPC_PROTOCOL_END;
			break;

		default: 
			ipcInfo.search_flag &=~ONVIF_START;
			ipcInfo.search_flag |= ONVIF_END;  // 搜索结束
			if(ONVIF_SEARCH_DEBUG) printf("ipc_num...%d..\n",ipcInfo.ipc_num);
			break;
	}
	return 0;
}

/************************************************************
*
* Function name	: ONVIF_IPC_Bind_API
* Description	: 发送探测消息
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
int ONVIF_IPC_Bind_API(int sockfd,int port)
{
	int ret = 0;
	int opts = 1; //1 开启端口复用 0关闭
	struct sockaddr_in local_addr;
	
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opts, sizeof(opts));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr(ip_param); /*<! 待与 socket 绑定的本地网络接口 IP */   
	local_addr.sin_port = htons(port); /*<! 待与 socket 绑定的本地端口号 */  				
	ret = bind(onvif_sock, (struct sockaddr*)&local_addr, sizeof(local_addr));		// 将 Socket 与本地某网络接口绑定 
//	if(ret != 0 )
//	{	
//		printf("bind error \r\n");
//	}
//	else
//	{
//		printf("onvif bind success\r\n");
//	}
	return ret;
}

/************************************************************
*
* Function name	: ONVIF_IPC_Send_API
* Description	: 发送探测消息
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
int ONVIF_IPC_Send_API(int sockfd,char* data,int len,char* ip,int port)
{
	int ret = 0;
	struct sockaddr_in addr;      // internet环境下套接字的地址形式
	
	memset((void *)&addr,0,sizeof(struct sockaddr_in));
	addr.sin_family      = AF_INET;  
	addr.sin_port        = htons(port);    // 转换成网络
	addr.sin_addr.s_addr = inet_addr(ip);  // 将一个字符串格式的ip地址转换成一个uint32_t数字格式

	ret = sendto(sockfd,data,len,0,(struct sockaddr*)&addr,sizeof(addr)); // sendto主要在UDP连接中使用，作用是向另一端发送UDP报文   
	return ret;
}

/************************************************************
*
* Function name	: ONVIF_IPC_Search_API
* Description	: 发送ONVIF探测消息
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
/* 探测消息(Probe)，这些内容是ONVIF Device Test Tool 15.06工具搜索IPC时的Probe消息，通过Wireshark抓包工具抓包到的 */
int ONVIF_IPC_Search_API(int sockfd,char* ip,int port)
{
	uint16_t   Flagrand[9]= { 0 };
	char uuid_string[50]= { 0 };
	int ret = 0;
	int length = 0;
	uint32_t onvif_id[3] = {0};
	char *searchstr;
	
	start_get_device_id(onvif_id);          // 获取芯片MAC地址
	searchstr = (char *)mymalloc(SRAMIN,ONVIF_TX_BUFSIZE);  // 申请内存
	//memset(searchstr,0,ONVIF_TX_BUFSIZE);
		
	// 生成uuid,为了保证每次搜索的时候MessageID都是不相同的！因为简单，直接取了随机值
	Flagrand[0] = rand()%9000 + 1000; //保证四位整数 
	Flagrand[1] = rand()%9000 + 1000; //保证四位整数
	Flagrand[2] = rand()%9000 + 1000; //保证四位整数
	Flagrand[3] = rand()%9000 + 1000; //保证四位整数	
	Flagrand[4] = rand()%9000 + 1000; //保证四位整数		
	
	Flagrand[5] = (uint16_t)(onvif_id[1]);
	Flagrand[6] = (uint16_t)(onvif_id[1]>>16);
	Flagrand[7] = (uint16_t)(onvif_id[2]);
	Flagrand[8] = (uint16_t)(onvif_id[2]>>16);		

	// 16445-6-d68a-1dd2-11b2-a105-010203040506
	sprintf(uuid_string,"%04X%01X-%01X-%04X-%04X-%04X-%04X-%04X%04X%04X",
		Flagrand[0],Flagrand[1]&0x000F,(Flagrand[1]&0x00F0)>>8,Flagrand[2],Flagrand[3],Flagrand[4],
		Flagrand[5],Flagrand[6],Flagrand[7],Flagrand[8]);  
	
	sprintf(uuid_string,"16445-6-d68a-1dd2-11b2-a105-010203040506");
	
	length  = sprintf(searchstr,"%s",           "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	length += sprintf(searchstr+length,"%s",    "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://www.w3.org/2003/05/soap-envelope\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:SOAP-ENC=\"http://www.w3.org/2003/05/soap-encoding\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:xop=\"http://www.w3.org/2004/08/xop/include\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:wsa=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:tns=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:dn=\"http://www.onvif.org/ver10/network/wsdl\" ");
	length += sprintf(searchstr+length,"%s",    "xmlns:wsa5=\"http://www.w3.org/2005/08/addressing\">");
	length += sprintf(searchstr+length,"%s%s%s","<SOAP-ENV:Header><wsa:MessageID>urn:uuid:",uuid_string,"</wsa:MessageID>");
	length += sprintf(searchstr+length,"%s",    "<wsa:To SOAP-ENV:mustUnderstand=\"true\">urn:schemas-xmlsoap-org:ws:2005:04:discovery</wsa:To>");
	length += sprintf(searchstr+length,"%s",    "<wsa:Action SOAP-ENV:mustUnderstand=\"true\">http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</wsa:Action>");
	length += sprintf(searchstr+length,"%s",    "</SOAP-ENV:Header><SOAP-ENV:Body><tns:UniviewProbe><tns:Types>dn:NetworkVideoTransmitter</tns:Types>");
	length += sprintf(searchstr+length,"%s",    "</tns:UniviewProbe></SOAP-ENV:Body></SOAP-ENV:Envelope>");
	searchstr[length] = 0;

	ret = ONVIF_IPC_Send_API(sockfd,(char*)searchstr,length,ip,port);
	
	//	printf("onvif_start:%s\n",searchstr);
	
	myfree(SRAMIN,searchstr);   // 释放内存
	return ret;
}

/************************************************************
*
* Function name	: HIKVISION_IPC_Search_API
* Description	: 发送海康摄像机探测消息
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
/* 探测消息(Probe)，这些内容是ONVIF Device Test Tool 15.06工具搜索IPC时的Probe消息，通过Wireshark抓包工具抓包到的 */
int HIKVISION_IPC_Search_API(int sockfd,char* ip,int port)
{
	uint16_t   Flagrand[6];
	char uuid_string[40]= { 0 };
	int ret = 0;
	int length = 0;
	uint32_t onvif_id[3] = {0};
	char *searchstr;
	
	start_get_device_id(onvif_id);          // 获取芯片MAC地址
	searchstr = (char *)mymalloc(SRAMIN,ONVIF_TX_BUFSIZE);  // 申请内存
	//memset(searchstr,0,ONVIF_TX_BUFSIZE);
	
	Flagrand[0] = (uint16_t)(onvif_id[1]>>16);
	Flagrand[1] = (uint16_t)(onvif_id[1]);
	Flagrand[2] = (uint16_t)(onvif_id[2]>>16);		
	
	// 生成uuid,为了保证每次搜索的时候MessageID都是不相同的！因为简单，直接取了随机值
	Flagrand[3] = rand()%9000 + 1000; //保证四位整数 
	Flagrand[4] = rand()%9000 + 1000; //保证四位整数
	Flagrand[5] = rand()%9000 + 1000; //保证四位整数
	
	sprintf(uuid_string,"%08X-%04X-%04X-%04X-%04X%04X%04X",onvif_id[0],
					Flagrand[0],Flagrand[1],Flagrand[2],Flagrand[3],Flagrand[4],Flagrand[5]);  

	length  = sprintf(searchstr,"%s",           "<?xml version=\"1.0\" encoding=\"utf-8\"?>");	
	length += sprintf(searchstr+length,"%s",    "<Probe><Uuid>");
	length += sprintf(searchstr+length,"%s",    uuid_string);
	length += sprintf(searchstr+length,"%s",    "</Uuid><Types>inquiry</Types></Probe>");
	searchstr[length] = 0;

	//printf("%s\r\n",searchstr);

	ret = ONVIF_IPC_Send_API(sockfd,(char*)searchstr,length,ip,port);
	
	myfree(SRAMIN,searchstr);   // 释放内存
	return ret;
}

/************************************************************
*
* Function name	: DAHUA_IPC_Search_API
* Description	: 发送大华摄像机探测消息
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
/* 探测消息(Probe)，这些内容是ONVIF Device Test Tool 15.06工具搜索IPC时的Probe消息，通过Wireshark抓包工具抓包到的 */
int DAHUA_IPC_Search_API(int sockfd,char* ip,int port)
{
	int ret = 0;
	if(port == DAHUA_MULTICAST_PROT1){ ret = ONVIF_IPC_Send_API(sockfd,dahua_protocol_buf1,sizeof(dahua_protocol_buf1),ip,port); }
	else{ ret = ONVIF_IPC_Send_API(sockfd,dahua_protocol_buf,sizeof(dahua_protocol_buf),ip,port); }
	return ret;
}

/************************************************************
*
* Function name	: ONVIF_IPC_Recv_API
* Description	: 处理接收数据
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
int ONVIF_IPC_Recv_API(int sockfd,uint8_t cmd)
{
	//char rcvData[ONVIF_RX_BUFSIZE]={0}; // unclexu, 局部数组太大，触发系统重启。改为动态申请内存
	char *rcvData = NULL;
	int len=0;
	struct sockaddr_in recv_addr;
	int sender_len=sizeof(recv_addr);
	int temp[6] = {0};
	uint8_t ip[4] = {0};
	uint8_t mac[6] = {0};
	char ret=0;
	////

	rcvData = (char *)mymalloc(SRAMIN, ONVIF_RX_BUFSIZE);

	len = recvfrom(sockfd,rcvData,ONVIF_RX_BUFSIZE,0,(struct sockaddr*)&recv_addr,(socklen_t *)&sender_len);
	if(len > 0)
	{
		rcvData[len] = 0;
		//printf("%d:%s\r\n",len,rcvData);
		sscanf(inet_ntoa(recv_addr.sin_addr), "%d.%d.%d.%d", &temp[0],&temp[1],&temp[2],&temp[3]);
		ip[0] = temp[0];
		ip[1] = temp[1];
		ip[2] = temp[2];
		ip[3] = temp[3];
		onvif_get_ipc_mac_info(rcvData,mac,cmd);
		
		ret = onvif_match_camera_ip(ip);
		if(ret == 0)
		{		
			memcpy(ipcInfo.ip[ipcInfo.ipc_num],ip,4);
			memcpy(ipcInfo.mac[ipcInfo.ipc_num],mac,6);
			if( ipcInfo.ipc_num < IPC_NUM_MAX){ ipcInfo.ipc_num++; }  /*ipc个数增加*/
		}			
	}	
	else{ len = -1; }

	myfree(SRAMIN, (void *)rcvData);

	return len;
}

/************************************************************
*
* Function name	: onvif_get_ipc_mac_info
* Description	: 获取摄像机信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void onvif_get_ipc_mac_info(char *buff,uint8_t *mac,uint8_t cmd)  
{
	char *mac_data;
	int temp[6] = {0};
	char ret=0;
	
	switch(cmd)
	{
		case HIKVISION_P:	
			mac_data = strstr(buff,"<MAC>"); // 查找字符串
			if(mac_data != NULL)
			{
				ret = sscanf(mac_data,"<MAC>%x:%x:%x:%x:%x:%x</MAC>",&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5]);
				if(ret != 6)
				{
					ret = sscanf(mac_data,"<MAC>%x-%x-%x-%x-%x-%x</MAC>",&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5]);
				}
				if(ret ==6 )
				{
					mac[0] = temp[0];
					mac[1] = temp[1];
					mac[2] = temp[2];
					mac[3] = temp[3];
					mac[4] = temp[4];
					mac[5] = temp[5];
				}
				else
					if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");
			}
			else
				if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");	
		break;
			
		case DAHUA_P:	
			mac_data = strstr(buff,"mac"); // 查找字符串
			if(mac_data != NULL)
			{
				ret = sscanf(mac_data,"\"mac\":\"%x:%x:%x:%x:%x:%x\"",&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5]);
				if(ret == 6)
				{
					mac[0] = temp[0];
					mac[1] = temp[1];
					mac[2] = temp[2];
					mac[3] = temp[3];
					mac[4] = temp[4];
					mac[5] = temp[5];
				}
				else
					if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");				
			}
			else
			{
				mac_data = buff+120;
				ret = sscanf(mac_data,"%x:%x:%x:%x:%x:%x",&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5]);
				if(ret == 6)
				{
					mac[0] = temp[0];
					mac[1] = temp[1];
					mac[2] = temp[2];
					mac[3] = temp[3];
					mac[4] = temp[4];
					mac[5] = temp[5];
				}
				else
					if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");				
			}
		break;	
	
		case UNW_P:	
			mac_data = strstr(buff,"/macaddr/"); // 查找字符串
			if(mac_data != NULL)
			{
//				printf("macaddr\n");
				ret = sscanf(mac_data,"/macaddr/%x%x%x%x%x%x",&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5]);
				if(ret == 6)
				{
					mac[0] = temp[0];
					mac[1] = temp[1];
					mac[2] = temp[2];
					mac[3] = temp[3];
					mac[4] = temp[4];
					mac[5] = temp[5];
				}
				else
					if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");
			}
			else
				if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");	
		break;	

		case ONVIF_P:	
			mac_data = strstr(buff,"/macaddr/"); // 查找字符串
			if(mac_data != NULL)
			{
//				printf("macaddr\n");
				ret = sscanf(mac_data,"/macaddr/%02x%02x%02x%02x%02x%02x",&temp[0],&temp[1],&temp[2],&temp[3],&temp[4],&temp[5]);
				if(ret == 6)
				{
					mac[0] = temp[0];
					mac[1] = temp[1];
					mac[2] = temp[2];
					mac[3] = temp[3];
					mac[4] = temp[4];
					mac[5] = temp[5]; 
				}
				else
					if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");
			}
			else
				if(ONVIF_SEARCH_DEBUG) printf("onvif mac error\r\n");	
		break;	
			
		default:	
		break;	
	}		
}

/************************************************************
*
* Function name	: ONVIF_IPC_NET_Detection_API
* Description	: 网络摄像机IP比较
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
void ONVIF_IPC_NET_Detection_API(void)
{
	uint8_t  ip[4] = {0};
	uint8_t  cmac[6] = {0};
	int8_t   ret = 0;
	uint8_t  save_ipc_locat[10]= {0};  // 需要存储的摄像机位置
	uint8_t  save_ipc_num = 0;  // 需要存储的摄像机数量
	
	if(1 /*lwipdev.udp_onvif_flag == 2*/) // 配置摄像机, unclexu,改为 true.
	{
		lwipdev.udp_onvif_flag = 0;
		if(ONVIF_SEARCH_DEBUG) printf("udp_onvif_flag_config \r\n");
		for(uint8_t j=0; j< ipcInfo.ipc_num ;j++)  // 判断摄像机是否在里面
		{
			for(uint8_t i=0; i< 10 ;i++)  // 10路摄像机循环检测
			{
				app_get_camera_function(ip,i); // 获取摄像机IP				
				if(onvif_match_ip(ipcInfo.ip[j],ip) >= 0)  // IP没有重复
				  ret = 1;
				else
				{
					if(app_get_camera_mac_function(cmac,i) < 0)  // MAC地址为0
						app_set_camera_mac_function(ipcInfo.mac[j],i);
					ret = 0;	
					break;
				}
			}
			if(ret ==1)
			{
				save_ipc_locat[save_ipc_num] = j+1;
				save_ipc_num++;
			}
		}
		if(save_ipc_num > 0)
		{
			for(uint8_t i=0; i< 10 ;i++)  // 10路摄像机循环检测
			{		
				ret = app_get_camera_function(ip,i); // 获取摄像机IP
				if( ret < 0)  // IP不存在
				{
					save_ipc_num--;
					app_set_camera_num_function(ipcInfo.ip[save_ipc_locat[save_ipc_num]-1],i);
					app_set_camera_mac_function(ipcInfo.mac[save_ipc_locat[save_ipc_num]-1],i);
					if(save_ipc_num == 0)
						break;
				}	
			}
		}
		app_send_query_configuration_infor();  // 发送一次配置		
	}
	//else

	if(1) // unclexu,改为 true.
	{
		for(uint8_t i=0; i< 10 ;i++) // 10路摄像机循环检测
		{
			ret = app_get_camera_function(ip,i); // 获取摄像机IP
			if( ret < 0)  // IP不存在
			{}	
			else
			{
				if(ipcInfo.ipc_num == 0)  // 没有搜索到摄像机
				{
					det_set_camera_status(i,0);  // 网络故障
				}
				else 
				{
					for(uint8_t j=0; j< ipcInfo.ipc_num ;j++)  // 判断摄像机是否在里面
					{
						if(onvif_match_ip(ipcInfo.ip[j],ip) < 0)  // IP有重复
						{
							det_set_camera_status(i,1);  // 网络正常
							break;
						}
						else
						{
							det_set_camera_status(i,0);  // 网络故障
						}
					}				
				}	
			}	
		}
	}
}


/************************************************************
*
* Function name	: onvif_init
* Description	: 创建UDP线程
* Parameter		: 
* Return		: 
*	  20230810
//返回值:0 UDP创建成功
//		其他 UDP创建失败
************************************************************/
INT8U onvif_init(void)
{
	INT8U res,err;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//关中断
//	res = OSTaskCreate(onvif_thread,(void*)0,(OS_STK*)&ONVIF_TASK_STK[ONVIF_STK_SIZE-1],ONVIF_PRIO); //创建UDP线程
	
	res = OSTaskCreateExt(onvif_thread, 															//建立扩展任务(任务代码指针) 
										(void *)0,																					//传递参数指针 
										(OS_STK*)&ONVIF_TASK_STK[ONVIF_STK_SIZE-1], 					//分配任务堆栈栈顶指针 
										(INT8U)ONVIF_PRIO, 															//分配任务优先级 
										(INT16U)ONVIF_PRIO,															//(未来的)优先级标识(与优先级相同) 
										(OS_STK *)&ONVIF_TASK_STK[0], 											//分配任务堆栈栈底指针 
										(INT32U)ONVIF_STK_SIZE, 															//指定堆栈的容量(检验用) 
										(void *)0,																					//指向用户附加的数据域的指针 
										(INT16U)OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);		//建立任务设定选项 	
	
	OSTaskNameSet(ONVIF_PRIO, (INT8U *)(void *)"onvif", &err);
	
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}

/************************************************************
*
* Function name	: onvif_udp_start
* Description	: udp启动函数
* Parameter		: 
* Return		: 
*	  20230810
************************************************************/
void onvif_udp_start(void)
{
	/* 创建TCP客户端 */
	lwipdev.udp_reset = 0;
	lwipdev.udp_status = LWIP_UDP_INIT_CONNECT;
	memset(&ipcInfo, 0, sizeof(IPC_Info_t));  // 全部参数初始化为0
	onvif_init();
	if(ONVIF_SEARCH_DEBUG)  printf("onvif_udp_start...\n");
	
}
/************************************************************
*
* Function name	: onvif_tcp_stop
* Description	: tcp客户端停止函数
* Parameter		: 
* Return		: 
*	
************************************************************/
void onvif_udp_stop(void)
{
	OS_CPU_SR cpu_sr;
	lwipdev.udp_status = LWIP_UDP_NO_CONNECT;
	OS_ENTER_CRITICAL();		// 关中断
	OSTaskDel(ONVIF_PRIO);	// 删除TCP任务
	OS_EXIT_CRITICAL();			// 开中断
}

/************************************************************
*
* Function name	: onvif_search_timer_function
* Description	: onvif 搜索时间
* Parameter		: 
* Return		: 
*	
************************************************************/
void onvif_search_timer_function(void)
{
	if(ipcInfo.onvif_times != 0)
	{
		ipcInfo.onvif_times--;
		if(ipcInfo.onvif_times == 0)
		{
			ipcInfo.onvif_times = app_get_onvif_time();
			ipcInfo.search_flag |= ONVIF_START|ONVIF_INIT;
			if(ONVIF_SEARCH_DEBUG)  printf("onvif_search_start...\n");
		}
	}
}

/************************************************************
*
* Function name	: onvif_save_ipc_info
* Description	: 保存摄像机信息
* Parameter		: 
* Return		: 
*	
************************************************************/
void onvif_save_ipc_info(char *ip,uint8_t id)  // 保存摄像机信息
{
	uint8_t addr[4] = {0};

	sscanf(ip, "%d.%d.%d.%d",(int*)&addr[0],(int*)&addr[1],(int*)&addr[2],(int*)&addr[3]);

	app_set_camera_num_function(addr, id);
}
/****************************************************************************
* 名    称: onvif_set_search_flag
* 功    能：设置标志位
* 入口参数：flag 标志位
* 返回参数：无
* 说    明：0：禁止   1：允许
****************************************************************************/
void onvif_set_search_flag(uint8_t flag)  // 
{
	ipcInfo.search_flag |= flag;
}
uint8_t onvif_get_search_flag(void)    
{
	return ipcInfo.search_flag;
}

/****************************************************************************
* 名    称: onvif_get_ipc_appoint_ip
* 功    能：指定id的摄像头IP
* 入口参数：buf 数组   id:编号
* 返回参数：无
* 说    明：
****************************************************************************/
void onvif_get_ipc_appoint_ip(char *buf,uint8_t id)    // 指定id的摄像头IP
{
	sprintf(buf,"%d.%d.%d.%d", ipcInfo.ip[id][0],ipcInfo.ip[id][1],ipcInfo.ip[id][2],ipcInfo.ip[id][3]);
//	sprintf(buf,"%s",ipcInfo.ip[id]);

}
/****************************************************************************
* 名    称: onvif_get_ipc_num
* 功    能：获取IPC数量
* 入口参数：
* 返回参数：ipcInfo.ipc_num  数量
* 说    明： 
****************************************************************************/
uint8_t onvif_get_ipc_num(void)    
{
	return ipcInfo.ipc_num;
}

/****************************************************************************
* 名    称: onvif_match_camera_ip
* 功    能：摄像头IP比较,判断是否收到重复的IP地址
* 入口参数：
* 返回参数：ipcInfo.ipc_num  数量
* 说    明： 
****************************************************************************/
int8_t onvif_match_camera_ip(uint8_t *ip)
{
	int8_t  ret   = 0;
	uint8_t index = 0;
		
	for(index = 0; index<10; index++)
	{	
		if(memcmp(ipcInfo.ip[index],ip,4) == 0)
		{
			ret = -1;
			break;
		}
	}
	return ret;
}

/************************************************************
*
* Function name	: onvif_match_ip
* Description	: 判断两个IP是否相同
* Parameter		: 
* Return		: 
*	
************************************************************/
int8_t onvif_match_ip(uint8_t *ip1,uint8_t *ip2)
{
	int8_t  ret   = 0;
	if(memcmp(ip2,ip1,4) == 0)
		ret = -1;

	return ret;
}


