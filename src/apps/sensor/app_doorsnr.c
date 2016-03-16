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
* @auther       chuanpengl
* @module       door sensor app
* @brief        door sensor app
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_doorsnr.h"
#if (APP_USE_DOORSNR == APP_MODULE_ON)
#include "stm32l0xx_hal.h"
#include "dataExchange.h"


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define DS_IOPORT           GPIOA
#define DS_PIN              GPIO_PIN_1

#define DS_TASK_RUN_PERIOD  (50000)     /* 50ms */
#define DS_TASK_SLP_PERIOD  (20000000)  /* 20s */

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
static void ds_Tsk__( void );

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
static osBool ds_Wakeup__(void);

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
static osBool ds_Sleep__(void);

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
static void ds_InitSnr__( dsBool a_bWake );

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
static dsBool ds_GetSnrSt__(void);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
/* door sensor state, each bit represent a door state, 1 is close, 0 is open */
/* when 8 bits is the same, get door state stable */
static dsU16 gs_u16DsState = 0x5555;
static dsU16 gs_u16DsStateLast = 0x5555;

static uint8_t s_ucState = TASK_SLEEP;
static TimerEvent_t s_tDsTmr;
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
void ds_Init( void )
{
    TimerInit( &s_tDsTmr, ds_Tsk__ );
    TimerSetValue( &s_tDsTmr, DS_TASK_RUN_PERIOD );
    TimerStart( &s_tDsTmr );
} /* ds_Init() */


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
void subEXIT1_IRQHandler( void )
{
    gs_u16DsState = 0x5555;
    gs_u16DsStateLast = 0xAAAA;

    __disable_irq();
    if ( TASK_SLEEP == s_ucState )
    {
        TimerStop( &s_tDsTmr );
        TimerSetValue( &s_tDsTmr, DS_TASK_RUN_PERIOD );
        TimerStart( &s_tDsTmr );
    }
    __enable_irq();
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
void ds_Tsk__( void )
{
        dsU8 door_state; // 0:close  1:open
        dsU8 event; //0x80:异常开 0x81:异常关
        
#if DOORSNR_NORMAL_OPEN
        static dsU8 au8Msg[6] = {DS_NORMAL_OPEN_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
#elif DOORSNR_NORMAL_CLOSE
        static dsU8 au8Msg[6] = {DS_NORMAL_CLOSE_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
#elif DOORSNR_BUILDING_GATE
        static dsU8 au8Msg[6] = {DS_BUILDING_GATE_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
        dsU8 au8EventNumMsg[6] = {DS_BUILDING_GATE_ID, 0x00, 0x00, 0x00, 0x00, 0x00};
#endif

    if ( TASK_SLEEP == s_ucState )
    {
        ds_Wakeup__();
        s_ucState = TASK_RUN;
        TimerSetValue( &s_tDsTmr, DS_TASK_RUN_PERIOD );
    }

    if ( TASK_RUN == s_ucState )
    {
        /* get value for 8 times */
        if((gs_u16DsState != 0x0000 ) && ( gs_u16DsState != 0xFFFF ))
        {
            gs_u16DsState = (gs_u16DsState << 1) | (ds_GetSnrSt__() ? 1 : 0);
        }
        else
        {
            /* send message */
            if( gs_u16DsState == gs_u16DsStateLast )
            {
                if(gs_u16DsState > 0)//开
                {
                    door_state = 1;
                    event = 0x80;
                }
                else//关
                {
                    door_state = 0;
                    event = 0x81;
                }

                /* send message */
                au8Msg[1] = door_state;
                au8Msg[5] = event;

                #if 0
                printf("\r\n net_sendData gs_u8DsState[%d] door_state[%d] event[%x]\r\n",
                        gs_u16DsState, door_state, event);
                #endif

                loraWanSendRequest( 2, au8Msg, 6 );

                s_ucState = TASK_SLEEP_REQUEST;
            }
            else
            {
                gs_u16DsStateLast = gs_u16DsState;
                gs_u16DsState = 0x5555;
                s_ucState = TASK_SLEEP_REQUEST;
            }
        }
    }
    
    if ( TASK_SLEEP_REQUEST == s_ucState )
    {
        /* deinit */
        ds_Sleep__();
        TimerSetValue( &s_tDsTmr, DS_TASK_SLP_PERIOD );
        s_ucState = TASK_SLEEP;
    }

    TimerStart( &s_tDsTmr );

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
osBool ds_Wakeup__(void)
{
    ds_InitSnr__(dsTrue);
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
osBool ds_Sleep__(void)
{
    ds_InitSnr__(dsFalse);
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
void ds_InitSnr__( dsBool a_bWake )
{
    GPIO_InitTypeDef tGpioInit;
    tGpioInit.Pull      = GPIO_NOPULL;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    tGpioInit.Pin       = DS_PIN;
    __GPIOA_CLK_ENABLE();
//    if(dsTrue == a_bWake){
//        HAL_GPIO_DeInit(DS_IOPORT, DS_PIN);
//        tGpioInit.Mode = GPIO_MODE_INPUT;
//        HAL_GPIO_Init(DS_IOPORT, &tGpioInit);
//    }
//    else{
        tGpioInit.Mode = GPIO_MODE_IT_RISING_FALLING;
        HAL_GPIO_Init(DS_IOPORT, &tGpioInit);
        HAL_NVIC_SetPriority(EXTI0_1_IRQn, 3, 1);
        HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
//    }

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
dsBool ds_GetSnrSt__(void)
{
    return HAL_GPIO_ReadPin( DS_IOPORT, DS_PIN ) == GPIO_PIN_SET ? dsTrue : dsFalse;
}

#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
