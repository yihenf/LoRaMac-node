#ifndef __CLI_STR_H__
#define __CLI_STR_H__

#include "app_cfg.h"

#if APP_USE_CLI

typedef char cliChar;
typedef unsigned char cliU8;
typedef signed char cliI8;
typedef unsigned short cliU16;
typedef signed short cliI16;
typedef unsigned int cliU32;
typedef signed int cliI32;
typedef enum{cliFalse = 0, cliTrue = !cliFalse}cliBool;
#define cliNull              ((void*)0)
 
cliU8 cli_get_parmpos(cliU8 num); //得到某个参数在参数列里面的起始位置
cliU8 cli_strcmp(cliU8*str1,cliU8 *str2); //对比两个字符串是否相等
cliU32 cli_pow(cliU8 m,cliU8 n); //M^N次方
cliU8 cli_str2num(cliU8*str,cliU32 *res); //字符串转为数字
cliU8 cli_get_cmdname(cliU8*str,cliU8*cmdname,cliU8 *nlen,cliU8 maxlen);//从str中得到指令名,并返回指令长度
cliU8 cli_get_fname(cliU8*str,cliU8*fname,cliU8 *pnum,cliU8 *rval); //从str中得到函数名
cliU8 cli_get_aparm(cliU8 *str,cliU8 *fparm,cliU8 *ptype);  //从str中得到一个函数参数
cliU8 cli_get_fparam(cliU8*str,cliU8 *parn); //得到str中所有的函数参数.
#endif

#endif

