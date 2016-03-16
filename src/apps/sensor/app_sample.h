/** ****************************************************************************
 * Copyright (c) 2015 ~ 2018 EasyLinkIn\n
 * All rights reserved.
 *
 * @file    app_sample.h
 * @brief   app sample
 * @version 1.0
 * @author  liucp
 * @date    2016-3-15
 * @warning No Warnings
 * @note <b>history:</b>
 * 1. first version
 * 
 */
#ifndef __APP_SAMPLE_H__
#define __APP_SAMPLE_H__

/*******************************************************************************
 * INCLUDES
 */
#include "app_cfg.h"
#if ( defined( APP_USE_SAMPLE ) && ( APP_MODULE_ON == APP_USE_SAMPLE ) )


#ifdef __cplusplus
extern "C"{
#endif

/*******************************************************************************
 * DEBUG SWITCH MACROS
 */


/*******************************************************************************
 * MACROS
 */



/*******************************************************************************
 * CONSTANTS
 */


/*******************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */

/*******************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */
void sampleInit( void );

#ifdef __cplusplus
}
#endif

#endif

#endif
