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
#ifndef __ELI_UART_DEVICE__
#define __ELI_UART_DEVICE__
/***************************************************************************************************
 * INCLUDES
 */
#include "iuUart/kernel/uart.h"
#include "stm32l0xx_hal.h"


#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define UART_USE_UART1          (0)
#define UART_USE_UART2          (1)
#define UART_USE_UART3          (0)
#define UART_USE_UART4          (0)
#define UART_USE_UART5          (0)


#if ( 1 == UART_USE_UART1 )
#include "stm32l0xx/stm32l051Uart5.h"
#endif

#if ( 1 == UART_USE_UART2 )
#include "stm32l0xx/stm32l051Uart2.h"
#endif

#if ( 1 == UART_USE_UART3 )
#include "stm32l0xx/stm32l051Uart5.h"
#endif

#if ( 1 == UART_USE_UART4 )
#include "stm32l0xx/stm32l051Uart5.h"
#endif

#if ( 1 == UART_USE_UART5 )
#include "stm32l0xx/stm32l051Uart5.h"
#endif


/***************************************************************************************************
 * TYPEDEFS
 */
enum{
#if ( 1 == UART_USE_UART1 )
    IU_UART_1,
#endif
#if ( 1 == UART_USE_UART2 )
    IU_UART_2,
#endif

#if ( 1 == UART_USE_UART3 )
    IU_UART_3,
#endif

#if ( 1 == UART_USE_UART4 )
    IU_UART_4,
#endif

#if ( 1 == UART_USE_UART5 )
    IU_UART_5,
#endif
    IU_UART_CNT
};

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */

#ifdef __cplusplus
}
#endif

#endif /* __ELI_UART_DEVICE__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 27/08/2015
*   context: here write modified history
*
***************************************************************************************************/
