/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    file name.h
* @data         
* @auther       
* @module       module name
* @brief        file description
***************************************************************************************************/
#ifndef __APP_PMB_H__
#define __APP_PMB_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "app_cfg.h"

#if APP_USE_PMB_MASTER

#ifdef __cplusplus
extern "C"{
#endif


typedef char pmbChar;
typedef unsigned char pmbU8;
typedef signed char pmbI8;
typedef unsigned short pmbU16;
typedef signed short pmbI16;
typedef unsigned int pmbU32;
typedef signed int pmbI32;
typedef enum{pmbFalse = 0, pmbTrue = !pmbFalse}pmbBool;
#define pmbNull              ((void*)0)



typedef struct{
	pmbU8 u8CCode;// control code 
	pmbU8 u8FCode; // function code 
}tOperationTypeDef;



typedef struct{
	pmbU16 u16Header;
	pmbU16 u16SourceAddr;
	pmbU16 u16DestAddr;
	pmbU8 u8ControlCode;
	pmbU8 u8FunCode;
	pmbU8 u8DataLen;
	pmbU8 *pu8Data;
	pmbU16 u16CheckSum;
}tPktFmtTypeDef;






void pmb_Init( void );



#ifdef __cplusplus
}
#endif

 
#endif

#endif /* __APP_PMB_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
