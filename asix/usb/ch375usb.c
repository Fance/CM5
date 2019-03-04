#include "ch375usb.h"

unsigned char endp_out_addr = 0x05;		  // Change the endpoint to which endpoint you want to send data
unsigned short endp_out_size = 0x200;
unsigned char endp_in_addr = 0x06;		  // Change the endpoint to which endpoint you want to receive data
unsigned char mDeviceOnline = 0;		  // Once a USB device pluging in, mDeviceOnline will be 1


static BOOL1 tog_send, tog_recv;

extern unsigned int far Test[50];

static void toggle_recv( BOOL1 tog );
static void toggle_send( BOOL1 tog );

unsigned char usb_poll(void)
{
  if(CH375_INT_WIRE == 0)
	{
     return TRUE;
	}
  else
	{
     return FALSE;
	}
}


static void Delay10us(void)
{
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
	DELAY_1_US();
}

/* ��ʱ50ms�� ����ȷ */
static void Delay50ms(void)
{
	unsigned char i, j;
	for(i = 0; i < 200; i++)
		for(j = 0; j < 250; j++)
			DELAY_1_US();
}

///* CH375д����˿� */
//void CH375_WR_CMD_PORT(unsigned char cmd)  					// ��CH375������˿�д������,���ڲ�С��4uS
//{
//	Delay10us();
//    CH375_CMD_PORT = cmd;
//	Delay10us();
//	Delay10us();
//}
//
///* CH375д���ݶ˿� */
//void CH375_WR_DAT_PORT(unsigned char dat) 
//{
//	Delay10us();
//	CH375_DAT_PORT = dat;
//	Delay10us();
//	Delay10us();
//}
//
///* CH375�����ݶ˿� */
//unsigned char CH375_RD_DAT_PORT(void)
//{
//	unsigned char ret;
//	Delay10us();
//	ret = CH375_DAT_PORT;
//	return ret;
//}

/* ��CH375�Ķ˵㻺������ȡ���յ������� */
unsigned char	mReadCH375Data( unsigned char *buf )
{
	unsigned char len, i;
	unsigned char *p;
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* ��CH375�Ķ˵㻺������ȡ���ݿ� */
	p = buf;
	len = CH375_RD_DAT_PORT();  /* ���ݳ��� */
	for ( i=0; i<len; i++ ) *p++ = CH375_RD_DAT_PORT( );  /* ������ȡ���� */
	return( len );
}

/* ��CH375�Ķ˵㻺����д��׼�����͵����� */
void	mWriteCH375Data( unsigned char *buf, unsigned char len )
{
	unsigned char i;
	unsigned char *p;
	CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��CH375�Ķ˵㻺����д�����ݿ� */
	p = buf;
	CH375_WR_DAT_PORT( len );  /* ���ݳ��� */
	for ( i=0; i<len; i++ ) CH375_WR_DAT_PORT( *p++ );  /* ����д������ */
}


/* ��Ŀ��USB�豸ִ�п��ƴ���: ��ȡUSB������ */
unsigned char mCtrlGetDescr( unsigned char type)
{
//	mIntStatus = 0;		/* ���ж�״̬ */
	CH375_WR_CMD_PORT( CMD_GET_DESCR);	 /* ���ƴ���-��ȡ������ */
	CH375_WR_DAT_PORT( type);	  /* 0:�豸������, 1:���������� */
//	while( mIntStatus ==0);	   /* �ȴ�������� */
	return( mCH375Interrupt_host( ) );  /* �ȴ�������� */
}

/* ��Ŀ��USB�豸ִ�п��ƴ���: ����USB��ַ */
unsigned char	mCtrlSetAddress( unsigned char addr )
{
//	mIntStatus = 0;  /* ���ж�״̬ */
	unsigned char c;
	CH375_WR_CMD_PORT( CMD_SET_ADDRESS );  /* ���ƴ���-����USB��ַ */
	CH375_WR_DAT_PORT( addr );  /* 1 - 7eh */
//	while ( mIntStatus == 0 );  /* �ȴ�������� */
//	if ( mIntStatus != USB_INT_SUCCESS ) return;  /* ����ʧ�� */
	c = mCH375Interrupt_host();
	if( c!= USB_INT_SUCCESS)
	{
//		gsm_debug( "USB set address failed");
		return c;
	}
//	else
//		gsm_debug("USB SET ADDRESS SUCCESS");
/* ��Ŀ��USB�豸�ĵ�ַ�ɹ��޸ĺ�,Ӧ��ͬ���޸�CH375��USB��ַ,����CH375���޷���Ŀ���豸ͨѶ */
	CH375_WR_CMD_PORT( CMD_SET_USB_ADDR );  /* ����CH375��USB��ַ */
	CH375_WR_DAT_PORT( addr );  /* �޸�CH375��USB�豸�ܹ��������,��������ж�֪ͨ */
	return c;
}

/* ��Ŀ��USB�豸ִ�п��ƴ���: ��������ֵ */
unsigned char	mCtrlSetConfig( unsigned char value )
{
//	mIntStatus = 0;  /* ���ж�״̬ */
	tog_send = tog_recv = 0;
	CH375_WR_CMD_PORT( CMD_SET_CONFIG );  /* ���ƴ���-����USB���� */
	CH375_WR_DAT_PORT( value );
	return( mCH375Interrupt_host( ) );
//	while ( mIntStatus == 0 );  /* �ȴ�������� */
}

unsigned char issue_token( unsigned char endp_and_pid ) {  /* ִ��USB���� */
/* ִ����ɺ�, �������ж�֪ͨ��Ƭ��, �����USB_INT_SUCCESS��˵�������ɹ� */
	CH375_WR_CMD_PORT( CMD_ISSUE_TOKEN );
	CH375_WR_DAT_PORT( endp_and_pid );  /* ��4λĿ�Ķ˵��, ��4λ����PID */
	return( mCH375Interrupt_host() );  /* �ȴ�CH375������� */
}

/* ����ͬ�� */
/* USB������ͬ��ͨ���л�DATA0��DATA1ʵ��: ���豸��, USB��ӡ�������Զ��л�;
   ��������, ������SET_ENDP6��SET_ENDP7�������CH375�л�DATA0��DATA1.
   �����˵ĳ���������Ϊ�豸�˵ĸ����˵�ֱ��ṩһ��ȫ�ֱ���,
   ��ʼֵ��ΪDATA0, ÿִ��һ�γɹ������ȡ��, ÿִ��һ��ʧ��������临λΪDATA1 */

static void toggle_recv( BOOL1 tog ) {  /* ��������ͬ������:0=DATA0,1=DATA1 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP6 );
	CH375_WR_DAT_PORT( tog ? 0xC0 : 0x80 );
	DELAY_1_US();DELAY_1_US();
}

static void toggle_send( BOOL1 tog ) {  /* ��������ͬ������:0=DATA0,1=DATA1 */
	CH375_WR_CMD_PORT( CMD_SET_ENDP7 );
	CH375_WR_DAT_PORT( tog ? 0xC0 : 0x80 );
	DELAY_1_US();DELAY_1_US();
}

unsigned char clr_stall( unsigned char endp_addr ) {  /* USBͨѶʧ�ܺ�,��λ�豸�˵�ָ���˵㵽DATA0 */
	CH375_WR_CMD_PORT( CMD_CLR_STALL );
	CH375_WR_DAT_PORT( endp_addr );
	return( mCH375Interrupt_host() );
}

/* ���ݶ�д, ��Ƭ����дCH375оƬ�е����ݻ����� */

unsigned char rd_usb_data( unsigned char *buf ) {  /* ��CH37X�������ݿ� */
	unsigned char i, len;
	CH375_WR_CMD_PORT( CMD_RD_USB_DATA );  /* ��CH375�Ķ˵㻺������ȡ���յ������� */
	len=CH375_RD_DAT_PORT();  /* �������ݳ��� */
	for ( i=0; i!=len; i++ ) *buf++=CH375_RD_DAT_PORT();
	return( len );
}

void wr_usb_data( unsigned char len, unsigned char *buf ) {  /* ��CH37Xд�����ݿ� */
	CH375_WR_CMD_PORT( CMD_WR_USB_DATA7 );  /* ��CH375�Ķ˵㻺����д��׼�����͵����� */
	CH375_WR_DAT_PORT( len );  /* �������ݳ���, len���ܴ���64 */
	while( len-- ) CH375_WR_DAT_PORT( *buf++ );
}
extern unsigned int far Test[50];
#define	USB_INT_RET_NAK		0x2A		/* 00101010B,����NAK */
unsigned char USB_send_data(unsigned char len, unsigned char *buf)
{
	unsigned char ret;
	unsigned char  s;
//	while( len ) {  /* ����������ݿ��USB��ӡ�� */
//		l = len>endp_out_size?endp_out_size:len;  /* ���η��Ͳ��ܳ����˵�ߴ� */
		wr_usb_data( len, buf );  /* �������ȸ��Ƶ�CH375оƬ�� */
		toggle_send( tog_send );  /* ����ͬ�� */
		s = issue_token( ( endp_out_addr << 4 ) | DEF_USB_PID_OUT );  /* ����CH375������� */
		if ( s==USB_INT_SUCCESS ) {  /* CH375�ɹ��������� */
	//		gsm_debug("Send success!!!");
			Test[15]++;
			tog_send = ~ tog_send;  /* �л�DATA0��DATA1��������ͬ�� */
			//len-=l;  /* ���� */
			//buf+=l;  /* �����ɹ� */
			ret = 1;
		}
		else if ( s==USB_INT_RET_NAK ) {  /* USB��ӡ����æ,���δִ��SET_RETRY������CH375�Զ�����,���Բ��᷵��USB_INT_RET_NAK״̬ */
			/* USB��ӡ����æ,���������Ӧ���Ժ����� */
			/* s=get_port_status( );  ����б�Ҫ,���Լ����ʲôԭ���´�ӡ��æ */
	//		gsm_debug("Send:Device busy");
			ret = 0;
		}
		else {  /* ����ʧ��,��������²���ʧ�� */
	//		gsm_debug("Send failed!");
			Test[17]++;
			clr_stall( endp_out_addr );  /* �����ӡ�������ݽ��ն˵�,���� soft_reset_print() */
/*			soft_reset_print();  ��ӡ�������������,��λ */
			tog_send = 0;  /* ����ʧ�� */
			ret = 0;
//		}
/* ����������ϴ�,���Զ��ڵ���get_port_status()����ӡ��״̬ */
	}
	return ret;
}

unsigned char USB_recv_data( unsigned char *buf)
{
	unsigned char s;
	toggle_recv( tog_recv);
	s = issue_token( (endp_in_addr << 4) | DEF_USB_PID_IN);
	if ( s == USB_INT_SUCCESS)
	{
//		Test[8]++;
	//	gsm_debug("Receive success!!!");
		tog_recv = ~tog_recv;
		return (rd_usb_data(buf));
	}
	else
	{		
//		Test[9]++;
	//	gsm_debug("Receive failed!");
		clr_stall( endp_in_addr | 0x80);
		tog_recv = 0;
		return 0;
	}
//	return (rd_usb_data(buf));
}


void Reset_CH375(void)
{
	CH375_WR_CMD_PORT( CMD_RESET_ALL);

	CH375_WR_CMD_PORT(CMD_CHECK_EXIST);
	CH375_WR_DAT_PORT(0x55);
	
	Test[42] = CH375_RD_DAT_PORT();
}


/* CH375��ʼ���ӳ��� */
unsigned char CH375Host_Init(unsigned char host_mode)
{
	unsigned char c,i;

	CH375_WR_CMD_PORT( CMD_SET_USB_MODE);
	CH375_WR_DAT_PORT(host_mode);
	Delay50ms();

	for( i = 0xff; i!= 0; i--){
	 	c = CH375_RD_DAT_PORT();
		if ( c== CMD_RET_SUCCESS)
		{
			Test[40]++;
			break;
		}
		else
        {
          	Test[41]++;
        }
	}
	if( i!=0) 
	{

		return (USB_INT_SUCCESS);
	}
        else
          return USB_INT_DISK_ERR;
}




//unsigned char CH375_Device_Init(void)
//{
//    unsigned char ret = FALSE;
//	unsigned char i = 0;
//
//	CH375_WR_CMD_PORT(CMD_SET_USB_MODE);				// ����USB����ģʽ, ��Ҫ����
//	CH375_WR_DAT_PORT(1);  								// ����Ϊʹ���ⲿ�̼���USB�豸��ʽ
//
//	
//	for(i = 0; i < 20; i++)   							// �ȴ������ɹ�,ͨ����Ҫ�ȴ�10uS-20uS 
//    {
//		if(CH375_RD_DAT_PORT() == CMD_RET_SUCCESS)
//		{
//			ret = TRUE;
//			break;
//		}
//	}
//	return ret;
//}

/* CH375�жϷ������ʹ�ò�ѯ��ʽ */
unsigned char mCH375Interrupt_host(void) 
{
	unsigned char mIntStatus;
	unsigned int count = 0;
	count = 0;
	while( CH375_INT_WIRE)	  /* ��ѯ�ȴ�CH375��������ж�(INT#�͵�ƽ) */
	{
		count++;
		if(count > 5000)	{ // 50ms
		CH375_WR_CMD_PORT( CMD_ABORT_NAK );
		break; 
		}
		Delay10us();
	//	vTaskDelay(5/ portTICK_RATE_MS);
	}

//	CH375_WR_CMD_PORT( CMD_GET_STATUS );  /* ������������ж�, ��ȡ�ж�״̬ */
	//return( CH375_RD_DAT_PORT() );
	CH375_WR_CMD_PORT(CMD_GET_STATUS);								// ��ȡ�ж�״̬��ȡ���ж����� 
	mIntStatus = CH375_RD_DAT_PORT();  						// ��ȡ�ж�״̬

	if( mIntStatus == USB_INT_DISCONNECT)
	{
		mDeviceOnline = 0;
	}
	else if( mIntStatus == USB_INT_CONNECT)
	{
		mDeviceOnline = 1;
	}
	else
	{
	}
	return mIntStatus;
}



