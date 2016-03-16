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
#ifndef __APP_DISTRIBUTOR_H__
#define __APP_DISTRIBUTOR_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "app_cfg.h"

#if APP_USE_DISTRIBUTOR

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
typedef char dbChar;
typedef unsigned char dbU8;
typedef signed char dbI8;
typedef unsigned short dbU16;
typedef signed short dbI16;
typedef unsigned int dbU32;
typedef signed int dbI32;
typedef enum{dbFalse = 0, dbTrue = !dbFalse}dbBool;
#define dbNull              ((void*)0)

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
void db_Init( void );



#ifdef __cplusplus
}
#endif

#endif

#endif /* __APP_DISTRIBUTOR_H__ */

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
