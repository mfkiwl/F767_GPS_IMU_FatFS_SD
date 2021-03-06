#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timer.h"
#include "mpu9250.h"
#include "myiic.h"
#include "exti.h"
#include "dma.h"
#include "GetGPS.h"
#include "VariableType.h"
#include "GINavMain.h"

#include "task1.h"
#include "task2.h"
#include "GpsTask.h"
////////////////
#include "malloc.h"
#include "ff.h"
#include "exfuns.h"
#include "sdmmc_sdcard.h"

DIR recdir;




//任务优先级
#define START_TASK_PRIO		1
//任务堆栈大小	
#define START_STK_SIZE 		512  
//任务句柄
TaskHandle_t StartTask_Handler;
//任务函数
void start_task(void *pvParameters);

#define LED1_TASK_PRIO		3
#define LED1_STK_SIZE 		50 
TaskHandle_t LED1Task_Handler;
//任务函数

CONFIG_DATE_T GpsCon_Data;      //GPS配置
CONFIG_GPSInit_T   Gps_Init;    //Gps初始化
RAW_NMEA Gps_raw;

GPGGA_DATA  GPGGAData;				   //GGA结构体，存储各个协议中的内容
GPRMC_DATA  GPRMCData;           //RMC结构体，存储各个协议中的内容
GETGPS_DATE_T GpsGet_Data;      //GPS获得 

OUTPUT_INFO_T GINavResult;     // 组合导航输出的所有信息
IMU_DATA_T    IMUDataBuffer;	 // IMU所有指针均指向该地址....
GNSS_DATA_T   GNSSDataBuffer;  // GPS所有指针均指向该地址....

BOOL IMUDataReady;             // IMU数据是否准备好，如果准备好，每次用完，清零
BOOL GNSSDataReady;            // GNSS数据是否准备好，如果准备好，每次用完，清零

void Data_Init(void)
{
	Gps_Init.RATE_Init_flag = 1;		//5HZ
	Gps_Init.RMC_Init_flag = 1;		//GST open
	Gps_Init.GGA_Init_flag = 1;		//GST open
	Gps_Init.GST_Init_flag = 1;		//GST open
	Gps_Init.GSA_Init_flag = 0;		//GSA close
	Gps_Init.GSV_Init_flag = 0;		//GSV close
	Gps_Init.VTG_Init_flag = 0;		//VTG close
	Gps_Init.GLL_Init_flag = 0;		//GLL close
}	

void Set_System(void)
{
		uart_init(9600);		        //串口初始化
	  EXTI_Init();
    LED_Init();                     //初始化LED
	  MYDMA_Config(DMA2_Stream7,DMA_CHANNEL_4);
	  MYDMA_Config(DMA1_Stream6,DMA_CHANNEL_4);
//	  TIM3_Init(10-1,10800-1);      //定时器3初始化，定时器时钟为108M，分频系数为10800-1，                               
//		TIM2_Init(10000-1,10800-1);			//TIM2定时器1ms
	  IIC_Init();
		MPU9250_Init();
		Data_Init();
	////////////////////////SD卡
	my_mem_init(SRAMIN);		    //初始化内部内存池
	my_mem_init(SRAMEX);		    //初始化外部内存池
	my_mem_init(SRAMDTCM);		    //初始化CCM内存池
	 	while(SD_Init())//检测不到SD卡
	{
		delay_ms(500);	
	}
	exfuns_init();							//为fatfs相关变量申请内�
	f_mount(fs[0],"0:",1); 					//挂载SD卡
  sd_creatfile();  //创建SD文件	
	
}

int main(void)
{
    Cache_Enable();                 //打开L1-Cache
    HAL_Init();				        //初始化HAL库
    Stm32_Clock_Init(432,25,2,9);   //设置时钟,216Mhz 
    delay_init(216);                //延时初始化
	  Set_System();
			
		GPS_Rate_Config();
		GpsConfig();	
		
    //创建开始任务
    xTaskCreate((TaskFunction_t )start_task,            //任务函数
                (const char*    )"start_task",          //任务名称
                (uint16_t       )START_STK_SIZE,        //任务堆栈大小
                (void*          )NULL,                  //传递给任务函数的参数
                (UBaseType_t    )START_TASK_PRIO,       //任务优先级
                (TaskHandle_t*  )&StartTask_Handler);   //任务句柄              
    vTaskStartScheduler();          //开启任务调度
}

//开始任务任务函数
void start_task(void *pvParameters)
{
    taskENTER_CRITICAL();           //进入临界区
    //创建LED0任务
    task1_init();
    //创建LED1任务
    task2_init();

		GpsTask_init();	
  
    vTaskDelete(StartTask_Handler); //删除开始任务
    taskEXIT_CRITICAL();            //退出临界区
}






