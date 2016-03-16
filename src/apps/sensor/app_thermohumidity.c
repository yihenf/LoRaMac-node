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
#include "app_thermohumidity.h"
#if APP_USE_THERMO_HUMIDITY
#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "bal_cfg.h"

#if defined (BAL_USE_DHT11)
	#include "dht11/dht11.h"
#elif defined (BAL_USE_HTU21D)
	#include "htu21d/htu21d.h"
#elif defined (BAL_USE_DS18B20)
	#include "ds18b20/ds18b20.h"
#elif defined (BAL_USE_HDC1050)
	#include "hdc1050/hdc1050.h"
#endif



#include "stm32l0xx_hal.h"

#include "app_net.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define TH_FILTER_MAX_NUM       8       // 加权平均次数
 
#define TH_TASK_INTERVAL        10*60*1000 //ms
#define TH_TASK_SLEEP           1  //minute

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
static OS_TASK( th_Tsk__ );

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
static osBool th_Wakeup__(void);

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
static osBool th_Sleep__(void);

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
static void th_Init__( dsBool a_bWake );

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
static dsU32 th_GetValue(float *temp, float *humi);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tThTsk;

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
void th_Init( void )
{
    os_TaskCreate( &gs_tThTsk, th_Tsk__, TH_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tThTsk, th_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tThTsk, th_Sleep__ );
    
    os_TaskRun( &gs_tThTsk );

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
inline void th_WakeUpEvent( void )
{
    // TODO: 
    //os_TaskWakeup(&gs_tTrashCanTsk);
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
OS_TASK( th_Tsk__ )
{
    float temp_value = 0;
    float humidity_value = 0;
    dsU8 retry_count = 3;
 
#if defined (BAL_USE_DS18B20)
	dsU8 au8Msg[6] = {0x1F, 0x00, 0x00, 0x00, 0x00, 0x00};
#else		
    dsU8 au8Msg[6] = {TH_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif    
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {        
        //采集温湿度数据
        while(retry_count)
        {
            th_GetValue(&temp_value, &humidity_value);
            retry_count --;
            
            if(humidity_value > 0)
                break;
        }
           
        //填写温度整数部分
				if(temp_value < 0)
				{
					au8Msg[1] = (dsI8)temp_value;
        //填写温度小数部分
					au8Msg[2] = (dsU8)(0 - (temp_value - (dsU8)temp_value)*100);
  			}
				else
				{
					au8Msg[1] = (dsU8)temp_value;
					au8Msg[2] = (dsU8)((temp_value - (dsU8)temp_value)*100);
				}					
        //填写湿度整数部分
        au8Msg[3] = (dsU8)humidity_value;

        //printf("temp_value[%.2f]C° humidity_value[%.2f] au8Msg[%02x][%02x][%02x]\r\n", 
         //           temp_value, humidity_value, au8Msg[1], au8Msg[2], au8Msg[3]);

        //采集的数据定时随心跳包上报        
        net_SendData(au8Msg, 6, netFalse);

        /* task sleep */
        //os_TaskSleep( &gs_tThTsk, TH_TASK_SLEEP*60*1000);

        OS_SIG_CLEAR(OS_SIG_SYS);
    }
    
    OS_SIG_REBACK()
}

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
osBool th_Wakeup__(void)
{
    th_Init__(dsTrue);

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
osBool th_Sleep__(void)
{
    th_Init__(dsFalse);

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
void th_Init__( dsBool a_bWake )
{
	if(dsTrue == a_bWake)
	{
#if defined (BAL_USE_DHT11)
		DHT11_Init();
#elif defined (BAL_USE_HTU21D)
		HTU21D_Init();
#elif defined (BAL_USE_DS18B20)
		DS18B20_Init();
#elif defined (BAL_USE_HDC1050)
		Hdc1050_Init();
#endif
	}
	else
	{
#if defined (BAL_USE_DHT11)
		DHT11_DeInit();
#elif defined (BAL_USE_HTU21D)
		HTU21D_DeInit();
#endif
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
#if defined (BAL_USE_DHT11) 
dsU32 th_GetValue(float *temp, float *humi)
{
    dsU8 temp_raw = 0; //温度采集值
    dsU8 humi_raw = 0; //湿度采集值
    dsU8 temp_value_v = 0; //温度平均值
    dsU8 humi_value_v = 0; //湿度平均值

    dsU8 filter_count = TH_FILTER_MAX_NUM;//读取8次加权平均
    dsU8 valid_count = 0; //有效数据读取次数

    dsU8 sum_temp_buff[TH_FILTER_MAX_NUM];
    dsU8 sum_humi_buff[TH_FILTER_MAX_NUM];
    dsU32 sum_temp = 0;
    dsU32 sum_humi = 0;

    dsU32 i;

    memset(sum_temp_buff, 0, TH_FILTER_MAX_NUM*sizeof(dsU8));
    memset(sum_humi_buff, 0, TH_FILTER_MAX_NUM*sizeof(dsU8));
        
    while(filter_count > 0)
    {        
        DHT11_Read_Data(&temp_raw, &humi_raw);
        
    	//------数据滑动滤波取平均值---------------------------
        sum_temp = 0;
        sum_humi = 0;
    
    	for(i=1;i<TH_FILTER_MAX_NUM;i++)
    	{	    		
    		sum_temp_buff[i-1] = sum_temp_buff[i]; 
    		sum_temp += sum_temp_buff[i-1];	

            sum_humi_buff[i-1] = sum_humi_buff[i]; 
            sum_humi += sum_humi_buff[i-1]; 
    	}

        if(humi_raw > 0)//本次采集数据有效
        {
            valid_count ++;

        	sum_temp_buff[TH_FILTER_MAX_NUM-1] = temp_raw;
        	sum_temp += sum_temp_buff[TH_FILTER_MAX_NUM-1];
        	temp_value_v = sum_temp/valid_count;
            sum_temp_buff[TH_FILTER_MAX_NUM-1] = temp_value_v; 

        	sum_humi_buff[TH_FILTER_MAX_NUM-1] = humi_raw;
        	sum_humi += sum_humi_buff[TH_FILTER_MAX_NUM-1];
        	humi_value_v = sum_humi/valid_count;
            sum_humi_buff[TH_FILTER_MAX_NUM-1] = humi_value_v;
        }        
        
    	filter_count --;
        HAL_Delay(5);
    }    

    *temp = (float)temp_value_v;
    *humi = (float)humi_value_v;
		
    return SUCCESS;
}
#elif defined (BAL_USE_HTU21D)
dsU32 th_GetValue(float *temp, float *humi)
{
	dsU8 error = 0;
    dsU16 temp_raw = 0; //温度采集值
    dsU16 humi_raw = 0; //湿度采集值
    dsU16 temp_value_v = 0; //温度平均值
    dsU16 humi_value_v = 0; //湿度平均值

    dsU8 filter_count = TH_FILTER_MAX_NUM;//读取8次加权平均
    dsU8 valid_count = 0; //有效数据读取次数    
    dsU16 sum_temp_buff[TH_FILTER_MAX_NUM];
    dsU16 sum_humi_buff[TH_FILTER_MAX_NUM];
    dsU32 sum_temp = 0;
    dsU32 sum_humi = 0;

    dsU32 i;

    memset(sum_temp_buff, 0, TH_FILTER_MAX_NUM*sizeof(dsU16));
    memset(sum_humi_buff, 0, TH_FILTER_MAX_NUM*sizeof(dsU16));

    while(filter_count > 0)
    {        
		error = HTU21D_Read_Data(&temp_raw, &humi_raw);
         
    	//------数据滑动滤波取平均值---------------------
        sum_temp = 0;
        sum_humi = 0;
    
    	for(i=1;i<TH_FILTER_MAX_NUM;i++)
    	{	    		
    		sum_temp_buff[i-1] = sum_temp_buff[i]; 
    		sum_temp += sum_temp_buff[i-1];	

            sum_humi_buff[i-1] = sum_humi_buff[i]; 
            sum_humi += sum_humi_buff[i-1]; 
    	}

        if(error == 0)//读取到有效数据
        {
            valid_count ++;

        	sum_temp_buff[TH_FILTER_MAX_NUM-1] = temp_raw;
        	sum_temp += sum_temp_buff[TH_FILTER_MAX_NUM-1];
        	temp_value_v = sum_temp/valid_count;
            sum_temp_buff[TH_FILTER_MAX_NUM-1] = temp_value_v; 

        	sum_humi_buff[TH_FILTER_MAX_NUM-1] = humi_raw;
        	sum_humi += sum_humi_buff[TH_FILTER_MAX_NUM-1];
        	humi_value_v = sum_humi/valid_count;
            sum_humi_buff[TH_FILTER_MAX_NUM-1] = humi_value_v;
        }

    	filter_count --;
        HAL_Delay(5);        
    }    

    *temp = HTU21D_CalcTemperatureC(temp_value_v);
    *humi = HTU21D_CalcHumdityRH(humi_value_v);
	
	if(error == 0)	
		return SUCCESS;
	else
		return ERROR;
}


#elif defined (BAL_USE_DS18B20)
dsU32 th_GetValue(float *temp, float *humi)
{	
	float tep[8] = { 0 };
	dsU8 u8Index;
	float fTepSum = 0;
	
	for(u8Index = 0; u8Index < 8; u8Index++)
	{
		tep[u8Index] = DS18B20_Get_Temp();
		fTepSum += tep[u8Index];
	}
	
	*temp = fTepSum/8;
	*humi = 0;
	return 0;
}


#elif defined (BAL_USE_HDC1050)
dsU32 th_GetValue(float *temp, float *humi)
{
	dsU8	u8Index;
	float temp_raw[TH_FILTER_MAX_NUM] = { 0 }; //温度采集值
	float humi_raw[TH_FILTER_MAX_NUM] = { 0 }; //湿度采集值	
	float fTemp = 0, fHumi = 0;
	
	for(u8Index = 0; u8Index < TH_FILTER_MAX_NUM; u8Index ++)
	{
		Hdc1050_GetData(&temp_raw[u8Index], &humi_raw[u8Index]);
		fTemp = fTemp + temp_raw[u8Index];
		fHumi = fHumi + humi_raw[u8Index];
	}
	
	fTemp = ( fTemp + TH_FILTER_MAX_NUM / 2 ) / TH_FILTER_MAX_NUM;
	fHumi = ( fHumi + TH_FILTER_MAX_NUM / 2 ) / TH_FILTER_MAX_NUM;
	
	*temp = fTemp;
	*humi = fHumi;
	
	return 0;
}


#endif

#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
