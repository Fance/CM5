#include	"reg80390.h"
#include   "types.h"
#include 	"stdio.h"

#define 	SERAIL_IO 	TRUE

#if (SERAIL_IO == TRUE)
/* Software UART */

/* software serial port :  2 General IO and TIME0 */
#define 	GSM_ENABLE 	P3_1

#define 	GSM_TXD		P1_1
#define 	GSM_RXD		P3_0

U8_T   bdata rs_BUF;                  //�����ա���ʱ�õ���λ�ݴ�����   `
sbit rs_BUF_bit7 = rs_BUF^7;        //��λ�ݴ��������λ�� 
U8_T   rs_shift_count;                //��λ��������  
U8_T bdata rsFlags;
sbit rs_f_TI        = rsFlags^0;    //0:���ڷ���; 1: һ���ַ����  
sbit rs_f_RI_enable = rsFlags^1;   //0:��ֹ����; 1:�������  
sbit rs_f_TI_enable = rsFlags^2;   //0:��ֹ����; 1:������  



//�ա���һλ���趨ʱ������   
#define rs_FULL_BIT0	 217		// (Fosc/ 2 ) / Baud        1302 ( 9600 )   2604 ( 4800 )
#define rs_FULL_BIT 	(65536 - rs_FULL_BIT0)
#define rs_FULL_BIT_H 	rs_FULL_BIT >> 8        //�ա���һλ���趨ʱ��������λ   
#define rs_FULL_BIT_L 	(rs_FULL_BIT & 0x00FF)  //�ա���һλ���趨ʱ��������λ   
//�����ʼλ��ʱ�������趨ʱ������    
#define rs_TEST0 rs_FULL_BIT0 / 4             //�����ʽϵ�ʱ���Գ��� 3 ����� 2    
#define rs_TEST ((~rs_TEST0))
#define rs_TEST_H rs_TEST >> 8                //��λ  
#define rs_TEST_L rs_TEST & 0x00FF            //��λ  
//������ʼλ���趨ʱ���ܼ���   
#define rs_START_BIT 0xFFFF - 217/*(Fosc/12/Baud)*/ + 0x28
#define rs_START_BIT_H rs_START_BIT >> 8      //������ʼλ���趨ʱ��������λ    
#define rs_START_BIT_L rs_START_BIT & 0x00FF  //������ʼλ���趨ʱ��������λ   
#define rs_RECEIVE_MAX   128                  //�����ճ���  
U8_T rs232buffer[rs_RECEIVE_MAX];          //�ա���������
U16_T ReceivePoint;                       //�������ݴ洢ָ��  


void soft_rs232_interrupt( void );

void soft_rs232_init (void)            //���ڳ�ʼ��  
{
    TR0 = 0;             //ֹͣ��ʱ��  
    TH0 = 0xf8;
    TL0 = 0x06;
	GSM_RXD = 1;
	GSM_TXD = 1;
    TMOD &= 0xf0;		// set timer way
    TMOD |= 0x01;
    PT0 = 1;                        //���ж����ȼ�Ϊ��  
    ET0 = 1;                        //����ʱ���ж� 
	TR0 = 1;             //������ʱ��  
}

U32_T Timer0Counter = 0;

void vTimer0ISR( void ) interrupt 1   
{  // 		U8_T	temp;
		ET0 = 0;
	//	TH0 = 0xf8;
    //	TL0 = 0x06;
	//	Timer0Counter++;
		soft_rs232_interrupt();
		
		ET0 = 1;

	//	P3_1 = ~P3_1;
	/*	if (GSM_RXD == 0 | rs_shift_count > 0)
        { soft_rs232_interrupt(); }
        else
        {
            TH0 = rs_TEST_H;
            TL0 = rs_TEST_L;
        }*/
}
 

void soft_receive_init()               //�����ʼλ  
{
    TR0 = 0;             //ֹͣ��ʱ��  
    TH0 = rs_TEST_H;
    TL0 = rs_TEST_L;
    rs_shift_count = 0;
    TR0 = 1;             //������ʱ��  
}

void soft_receive_enable()             //�������  
{
    rs_f_RI_enable = 1;                //�������  
    rs_f_TI_enable = 0;                //��ֹ����   
    soft_receive_init();               //�����ʼλ, RXD �½��ش��������ֽڹ���.     
}
void soft_send_enable (void)        //������  
{
    TR0 = 0;             //ֹͣ��ʱ��  
    rs_f_TI_enable = 1;                //������  
    rs_f_RI_enable = 0;                //��ֹ����  
    rs_shift_count = 0;                //����λ������  
    rs_f_TI   = 1;                     //����һ���ַ���ϱ�־  
    TR0 = 1;             //������ʱ��
}
void soft_rs232_interrupt( void )
{
    /************************ ���� ****************************/
    if (rs_f_RI_enable == 1)
    {
        if (rs_shift_count == 0)        //��λ������==0, ��ʾ��⵽��ʼλ�����   
        {
            if ( GSM_RXD == 1 )
            {
                soft_receive_enable (); //��ʼλ��, ���¿�ʼ   
            }
            else
            {
                //�´��ж�������λ��ֹͣλ�е�ĳʱ�̷���    
                TL0 += rs_FULL_BIT_L/* + 0x10*/; 
                TH0 = rs_FULL_BIT_H;
                rs_shift_count++;              
                rs_BUF = 0;             //����λ�������   
            }
        }
        else
        {
            TL0 += rs_FULL_BIT_L; //�´��ж�������λ��ֹͣλ�з���    
            TH0 = rs_FULL_BIT_H;
                                       
            rs_shift_count++;           //2--9:����λ 10:ֹͣλ 
                                       
            if ( rs_shift_count == 9)
            {
                rs_BUF = rs_BUF >> 1;   //���յ�8λ   
                rs_BUF_bit7 = GSM_RXD;
                if( ReceivePoint < rs_RECEIVE_MAX)
                {                       //�����յ����ֽ�    
                    rs232buffer[ReceivePoint++] = rs_BUF;
                }
                else
                {
                    rs_f_RI_enable = 0; //��������, ��ֹ����   
                }
            }
            else
            {
                if (rs_shift_count < 9 ) //�յ���������λ 1 -- 7  
                {
                    rs_BUF = rs_BUF >> 1;
                    rs_BUF_bit7 = GSM_RXD;
                }
                else
                {   //�յ�ֹͣλ��������� PC ����������һ����ʼλ    
                    soft_receive_init(); 
                }
            }
        }
        TF0 = 0;                  //�嶨ʱ���жϱ�־   
    }
    else
    {
        /************************ ���� ****************************/  
        if (rs_f_TI_enable == 1)
        {
            TL0 += rs_FULL_BIT_L;//�´��ж�������λ��ĩβʱ��   
            TH0 = rs_FULL_BIT_H;
            rs_shift_count--;          //0:ֹͣλĩβʱ�̵�  
                                       //1:����ֹͣλ  
                                       //2--9:��������λ  
            if (rs_shift_count > 9)    //����״̬  
            {
                rs_shift_count = 9;
                rs_BUF = 0xFF;
            }
            if (rs_shift_count > 1)    //2--9:��������λ  
            {
                ACC = rs_BUF;
                ACC = ACC >> 1;
                GSM_TXD = CY;
                rs_BUF = ACC;
            }
            else
            {
                if (rs_shift_count == 0) //0:ֹͣλĩβʱ�̵�  
                {
                    GSM_TXD = 1;
                    rs_f_TI = 1;       //�ѷ������һ���ֽ�  
                }
                else
                {
                    GSM_TXD = 1;        //1:����ֹͣλ  
                }
            }
        }
    }
}
//����ת����ʱ��Ҫ�ȵ��� soft_send_enable ()  
void rs_send_byte(U8_T SendByte)      //����һ���ֽ�  
{
    while ( rs_f_TI == 0);             //�ȴ��������ǰһ���ֽ�  
    GSM_TXD = 1;
    TL0 = rs_START_BIT_L;        //�´��ж�����ʼλ��ĩβʱ��   
    TH0 = rs_START_BIT_H;
    rs_BUF = SendByte;
    rs_shift_count = 10;
    GSM_TXD = 0;                        //������ʼλ  
    rs_f_TI = 0;                       //���ѷ������һ���ֽڵı�־   
}


void initiate_MCU (void)               //ϵͳ��ʼ��  
{
    soft_rs232_init();                 //���ڳ�ʼ��
    
    EA = 1;                            //���ж�  
}


void DELAY_Us(U16_T loop);
#if 0
void Test_serial(void)
{
//���ȷ��� 128 ���ֽ� 00H--7FH, Ȼ��ȴ� PC �����͵����ݡ����յ� 128
//���ֽں����̽��յ��� 128 �����ݻط��͸� PC ����Ȼ������ȴ���һ��
//���ݿ顣
  
    U8_T i;
    initiate_MCU();                    //ϵͳ��ʼ��  
   soft_send_enable ();               //�����ͣ���ֹ����  
   printf("enter\r\n");
	//while(1)
    for (i=0; i < rs_RECEIVE_MAX; i++ )
    {
	//	printf("send\r\n");
        rs_send_byte(i);
		DELAY_Us(1000);
    }
    while ( rs_f_TI == 0)  ;           // �ȴ����һ���ֽڷ������   
    while(1)
    {
	//	printf("enter\r\n");
        soft_receive_enable ();        //��������ʼ���գ���ֹ����  
        while (ReceivePoint < 20); // �ȴ����ջ�������  
	//	printf("rx0 %s\r\n",rs232buffer);
	//	printf("rx1 %d\r\n",(int)rs232buffer[5]);
        soft_send_enable ();           //�����ͣ���ֹ����  
        for (i=0; i < 20; i++ )
        {
            rs_send_byte(rs232buffer[i]);
        }
        while ( rs_f_TI == 0)  ;       //�ȴ����һ���ֽڷ������*/
        ReceivePoint = 0;
    }
}
#endif
#if 1
const char GSM_Test[] = "AT\r";
void Test_serial(void)
{
	U8_T i;
	initiate_MCU();	 /* initial mcu */
	printf("test");
	/* initial gsm module */
	soft_send_enable();
	while(1)
	{
		/*send "at" */
		printf("at\r");
		rs_send_byte('A');
		rs_send_byte('T');
		rs_send_byte('\r');
		
		/*receive "at" */
		#if 0
		soft_receive_enable ();      
        while (ReceivePoint < 3); 
	//	printf("rx0 %s\r\n",rs232buffer);
		printf("rx1 %c %c %c \r\n",rs232buffer[0],rs232buffer[1],rs232buffer[2]);
     /*   soft_send_enable ();           //�����ͣ���ֹ����  
        for (i=0; i < 5; i++ )
        {
            rs_send_byte(rs232buffer[i]);
        }
        while ( rs_f_TI == 0)  ;       //�ȴ����һ���ֽڷ������*/
       // ReceivePoint = 0;
		#endif
		//DELAY_Us(50000);DELAY_Us(50000);DELAY_Us(50000);DELAY_Us(50000);DELAY_Us(50000);
		DELAY_Us(50000);DELAY_Us(50000);DELAY_Us(50000);DELAY_Us(50000);DELAY_Us(50000);
	}

}

#endif


#else



/**********************************************************/
//   created by chelsea 2009/10/16

void UART0_PutString(char* str)
{
	U8_T i;
	U8_T len = strlen(str);
	for(i = 0;i < len; i++)
	{
		uart0_PutChar(str[i]);
	}
	uart0_PutChar('\0');
}


/*
char* UART0_GetString(void)
{
	
	U8_T i = 0;
	for(i = 0;i < 255;i++)
	{
	//	printf("str %d %c\r\n",(int)i,uart0_GetKey());
		if(uart0_GetKey() != '\0')
			str[i] = uart0_GetKey();
		else break;
	}
	str[++i] = '\0';
	printf("str %s\r\n",str);
	return str;
}*/


#endif 
