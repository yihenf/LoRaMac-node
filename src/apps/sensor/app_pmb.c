

#include "app_pmb.h"

#if APP_USE_PMB_MASTER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "os.h"
#include "os_signal.h"
#include "cfifo.h"
#include "crc/crc.h"
#include "stm32l0xx_hal.h"
#include "uart/uart_config.h"

#include "app_net.h"



#define GET_HEX16_HBYTE( u16Data )	( (pmbU8)((u16Data & 0xFF00) >> 8) )
	
#define GET_HEX16_LBYTE( u16Data )	( (pmbU8)((u16Data & 0x00FF) >> 0) )

#define MAKE_HEX16(u8DataH, u8DataL)	( ((pmbU16)u8DataH) << 8 | (pmbU16)u8DataL )


#define PMB_HEADER	( 0x3A3A )
#define MSG_TMO_INTERVAL	( 1000 ) //500 ms
#define MAX_RETRY	( 0x03 )

#define APP_ADDR( u8Addr ) ( (((pmbU16)u8Addr) << 8) & 0xFF00 )
#define INV_ADDR( u8Addr ) ( (((pmbU16)u8Addr) << 0) & 0x00FF )	

#define MASK_FUNCODE( FunCode )		( FunCode | 0x80 )		


#define OS_SIG_TASK_PMB_OFFLINEQUERYTMR              		(((osUint32)1) << 1)	//get data info flag 
#define OS_SIG_TASK_PMB_PRASEFRAMETMR              			(((osUint32)1) << 2)	//get data info flag 
#define OS_SIG_TASK_PMB_READDESCRIPTTMR              		(((osUint32)1) << 3)	//get data info flag 
#define OS_SIG_TASK_PMB_READNORMALINFOTMR              		(((osUint32)1) << 4)	//get data info flag 
#define OS_SIG_TASK_PMB_UPLOADTMR              				(((osUint32)1) << 5)	//upload info tmr

enum{
	ERROR_NONE,
	ERROR_PARAM,
	ERROR_MEMORY,
	ERROR_ADDR,
	ERROR_MEMBER,
	ERROR_EXIST,
	ERROR_NONEEXIST,
};



//control code 
enum{
	CC_REGISTER	= 0x10,
	CC_READ		= 0x11,
	CC_WRITE	= 0x12,
	CC_EXECUTE	= 0x13,
};



enum{
	FC_00	= 0x00,
	FC_01	= 0x01,
	FC_02	= 0x02,
	FC_03	= 0x03,
	FC_04	= 0x04,
	FC_05	= 0x05,
	FC_06	= 0x06,
	FC_07	= 0x07,
	FC_08	= 0x08,
	FC_09	= 0x09,
};


enum{
	FC_00_REPLAY	= MASK_FUNCODE(FC_00),
	FC_01_REPLAY	= MASK_FUNCODE(FC_01),
	FC_02_REPLAY	= MASK_FUNCODE(FC_02),
	FC_03_REPLAY	= MASK_FUNCODE(FC_03),
	FC_04_REPLAY	= MASK_FUNCODE(FC_04),
	FC_05_REPLAY	= MASK_FUNCODE(FC_05),
	FC_06_REPLAY	= MASK_FUNCODE(FC_06),
	FC_07_REPLAY	= MASK_FUNCODE(FC_07),
	FC_08_REPLAY	= MASK_FUNCODE(FC_08),
	FC_09_REPLAY	= MASK_FUNCODE(FC_09),
};


typedef struct{
	
	void(*SendInfo)(pmbU8 u8Chn,pmbU8 *pu8Buf, pmbU16 u16Len);
	void(*RecvInfo)(pmbU8 *pu8Buf, pmbU16 u16Len);
	
	pmbU8 u8TxBuf[64];
	pmbU8 u8RxBuf[64];
	pmbU8 u8TxLen;
	pmbU8 u8RxLen;
}tPmbSysInfoTypeDef;

//节点状态
typedef enum{
	STATE_NONE,//初始狀態
	STATE_REGISTER,//請求註冊
	STATE_NETTED,//已入網
	STATE_REMOVED,//已删除
}eApStateTypeDef;

//节点属性信息
typedef struct {
	pmbU16 u16INVAddress;
	pmbU8 u8INVSerNum[16];	
	eApStateTypeDef eStatus;		
}tNodeInfoTypeDef;

//链表类型
typedef enum{
	LIST_TYPE_NULL,
	LIST_TYPE_REGISTER,
	LIST_TYPE_NETTED,
	LIST_TYPE_REMOVED,
}eListTypeDef;


typedef struct PList{
	tNodeInfoTypeDef tNode;//节点信息
	eListTypeDef eListType;//链表类型
	pmbU32 u32Time;//注册/入网时间
	pmbU32 u32Interval;//周期/超时时间
	pmbU8  u8ReTry;//重复次数
	void(*TimeOutProcess)(struct PList **tSnList, eListTypeDef);
	struct PList *Next;
	struct PList *Prev;
}tPSnListTypeDef;



#define NODE_LIST_NULL	( (tPSnListTypeDef *)0 )

//pend node list
static tPSnListTypeDef	*tRegisterNodeList = NODE_LIST_NULL;

//communication node list
static tPSnListTypeDef	*tComNodeList = NODE_LIST_NULL;

//removed node list 
static tPSnListTypeDef	*tRemovedNodeList = NODE_LIST_NULL;


//REGISTER 
const tOperationTypeDef  tcQueryOffLine = {(pmbU8)CC_REGISTER, (pmbU8)FC_00};
const tOperationTypeDef  tcSendRegisterAddr = {(pmbU8)CC_REGISTER, (pmbU8)FC_01};
const tOperationTypeDef  tcRemoveRegister = {(pmbU8)CC_REGISTER, (pmbU8)FC_02};
const tOperationTypeDef  tcReConectInverter = {(pmbU8)CC_REGISTER, (pmbU8)FC_03};
const tOperationTypeDef  tcReRegister = {(pmbU8)CC_REGISTER, (pmbU8)FC_04};

//READ
const tOperationTypeDef  tcReadDescription = {(pmbU8)CC_READ, (pmbU8)FC_00};
const tOperationTypeDef  tcRWDescription = {(pmbU8)CC_READ, (pmbU8)FC_01};
const tOperationTypeDef  tcQueryNormalInfo = {(pmbU8)CC_READ, (pmbU8)FC_02};
const tOperationTypeDef  tcQueryInvIDInfo = {(pmbU8)CC_READ, (pmbU8)FC_03};
const tOperationTypeDef  tcReadSetInfo = {(pmbU8)CC_READ, (pmbU8)FC_04};
const tOperationTypeDef  tcQueryInvTime = {(pmbU8)CC_READ, (pmbU8)FC_05};
const tOperationTypeDef  tcQueryInvSafeType = {(pmbU8)CC_READ, (pmbU8)FC_06};
const tOperationTypeDef  tcQueryInvDevType = {(pmbU8)CC_READ, (pmbU8)FC_07};
const tOperationTypeDef  tcQueryInvSwVersion = {(pmbU8)CC_READ, (pmbU8)FC_08};






osTask_t gs_tPmbTsk;
osSigTimer_t gs_tPmbOffLineQueryTmr;
osSigTimer_t gs_tPmbReadInfoTmr;

pmbU16 gs_u16APAddress = APP_ADDR(0x01);
pmbU16 gs_u16INVAddress = INV_ADDR(0x01);

tPktFmtTypeDef gs_tSendFrm = { 0 };
//tPktFmtTypeDef gs_tOffLineQuery = { 0 };

tPktFmtTypeDef gs_tRecvdFrm = { 0 };

tPmbSysInfoTypeDef gs_tPmbSysInfo = { 0 };

xphU8 gs_u8Msg[6] = {DB_SENSOR_ID, 0x00, 0x00, 0x00, 0x0A, 0x00};



// main task process function
static OS_TASK( pmb_Tsk__ );
// main task wakeup function
static osBool pmb_Wakeup__(void);
// main task sleep function
static osBool pmb_Sleep__(void);
// wake-sleep init function
static void pmb_Init__( pmbBool a_bWake );

static void pmb_OfflineQueryFrame( tPktFmtTypeDef *ptOffQuery );
static void pmb_SendRegisterAddress( tPktFmtTypeDef *ptOffQuery, pmbU16 u16Addr, pmbU8 *pu8Sn);
static void pmb_RemoveRegisteredDevice( tPktFmtTypeDef *ptOffQuery );
static void pmb_ReconnectRemovedDevice( tPktFmtTypeDef *ptOffQuery );
static void pmb_ReregisterDevice( tPktFmtTypeDef *ptOffQuery );
static void pmb_AllocateAddress(pmbU16 *u16Address);

static pmbU8 pmb_MakeTxdBuff(tPktFmtTypeDef *ptPkg, pmbU8 *u8buf);

void pmb_ParseProcessTest(void);
/*****************************************************************************************************
*****************************************************************************************************/





pmbU8 pmb_DeleteFromNodeList(tPSnListTypeDef **ptList, pmbU16 u16Addr, pmbU8 *u8Sn);
pmbU8 pmb_AddToNodeList(tPSnListTypeDef **ptList, tNodeInfoTypeDef *ptNode, eListTypeDef eType);
void pmb_ParseRegisterFunCode(tPktFmtTypeDef *ptPkg);
void pmb_ParseReadFunCode(tPktFmtTypeDef *ptPkg);


/*
CheckSum生成：将发送数据逐个累加，如果这个对累加和超过范围 则取0xFF或者0xFFFF 余数，再取反，该值即为CheckSum校验码。
			取反的方式在C语言中可以使用按位取反“~”来实现。
CheckSum校验：将接收到的数据与校验码连续累加，将结果取反，若取反后的值为0，则表示校验正确，数据有效；否则，校验错误，数据无效。
*/
pmbU16 CheckSum16(tPktFmtTypeDef *ptOffQuery)
{
	pmbU16 u16Ret = 0;
	pmbU8 u8Index = 0;
	pmbU32 u32CheckSum = 0;
	
	u32CheckSum += ptOffQuery->u16Header;
	u32CheckSum += ptOffQuery->u16SourceAddr;
	u32CheckSum += ptOffQuery->u16DestAddr;
	u32CheckSum += ptOffQuery->u8ControlCode;
	u32CheckSum += ptOffQuery->u8FunCode;
	u32CheckSum += ptOffQuery->u8DataLen;

	for(u8Index = 0; u8Index < ptOffQuery->u8DataLen; u8Index++)
	{
		u32CheckSum += ptOffQuery->pu8Data[u8Index];
	}
	
	u16Ret = ~(u32CheckSum % 0xFFFF);

	return u16Ret;
}

// offline query frame
void pmb_OfflineQueryFrame( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = 0;//gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcQueryOffLine.u8CCode;
	ptOffQuery->u8FunCode = tcQueryOffLine.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);
}

// allocate address 
void pmb_SendRegisterAddress( tPktFmtTypeDef *ptOffQuery, pmbU16 u16Addr, pmbU8 *pu8Sn)
{
	pmbU8 u8buff[0x11];

	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = 0;//gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcSendRegisterAddr.u8CCode;
	ptOffQuery->u8FunCode = tcSendRegisterAddr.u8FCode;
	ptOffQuery->u8DataLen = 0x11;
	ptOffQuery->pu8Data = malloc(ptOffQuery->u8DataLen);
	
	memcpy(u8buff, pu8Sn, ptOffQuery->u8DataLen - 1);
	u8buff[0x10] = (pmbU8)(u16Addr & 0x00FF);
	memcpy(ptOffQuery->pu8Data, u8buff, ptOffQuery->u8DataLen);
	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);	
}

// remove registered inverter 
void pmb_RemoveRegisteredDevice( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcRemoveRegister.u8CCode;
	ptOffQuery->u8FunCode = tcRemoveRegister.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);	
}

// reconnect removed inverter 
void pmb_ReconnectRemovedDevice( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = 0;//gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcReConectInverter.u8CCode;
	ptOffQuery->u8FunCode = tcReConectInverter.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);	
}

// re-register Inverter
void pmb_ReregisterDevice( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = 0;//gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcReRegister.u8CCode;
	ptOffQuery->u8FunCode = tcReRegister.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);	
}

//query read descrption
void pmb_QueryInvReadDescription( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcReadDescription.u8CCode;
	ptOffQuery->u8FunCode = tcReadDescription.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);	
}

//query read write description
void pmb_QueryInvReadWriteDescription( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcRWDescription.u8CCode;
	ptOffQuery->u8FunCode = tcRWDescription.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);	
}

//query normal info
void pmb_QueryInvNormalInfo( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcQueryNormalInfo.u8CCode;
	ptOffQuery->u8FunCode = tcQueryNormalInfo.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);		
	
}

// query ID info
void pmb_QueryInvIDInfo( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcQueryInvIDInfo.u8CCode;
	ptOffQuery->u8FunCode = tcQueryInvIDInfo.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);		
	
}

//query setting info 
void pmb_QueryInvSettingInfo( tPktFmtTypeDef *ptOffQuery )
{
	ptOffQuery->u16Header = PMB_HEADER;
	ptOffQuery->u16SourceAddr = gs_u16APAddress;
	ptOffQuery->u16DestAddr = gs_u16INVAddress;
	ptOffQuery->u8ControlCode = tcReadSetInfo.u8CCode;
	ptOffQuery->u8FunCode = tcReadSetInfo.u8FCode;
	ptOffQuery->u8DataLen = 0;
	ptOffQuery->pu8Data = pmbNull;

	ptOffQuery->u16CheckSum = CheckSum16(ptOffQuery);		
}


/*
*	ptList:	list pointer	 
*	ptNode: node information
*	eType :list type
*/


void pmb_SetNodeInfo(tNodeInfoTypeDef *ptNode, pmbU16 u16Addr, eApStateTypeDef eStat, pmbU8 *u8Sn)
{
	ptNode->eStatus = eStat;
	ptNode->u16INVAddress = u16Addr;
	memcpy(ptNode->u8INVSerNum, u8Sn, 16);
}


/*
#define MSG_TMO_INTERVAL	( 500 ) //500 ms
#define MAX_RETRY	( 0x03 )
*/
void pmb_AllocConfirmTmoProcess(tPSnListTypeDef **tSnList)
{
	tPktFmtTypeDef tFrame;
	pmbU8 u8buf[32];
	tNodeInfoTypeDef tNode;

	if((*tSnList)->u8ReTry < MAX_RETRY)
	{
		(*tSnList)->u8ReTry++;
		(*tSnList)->u32Time = os_GetTick();

		pmb_SendRegisterAddress(&tFrame, (*tSnList)->tNode.u16INVAddress, (*tSnList)->tNode.u8INVSerNum);
		//pmb_MakeTxdBuff(&tFrame, u8buf);
		gs_tPmbSysInfo.u8TxLen = pmb_MakeTxdBuff(&tFrame, gs_tPmbSysInfo.u8TxBuf);		
		gs_tPmbSysInfo.SendInfo( UART_CHN_2, gs_tPmbSysInfo.u8TxBuf, gs_tPmbSysInfo.u8TxLen );	
	
	}
	else
	{
		pmb_SetNodeInfo(&tNode, (*tSnList)->tNode.u16INVAddress, STATE_REMOVED, (*tSnList)->tNode.u8INVSerNum);
		pmb_AddToNodeList(&tRemovedNodeList, &tNode, LIST_TYPE_REMOVED);
		//pmb_DeleteFromNodeList(&tComNodeList, ptPkg->u16SourceAddr, u8Sn);

		pmb_DeleteFromNodeList(&tRegisterNodeList, (*tSnList)->tNode.u16INVAddress, (*tSnList)->tNode.u8INVSerNum);//根據序列號從register鏈表中刪除

	}
}



//掉线处理
void pmb_NettedTmoProcess(tPSnListTypeDef **tSnList)
{
	tPktFmtTypeDef tFrame;
	pmbU8 u8buf[32];

	tNodeInfoTypeDef tNode;

	if((*tSnList)->u8ReTry < MAX_RETRY)
	{
		(*tSnList)->u8ReTry++;
		(*tSnList)->u32Time = os_GetTick();

		pmb_SendRegisterAddress(&tFrame, (*tSnList)->tNode.u16INVAddress, (*tSnList)->tNode.u8INVSerNum);
		//pmb_MakeTxdBuff(&tFrame, u8buf);
		gs_tPmbSysInfo.u8TxLen = pmb_MakeTxdBuff(&tFrame, gs_tPmbSysInfo.u8TxBuf);		
		gs_tPmbSysInfo.SendInfo( UART_CHN_2, gs_tPmbSysInfo.u8TxBuf, gs_tPmbSysInfo.u8TxLen );	
	}
	else
	{
		pmb_SetNodeInfo(&tNode, (*tSnList)->tNode.u16INVAddress, STATE_REMOVED, (*tSnList)->tNode.u8INVSerNum);
		pmb_AddToNodeList(&tRemovedNodeList, &tNode, LIST_TYPE_REMOVED);
		//pmb_DeleteFromNodeList(&tComNodeList, ptPkg->u16SourceAddr, u8Sn);

		pmb_DeleteFromNodeList(&tComNodeList, (*tSnList)->tNode.u16INVAddress, (*tSnList)->tNode.u8INVSerNum);//根據序列號從register鏈表中刪除

	}
}



void pmb_TmoProcess(tPSnListTypeDef **tSnList, eListTypeDef eList)
{
	switch(eList)
	{
	case LIST_TYPE_NULL:
		break;
	case LIST_TYPE_REGISTER:
		pmb_AllocConfirmTmoProcess(tSnList);
		break;
	case LIST_TYPE_NETTED:
		pmb_NettedTmoProcess(tSnList);
		break;
	case LIST_TYPE_REMOVED:
		break;
	default:
		break;
	}
}



//add node to list 
pmbU8 pmb_AddToNodeList(tPSnListTypeDef **ptList, tNodeInfoTypeDef *ptNode, eListTypeDef eType)
{
	tPSnListTypeDef *p = NODE_LIST_NULL;

	if(NODE_LIST_NULL == *ptList)//first 
	{
		*ptList = (tPSnListTypeDef *)malloc(sizeof(tPSnListTypeDef));
		(*ptList)->tNode.u16INVAddress = ptNode->u16INVAddress;
		memcpy((*ptList)->tNode.u8INVSerNum, ptNode->u8INVSerNum, 16);
		(*ptList)->tNode.eStatus = ptNode->eStatus;
		(*ptList)->eListType = eType;
		(*ptList)->u32Time = os_GetTick() ;
		(*ptList)->u32Interval = MSG_TMO_INTERVAL;
		(*ptList)->u8ReTry = 0;
		(*ptList)->TimeOutProcess = pmb_TmoProcess;
		//(ptList, eType)
		(*ptList)->Next = NODE_LIST_NULL;
		(*ptList)->Prev = NODE_LIST_NULL;		
	}
	else
	{	
		p = *ptList;
		while( NODE_LIST_NULL != p->Next )
		{	
			if(0 == strcmp((const char *)(ptNode->u8INVSerNum), (const char *)(p->tNode.u8INVSerNum)))
				return ERROR_EXIST;
			p = p->Next;
		}
		 
		p->Next = (tPSnListTypeDef *)malloc(sizeof(tPSnListTypeDef));
		memset(p->Next, 0, sizeof(tPSnListTypeDef));
		p->Next->tNode.u16INVAddress = ptNode->u16INVAddress;
		memcpy(p->Next->tNode.u8INVSerNum, ptNode->u8INVSerNum, 16);
		p->Next->tNode.eStatus = ptNode->eStatus;
		p->Next->eListType = eType;
		p->Next->u32Time = os_GetTick() ;
		p->Next->u32Interval = MSG_TMO_INTERVAL;
		p->Next->u8ReTry = 0;
		p->Next->TimeOutProcess = pmb_TmoProcess;//(&p, eType);
		p->Next->Next = NODE_LIST_NULL;
		p->Next->Prev = p;
	}

	return ERROR_NONE;
}



//delete node from list 
pmbU8 pmb_DeleteFromNodeList(tPSnListTypeDef **ptList, pmbU16 u16Addr, pmbU8 *u8Sn)
{
	tPSnListTypeDef *p = NODE_LIST_NULL;

	if(NODE_LIST_NULL != *ptList) 
	{
		p = *ptList;

		if(u16Addr != 0)
		{
			while( (p->tNode.u16INVAddress != u16Addr) && (p != NODE_LIST_NULL) )
			{
				p = p->Next;
			}
		}
		else
		{
			while( ( strcmp((const char *)(p->tNode.u8INVSerNum), (const char *)u8Sn) ) && (p != NODE_LIST_NULL) )
			{
				p = p->Next;
			}			
		}

		if( p != NODE_LIST_NULL )
		{
			if(NODE_LIST_NULL == p->Prev)//第一个
			{
				*ptList = p->Next;
				if(NODE_LIST_NULL != p->Next)
					p->Next->Prev = NODE_LIST_NULL;	
			}
			else
			{
				p->Prev->Next = p->Next;
				if(NODE_LIST_NULL != p->Next)
					p->Next->Prev = p->Prev;
			}
			free(p);
		}
		else
		{
			return	ERROR_NONEEXIST;
		}
	}
	else
	{
		return	ERROR_NONEEXIST;
	}

	return ERROR_NONE;
}

//根据地址获取序列号
pmbU8  pmb_GetSNWithAddress(tPSnListTypeDef *ptList, pmbU16 u16Address, pmbU8 *u8Sn)
{
	tPSnListTypeDef *p = ptList;
	pmbU8 u8Ret = 1;
		
	while(p != NODE_LIST_NULL)
	{
		if(p->tNode.u16INVAddress == u16Address)
		{
			memcpy(u8Sn ,p->tNode.u8INVSerNum, sizeof(p->tNode.u8INVSerNum));
			u8Ret = 0;
			break;	
		}
		p = p->	Next;
	}
	
	return u8Ret ;		
}

//根据序列号获取地址
pmbU8 pmb_GetAddressWithSn(tPSnListTypeDef *ptList, pmbU16 *u16Address, pmbU8 *u8Sn)
{
	tPSnListTypeDef *p = ptList;
	pmbU8 u8Ret = 1;

	while(p != NODE_LIST_NULL)
	{
		if(0 == strcmp((const char *)(p->tNode.u8INVSerNum), (const char *)u8Sn) )
		{
			*u16Address = p->tNode.u16INVAddress;
			u8Ret = 0;
			break;	
		}
		p = p->	Next;
	}
	
	return u8Ret ;					
}


//node address alloc 
void pmb_AllocateAddress(pmbU16 *u16Address)
{
	tPSnListTypeDef *pRtList = NODE_LIST_NULL;
	tPSnListTypeDef *pCtList = NODE_LIST_NULL;
	tPSnListTypeDef *pDtList = NODE_LIST_NULL;

	pCtList = tComNodeList;
	pRtList = tRegisterNodeList;
	pDtList = tRemovedNodeList;

	if(NODE_LIST_NULL != pDtList)
	{
		*u16Address = pDtList->tNode.u16INVAddress;
		pmb_DeleteFromNodeList(&tRemovedNodeList,pDtList->tNode.u16INVAddress, pmbNull);

	}
	else
	{
		//register not null
		while(NODE_LIST_NULL != pRtList)
		{
			*u16Address = pRtList->tNode.u16INVAddress + INV_ADDR( 1 );
			pRtList = pRtList->Next;
			if(pRtList == NODE_LIST_NULL)
				return;
		}
		
		while(NODE_LIST_NULL != pCtList)
		{
			*u16Address = pCtList->tNode.u16INVAddress + INV_ADDR( 1 );
			pCtList = pCtList->Next;
			if(pCtList == NODE_LIST_NULL)
				return;
		}
		*u16Address = INV_ADDR( 1 );	
	}
}


// make tx buff
pmbU8 pmb_MakeTxdBuff(tPktFmtTypeDef *ptPkg, pmbU8 *u8buf)
{
	pmbU8 u8TxdBuf[64];
	pmbU8 u8Offset = 0;
	pmbU8 Index = 0;

	u8TxdBuf[0] = GET_HEX16_HBYTE (ptPkg->u16Header) ;
	u8TxdBuf[1] = GET_HEX16_LBYTE (ptPkg->u16Header) ;
	
	u8TxdBuf[2] = GET_HEX16_HBYTE(ptPkg->u16SourceAddr);
	u8TxdBuf[3] = GET_HEX16_LBYTE(ptPkg->u16SourceAddr);

	u8TxdBuf[4] = GET_HEX16_HBYTE(ptPkg->u16DestAddr);
	u8TxdBuf[5] = GET_HEX16_LBYTE(ptPkg->u16DestAddr);

	u8TxdBuf[6] = ptPkg->u8ControlCode;
	u8TxdBuf[7] = ptPkg->u8FunCode;
	u8TxdBuf[8] = ptPkg->u8DataLen;

	u8Offset = 9 + ptPkg->u8DataLen;
	if(ptPkg->u8DataLen != 0)
	{
		memcpy(&u8TxdBuf[9], ptPkg->pu8Data, ptPkg->u8DataLen );
	}

	u8TxdBuf[u8Offset] = GET_HEX16_HBYTE(ptPkg->u16CheckSum);
	u8TxdBuf[u8Offset + 1] = GET_HEX16_LBYTE(ptPkg->u16CheckSum);
	
	memcpy(u8buf, u8TxdBuf, u8Offset + 2 );

	return ( u8Offset + 2 );

}





void pmb_MakeRxdFrame(pmbU8 *pu8Data, tPktFmtTypeDef *ptPkg)
{
	pmbU8 u8Offset = 0;

	ptPkg->u16Header = MAKE_HEX16(pu8Data[0], pu8Data[1]);
	ptPkg->u16SourceAddr = MAKE_HEX16(pu8Data[2], pu8Data[3]);
	ptPkg->u16DestAddr = MAKE_HEX16(pu8Data[4], pu8Data[5]);
	ptPkg->u8ControlCode = pu8Data[6];
	ptPkg->u8FunCode = pu8Data[7];
	ptPkg->u8DataLen = pu8Data[8];

	u8Offset = ptPkg->u8DataLen + 9;
	if(ptPkg->u8DataLen != 0)
	{
		ptPkg->pu8Data = malloc(ptPkg->u8DataLen);
		memcpy(ptPkg->pu8Data, &pu8Data[9],  ptPkg->u8DataLen );
	}
	ptPkg->u16CheckSum = MAKE_HEX16(pu8Data[u8Offset], pu8Data[u8Offset + 1]);   
	
}


void pmb_ParseRecvdFrame(tPktFmtTypeDef *ptPkg)
{
	
	//校验错误
	if( ptPkg->u16CheckSum != CheckSum16(ptPkg) )
		return;
	//帧头错误
	if( ptPkg->u16Header != PMB_HEADER )
		return;
	//主机地址错误
	if( ptPkg->u16DestAddr != gs_u16APAddress )
		return;
	
	switch(ptPkg->u8ControlCode)
	{
	case CC_REGISTER:
		pmb_ParseRegisterFunCode(ptPkg);	
		break;
	case CC_READ:
		pmb_ParseReadFunCode(ptPkg);
		break;
	case CC_WRITE:
		break;
	case CC_EXECUTE:
		break;
	default:
		break;
	}
}


//解析註冊相關數據幀
void pmb_ParseRegisterFunCode(tPktFmtTypeDef *ptPkg)
{
	pmbU16 u16Addr = 0;
	pmbU8 u8Sn[16] = { 0 }; 
	tNodeInfoTypeDef tNode;
	tPktFmtTypeDef tFrame;
	pmbU8 u8buf[32];

	switch(ptPkg->u8FunCode)
	{
	//received register request frame 
	//case( MASK_FUNCODE( tcQueryOffLine.u8FCode ) ):
	case FC_00_REPLAY:
		memcpy(u8Sn, ptPkg->pu8Data, 16);
		pmb_AllocateAddress(&u16Addr);//分配一個地址
		gs_u16INVAddress = u16Addr;
		pmb_SetNodeInfo(&tNode, u16Addr, STATE_REGISTER, u8Sn);
		pmb_AddToNodeList(&tRegisterNodeList, &tNode, LIST_TYPE_REGISTER);//添加到register链表
		//發送地址分配幀
		pmb_SendRegisterAddress(&tFrame, u16Addr, u8Sn);
		//pmb_MakeTxdBuff(&tFrame, u8buf);
		gs_tPmbSysInfo.u8TxLen = pmb_MakeTxdBuff(&tFrame, gs_tPmbSysInfo.u8TxBuf);		
		gs_tPmbSysInfo.SendInfo( UART_CHN_2, gs_tPmbSysInfo.u8TxBuf, gs_tPmbSysInfo.u8TxLen );	
		break;
	//received address confirm  frame 
	//case( MASK_FUNCODE( tcSendRegisterAddr.u8FCode ) ):
	case FC_01_REPLAY:
		if(0 == pmb_GetSNWithAddress(tRegisterNodeList, ptPkg->u16SourceAddr, u8Sn) )//獲取序列號
		{
			pmb_SetNodeInfo(&tNode, ptPkg->u16SourceAddr, STATE_NETTED, u8Sn);
			pmb_AddToNodeList(&tComNodeList, &tNode, LIST_TYPE_NETTED );//添加到netted链表
			pmb_DeleteFromNodeList(&tRegisterNodeList, ptPkg->u16SourceAddr, u8Sn);//根據序列號從register鏈表中刪除
			os_SigSetTimer(&gs_tPmbReadInfoTmr, &gs_tPmbTsk, OS_SIG_TASK_PMB_READDESCRIPTTMR,1000);//启动读定时器
		}
		break;
	//rceived removing confirm frame
	//case( MASK_FUNCODE( tcRemoveRegister.u8FCode ) ):
	case FC_02_REPLAY:
		if(0 == pmb_GetSNWithAddress(tComNodeList, ptPkg->u16SourceAddr, u8Sn) )
		{
			pmb_SetNodeInfo(&tNode, ptPkg->u16SourceAddr, STATE_REMOVED, u8Sn);
			pmb_AddToNodeList(&tRemovedNodeList, &tNode, LIST_TYPE_REMOVED);
			pmb_DeleteFromNodeList(&tComNodeList, ptPkg->u16SourceAddr, u8Sn);
		}
		break;
	default:
		break;
	}
}


//解析讀數據相關幀
void pmb_ParseReadFunCode(tPktFmtTypeDef *ptPkg)
{
	switch(ptPkg->u8FunCode)
	{
	//read description response 
	case FC_00_REPLAY:
		break;
	//read/write description response
	case FC_01_REPLAY:
		break;
	//query normal info response
	case FC_02_REPLAY:
		
		os_SigSet( &gs_tPmbTsk, OS_SIG_TASK_PMB_UPLOADTMR);//启动upload 
		break;
	case FC_03_REPLAY:
		break;
	default:
		break;
	}
	
}


void pmb_Init( void )
{ 
	uartCfg_Init(UART_CHN_2);
	uartCfg_EnableRx(UART_CHN_2,uartTrue);	
	
	gs_tPmbSysInfo.SendInfo = uartCfg_SendBytes;
	gs_tPmbSysInfo.RecvInfo = uartCfg_Usart2ReceiveBytes;
	
	os_TaskCreate( &gs_tPmbTsk, pmb_Tsk__, 5);

	os_TaskAddWakeupInit( &gs_tPmbTsk, pmb_Wakeup__ );
	os_TaskAddSleepDeInit( &gs_tPmbTsk, pmb_Sleep__ );

	os_TaskRun( &gs_tPmbTsk );

} /* pmb_Init() */


// main task process function
OS_TASK( pmb_Tsk__ )
{	
	tPSnListTypeDef **p = &tRegisterNodeList;
	
	if( OS_SIG_STATE_SET(OS_SIG_SYS) )
	{      
		uartCfg_Poll(UART_CHN_2);

		while(*p != NODE_LIST_NULL)
		{
			if( os_CheckTimeout(((*p)->u32Time + (*p)->u32Interval), (*p)->u32Time) )
			{
				(*p)->TimeOutProcess(p,(*p)->eListType);
			}
			*p = (*p)->Next;
		}		
		
		OS_SIG_CLEAR(OS_SIG_SYS);	
	}
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_PMB_OFFLINEQUERYTMR) )
	{
		pmb_OfflineQueryFrame(&gs_tSendFrm);
		gs_tPmbSysInfo.u8TxLen = pmb_MakeTxdBuff(&gs_tSendFrm, gs_tPmbSysInfo.u8TxBuf);		
		gs_tPmbSysInfo.SendInfo( UART_CHN_2, gs_tPmbSysInfo.u8TxBuf, gs_tPmbSysInfo.u8TxLen );
		
		os_SigSetTimer(&gs_tPmbOffLineQueryTmr, &gs_tPmbTsk, OS_SIG_TASK_PMB_OFFLINEQUERYTMR,5000);
		OS_SIG_CLEAR(OS_SIG_TASK_PMB_OFFLINEQUERYTMR);
	}
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_PMB_READDESCRIPTTMR) )
	{
		
		pmb_QueryInvReadDescription(&gs_tSendFrm);
		gs_tPmbSysInfo.u8TxLen = pmb_MakeTxdBuff(&gs_tSendFrm, gs_tPmbSysInfo.u8TxBuf);		
		gs_tPmbSysInfo.SendInfo( UART_CHN_2, gs_tPmbSysInfo.u8TxBuf, gs_tPmbSysInfo.u8TxLen );		
			
		os_SigSetTimer(&gs_tPmbReadInfoTmr, &gs_tPmbTsk, OS_SIG_TASK_PMB_READDESCRIPTTMR,60*500);
		os_SigSetTimer(&gs_tPmbReadInfoTmr, &gs_tPmbTsk, OS_SIG_TASK_PMB_READNORMALINFOTMR,1000);
		OS_SIG_CLEAR(OS_SIG_TASK_PMB_READDESCRIPTTMR);
	}
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_PMB_READNORMALINFOTMR) )
	{
		pmb_QueryInvNormalInfo(&gs_tSendFrm);
		gs_tPmbSysInfo.u8TxLen = pmb_MakeTxdBuff(&gs_tSendFrm, gs_tPmbSysInfo.u8TxBuf);		
		gs_tPmbSysInfo.SendInfo( UART_CHN_2, gs_tPmbSysInfo.u8TxBuf, gs_tPmbSysInfo.u8TxLen );	
					
		OS_SIG_CLEAR(OS_SIG_TASK_PMB_READNORMALINFOTMR);
	}

	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_PMB_PRASEFRAMETMR) )
	{
		pmb_MakeRxdFrame(gs_tPmbSysInfo.u8RxBuf, &gs_tRecvdFrm);
		pmb_ParseRecvdFrame(&gs_tRecvdFrm);
		OS_SIG_CLEAR(OS_SIG_TASK_PMB_PRASEFRAMETMR);
	}
	
	
	if( OS_SIG_STATE_SET(OS_SIG_TASK_PMB_UPLOADTMR) )
	{
		
		net_SendData(gs_u8Msg, sizeof(gs_u8Msg), netTrue);
		OS_SIG_CLEAR(OS_SIG_TASK_PMB_UPLOADTMR);
	}

    OS_SIG_REBACK();
}


	
void uartCfg_Usart2ReceiveBytes( uartU8  *a_pu8Data, uartU16 a_u16Length )
{
	memcpy(gs_tPmbSysInfo.u8RxBuf,a_pu8Data,a_u16Length);
	gs_tPmbSysInfo.u8RxLen = a_u16Length;
	os_SigSet(&gs_tPmbTsk, OS_SIG_TASK_PMB_PRASEFRAMETMR);
}

// main task wakeup function
osBool pmb_Wakeup__(void)
{
    pmb_Init__(pmbTrue);
    
    return osTrue;	
}
// main task sleep function
osBool pmb_Sleep__(void)
{
    pmb_Init__(pmbTrue);
    
    return osTrue;
}
// wake-sleep init function
void pmb_Init__( pmbBool a_bWake )
{
    if(pmbTrue == a_bWake)
    {
		GPIO_InitTypeDef GPIO_InitStructure;
		/* send control  high available */	 
		/* receive control  low available */
		GPIO_InitStructure.Pin = GPIO_PIN_0;	
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;	
		GPIO_InitStructure.Pull = GPIO_PULLUP;	
		__GPIOA_CLK_ENABLE();
		HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

		uartCfg_Init(UART_CHN_2);
		uartCfg_EnableRx(UART_CHN_2,uartTrue);
		
		os_SigSetTimer(&gs_tPmbOffLineQueryTmr, &gs_tPmbTsk, OS_SIG_TASK_PMB_OFFLINEQUERYTMR,2000);

    }
	
}





#endif

