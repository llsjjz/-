#include "all.h"

//CNBR:1234:200000000222


uint8_t Data[8][23];//八个数据
uint8_t Receive_Data[23]; //接收数据
uint8_t Receive_Data_s[23]={'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','\0'};//初步判断

uint8_t LCD_PM=0;//屏幕
uint8_t CNBR=0,VNBR=0,IDLE=8;//车位
double CNBR_F=3.50,VNBR_F=2.00;//单价

//**********************************************LED*******************************************************//
uint8_t LED1_mode=1,LED2_mode=1;//LED状态

void LED_Show(uint8_t led,uint8_t mode)
{
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_8<<(led-1),(GPIO_PinState)mode);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}

void LED_scan(void)
{
	LED_Show(1,LED1_mode);
	LED_Show(2,LED2_mode);
	
	if(IDLE!=0)LED1_mode=0;
	else LED1_mode=1;
	
	if(TIM17->CCR1*100/(TIM17->ARR+1)!=0)LED2_mode=0;
	else LED2_mode=1;
}
//**********************************************KEY*******************************************************//
uint8_t B1_state=0,B2_state=0,B3_state=0,B4_state=0;
uint8_t B1_restate=0,B2_restate=0,B3_restate=0,B4_restate=0;
void KEY_scan(void)
{
	B1_state=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	B2_state=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	B3_state=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	B4_state=HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	
	if(B1_state==0 && B1_restate==1)
	{
	
		LCD_PM++;
		LCD_PM=LCD_PM%2;
	}
	if(B2_state==0 && B2_restate==1)
	{
		if (LCD_PM==1) CNBR_F+=0.5,VNBR_F+=0.5;
	}
	if(B3_state==0 && B3_restate==1)
	{
		if (LCD_PM==1) CNBR_F-=0.5,VNBR_F-=0.5;
	}
	if(B4_state==0 && B4_restate==1)
	{
		if(TIM17->CCR1*100/(TIM17->ARR+1)==0)TIM17->CCR1=10;
		else TIM17->CCR1=0;
	}
	
	B1_restate=B1_state;
	B2_restate=B2_state;
	B3_restate=B3_state;
	B4_restate=B4_state;
}

int8_t JUDEG_num=0,JUDEG_state=0;
//**********************************************USART*******************************************************//
uint8_t Receive=0; //接收
uint8_t Receive_state=0;//接收下标
uint8_t Receive_flag=0;//是否接收结束

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	Receive_flag=1;
	TIM2->CNT=0;
	HAL_UART_Receive_DMA(&huart1,&Receive,1);
	if(Receive_state < 22)
	{
		Receive_Data[Receive_state]=Receive;
	}
	else
	{
		Receive_Data[22]=Receive;
	}
	Receive_state++;
}

void USART_scan(void)
{
	if (Receive_flag==1)
	{
		if(TIM2->CNT > 15)
		{
			for(;Receive_state < 23; Receive_state++)Receive_Data[Receive_state]=' ';
			if(Receive_Data[22] == ' ' && Receive_Data[21] != ' ')
			{
				if(Receive_Data[4]==':' && Receive_Data[9]==':')
				{
					if((Receive_Data[0]=='C' || Receive_Data[0]=='V') && Receive_Data[1]=='N' && Receive_Data[2]=='B' && Receive_Data[3]=='R')
					{
						uint8_t x=0;
						for(uint8_t i = 10;i <22;i++)
						{
							for(uint8_t j=0;j<10;j++)
							{
								if(Receive_Data[i] == '0'+j)
								{
									x++;
								}
							}		
						}
						if(x==12)
						{
							for(uint8_t i=0;i<22;i++)Receive_Data_s[i]=Receive_Data[i];
						}
						else JUDEG_state=-1;

					}
					else JUDEG_state=-1;
				}
				else JUDEG_state=-1;
			}
			else JUDEG_state=-1;
			for(uint8_t i=0;i < 23; i++)Receive_Data[i]=' ';
			Receive_state=0;
			Receive_flag=0;
			
			JUDEG();
		}
	}
}


//**********************************************JUDEG*******************************************************//
uint16_t year[2],month[2],day[2],h[2],m[2],s[2];
char JUDEG_1[4];
char JUDEG_2[4];
uint64_t JUDEG_3;

void JUDEG(void)
{
	sscanf((char *)Receive_Data_s,"%c%c%c%c:%c%c%c%c:%lld",&JUDEG_1[0],&JUDEG_1[1],&JUDEG_1[2],&JUDEG_1[3],&JUDEG_2[0],&JUDEG_2[1],&JUDEG_2[2],&JUDEG_2[3],&JUDEG_3);
	year[0]=JUDEG_3/10000000000,month[0]=(JUDEG_3/100000000)%100,day[0]=(JUDEG_3/1000000)%100,h[0]=(JUDEG_3/10000)%100,m[0]=(JUDEG_3/100)%100,s[0]=JUDEG_3%100;
	
	if(month[0]>12 || day[0]>31 || h[0] > 23 || m[0] > 59 || s[0] > 59)JUDEG_state=-1;
	
	if(JUDEG_state != -1)
	{
		//相同
		uint8_t same_state=0;
		for(JUDEG_num=0;JUDEG_num<8;JUDEG_num++)
		{
			for(uint8_t i=0;i<4;i++)
			{
				if(JUDEG_2[i] != Data[JUDEG_num][5+i])
				{
					same_state=0;
					break;
				}
				else same_state=1;
			}
			if(same_state == 1)break;
		}
		
		//空位
		uint8_t space_state=0;
		if(same_state==0)
		{
			for(JUDEG_num=0;JUDEG_num<8;JUDEG_num++)
			{
				if(Data[JUDEG_num][0] == 'C' || Data[JUDEG_num][0] == 'V')space_state=0;
				else
				{
					space_state=1;
					break;
				}
			}
		}
		
		//有相同
		if(same_state == 1)
		{
			sscanf((char *)Data[JUDEG_num],"%c%c%c%c:%c%c%c%c:%lld",&JUDEG_1[0],&JUDEG_1[1],&JUDEG_1[2],&JUDEG_1[3],&JUDEG_2[0],&JUDEG_2[1],&JUDEG_2[2],&JUDEG_2[3],&JUDEG_3);
			year[1]=JUDEG_3/10000000000,month[1]=(JUDEG_3/100000000)%100,day[1]=(JUDEG_3/1000000)%100,h[1]=(JUDEG_3/10000)%100,m[1]=(JUDEG_3/100)%100,s[1]=JUDEG_3%100;
			
			struct tm now={s[0],m[0],h[0],day[0],month[0]-1,year[0]+2000-1900,0,0,0};
			struct tm last={s[1],m[1],h[1],day[1],month[1]-1,year[1]+2000-1900,0,0,0};
			
			time_t now_s=mktime(&now);
			time_t last_s=mktime(&last);
			
			int64_t s=(int64_t)difftime(now_s,last_s);
			
			if(s <= 0)
			{
				JUDEG_state = -1;
			}
			else 
			{
				for(uint8_t i=0;i<22;i++)Data[JUDEG_num][i]=' ';
				uint32_t h;
				if(s%3600 != 0)h=s/3600+1;
				else h=s/3600;
				
				float change=0;
				if(JUDEG_1[0]=='V')change=VNBR_F;
				else if(JUDEG_1[0]=='C')change=CNBR_F;
				
				
				printf("%s:%s:%d:%.2f",JUDEG_1,JUDEG_2,h,(h*change));
//				char *Transmit;
//				sprintf(Transmit,"%s:%d:%.2f",JUDEG_1,h,(h*change));
//				HAL_UART_Transmit_IT(&huart1,(uint8_t *)Transmit,strlen(Transmit));
			}
		}
		else if(space_state==1)
		{
			for(uint8_t i=0;i<23;i++)Data[JUDEG_num][i]=Receive_Data_s[i];
		}
		else JUDEG_state=-1;
	}
	
	
	
	if(JUDEG_state == -1)
	{
		char *Error="Error";
		HAL_UART_Transmit_IT(&huart1,(uint8_t *)Error,5);
	}
	
	JUDEG_state =0;
	for(uint8_t i=0;i < 22; i++)Receive_Data_s[i]=' ';
	for(uint8_t i=0;i<2;i++)
	{
		year[i]=0,month[i]=0,day[i]=0,h[i]=0,m[i]=0,s[i]=0;
	}
	
	CNBR=0,VNBR=0,IDLE=8;
	for(uint8_t i=0;i<8;i++)
	{
		if(Data[i][0]=='C')CNBR++;
		else if(Data[i][0]=='V')VNBR++;
	}
	IDLE=8-CNBR-VNBR;
}
//**********************************************LCD*******************************************************//
void LCD_scan(void)
{
	char text[20];
	switch (LCD_PM)
	{
		case 0:
		{
			LCD_DisplayStringLine(Line1,(uint8_t *)"       Data         ");
			
			sprintf(text,"   CNBR:%d           ",CNBR);
			LCD_DisplayStringLine(Line3,(uint8_t *)text);
			
			sprintf(text,"   VNBR:%d          ",VNBR);
			LCD_DisplayStringLine(Line5,(uint8_t *)text);
			
			sprintf(text,"   IDLE:%d          ",IDLE);
			LCD_DisplayStringLine(Line7,(uint8_t *)text);
			
			break;
		}
		
		case 1:
		{
			LCD_DisplayStringLine(Line1,(uint8_t *)"       Para        ");
			
			sprintf(text,"   CNBR:%0.2f       ",CNBR_F);
			LCD_DisplayStringLine(Line3,(uint8_t *)text);
			
			sprintf(text,"   VNBR:%0.2f       ",VNBR_F);
			LCD_DisplayStringLine(Line5,(uint8_t *)text);
			
			LCD_DisplayStringLine(Line7,(uint8_t *)"                   ");
			
			break;
		}
	}
}






