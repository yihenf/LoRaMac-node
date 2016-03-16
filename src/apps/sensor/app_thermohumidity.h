/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.h
* @data         2015/08/25
* @auther       sdli
* @module       
* @brief        
***************************************************************************************************/
#ifndef __APP_THERMOHUMIDITY_H__
#define __APP_THERMOHUMIDITY_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "app_cfg.h"

#if APP_USE_THERMO_HUMIDITY

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
typedef char dsChar;
typedef unsigned char dsU8;
typedef signed char dsI8;
typedef unsigned short dsU16;
typedef signed short dsI16;
typedef unsigned int dsU32;
typedef signed int dsI32;
typedef enum{dsFalse = 0, dsTrue = !dsFalse}dsBool;
#define dsNull              ((void*)0)

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
void th_Init( void );

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
void th_WakeUpEvent( void );

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
