/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.c
* @data         2015/08/04
* @auther       sdli
* @module       
* @brief        
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_waterlevel.h"
#if (APP_USE_WATER_LEVEL == APP_MODULE_ON)
#include "os.h"
#include "os_signal.h"
#include "sx1278/sx1278.h"
#include "cfifo.h"
#include "stm32l0xx_hal.h"

#include "app_net.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define WL_IOPORT           GPIOB
#define WL_PIN              GPIO_PIN_7
    
#define WL_TASK_INTERVAL        10 //ms

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
 * @fn      wl_Tsk__()
 *
 * @brief   door sensor task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static OS_TASK( wl_Tsk__ );

/***************************************************************************************************
 * @fn      wl_Wakeup__()
 *
 * @brief   door sensor wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static osBool wl_Wakeup__(void);

/***************************************************************************************************
 * @fn      wl_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static osBool wl_Sleep__(void);

/***************************************************************************************************
 * @fn      wl_InitSnr__()
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
static void wl_InitSnr__( dsBool a_bWake );

/***************************************************************************************************
 * @fn      wl_GetSnrSt__()
 *
 * @brief   door sensor get state
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  dsTrue  - door close, dsFalse  - door open
 */
static dsBool wl_GetSnrSt__(void);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
osTask_t gs_tWlTsk;

/***************************************************************************************************
 * STATIC VARIABLES
 */
/* door sensor state, each bit represent a door state, 1 is close, 0 is open */
/* when 8 bits is the same, get door state stable */
static dsU16 gs_u16WlState = 0x5555;
static dsU16 gs_u16WlStateLast = 0x5555;

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      wl_Init()
 *
 * @brief   init door sensor app
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void wl_Init( void )
{
    os_TaskCreate( &gs_tWlTsk, wl_Tsk__, WL_TASK_INTERVAL);
    
    os_TaskAddWakeupInit( &gs_tWlTsk, wl_Wakeup__ );
    os_TaskAddSleepDeInit( &gs_tWlTsk, wl_Sleep__ );
    
    
    os_TaskAddItWake( IT_WAKE_ID_2, &gs_tWlTsk );
    
    os_TaskRun( &gs_tWlTsk );
    

} /* wl_Init() */


/***************************************************************************************************
 * @fn      subEXIT1_IRQHandler()
 *
 * @brief   exit 1 irq
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void subEXIT7_IRQHandler( void )
{
    gs_u16WlState = 0x5555;
    gs_u16WlStateLast = 0xAAAA;
    if( osFalse == os_TaskIsRun( &gs_tWlTsk ) ){
        os_TaskWakeupByIt(IT_WAKE_ID_2);
    }else{

    }
}



/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */


/***************************************************************************************************
 * @fn      wl_Tsk__()
 *
 * @brief   door sensor task
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
OS_TASK( wl_Tsk__ )
{
    dsU8 level_state; // 0:液位未达标  1:液位已达标
    dsU8 event; //0x80:液位未达标 0x81:液位已达标
    static dsU8 au8Msg[6] = {WL_SENSOR_ID, 0x00, 0x00, 0x00, 0x00, 0x00};

    if( OS_SIG_STATE_SET(OS_SIG_SYS) ){
        
        /* get value for 8 times */
        if((gs_u16WlState != 0x0000 ) && ( gs_u16WlState != 0xFFFF ))
        {
            gs_u16WlState = (gs_u16WlState << 1) | (wl_GetSnrSt__() ? 1 : 0);
        }
        else
        {
            /* send message */
            if( gs_u16WlState == gs_u16WlStateLast )
            {

                if(gs_u16WlState > 0)//液位已达标
                {
                    level_state = 1;
                    event = 0x81;
                }
                else//液位未达标
                {
                    level_state = 0;
                    event = 0x80;
                }
                
                /* send message */
                au8Msg[1] = level_state;
                au8Msg[5] = event;
                
            //    printf("\r\n net_sendData gs_u8WaterLevelState[%d] level_state[%d] event[%x]\r\n",
             //           gs_u16WlState, level_state, event);
                
                HAL_Delay(50);//等待lora wakeup     
                
                //采集的数据立即上报            
                net_SendData(au8Msg, 6, netTrue);      
                
                /* task sleep */
                os_TaskSleep( &gs_tWlTsk, 0 );

            }
            else
            {
                gs_u16WlStateLast = gs_u16WlState;
                gs_u16WlState = 0x5555;
                os_TaskSleep( &gs_tWlTsk, 300 );
            }
            
        }
        OS_SIG_CLEAR(OS_SIG_SYS);
    }
    
    OS_SIG_REBACK()
}

/***************************************************************************************************
 * @fn      wl_Wakeup__()
 *
 * @brief   door sensor wakeup
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool wl_Wakeup__(void)
{
    wl_InitSnr__(dsTrue);
    return osTrue;
}

/***************************************************************************************************
 * @fn      wl_Sleep__()
 *
 * @brief   door sensor sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
osBool wl_Sleep__(void)
{
    wl_InitSnr__(dsFalse);
    return osTrue;
}

/***************************************************************************************************
 * @fn      wl_InitSnr__()
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
void wl_InitSnr__( dsBool a_bWake )
{
    GPIO_InitTypeDef tGpioInit;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    tGpioInit.Pin       = WL_PIN;
    __GPIOB_CLK_ENABLE();
//    if(dsTrue == a_bWake){
//        HAL_GPIO_DeInit(DS_IOPORT, DS_PIN);
//        tGpioInit.Mode = GPIO_MODE_INPUT;
//        HAL_GPIO_Init(DS_IOPORT, &tGpioInit);
//    }
//    else{
        tGpioInit.Mode = GPIO_MODE_IT_RISING_FALLING;
        HAL_GPIO_Init(WL_IOPORT, &tGpioInit);
        HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
//    }

}   /* wl_InitSnr__() */


/***************************************************************************************************
 * @fn      wl_GetSnrSt__()
 *
 * @brief   door sensor get state
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  dsTrue  - door close, dsFalse  - door open
 */
dsBool wl_GetSnrSt__(void)
{
    return HAL_GPIO_ReadPin( WL_IOPORT, WL_PIN ) == GPIO_PIN_SET ? dsTrue : dsFalse;
}

#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
