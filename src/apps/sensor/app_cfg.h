/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    app_cfg.h
* @data         2015/07/08
* @auther       chuanpengl
* @module       application config
* @brief        application config
***************************************************************************************************/
#ifndef __APP_CFG_H__
#define __APP_CFG_H__
/***************************************************************************************************
 * INCLUDES
 */
#include <stdbool.h>

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define APP_MODULE_ON           (1)
#define APP_MODULE_OFF          (0)
#define APP_USE_CLI             (APP_MODULE_ON)

//sdli 调试打印宏
#define APP_USE_DEBUG_PRINTF    (APP_MODULE_OFF)


#define APP_USE_SAMPLE          (APP_MODULE_OFF)
//1 1.方顺恒瑞大厦编译宏
#define APP_USE_SMART_COMMUNITY_FANGSHUN    (APP_MODULE_OFF) //方顺恒瑞大厦
#if APP_USE_SMART_COMMUNITY_FANGSHUN
    //3 连接的传感器， 以下宏只能开启一个
    #define APP_USE_THERMO_HUMIDITY_COM     (APP_MODULE_OFF) //商业级温湿度
    #define APP_USE_WATER_PRESS             (APP_MODULE_OFF) //水泵房水压
    #define APP_USE_WATER_LEVEL             (APP_MODULE_OFF) //天沟液位
    #define APP_USE_DISTRIBUTE_CURRENT      (APP_MODULE_OFF) //配电房电流
    #define APP_USE_ELEVATOR_KONE           (APP_MODULE_OFF) //
    #define APP_USE_ELECTRIC_METER          (APP_MODULE_OFF) //
    #define APP_USE_WATER_METER             (APP_MODULE_OFF)//
#endif

//1 2.丽岛花园小区编译宏
#define APP_USE_SMART_COMMUNITY_LIDAO       (APP_MODULE_ON) //丽岛花园小区
#if APP_USE_SMART_COMMUNITY_LIDAO
    //3 连接的传感器， 以下宏只能开启一个
    #define APP_USE_DOORSNR                 (APP_MODULE_ON) //门磁
    #if APP_USE_DOORSNR
        #define DOORSNR_BUILDING_GATE       (APP_MODULE_OFF) //单元门
        #define DOORSNR_NORMAL_OPEN         (APP_MODULE_OFF) //常开门
        #define DOORSNR_NORMAL_CLOSE        (APP_MODULE_ON) //常闭门
    #endif
    #define APP_USE_THERMO_HUMIDITY_COM     (APP_MODULE_OFF) //商业级温湿度
    #define APP_USE_WATER_PRESS             (APP_MODULE_OFF) //水泵房水压
    #define APP_USE_WATER_LEVEL             (APP_MODULE_OFF) //天沟液位
    #define APP_USE_DISTRIBUTE_CURRENT      (APP_MODULE_OFF) //配电房电流
    #define APP_USE_TRASH_CAN               (APP_MODULE_OFF) //垃圾桶
#endif

//1 3.盘龙城园区编译宏
#define APP_USE_SMART_COMMUNITY_PANLONG     (APP_MODULE_OFF) //盘龙城园区
#if APP_USE_SMART_COMMUNITY_PANLONG
    #if 0
    //3 网络传输方式，以下宏 只能开启一个
    #define APP_USE_NET_LONG_DISTANCE       (APP_MODULE_OFF) //远距离、低速率、低容量
    #define APP_USE_NET_HIGH_SPEED          (APP_MODULE_OFF) //近距离、高速率、高容量
    #endif
    //3 连接的传感器， 以下宏只能开启一个
    // TODO: 增加传感器应用
#endif

//sdli 传感器ID定义
#define DS_BUILDING_GATE_ID         0X01 //单元门传感器ID
#define DS_NORMAL_OPEN_ID           0X02 //常开门传感器ID
#define DS_NORMAL_CLOSE_ID          0X03 //常闭门传感器ID
#define TC_SENSOR_ID                0X04 //垃圾桶传感器ID
#define TH_SENSOR_ID                0X07 //温湿度传感器ID
#define WP_SENSOR_ID                0X08 //水压传感器ID
#define WL_SENSOR_ID                0X09 //液位传感器ID
#define DC_SENSOR_ID                0X0C //配电房电流传感器ID
#define WM_SENSOR_ID                0x0B
#define EM_SENSOR_ID                0x14
#define ELEV_SENSOR_ID              0x18

#define osBool                      bool
#define osTrue                      true
#define osFalse                     false

/* include header here */
#if ( defined( APP_USE_DOORSNR ) && ( APP_MODULE_ON == APP_USE_DOORSNR ) )
#include "app_doorsnr.h"
#endif

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


#ifdef __cplusplus
}
#endif

#endif /* __APP_CFG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150708
*   context: here write modified history
*
***************************************************************************************************/
