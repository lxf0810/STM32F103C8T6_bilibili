#include "led.h" 
#include "delay.h"
#include "usart.h"
#include "rtc.h" 		    

/*
功能：设置RTC，进行1秒钟中断，串口打印当前时间
编程：
	1，开启时钟，BKP后备域（backup domain），PWR
	2，RTC CORE BKP 解锁
	3，RTC（后备电源的情况）设置一次进行工作
	后备寄存器1:0x6666
		进行判断RTC是否已经设置过，若没有：
		
		复位备份区域 
		LSE:32768hz开启外部低速时钟
		等待时钟稳定（）
		{
		while(GETFLAG && 时间);//超时时间
			
		}
		if()
		return 
		（1）
		（2）
		配置RTC输入时钟
		使能RTC时钟 
		判断和等待最近一次对RTC寄存器的写操作完成
		使能RTC秒中断
		等待最近一次对RTC寄存器的写操作完成
		
		进入配置模式
		设置分屏
		等待完成
		设置初始时间
		等待完成
		退出配置模式
		NVIC RTC中断
		向指定的后备寄存器1中写入用户程序数据：0x6666
	4，更新并开启
*/
RTC_VARIABLE rtc;//时钟结构体 

//实时时钟配置
//初始化RTC时钟,同时检测时钟是否工作正常
//BKP->DR1用于保存是否第一次配置的设置
//返回0:正常
//其他:错误代码

unsigned char RTC_Init(void)
{
	//检查是不是第一次配置时钟
	unsigned char  temp=0;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟   
	PWR_BackupAccessCmd(ENABLE);	//使能后备寄存器访问  
	if (BKP_ReadBackupRegister(BKP_DR1) != 0x6666)		//从指定的后备寄存器中读出数据:读出了与写入的指定数据不相乎
	{	 			
		BKP_DeInit();	//复位备份区域 	
		RCC_LSEConfig(RCC_LSE_ON);	//设置外部低速晶振(LSE),使用外设低速晶振
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET&&temp<200)	//检查指定的RCC标志位设置与否,等待低速晶振就绪
		{
			temp++;
			delay_ms(10);
		}
		if(temp>=200)
			return 1;//初始化时钟失败,晶振有问题	
		else
		{
			RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);		//设置RTC时钟(RTCCLK),选择LSE作为RTC时钟    
			RCC_RTCCLKCmd(ENABLE);	//使能RTC时钟  
			RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
			RTC_WaitForSynchro();		//等待RTC寄存器同步  
			RTC_ITConfig(RTC_IT_SEC, ENABLE);		//使能RTC秒中断
			RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
			RTC_EnterConfigMode();/// 允许配置	
			RTC_SetPrescaler(32768-1); //设置RTC预分频的值 32768 1hz 1s
			RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
			RTC_Set(2021,7,15,17,42,55);  //设置时间	
			RTC_ExitConfigMode(); //退出配置模式  
			
			BKP_WriteBackupRegister(BKP_DR1, 0x6666);	//向指定的后备寄存器中写入用户程序数据
		}
	}
	else//系统继续计时
	{
		RTC_WaitForSynchro();	//等待最近一次对RTC寄存器的写操作完成
		RTC_ITConfig(RTC_IT_SEC, ENABLE);	//使能RTC秒中断
		RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成
	}

	NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;		//RTC全局中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//先占优先级1位,从优先级3位
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;	//先占优先级0位,从优先级4位
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;		//使能该通道中断
	NVIC_Init(&NVIC_InitStructure);		//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器    				     
	RTC_Get();//更新时间	
	return 0; //ok

}		 				    
//RTC时钟中断
//每秒触发一次  
//extern unsigned int tcnt; 
void RTC_IRQHandler(void)
{		
	if (RTC_GetITStatus(RTC_IT_SEC) != RESET)//秒钟中断
	{							
		RTC_Get();//更新时间   
		printf("Time:%d-%d-%d %d:%d:%d\t\n",rtc.w_year,rtc.w_month,rtc.w_date,rtc.hour,rtc.min,rtc.sec);
 	}
			  								 
	RTC_ClearITPendingBit(RTC_IT_SEC|RTC_IT_OW);		//清闹钟中断
	RTC_WaitForLastTask();	  	    						 	   	 
}

//判断是否是闰年函数
//月份   1  2  3  4  5  6  7  8  9  10 11 12
//闰年   31 29 31 30 31 30 31 31 30 31 30 31
//非闰年 31 28 31 30 31 30 31 31 30 31 30 31
//输入:年份
//输出:该年份是不是闰年.1,是.0,不是
unsigned char  Is_Leap_Year(unsigned int year)
{			  
	if(year%4==0) //必须能被4整除 //for if 例题：判断是否闰年 2，水仙花
	{ 
		if(year%100==0) 
		{ 
			if(year%400==0)return 1;//如果以00结尾,还要能被400整除 	   
			else return 0;   
		}else return 1;   
	}else return 0;	
}

//设置时钟
//把输入的时钟转换为秒钟
//以1970年1月1日为基准
//1970~2099年为合法年份
//返回值:0,成功;其他:错误代码.
//月份数据表											 
unsigned char  const table_week[12]={0,3,3,6,1,4,6,2,5,0,3,5}; //月修正数据表	  
//平年的月份日期表
const unsigned char  mon_table[12]={31,28,31,30,31,30,31,31,30,31,30,31};
unsigned char  RTC_Set(unsigned int syear,unsigned char  smon,unsigned char  sday,unsigned char  hour,unsigned char  min,unsigned char  sec)
{
	//计时的基准：1970 RTC时钟模块
	//1 判断  2 设置数据 3，同步并等待完成
	
	unsigned int t;
	u32 seccount=0;
	if(syear<1970||syear>2099)return 1;	   
	for(t=1970;t<syear;t++)	//把所有年份的秒钟相加
	{
		if(Is_Leap_Year(t))seccount+=31622400;//闰年的秒钟数
		else seccount+=31536000;			  //平年的秒钟数
	}
	smon-=1;
	for(t=0;t<smon;t++)	   //把前面月份的秒钟数相加
	{
		seccount+=(u32)mon_table[t]*86400;//月份秒钟数相加
		if(Is_Leap_Year(syear)&&t==1)seccount+=86400;//闰年2月份增加一天的秒钟数	   
	}
	seccount+=(u32)(sday-1)*86400;//把前面日期的秒钟数相加 
	seccount+=(u32)hour*3600;//小时秒钟数
    seccount+=(u32)min*60;	 //分钟秒钟数
	seccount+=sec;//最后的秒钟加上去

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟  
	PWR_BackupAccessCmd(ENABLE);	//使能RTC和后备寄存器访问 
	RTC_SetCounter(seccount);	//设置RTC计数器的值

	RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成  	
	return 0;	    
}

//初始化闹钟		  
//以1970年1月1日为基准
//1970~2099年为合法年份
//syear,smon,sday,hour,min,sec：闹钟的年月日时分秒   
//返回值:0,成功;其他:错误代码.
unsigned char  RTC_Alarm_Set(unsigned int syear,unsigned char  smon,unsigned char  sday,unsigned char  hour,unsigned char  min,unsigned char  sec)
{
	unsigned int t;
	u32 seccount=0;
	if(syear<1970||syear>2099)return 1;	   
	for(t=1970;t<syear;t++)	//把所有年份的秒钟相加
	{
		if(Is_Leap_Year(t))seccount+=31622400;//闰年的秒钟数
		else seccount+=31536000;			  //平年的秒钟数
	}
	smon-=1;
	for(t=0;t<smon;t++)	   //把前面月份的秒钟数相加
	{
		seccount+=(u32)mon_table[t]*86400;//月份秒钟数相加
		if(Is_Leap_Year(syear)&&t==1)seccount+=86400;//闰年2月份增加一天的秒钟数	   
	}
	seccount+=(u32)(sday-1)*86400;//把前面日期的秒钟数相加 
	seccount+=(u32)hour*3600;//小时秒钟数
    seccount+=(u32)min*60;	 //分钟秒钟数
	seccount+=sec;//最后的秒钟加上去 			    
	//设置时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);	//使能PWR和BKP外设时钟   
	PWR_BackupAccessCmd(ENABLE);	//使能后备寄存器访问  
	//上面三步是必须的!
	
	RTC_SetAlarm(seccount);
 
	RTC_WaitForLastTask();	//等待最近一次对RTC寄存器的写操作完成  	
	
	return 0;	    
}


//得到当前的时间
//返回值:0,成功;其他:错误代码.
unsigned char  RTC_Get(void)
{
	static unsigned int daycnt=0;
	u32 timecount=0; 
	u32 temp=0;
	unsigned int temp1=0;	  
    timecount=RTC_GetCounter();	 
 	temp=timecount/86400;   //得到天数(秒钟数对应的)
	if(daycnt!=temp)//超过一天了
	{	  
		daycnt=temp;
		temp1=1970;	//从1970年开始
		while(temp>=365)
		{				 
			if(Is_Leap_Year(temp1))//是闰年
			{
				if(temp>=366)temp-=366;//闰年的秒钟数
				else {temp1++;break;}  
			}
			else temp-=365;	  //平年 
			temp1++;  
		}   
		rtc.w_year=temp1;//得到年份
		temp1=0;
		while(temp>=28)//超过了一个月
		{
			if(Is_Leap_Year(rtc.w_year)&&temp1==1)//当年是不是闰年/2月份
			{
				if(temp>=29)temp-=29;//闰年的秒钟数
				else break; 
			}
			else 
			{
				if(temp>=mon_table[temp1])temp-=mon_table[temp1];//平年
				else break;
			}
			temp1++;  
		}
		rtc.w_month=temp1+1;	//得到月份
		rtc.w_date=temp+1;  	//得到日期 
	}
	temp=timecount%86400;     		//得到秒钟数   	   
	rtc.hour=temp/3600;     	//小时
	rtc.min=(temp%3600)/60; 	//分钟	
	rtc.sec=(temp%3600)%60; 	//秒钟
	rtc.week=RTC_Get_Week(rtc.w_year,rtc.w_month,rtc.w_date);//获取星期   
	return 0;
}	

//获得现在是星期几
//功能描述:输入公历日期得到星期(只允许1901-2099年)
//输入参数：公历年月日 
//返回值：星期号																						 
unsigned char  RTC_Get_Week(unsigned int year,unsigned char  month,unsigned char  day)
{	
	unsigned int temp2;
	unsigned char yearH,yearL;
	
	yearH=year/100;	yearL=year%100; 
	// 如果为21世纪,年份数加100  
	if (yearH>19)yearL+=100;
	// 所过闰年数只算1900年之后的  
	temp2=yearL+yearL/4;
	temp2=temp2%7; 
	temp2=temp2+day+table_week[month-1];
	if (yearL%4==0&&month<3)temp2--;
	return(temp2%7);
}			  
















