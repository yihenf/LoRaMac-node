/** ****************************************************************************
 * Copyright (c) 2015 ~ 2018 EasyLinkIn\n
 * All rights reserved.
 *
 * @file    LoRaMacLog.h
 * @brief   lora mac layer log
 * @version 1.0
 * @author  liucp
 * @date    2016-3-24
 * @warning No Warnings
 * @note <b>history:</b>
 * 1. first version
 *
 */
#ifndef __LORAMACLOG_H__
#define __LORAMACLOG_H__

/*******************************************************************************
 * INCLUDES
 */
#include "LoRaMac.h"


#ifdef __cplusplus
extern "C"{
#endif

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */
#define LORAMAC_LOG_ON              true

/*******************************************************************************
 * MACROS
 */



/*******************************************************************************
 * CONSTANTS
 */

/*******************************************************************************
 * TYPEDEF
 */
typedef struct LoRaMacLog_s
{
    struct txLog_s
    {
        uint32_t txReqCnt;
        uint32_t txSucCnt;
        uint32_t txRetryCnt;
        uint32_t txFailedCnt;
    }txLog;
    struct rxLog_s
    {
        uint32_t rxCnt;
        uint32_t rxLostCnt;
        uint32_t rxRetryCnt;
    }rxLog;

}LoRaMacLog_t;


/*******************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */



/*******************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */
#if ( LORAMAC_LOG_ON == true )

void loraMacLogTxSuccess( void );


#define LORAMAC_LOG_TX_SUCCESS()            loraMacTxSuccess()
#define LORAMAC_LOG_TX_LOST()
#define LORAMAC_LOG_TX_RETRY()
#define LORAMAC_LOG_TX_REQUEST()

#define LORAMAC_LOG_RX_SUCCESS()
#define LORAMAC_LOG_RX_LOST( _n )
#define LORAMAC_LOG_RX_RETRY()

#define LORAMAC_LOG_GET()
#else
#define LORAMAC_LOG_TX_SUCCESS()
#define LORAMAC_LOG_TX_LOST()
#define LORAMAC_LOG_TX_RETRY()
#define LORAMAC_LOG_TX_REQUEST()

#define LORAMAC_LOG_RX_SUCCESS()
#define LORAMAC_LOG_RX_LOST( _n )
#define LORAMAC_LOG_RX_RETRY()

#define LORAMAC_LOG_GET()
#endif


#ifdef __cplusplus
}
#endif


#endif
