#include "stm32f10x.h"
#include "usart.h"	 		   
#include "uip.h"	    
#include "enc28j60.h"
#include "httpd.h"
#include "tcp_demo.h"

//TCPӦ�ýӿں���(UIP_APPCALL)
//���TCP����(����server��client)��HTTP����


//��ӡ��־��
void uip_log(char *m)
{
	//printf("uIP log:%s\r\n", m);
}
