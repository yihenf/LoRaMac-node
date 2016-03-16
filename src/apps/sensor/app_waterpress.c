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
#include "app_waterpress.h"
#if APP_USE_WATER_PRESS
#include <stdio.h>
#include <string.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"
#include "adc/adc.h"

#include "app_net.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define WP_ADC_REF_VOLT         3286    // �ο���ѹ3300mV
#define WP_ADC_START_VOLT       111     // ��ʼ��ѹ111mV(14ŷ��Ӧ��ѹ/0KPa)
#define WP_ADC_OFFSET_VOLT      51     	// ʧ����ѹ51mV
#define WP_FULL_RANGE_VOLT      3094    // �����̵�ѹ3.094v 388ŷ	/* GPIO�������������ѹ��������ѹ���Ӹ��ش�С���� */
#define WP_ADC_MAX_PRESSURE     1600    // ˮѹ��������� 1600KPa ��Ӧ���跶Χ14~388ŷ
#define WP_FILTER_MAX_NUM       8       // ��Ȩƽ������

#define WP_TASK_INTERVAL        100 //ms
#define WP_TASK_SLEEP           30  //minute

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
static OS_TASK( wp_Tsk__ );

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
static osBool wp_Wakeup__(void);

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
static osBool wp_Sleep__(void);

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
static void wp_Init__( dsBool a_bWake );

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
static dsU32 wp_GetValue(dsU16 *value);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tWpTsk;
ADC_HandleTypeDef gs_tWpAdcHandler;

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
void wp_Init( void )
{
    os_TaskCreate( &gs_tWpTsk, wp_Tsk__, WP_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tWpTsk, wp_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tWpTsk, wp_Sleep__ );
    
    os_TaskRun( &gs_tWpTsk );

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
inline void wp_WakeUpEvent( void )
{
    os_TaskWakeup(&gs_tWpTsk);
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

/*
//�ɼ��������� mA
dc_GetValue(&current_raw);

//����mAΪA
current_integer = current_raw/1000;
current_decimal = (current_raw % 1000)/10;

//��д��������
au8Msg[1] = (current_integer & 0xFF00) > 8;
au8Msg[2] = current_integer & 0x00FF;
//��дС������
au8Msg[3] = current_decimal;

*/ 
OS_TASK( wp_Tsk__ )
{    
    static dsU8 au8Msg[6] = {WP_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
    dsU16 press_raw = 0;
    dsU8 press_integer = 0;
    dsU8 press_decimal = 0;
        
    if( OS_SIG_STATE_SET(OS_SIG_SYS) )
    {      
        //�ɼ�ѹ������ KPa
        wp_GetValue(&press_raw);

        //����KPaΪMPa
        press_integer = press_raw/1000;
        press_decimal = (press_raw % 1000)/10;
        
        //��д��������
        au8Msg[1] = press_integer & 0x00FF;
        //��дС������
        au8Msg[2] = press_decimal;

        printf("press_value[%d.%02d]MPa au8Msg[%02x][%02x]\r\n", 
                    press_integer, press_decimal, au8Msg[1], au8Msg[2]);

        //�ɼ������ݶ�ʱ���������ϱ�                
        net_SendData(au8Msg, 6, netFalse);

        /* task sleep */
        os_TaskSleep( &gs_tWpTsk, WP_TASK_SLEEP*60*1000 );

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
osBool wp_Wakeup__(void)
{
    wp_Init__(dsTrue);
    
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
osBool wp_Sleep__(void)
{
    wp_Init__(dsFalse);

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
void wp_Init__( dsBool a_bWake )
{
    if(dsTrue == a_bWake)
    {
        MX_ADC_Init(&gs_tWpAdcHandler);
    }
    else
    {
        MX_ADC_DeInit(&gs_tWpAdcHandler);
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
dsU32 wp_GetValue(dsU16 *value)
{    
    dsU32 adc_value = 0; //ADC�ɼ�ֵ
    dsU32 adc_value_v = 0; //ADCƽ��ֵ
    dsU32 volt;
    dsU32 adc_state;
    
    dsU8 filter_count = WP_FILTER_MAX_NUM;//��ȡ8�μ�Ȩƽ��
    dsU8 valid_count = 0; //��Ч���ݶ�ȡ����
    
    dsU32 sum_adc_buff[WP_FILTER_MAX_NUM];
    dsU32 sum_adc = 0;
    dsU32 i;

    memset(sum_adc_buff, 0, WP_FILTER_MAX_NUM*sizeof(dsU32));

    /* added by GuangYu.Yu @10/31 */
	if(HAL_ADC_Start(&gs_tWpAdcHandler) != HAL_OK)	/* adc��һ������ȡ��ֵ���ԣ����Խ���һ�ζ�ȡ��ֵ��ȥ */
	{
		return ERROR;
	}
	HAL_ADC_PollForConversion(&gs_tWpAdcHandler, 10);
	adc_state = HAL_ADC_GetState(&gs_tWpAdcHandler);
	if(adc_state != HAL_ADC_STATE_EOC)
	{
		 return ERROR;
	}	
	adc_value = HAL_ADC_GetValue(&gs_tWpAdcHandler);
    
    while(filter_count > 0)
    {
        if(HAL_ADC_Start(&gs_tWpAdcHandler) != HAL_OK)
        {
            return ERROR;
        }
        
        HAL_ADC_PollForConversion(&gs_tWpAdcHandler, 10);
        
        /* Check if the continous conversion of regular channel is finished */
        adc_state = HAL_ADC_GetState(&gs_tWpAdcHandler);
        if(adc_state != HAL_ADC_STATE_EOC)
        {
             return ERROR;
        }
        
        adc_value = HAL_ADC_GetValue(&gs_tWpAdcHandler);
         
    	//------���ݻ����˲�ȡƽ��ֵ---------------------------
    	sum_adc=0;
    	for(i=1;i<WP_FILTER_MAX_NUM;i++)
    	{	    		
    		sum_adc_buff[i-1] = sum_adc_buff[i]; 
    		sum_adc += sum_adc_buff[i-1];	
    	}

        valid_count ++;
        
    	sum_adc_buff[WP_FILTER_MAX_NUM-1] = adc_value;
    	sum_adc += sum_adc_buff[WP_FILTER_MAX_NUM-1];
    	adc_value_v = sum_adc/valid_count;
        sum_adc_buff[WP_FILTER_MAX_NUM-1] = adc_value_v;

    	filter_count --;
    }


    /*
        12λ������ADֵ��2��12�η�,��4096
        ʵ�ʵ�ѹֵ=ADֵ*��׼��ѹ/4096
    */
	volt = adc_value_v*WP_ADC_REF_VOLT/4096;
	printf ("Volt: %d mV\r\n", volt);
	if(volt < WP_ADC_OFFSET_VOLT)	//ʧ����ѹ
		volt = 0;
	else
		volt = volt-WP_ADC_OFFSET_VOLT;
    printf ("Volt: %d mV\r\n", volt);

    if(volt <= WP_ADC_START_VOLT)
	{
		*value = 0; //0 KPa
	}

    else if (volt >= WP_FULL_RANGE_VOLT)
	{
		*value = WP_ADC_MAX_PRESSURE; // 1600 KPa
	}      
    else
    {
        *value = WP_ADC_MAX_PRESSURE*(volt - WP_ADC_START_VOLT)/(WP_FULL_RANGE_VOLT - WP_ADC_START_VOLT);	/* WP_FULL_RANGE_VOLT �������˵�ѹ��С */
    }

    printf("Press: %d KPa\r\n", *value);
    return SUCCESS;

}
  
#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/