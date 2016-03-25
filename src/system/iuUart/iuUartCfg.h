/***************************************************************************************************
 * Copyright (c) 2015-2020 XXXXXXXXXXXXXXXX. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    uart_config
* @data         28/08/2015
* @author       chuanpengl
* @module       module name
* @brief        file description
***************************************************************************************************/
#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "iuUartType.h"
#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */


/***************************************************************************************************
 * TYPEDEFS
 */



/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      uartCfg_Init()
 *
 * @brief   Init usart
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_Init( uartU8 a_ucChn, iuUartPara_t *a_ptUartPara );

/***************************************************************************************************
 * @fn      uartCfg_DeInit()
 *
 * @brief   DeInit usart
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_DeInit( uartU8 a_u8Chn );

/***************************************************************************************************
 * @fn      uartCfg_SetBaud()
 *
 * @brief   set uart baudrate
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_SetBaud( uartU8 a_u8Chn, uartU32 a_u32Baud );

/***************************************************************************************************
 * @fn      uartCfg_EnableRx()
 *
 * @brief   enable rx or not
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_EnableRx( uartU8 a_u8Chn, uartBool a_bRxEn );

/***************************************************************************************************
 * @fn      uartCfg_SendBytes()
 *
 * @brief   send data
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Data  - data for sending
 *          a_u16Length  - data length
 *
 * @return  none
 */
void uartCfg_SendBytes( uartU8 a_u8Chn, uartU8  *a_pu8Data, uartU16 a_u16Length );

/***************************************************************************************************
 * @fn      uartCfg_GetReceiveBytes()
 *
 * @brief   get receive data
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Data  - data pointer
 *          a_u16Length  - data length
 *
 * @return  none
 */
uartU16 uartCfg_GetReceiveBytes( uartU8 a_u8Chn, uartU8  *a_pu8Data, uartU16 a_u16Length );


/***************************************************************************************************
 * @fn      uartCfg_Poll()
 *
 * @brief   usart poll
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uartCfg_Poll( uartU8 a_u8Chn );
uartBool uartCfg_IsBusy( uartU8 a_u8Chn );
#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 28/08/2015
*   context: here write modified history
*
***************************************************************************************************/
