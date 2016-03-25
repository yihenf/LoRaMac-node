/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.h
* @data         2015/08/04
* @auther       chuanpengl
* @module       door sensor app
* @brief        door sensor app
***************************************************************************************************/
#ifndef __APP_CLI_H__
#define __APP_CLI_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "app_cfg.h"

#if APP_USE_CLI

#include "cli.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define CLI_MAX_CMD_NUM     33


/***************************************************************************************************
 * TYPEDEFS
 */

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */

typedef struct sn_config
{   
    cliU8 sn_mac_addr[3];
    cliU8 manufactory[16];
    cliU8 serial_num[16];
    cliU8 reserve[2];
    cliU8 hw_version[2];
}sn_config_t;


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
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
void cli_Init( void );

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
void cli_WakeUpEvent( void );

void cli_Send( uint8_t *a_pucBuf, uint16_t a_usLen );
extern cli_cmd_t gs_tCliCmd[CLI_MAX_CMD_NUM];
extern cliBool gs_bCliActive;
extern cliBool gs_bCliUseUart;
#ifdef __cplusplus
}
#endif

#endif

#endif /* __APP_DOORSNR_H__ */

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
