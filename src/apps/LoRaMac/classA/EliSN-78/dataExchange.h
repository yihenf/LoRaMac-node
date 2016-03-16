/** ****************************************************************************
 * Copyright (c) 2015 ~ 2018 EasyLinkIn\n
 * All rights reserved.
 *
 * @file    dataExchange.h
 * @brief   data exchange for user and lora wan
 * @version 1.0
 * @author  liucp
 * @date    2016-3-14
 * @warning No Warnings
 * @note <b>history:</b>
 * 1. first version
 * 
 */
#ifndef __DATAEXCHANGE_H__
#define __DATAEXCHANGE_H__

/*******************************************************************************
 * INCLUDES
 */

#include "board.h"

#include "LoRaMac.h"
#include "Comissioning.h"


#ifdef __cplusplus
extern "C"{
#endif

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */


/*******************************************************************************
 * MACROS
 */



/*******************************************************************************
 * CONSTANTS
 */

/*******************************************************************************
 * TYPEDEF
 */

typedef enum{
    TASK_SLEEP = 0,
    TASK_RUN = 1,
    TASK_SLEEP_REQUEST = 2
}taskState_e;

/*******************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */
void loraWanUsrAppInit( void );
bool loraWanSendRequest( uint8_t a_ucPort, uint8_t *a_pucData, uint8_t a_ucLen );
void loraWanSendConfirm( McpsConfirm_t *McpsConfirm );
void loraWanRecvIndication( McpsIndication_t *McpsIndication );




bool loraWanPrepareDataFrame( uint8_t a_ucPort, uint8_t *a_pucDat, uint8_t a_ucLen );
bool loraWanIsNextTxEnable( void );
bool loraWanIsComplianceTesting( void );
bool loraWanIsNetworkJoined( void );
void loraWanSendTrigger( void );

#ifdef __cplusplus
}
#endif


#endif
