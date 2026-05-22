//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã���������������ҵ��;
/****************************************************************************************************
//=========================================��Դ����================================================//
//     LCDģ��                STM32��Ƭ��
//      VCC          ��        DC5V/3.3V      //��Դ
//      GND          ��          GND          //��Դ��
//=======================================Һ���������߽���==========================================//
//��ģ��Ĭ��������������ΪSPI����
//     LCDģ��                STM32��Ƭ��    
//    SDI(MOSI)      ��          PA7         //Һ����SPI��������д�ź�
//    SDO(MISO)      ��          PA6         //Һ����SPI�������ݶ��źţ��������Ҫ�������Բ�����
//=======================================Һ���������߽���==========================================//
//     LCDģ�� 					      STM32��Ƭ�� 
//       LED         ��          PB6         //Һ������������źţ��������Ҫ���ƣ���5V��3.3V
//       SCK         ��          PA5         //Һ����SPI����ʱ���ź�
//      DC/RS        ��          PB7         //Һ��������/��������ź�
//       RST         ��          PB8         //Һ������λ�����ź�
//       CS          ��          PB5         //Һ����Ƭѡ�����ź�
//=========================================������������=========================================//
//���ģ�鲻���������ܻ��ߴ��д������ܣ����ǲ���Ҫ�������ܣ�����Ҫ���д���������
//	   LCDģ��                STM32��Ƭ�� 
//      T_IRQ        ��          PA0         //�����������ж��ź�
//      T_DO         ��          PA1         //������SPI���߶��ź�
//      T_DIN        ��          PB3         //������SPI����д�ź�
//      T_CS         ��          PB4        //������Ƭѡ�����ź�
//      T_CLK        ��          PA8         //������SPI����ʱ���ź�
**************************************************************************************************/	

#include "touch.h" 
#include "lcd.h"
#include "delay.h"
#include "stdlib.h"
#include "math.h"
#include "flash.h"
#include "gui.h"	    

_m_tp_dev tp_dev=
{
	TP_Init,
	TP_Scan,
	TP_Adjust,
	0,
	0,
 	0,
	0,
	0,
	0,
	0,
	0,	  	 		
	0,
	0,	  	 		
};					

//Ĭ��Ϊtouchtype=0������.
u8 CMD_RDX=0xD0;
u8 CMD_RDY=0x90;

static void TP_DIN_Write(u8 state)
{
	HAL_GPIO_WritePin(TP_DIN_GPIO_Port, TP_DIN_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void TP_CLK_Write(u8 state)
{
	HAL_GPIO_WritePin(TP_CLK_GPIO_Port, TP_CLK_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void TP_CS_Write(u8 state)
{
	HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static u8 TP_DOUT_Read(void)
{
	return (HAL_GPIO_ReadPin(TP_DO_GPIO_Port, TP_DO_Pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static u8 TP_PEN_IsPressed(void)
{
	return (HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

static void TP_GPIO_InitPins(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
	__HAL_AFIO_REMAP_SWJ_NOJTAG();

	TP_CS_Write(1);
	TP_DIN_Write(1);
	TP_CLK_Write(0);

	GPIO_InitStruct.Pin = TP_IRQ_Pin | TP_DO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = TP_DIN_Pin | TP_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = TP_CLK_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(TP_CLK_GPIO_Port, &GPIO_InitStruct);
}

/*****************************************************************************
 * @name       :void TP_Write_Byte(u8 num)   
 * @date       :2018-08-09 
 * @function   :Write a byte data to the touch screen IC with SPI bus
 * @parameters :num:Data to be written
 * @retvalue   :None
******************************************************************************/  	 			    					   
void TP_Write_Byte(u8 num)    
{  
	u8 count=0; 
  TP_CLK_Write(0);
  TP_CS_Write(0);
	for(count=0;count<8;count++)  
	{ 	  
		TP_DIN_Write((num & 0x80U) ? 1U : 0U);
		num<<=1;    
		TP_CLK_Write(0);
    delay_us(1);		
		TP_CLK_Write(1);		//��������Ч
    delay_us(1);		
	}		 			    
}

/*****************************************************************************
 * @name       :u16 TP_Read_AD(u8 CMD)	  
 * @date       :2018-08-09 
 * @function   :Reading adc values from touch screen IC with SPI bus
 * @parameters :CMD:Read command,0xD0 for x,0x90 for y
 * @retvalue   :Read data
******************************************************************************/    
u16 TP_Read_AD(u8 CMD)	  //�յ�0xfff������Ϣ
{ 	 
	u8 count=0; 	  
	u16 Num=0; 
	TP_CLK_Write(0);		//������ʱ��
	TP_DIN_Write(0); 	//����������
	TP_CS_Write(0); 		//ѡ�д�����IC
	TP_Write_Byte(CMD);//����������
		
   delay_us(6);//XPT2046��ת��ʱ���Ϊ6us		     	      	   
	 //TCLK=1;		//BUSYλ���ڶ�ȡ���ݵĵ�һ��ʱ�������ر������	    	    
	 //delay_us(1); 
	 //TCLK=0;	
	 //delay_us(1); 
	  
	for(count=0;count<12;count++)//����16λ����,ֻ�и�12λ��Ч 
	{ 				  
		Num<<=1; 
		TP_CLK_Write(1);
	  delay_us(1);  
		TP_CLK_Write(0);
    delay_us(1);
			
		if(TP_DOUT_Read())Num|=1;
	  
	} 
//	printf("TOUCH_DADA CMD :%X\r\n", CMD);
//	printf("TOUCH X_DADA REC :%X\r\n", Num);	

	//Num>>=4;   	//ֻ�и�12λ��Ч.
	TP_CS_Write(1);		//�ͷ�Ƭѡ
	TP_CLK_Write(0);		//������ʱ��
	TP_DIN_Write(0); 	//����������
	
	
	
	return(Num);  
//#endif
}

#define READ_TIMES 5 	//��ȡ����
#define LOST_VAL 1	  	//����ֵ
/*****************************************************************************
 * @name       :u16 TP_Read_XOY(u8 xy)  
 * @date       :2018-08-09 
 * @function   :Read the touch screen coordinates (x or y),
								Read the READ_TIMES secondary data in succession 
								and sort the data in ascending order,
								Then remove the lowest and highest number of LOST_VAL 
								and take the average
 * @parameters :xy:Read command(CMD_RDX/CMD_RDY)
 * @retvalue   :Read data
******************************************************************************/  
u16 TP_Read_XOY(u8 xy)
{
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;
	u16 temp;
	for(i=0;i<READ_TIMES;i++){buf[i]=TP_Read_AD(xy);		delay_us(2);} 		    
	for(i=0;i<READ_TIMES-1; i++)//����
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//��������
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 

/*****************************************************************************
 * @name       :u8 TP_Read_XY(u16 *x,u16 *y)
 * @date       :2018-08-09 
 * @function   :Read touch screen x and y coordinates,
								The minimum value can not be less than 100
 * @parameters :x:Read x coordinate of the touch screen
								y:Read y coordinate of the touch screen
 * @retvalue   :0-fail,1-success
******************************************************************************/ 
u8 TP_Read_XY(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;			 	 		  
	xtemp=TP_Read_XOY(CMD_RDX);
	//���ڵ���
//	printf("TOUCH X_DADA CMD :%X\r\n", CMD_RDX);//
//	printf("TOUCH X_DADA REC :%d\r\n", xtemp);//ÿ��������ʾ5��
	
	
  ytemp=TP_Read_XOY(CMD_RDY);	
//���ڵ���
//	printf("TOUCH Y_DADA CMD :%X\r\n", CMD_RDY);//
//	printf("TOUCH Y_DADA REC :%d\r\n", ytemp);  
	
	//if(xtemp<100||ytemp<100)return 0;//����ʧ��
	*x=xtemp;
	*y=ytemp;
	
	return 1;//�����ɹ�
}

#define ERR_RANGE 50 //��Χ 
/*****************************************************************************
 * @name       :u8 TP_Read_XY2(u16 *x,u16 *y) 
 * @date       :2018-08-09 
 * @function   :Read the touch screen coordinates twice in a row, 
								and the deviation of these two times can not exceed ERR_RANGE, 
								satisfy the condition, then think the reading is correct, 
								otherwise the reading is wrong.
								This function can greatly improve the accuracy.
 * @parameters :x:Read x coordinate of the touch screen
								y:Read y coordinate of the touch screen
 * @retvalue   :0-fail,1-success
******************************************************************************/ 
u8 TP_Read_XY2(u16 *x,u16 *y) 
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
    flag=TP_Read_XY(&x1,&y1);   
    if(flag==0)return(0);
    flag=TP_Read_XY(&x2,&y2);	   
    if(flag==0)return(0);   
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//ǰ�����β�����+-50��
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;	  
} 

/*****************************************************************************
 * @name       :void TP_Drow_Touch_Point(u16 x,u16 y,u16 color)
 * @date       :2018-08-09 
 * @function   :Draw a touch point,Used to calibrate							
 * @parameters :x:Read x coordinate of the touch screen
								y:Read y coordinate of the touch screen
								color:the color value of the touch point
 * @retvalue   :None
******************************************************************************/  
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color)
{
	POINT_COLOR=color;
	LCD_DrawLine(x-12,y,x+13,y);//����
	LCD_DrawLine(x,y-12,x,y+13);//����
	LCD_DrawPoint(x+1,y+1);
	LCD_DrawPoint(x-1,y+1);
	LCD_DrawPoint(x+1,y-1);
	LCD_DrawPoint(x-1,y-1);
	gui_circle(x,y,POINT_COLOR,6,0);//������Ȧ
}	

/*****************************************************************************
 * @name       :void TP_Draw_Big_Point(u16 x,u16 y,u16 color)
 * @date       :2018-08-09 
 * @function   :Draw a big point(2*2)					
 * @parameters :x:Read x coordinate of the point
								y:Read y coordinate of the point
								color:the color value of the point
 * @retvalue   :None
******************************************************************************/   
void TP_Draw_Big_Point(u16 x,u16 y,u16 color)
{	    
	POINT_COLOR=color;
	LCD_DrawPoint(x,y);//���ĵ� 
	LCD_DrawPoint(x+1,y);
	LCD_DrawPoint(x,y+1);
	LCD_DrawPoint(x+1,y+1);	 	  	
}	

/*****************************************************************************
 * @name       :u8 TP_Scan(u8 tp)
 * @date       :2018-08-09 
 * @function   :Scanning touch event				
 * @parameters :tp:0-screen coordinate 
									 1-Physical coordinates(For special occasions such as calibration)
 * @retvalue   :Current touch screen status,
								0-no touch
								1-touch
******************************************************************************/  					  
u8 TP_Scan(u8 tp)
{			   
	if(TP_PEN_IsPressed())//�а�������
	{
		if(tp){TP_Read_XY2(&tp_dev.x,&tp_dev.y);//��ȡ��������
		}
		
		else if(TP_Read_XY2(&tp_dev.x,&tp_dev.y))//��ȡ��Ļ����
		{
	 		tp_dev.x=tp_dev.xfac*tp_dev.x+tp_dev.xoff;//�����ת��Ϊ��Ļ����
			tp_dev.y=tp_dev.yfac*tp_dev.y+tp_dev.yoff;  
			
	 	} 
		if((tp_dev.sta&TP_PRES_DOWN)==0)//֮ǰû�б�����
		{		 
			tp_dev.sta=TP_PRES_DOWN|TP_CATH_PRES;//��������  
			tp_dev.x0=tp_dev.x;//��¼��һ�ΰ���ʱ������
			tp_dev.y0=tp_dev.y;  	  
		}			   
	}else
	{
		
		if(tp_dev.sta&TP_PRES_DOWN)//֮ǰ�Ǳ����µ�
		{
			tp_dev.sta&=~(1<<7);//��ǰ����ɿ�	
		}else//֮ǰ��û�б�����
		{
			tp_dev.x0=0;
			tp_dev.y0=0;
			tp_dev.x=0xffff;
			tp_dev.y=0xffff;
		}	    
	}
	return tp_dev.sta&TP_PRES_DOWN;//���ص�ǰ�Ĵ���״̬
}
	  
//////////////////////////////////////////////////////////////////////////	 
//������EEPROM����ĵ�ַ�����ַ,ռ��13���ֽ�(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+12)
//#define SAVE_ADDR_BASE 40
/*****************************************************************************
 * @name       :void TP_Save_Adjdata(void)
 * @date       :2018-08-09 
 * @function   :Save calibration parameters		
 * @parameters :None
 * @retvalue   :None
******************************************************************************/ 										    
void TP_Save_Adjdata(void)
{
	s32 data;
	u16 temp[2]={0};
	
	//����У�����!		
	data=tp_dev.xfac*100000000;
	temp[0]=(u16)(data>>16);//����xУ������
	temp[1]=(u16)data;
	FLASH_Write(TOUCH_ADDRESS,temp,2);	
//	AT24CXX_WriteLenByte(SAVE_ADDR_BASE,temp,4);   
	data=tp_dev.yfac*100000000;
	temp[0]=(u16)(data>>16);//����yУ������  
	temp[1]=(u16) data;
	FLASH_Write(TOUCH_ADDRESS+4,temp,2);
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE+4,temp,4);
	//����xƫ����
		FLASH_Write(TOUCH_ADDRESS+8,(u16*)&tp_dev.xoff,1);
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE+8,tp_dev.xoff,2);		    
	//����yƫ����
	FLASH_Write(TOUCH_ADDRESS+10,(u16*)&tp_dev.yoff,1);
//	AT24CXX_WriteLenByte(SAVE_ADDR_BASE+10,tp_dev.yoff,2);	
	//���津������
	FLASH_Write(TOUCH_ADDRESS+12,(u16*)&tp_dev.touchtype,1);
//	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+12,tp_dev.touchtype);	
	temp[0]=0X0A;//���У׼����
	FLASH_Write(TOUCH_ADDRESS+14,&temp[0],1);
//	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+13,temp); 
}

/*****************************************************************************
 * @name       :u8 TP_Get_Adjdata(void)
 * @date       :2018-08-09 
 * @function   :Gets the calibration values stored in the EEPROM		
 * @parameters :None
 * @retvalue   :1-get the calibration values successfully
								0-get the calibration values unsuccessfully and Need to recalibrate
******************************************************************************/ 	
u8 TP_Get_Adjdata(void)
{					  
	u16 tempfac[2]={0};
	s32 data;
	FLASH_Read(TOUCH_ADDRESS+14,&tempfac[0],1); 
//	tempfac=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+13);//��ȡ�����,���Ƿ�У׼���� 		 
	if(tempfac[0]==0X0A)//�������Ѿ�У׼����			   
	{    	
		FLASH_Read(TOUCH_ADDRESS,tempfac,2);
//		tempfac=AT24CXX_ReadLenByte(SAVE_ADDR_BASE,4);		   
		data=(s32)(((u32)tempfac[0]<<16)|tempfac[1]);
		tp_dev.xfac=(float)data/100000000;//�õ�xУ׼����
		FLASH_Read(TOUCH_ADDRESS+4,tempfac,2);
//		tempfac=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+4,4);			          
		data=(s32)(((u32)tempfac[0]<<16)|tempfac[1]);
		tp_dev.yfac=(float)data/100000000;//�õ�yУ׼����
	    //�õ�xƫ����
		FLASH_Read(TOUCH_ADDRESS+8,&tempfac[0],1);
		tp_dev.xoff=(short)tempfac[0];
//		tp_dev.xoff=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+8,2);			   	  
 	    //�õ�yƫ����
		FLASH_Read(TOUCH_ADDRESS+10,&tempfac[0],1);
		tp_dev.yoff=(short)tempfac[0];
//		tp_dev.yoff=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+10,2);	
		FLASH_Read(TOUCH_ADDRESS+12,&tempfac[0],1);
		tp_dev.touchtype=(u8)tempfac[0];
// 		tp_dev.touchtype=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+12);//��ȡ�������ͱ��
		if(tp_dev.touchtype>1||fabsf(tp_dev.xfac)<0.001f||fabsf(tp_dev.yfac)<0.001f||fabsf(tp_dev.xfac)>2.0f||fabsf(tp_dev.yfac)>2.0f)
		{
			tp_dev.xfac=0;
			tp_dev.yfac=0;
			tp_dev.xoff=0;
			tp_dev.yoff=0;
			tp_dev.touchtype=0;
			CMD_RDX=0XD0;
			CMD_RDY=0X90;
			return 0;
		}
	  if(tp_dev.touchtype)//X,Y��������Ļ�෴
		{
			CMD_RDX=0X90;
			CMD_RDY=0XD0;	 
		}else				   //X,Y��������Ļ��ͬ
		{
			CMD_RDX=0XD0;
			CMD_RDY=0X90;	 
		}		
   
		return 1;	 
		
	}
	
	return 0;
}	
 
//��ʾ�ַ���
const u8* TP_REMIND_MSG_TBL=(const u8*)"Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";

/*****************************************************************************
 * @name       :void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
 * @date       :2018-08-09 
 * @function   :Display calibration results	
 * @parameters :x0:the x coordinates of first calibration point
								y0:the y coordinates of first calibration point
								x1:the x coordinates of second calibration point
								y1:the y coordinates of second calibration point
								x2:the x coordinates of third calibration point
								y2:the y coordinates of third calibration point
								x3:the x coordinates of fourth calibration point
								y3:the y coordinates of fourth calibration point
								fac:calibration factor 
 * @retvalue   :None
******************************************************************************/ 	 					  
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
{	  
	POINT_COLOR=RED;
	LCD_ShowString(40,160,RED,BLUE,16,(u8 *)"x1:",1);
 	LCD_ShowString(40+80,160,RED,BLUE,16,(u8 *)"y1:",1);
 	LCD_ShowString(40,180,RED,BLUE,16,(u8 *)"x2:",1);
 	LCD_ShowString(40+80,180,RED,BLUE, 16,(u8 *)"y2:",1);
	LCD_ShowString(40,200,RED,BLUE,16,(u8 *)"x3:",1);
 	LCD_ShowString(40+80,200,RED,BLUE,16,(u8 *)"y3:",1);
	LCD_ShowString(40,220,RED,BLUE,16,(u8 *)"x4:",1);
 	LCD_ShowString(40+80,220,RED,BLUE,16,(u8 *)"y4:",1);  
 	LCD_ShowString(40,240,RED,BLUE,16,(u8 *)"fac is:",1);     
	LCD_ShowNum(40+24,160,RED,BLUE,x0,4,0,16,1);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,160,RED,BLUE,y0,4,0,16,1);	//��ʾ��ֵ
	LCD_ShowNum(40+24,180,RED,BLUE,x1,4,0,16,1);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,180,RED,BLUE,y1,4,0,16,1);	//��ʾ��ֵ
	LCD_ShowNum(40+24,200,RED,BLUE,x2,4,0,16,1);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,200,RED,BLUE,y2,4,0,16,1);	//��ʾ��ֵ
	LCD_ShowNum(40+24,220,RED,BLUE,x3,4,0,16,1);		//��ʾ��ֵ
	LCD_ShowNum(40+24+80,220,RED,BLUE,y3,4,0,16,1);	//��ʾ��ֵ
 	LCD_ShowNum(40+56,lcddev.width,RED,BLUE,fac,3,0,16,1); 	//��ʾ��ֵ,����ֵ������95~105��Χ֮��.
}

/*****************************************************************************
 * @name       :u8 TP_Get_Adjdata(void)
 * @date       :2018-08-09 
 * @function   :Calibration touch screen and Get 4 calibration parameters
 * @parameters :None
 * @retvalue   :None
******************************************************************************/ 		 
void TP_Adjust(void)
{								 
	u16 pos_temp[4][2];//���껺��ֵ
	u8  cnt=0;	
	u16 d1,d2;
	u32 tem1,tem2;
	float fac; 	
	u16 outtime=0;
 	cnt=0;				
	POINT_COLOR=BLUE;
	BACK_COLOR =WHITE;
	LCD_Clear(WHITE);//����   
	POINT_COLOR=RED;//��ɫ 
	LCD_Clear(WHITE);//���� 	   
	POINT_COLOR=BLACK;
	LCD_ShowString(10,40,RED,BLUE,16,(u8 *)"Please use the stylus click the",1);//��ʾ��ʾ��Ϣ
	LCD_ShowString(10,56,RED,BLUE,16,(u8 *)"cross on the screen.The cross will",1);//��ʾ��ʾ��Ϣ
	LCD_ShowString(10,72,RED,BLUE,16,(u8 *)"always move until the screen ",1);//��ʾ��ʾ��Ϣ
	LCD_ShowString(10,88,RED,BLUE,16,(u8 *)"adjustment is completed.",1);//��ʾ��ʾ��Ϣ

	TP_Drow_Touch_Point(20,20,RED);//����1 
	tp_dev.sta=0;//���������ź� 
	tp_dev.xfac=0;//xfac��������Ƿ�У׼��,����У׼֮ǰ�������!�������	 
	while(1)//�������10����û�а���,���Զ��˳�
	{
		tp_dev.scan(1);//ɨ����������
		if((tp_dev.sta&0xc0)==TP_CATH_PRES)//����������һ��(��ʱ�����ɿ���.)
		{	
			outtime=0;		
			tp_dev.sta&=~(1<<6);//��ǰ����Ѿ�����������.
						   			   
			pos_temp[cnt][0]=tp_dev.x;
			pos_temp[cnt][1]=tp_dev.y;
			cnt++;	  
			switch(cnt)
			{			   
				case 1:						 
					TP_Drow_Touch_Point(20,20,WHITE);				//�����1 
					TP_Drow_Touch_Point(lcddev.width-20,20,RED);	//����2
					break;
				case 2:
 					TP_Drow_Touch_Point(lcddev.width-20,20,WHITE);	//�����2
					TP_Drow_Touch_Point(20,lcddev.height-20,RED);	//����3
					break;
				case 3:
 					TP_Drow_Touch_Point(20,lcddev.height-20,WHITE);			//�����3
 					TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,RED);	//����4
					break;
				case 4:	 //ȫ���ĸ����Ѿ��õ�
	    		    //�Ա����
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//�õ�1,2�ľ���
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//�õ�3,4�ľ���
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05||d1==0||d2==0)//���ϸ�
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
 							continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//�õ�1,3�ľ���
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//�õ�2,4�ľ���
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//���ϸ�
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
						continue;
					}//��ȷ��
								   
					//�Խ������
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//�õ�1,4�ľ���
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//�õ�2,3�ľ���
					fac=(float)d1/d2;
					if(fac<0.95||fac>1.05)//���ϸ�
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
 						TP_Adj_Info_Show(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//��ʾ����   
						continue;
					}//��ȷ��
					//������
					tp_dev.xfac=(float)(lcddev.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//�õ�xfac		 
					tp_dev.xoff=(lcddev.width-tp_dev.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//�õ�xoff
						  
					tp_dev.yfac=(float)(lcddev.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//�õ�yfac
					tp_dev.yoff=(lcddev.height-tp_dev.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//�õ�yoff  
					if(fabsf(tp_dev.xfac)>2||fabsf(tp_dev.yfac)>2)//������Ԥ����෴��.
					{
						cnt=0;
 				    	TP_Drow_Touch_Point(lcddev.width-20,lcddev.height-20,WHITE);	//�����4
   	 					TP_Drow_Touch_Point(20,20,RED);								//����1
						LCD_ShowString(40,26,RED,BLUE,16,(u8 *)"TP Need readjust!",1);
						tp_dev.touchtype=!tp_dev.touchtype;//�޸Ĵ�������.
	     		if(tp_dev.touchtype)//X,Y��������Ļ�෴
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else				   //X,Y��������Ļ��ͬ
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					POINT_COLOR=BLUE;
					LCD_Clear(WHITE);//����
					LCD_ShowString(35,110,RED,BLUE,16,(u8 *)"Touch Screen Adjust OK!",1);//У�����
					delay_ms(1000);
					TP_Save_Adjdata();  
 					LCD_Clear(WHITE);//����   
					return;//У�����				 
			}
		}
		delay_ms(10);
		outtime++;
		if(outtime>1000)
		{
			TP_Get_Adjdata();
			break;
	 	} 
 	}
}		

/*****************************************************************************
 * @name       :u8 TP_Init(void)
 * @date       :2018-08-09 
 * @function   :Initialization touch screen
 * @parameters :None
 * @retvalue   :0-no calibration
								1-Has been calibrated
******************************************************************************/  
u8 TP_Init(void)
{			    		   
	TP_GPIO_InitPins();
	
  	TP_Read_XY(&tp_dev.x,&tp_dev.y);//��һ�ζ�ȡ��ʼ��	 
	if(TP_Get_Adjdata() == 0)
	{
		LCD_Clear(WHITE);
		TP_Adjust();
		TP_Get_Adjdata();
	}
	return 1; 									 
}



