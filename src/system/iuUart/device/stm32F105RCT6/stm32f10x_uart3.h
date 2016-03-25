/***************************************************************************************************
 * Copyright (c) 2015-2020 XXXXXXXXXXXXXXXX. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    Untitled2
* @data         27/08/2015
* @author       chuanpengl
* @module       module name
* @brief        file description
***************************************************************************************************/
#ifndef __UART_CONFIG_uart3_H__
#define __UART_CONFIG_uart3_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "iuUart/iuUartType.h"
#include "iuUart/kernel/uart.h"
#include "stm32f1xx_hal.h"

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
 * @fn      UART_INIT_DEF(uart3)()
 *
 * @brief   uart3 interface init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_INIT_DEF(uart3)( uartHandle_t *a_ptUartHdl, iuUartPara_t *a_ptUartPara );

/***************************************************************************************************
 * @fn      UART_EXIT_DEF(uart3)()
 *
 * @brief   uart interface deinit
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_EXIT_DEF(uart3)( void *a_ptUart );

/***************************************************************************************************
 * @fn      UART_SEND_BYTE_WITH_IT_DEF(uart3)()
 *
 * @brief   uart send byte with interrupt
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
uartBool UART_SEND_BYTE_WITH_IT_DEF(uart3)( uartU8 a_u8Byte );

/***************************************************************************************************
 * @fn      UART_EVENT_DEF(uart3)()
 *
 * @brief   uart event
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
uartU8 UART_EVENT_DEF(uart3)(  uartHandle_t *a_ptUart, uartU8 a_u8Evt );

/***************************************************************************************************
 * @fn      UART_TX_CPLT_CALLBACK_DEF(uart3)()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_TX_CPLT_CALLBACK_DEF(uart3)(UART_HandleTypeDef *huart);

/***************************************************************************************************
 * @fn      UART_RX_CPLT_CALLBACK_DEF(uart3)()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_RX_CPLT_CALLBACK_DEF(uart3)(UART_HandleTypeDef *huart);

/***************************************************************************************************
 * @fn      UART_ERROR_CPLT_CALLBACK_DEF(uart3)()
 *
 * @brief   dtu uart error callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void UART_ERROR_CPLT_CALLBACK_DEF(uart3)(UART_HandleTypeDef *huart);

#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_uart3_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 17/11/2015
*   context: here write modified history
*
***************************************************************************************************/
