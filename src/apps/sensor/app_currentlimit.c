#include "app_currentlimit.h"

#if APP_USE_CURRENTLIMIT

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"
#include "adc/adc.h"
#include "adc/adc_gpio.h"

#include "app_net.h"


#define MAX_DC_REFVOLTAGE   ( 5000 )
#define MAX_VOLTAGERANGE    ( 2000 )

#define MAX_DC_CURRENT      ( 50000 )   // 50000mA,



#define DC_FILTER_MAX_NUM       8           // 

#define DC_TASK_INTERVAL        100 //ms
#define DC_TASK_SLEEP           30  //minute


#define CURRENT_CHN	( 3 )


static OS_TASK( cl_Tsk__ );
static osBool cl_Wakeup__(void);
static osBool cl_Sleep__(void);
static void cl_Init__( clBool a_bWake );
//static clU32 cl_GetValue(float *value);
static clBool cl_GetSnrSt__(void);

osTask_t gs_tDcTsk;
ADC_HandleTypeDef gs_tDcAdcHandler;






void cl_Init( void )
{	
    os_TaskCreate( &gs_tDcTsk, cl_Tsk__, DC_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tDcTsk, cl_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tDcTsk, cl_Sleep__ );
    
		os_TaskAddItWake( IT_WAKE_ID_2, &gs_tDcTsk );
    os_TaskRun( &gs_tDcTsk );

} /* ds_Init() */



inline void cl_WakeUpEvent( void )
{
    os_TaskWakeup(&gs_tDcTsk);
}

 
OS_TASK( cl_Tsk__ )
{    
	static clU8 au8Msg[6] = {CL_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
	static clU8 au8Msg_b[6] = {CL_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
		
    clU8 u8Index = 0; 
    clU8 u8Stat = 0,u8LStat = 0; 
		
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {      
			for(u8Index = 0; u8Index < 8; u8Index++)
			{
				u8Stat = ( clTrue == cl_GetSnrSt__() ) ? (u8Stat+1) : u8Stat;   
			}
			
			if(u8Stat < 4) //??
			{	
				au8Msg[1] = 0;
				au8Msg[4] = 0x80;	
			}
			else //??
			{	
				au8Msg[1] = 1;
				au8Msg[4] = 0x81;
			}

			if(strcmp((const clChar *)au8Msg_b, (const clChar *)au8Msg))
				net_SendData(au8Msg, 6, netTrue);
			else	
				net_SendData(au8Msg, 6, netFalse);

			memcpy(au8Msg_b,au8Msg,6);
			/* task sleep */
			os_TaskSleep( &gs_tDcTsk, 0);//ms

			OS_SIG_CLEAR(OS_SIG_SYS);
    }
    
    OS_SIG_REBACK()
}



osBool cl_Wakeup__(void)
{
    cl_Init__(clTrue);
    return osTrue;
}



osBool cl_Sleep__(void)
{
    cl_Init__(clFalse);
    return osTrue;
}


void subEXIT7_IRQHandler( void )
{
    if( osFalse == os_TaskIsRun( &gs_tDcTsk ) ){
        os_TaskWakeupByIt(IT_WAKE_ID_2);
    }else{

    }
}


void cl_Init__( clBool a_bWake )
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.Pin = GPIO_PIN_7;	
	GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;	
	GPIO_InitStructure.Pull = GPIO_PULLUP;	
	__GPIOB_CLK_ENABLE();
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure); 
	HAL_NVIC_SetPriority(EXTI4_15_IRQn, 2, 1);
	HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);			
}   /* ds_InitSnr__() */




clBool cl_GetSnrSt__(void)
{
    return HAL_GPIO_ReadPin( GPIOB, GPIO_PIN_7 ) == GPIO_PIN_SET ? clTrue : clFalse;
}
  
#endif


