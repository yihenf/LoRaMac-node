#include "app_distributor.h"

#if APP_USE_DISTRIBUTOR

#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"
#include "adc/adc.h"
#include "adc/adc_gpio.h"

#include "app_net.h"


#define MAX_DC_REFVOLTAGE	( 3300 )
#define MAX_VOLTAGERANGE	( 2000 )

#define MAX_DC_VOLTAGE		( 450 )	// V,  450
#define MAX_DC_CURRENT		( 400 )	// A, 20, 200, 400



#define DC_FILTER_MAX_NUM       8           // 

#define DC_TASK_INTERVAL        100 //ms
#define DC_TASK_SLEEP           30  //minute


#define VOLTAGE_CHN	( 2 )
#define CURRENT_CHN	( 3 )


typedef struct
{
	float fVoltage;
	float fCurrent;
	dbU8 u8Index;
}db_tInfo;


static OS_TASK( db_Tsk__ );
static osBool db_Wakeup__(void);
static osBool db_Sleep__(void);
static void db_Init__( dbBool a_bWake );
static dbU32 db_GetValue(float *value);


osTask_t gs_tDcTsk;
ADC_HandleTypeDef gs_tDcAdcHandler;
db_tInfo gs_tSysInfo = { 0 };
dbU8 const gs_u8Map[2] = { VOLTAGE_CHN, CURRENT_CHN };
dbU8 const gs_u8MsgType[2] = { 0x0A, 0x0B };




void db_Init( void )
{
    os_TaskCreate( &gs_tDcTsk, db_Tsk__, DC_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tDcTsk, db_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tDcTsk, db_Sleep__ );
    
    os_TaskRun( &gs_tDcTsk );

} /* ds_Init() */



inline void db_WakeUpEvent( void )
{
    os_TaskWakeup(&gs_tDcTsk);
}

 
OS_TASK( db_Tsk__ )
{    
    static dbU8 au8Msg[6] = {DB_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
    float current_raw = 0; 
    
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {      
			db_GetValue(&current_raw);

			au8Msg[1] = ((dbU32)current_raw & 0xFF00) >> 8;
			au8Msg[2] = (dbU32)current_raw & 0x00FF;
			au8Msg[3] = ( current_raw - (dbU32)current_raw )*100;
			au8Msg[4] = gs_u8MsgType[gs_tSysInfo.u8Index];
			net_SendData(au8Msg, 6, netFalse);

			gs_tSysInfo.u8Index = ( gs_tSysInfo.u8Index + 1 )%2;
			/* task sleep */
			os_TaskSleep( &gs_tDcTsk, 5*60*1000);//ms

			OS_SIG_CLEAR(OS_SIG_SYS);
    }
    
    OS_SIG_REBACK()
}



osBool db_Wakeup__(void)
{
    db_Init__(dbTrue);
    return osTrue;
}



osBool db_Sleep__(void)
{
    db_Init__(dbFalse);
    return osTrue;
}



void db_Init__( dbBool a_bWake )
{
    if(dbTrue == a_bWake)
    {
        //MX_ADC_Init(&gs_tDcAdcHandler);
		ADC_Init(&gs_tDcAdcHandler,gs_u8Map[gs_tSysInfo.u8Index]);
    }
    else
    {
        MX_ADC_DeInit(&gs_tDcAdcHandler);
    }

}   /* ds_InitSnr__() */




dbU32 db_GetValue(float *value)
{
    dbU32 adc_value = 0; 
	dbU8 u8Chn = 0;

    float volt;
	float fRate;
	
	u8Chn = gs_u8Map[gs_tSysInfo.u8Index];
	adc_value = MX_ADC_GetValue(&gs_tDcAdcHandler, u8Chn, DC_FILTER_MAX_NUM);
	volt = ((float)adc_value)*MAX_DC_REFVOLTAGE / 4096;
	fRate = volt / MAX_VOLTAGERANGE;
	if(fRate > 1)
		fRate = 1;

	switch(u8Chn)
	{
	case VOLTAGE_CHN:
		*value = fRate*MAX_DC_VOLTAGE;
		break;
	case CURRENT_CHN:
		*value = fRate*MAX_DC_CURRENT;
		break;
	default:
		break;
	}

    return SUCCESS;
}
  
#endif


