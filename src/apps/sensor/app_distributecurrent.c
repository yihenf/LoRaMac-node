/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.c
* @data         2015/08/25
* @auther       sdli
* @module       
* @brief        
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_distributecurrent.h"

//配电房电流监测APP
#if APP_USE_DISTRIBUTE_CURRENT
#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"
#include "adc/adc.h"
#include "adc/adc_gpio.h"

#include "app_net.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define DC_ADC_REF_VOLT         3280        // 参考电压 mV
#define DC_ADC_START_VOLT       50          // 起始电压 mV
#define DC_ADC_MAX_VOLT         2000        // 电压测量最大值 mV
#if APP_USE_SMART_COMMUNITY_FANGSHUN
#define DC_ADC_MAX_CURRENT      1500*1000   // 电流最大量程 mA
#elif APP_USE_SMART_COMMUNITY_LIDAO
#define DC_ADC_MAX_CURRENT      1200*1000   // 电流最大量程 mA
#endif
#define DC_FILTER_MAX_NUM       8           // 加权平均次数

#define DC_TASK_INTERVAL        100 //ms
#define DC_TASK_SLEEP           30  //minute

/***************************************************************************************************
 * TYPEDEFS
 */
 
 
/***************************************************************************************************
 * CONSTANTS
 */

/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      ds_Tsk__()
 *
 * @brief   door sensor task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static OS_TASK( dc_Tsk__ );

/***************************************************************************************************
 * @fn      ds_Wakeup__()
 *
 * @brief   door sensor wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static osBool dc_Wakeup__(void);

/***************************************************************************************************
 * @fn      ds_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static osBool dc_Sleep__(void);

/***************************************************************************************************
 * @fn      ds_InitSnr__()
 *
 * @brief   door sensor init
 *
 * @author  chuanpengl
 *
 * @param   a_bWake  - dsTrue, init for wakeup
 *                   - dsFalse, init for sleep
 *
 * @return  none
 */
static void dc_Init__( dsBool a_bWake );

/***************************************************************************************************
 * @fn      ds_GetSnrSt__()
 *
 * @brief   door sensor get state
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  dsTrue  - door close, dsFalse  - door open
 */
static dsU32 dc_GetValue(dsU32 *value);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tDcTsk;
ADC_HandleTypeDef gs_tDcAdcHandler;


/***************************************************************************************************
 * STATIC VARIABLES
 */
/* door sensor state, each bit represent a door state, 1 is close, 0 is open */
/* when 8 bits is the same, get door state stable */
//static dsU16 gs_u16TrashCanDumpNum = 0;
//static dsU8  gs_u8TrashCanAngle = 0;

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */

 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      ds_Init()
 *
 * @brief   init door sensor app
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void dc_Init( void )
{
    os_TaskCreate( &gs_tDcTsk, dc_Tsk__, DC_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tDcTsk, dc_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tDcTsk, dc_Sleep__ );
    
    os_TaskRun( &gs_tDcTsk );

} /* ds_Init() */


/***************************************************************************************************
 * @fn      ds_WakeUpEvent()
 *
 * @brief   when wakeup event happed, invoke this
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline void dc_WakeUpEvent( void )
{
    os_TaskWakeup(&gs_tDcTsk);
}
/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */


/***************************************************************************************************
 * @fn      ds_Tsk__()
 *
 * @brief   door sensor task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */

 
OS_TASK( dc_Tsk__ )
{    
    static dsU8 au8Msg[6] = {DC_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
    dsU32 current_raw = 0; //电流数据 mA
    dsU32 current_integer = 0; //电流数据整数 A
    dsU8 current_decimal = 0; //电流数据小数 A
    
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {      
        //采集电流数据 mA
        dc_GetValue(&current_raw);

        //换算mA为A
        current_integer = current_raw/1000;
        current_decimal = (current_raw % 1000)/10;

        //填写整数部分
        au8Msg[1] = (current_integer & 0xFF00) >> 8;
        au8Msg[2] = current_integer & 0x00FF;
        //填写小数部分
        au8Msg[3] = current_decimal;

        printf("current_value[%d.%02d]A au8Msg[%02x][%02x][%02x]\r\n", 
                    current_integer, current_decimal, 
                    au8Msg[1], au8Msg[2], au8Msg[3]);
        
        //采集的数据定时随心跳包上报
        net_SendData(au8Msg, 6, netFalse);
        
        /* task sleep */
        os_TaskSleep( &gs_tDcTsk, DC_TASK_SLEEP*60*1000);//ms

        OS_SIG_CLEAR(OS_SIG_SYS);
    }
    
    OS_SIG_REBACK()
}

/***************************************************************************************************
 * @fn      dtu_UartIfInit__()
 *
 * @brief   dtu uart interface init, dtu will communicate with host through this interface
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */

/***************************************************************************************************
 * @fn      ds_Wakeup__()
 *
 * @brief   door sensor wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool dc_Wakeup__(void)
{
    dc_Init__(dsTrue);
    return osTrue;
}

/***************************************************************************************************
 * @fn      ds_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool dc_Sleep__(void)
{
    dc_Init__(dsFalse);
    return osTrue;
}

/***************************************************************************************************
 * @fn      ds_InitSnr__()
 *
 * @brief   door sensor init
 *
 * @author  chuanpengl
 *
 * @param   a_bWake  - dsTrue, init for wakeup
 *                   - dsFalse, init for sleep
 *
 * @return  none
 */
void dc_Init__( dsBool a_bWake )
{
    if(dsTrue == a_bWake)
    {
        MX_ADC_Init(&gs_tDcAdcHandler);
    }
    else
    {
        MX_ADC_DeInit(&gs_tDcAdcHandler);
    }

}   /* ds_InitSnr__() */


/***************************************************************************************************
 * @fn      ds_GetSnrSt__()
 *
 * @brief   door sensor get state
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  dsTrue  - door close, dsFalse  - door open
 */
dsU32 dc_GetValue(dsU32 *value)
{
    dsU32 adc_value = 0; //ADC采集值
    dsU32 adc_value_v = 0; //ADC平均值
    dsU32 volt;
    dsU32 adc_state;
    
    dsU8 filter_count = DC_FILTER_MAX_NUM;//读取8次加权平均
    dsU8 valid_count = 0; //有效数据读取次数

    dsU32 sum_adc_buff[DC_FILTER_MAX_NUM];
    dsU32 sum_adc = 0;
    dsU32 i;

    memset(sum_adc_buff, 0, DC_FILTER_MAX_NUM*sizeof(dsU32));

    //第一次采样值丢弃
    if(HAL_ADC_Start(&gs_tDcAdcHandler) != HAL_OK)
    {
        return ERROR;
    }
    HAL_ADC_PollForConversion(&gs_tDcAdcHandler, 10);
    adc_state = HAL_ADC_GetState(&gs_tDcAdcHandler);
    if(adc_state  != HAL_ADC_STATE_EOC)
    {
        return ERROR;
    }
    adc_value = HAL_ADC_GetValue(&gs_tDcAdcHandler);

    //开始正式采样
    while(filter_count > 0)
    {
        if(HAL_ADC_Start(&gs_tDcAdcHandler) != HAL_OK)
        {
            return ERROR;
        }
        
        HAL_ADC_PollForConversion(&gs_tDcAdcHandler, 10);
        
        /* Check if the continous conversion of regular channel is finished */
        adc_state = HAL_ADC_GetState(&gs_tDcAdcHandler);
        if(adc_state  != HAL_ADC_STATE_EOC)
        {
             return ERROR;
        }
        
        adc_value = HAL_ADC_GetValue(&gs_tDcAdcHandler);
         
    	//------数据滑动滤波取平均值---------------------------
    	sum_adc=0;
    	for(i=1;i<DC_FILTER_MAX_NUM;i++)
    	{	    		
    		sum_adc_buff[i-1] = sum_adc_buff[i]; 
    		sum_adc += sum_adc_buff[i-1];	
    	}

        valid_count ++;
        
    	sum_adc_buff[DC_FILTER_MAX_NUM-1] = adc_value;
    	sum_adc += sum_adc_buff[DC_FILTER_MAX_NUM-1];
    	adc_value_v = sum_adc/valid_count;
        sum_adc_buff[DC_FILTER_MAX_NUM-1] = adc_value_v;                

    	filter_count --;
        HAL_Delay(5);
    }
     
    /*
        12位满量程AD值是2的12次方,即4096
        实际电压值=AD值*基准电压/4096
    */
    volt = adc_value_v*DC_ADC_REF_VOLT/4096;

    //起始电压50mV
    if(volt >= DC_ADC_START_VOLT)
        volt = volt - DC_ADC_START_VOLT;
    else 
        volt = 0;
    
    printf ("Volt: %d mV\r\n", volt);
    
    //电压换算为电流 mA
    if(volt == 0)
        *value = 0;
    else if (volt >= DC_ADC_MAX_VOLT)
        *value = DC_ADC_MAX_CURRENT;
    else
        *value = (volt*DC_ADC_MAX_CURRENT)/DC_ADC_MAX_VOLT;

    printf("Current: %d mA\r\n", *value);

    return SUCCESS;

}
  
#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
