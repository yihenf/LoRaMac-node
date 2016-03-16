/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.h
* @data         2016/1/14 @auther       
* @module       module name
* @brief        file description
***************************************************************************************************/
#ifndef __APP_WEATHER_STATION_H__
#define __APP_WEATHER_STATION_H__
/***************************************************************************************************
 * INCLUDES
 */

#include "app_cfg.h"

#if APP_USE_WEATHERSTATION_XPH

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
typedef char xphChar;
typedef unsigned char xphU8;
typedef signed char xphI8;
typedef unsigned short xphU16;
typedef signed short xphI16;
typedef unsigned int xphU32;
typedef signed int xphI32;
typedef enum{xphFalse = 0, xphTrue = !xphFalse}xphBool;
#define xphNull              ((void*)0)





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
 * @fn      xphws_Init()
 *
 * @brief   init door sensor app
 *
 * @author  
 *
 * @param   none
 *
 * @return  none
 */
void xphws_Init( void );



#ifdef __cplusplus
}
#endif

#endif 

#endif /* __APP_WEATHER_STATION_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
