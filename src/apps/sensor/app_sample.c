/** ****************************************************************************
 * Copyright (c) 2015 ~ 2018 EasyLinkIn\n
 * All rights reserved.
 *
 * @file    app_sample.c
 * @brief   app sample
 * @version 1.0
 * @author  liucp
 * @date    2016-3-15
 * @warning No Warnings
 * @note <b>history:</b>
 * 1. first version
 * 
 */

/*******************************************************************************
 * INCLUDES
 */
#include "app_sample.h"
#include "dataExchange.h"

#if ( defined( APP_USE_SAMPLE ) && ( APP_MODULE_ON == APP_USE_SAMPLE ) )

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */


/*******************************************************************************
 * MACROS
 */


/*******************************************************************************
 * TYPEDEFS
 */


/*******************************************************************************
 * CONSTANTS
 */
#define SAMP_DATA_BUF_SZ         (16)
#define SAMP_TASK_RUN_PERIOD            (20000)     /* 20ms */
#define SAMP_TASK_SLEEP_PERIOD          (20000000)  /* 20s */

/*******************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
static void sampleTask__( void );

/*******************************************************************************
 * GLOBAL VARIABLES
 */


/*******************************************************************************
 * STATIC VARIABLES
 */
static uint8_t s_ucState = TASK_SLEEP;
static TimerEvent_t s_tSampleTimer;
static uint8_t s_ucSampData[ SAMP_DATA_BUF_SZ ];

/*******************************************************************************
 * EXTERNAL VARIABLES
 */


/*******************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
/** ****************************************************************************
 * @fn void sampleInit( void )
 * @brief sample app init
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void sampleInit( void )
{
    TimerInit( &s_tSampleTimer, sampleTask__ );
    TimerSetValue( &s_tSampleTimer, SAMP_TASK_RUN_PERIOD );
    TimerStart( &s_tSampleTimer );
}



/*******************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/** ****************************************************************************
 * @fn void sampleTask__( void )
 * @brief sample app task
 * @warning none
 * @exception none
 * @note <b>History:</b>
 * 1. create function
 *
 */
void sampleTask__( void )
{
    if ( TASK_SLEEP == s_ucState )
    {
        /* init  */
        s_ucState = TASK_RUN;
        TimerSetValue( &s_tSampleTimer, SAMP_TASK_RUN_PERIOD );
    }
    
    if ( TASK_RUN == s_ucState )
    {
        /* do */
        if ( true == loraWanSendRequest( 2, s_ucSampData, SAMP_DATA_BUF_SZ ) )
        {
            s_ucState = TASK_SLEEP_REQUEST;
            
            if ( s_ucSampData[ 3 ] == 0xFF ){
                if ( s_ucSampData[ 2 ] == 0xFF )
                {
                    if ( s_ucSampData[ 1 ] == 0xFF )
                    {
                        s_ucSampData[ 0 ] ++;
                    }
                    s_ucSampData[ 1 ] ++;
                }
                s_ucSampData[ 2 ] ++;
            }
            s_ucSampData[ 3 ] ++;
        }
        /* set state sleep request when need */
    }
    
    if ( TASK_SLEEP_REQUEST == s_ucState )
    {
        /* deinit */
        /* stop timer */
        TimerSetValue( &s_tSampleTimer, SAMP_TASK_SLEEP_PERIOD );
        s_ucState = TASK_SLEEP;
    }

    TimerStart( &s_tSampleTimer );
}   /* sampleTask__() */

#endif
