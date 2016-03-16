/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.h
* @data         2016/01/25
* @auther       
* @module       
* @brief        
***************************************************************************************************/
#ifndef __APP_CURRENTLIMIT_H__
#define __APP_CURRENTLIMIT_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "app_cfg.h"

#if APP_USE_CURRENTLIMIT

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
typedef char clChar;
typedef unsigned char clU8;
typedef signed char clI8;
typedef unsigned short clU16;
typedef signed short clI16;
typedef unsigned int clU32;
typedef signed int clI32;
typedef enum{clFalse = 0, clTrue = !clFalse}clBool;
#define clNull              ((void*)0)

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
void cl_Init( void );



#ifdef __cplusplus
}
#endif

#endif

#endif /* __APP_CURRENTLIMIT_H__ */

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
