/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    (C)2013 Semtech

Description: Timer objects and scheduling management

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#include "board.h"
#include "rtc-board.h"
#include "timer-board.h"



static bool LowPowerModeEnable = true;

/*!
 * This flag is used to make sure we have looped through the main several time to avoid race issues
 */
volatile uint8_t HasLoopedThroughMain = 0;

/*!
 * Timers list head pointer
 */
static TimerEvent_t *TimerListHead = NULL;
static TimerEvent_t *TimerWaitListHead = NULL;
static TimerHighEvent_t *TimerHighEventHead = NULL;
/*!
 * \brief Adds or replace the head timer of the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be become the new head
 * \param [IN]  remainingTime Remaining time of the previous head to be replaced
 */
static void TimerInsertNewHeadTimer( TimerEvent_t *obj, uint32_t remainingTime );

/*!
 * \brief Adds a timer to the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be added to the list
 * \param [IN]  remainingTime Remaining time of the running head after which the object may be added
 */
static void TimerInsertTimer( TimerEvent_t *obj, uint32_t remainingTime );

/*!
 * \brief Sets a timeout with the duration "timestamp"
 * 
 * \param [IN] timestamp Delay duration
 */
static void TimerSetTimeout( TimerEvent_t *obj );

/*!
 * \brief Check if the Object to be added is not already in the list
 * 
 * \param [IN] timestamp Delay duration
 * \retval true (the object is already in the list) or false  
 */
static bool TimerExists( TimerEvent_t *obj );

/*!
 * \brief Read the timer value of the currently running timer
 *
 * \retval value current timer value
 */
uint32_t TimerGetValue( void );


static inline void TimerFlushIsrReq( void );

void TimerSetLowPowerEnable( bool enable )
{
   LowPowerModeEnable = enable;
}

bool TimerGetLowPowerEnable( void )
{
    return LowPowerModeEnable;
}

void TimerInit( TimerEvent_t *obj, void ( *callback )( void ) )
{
    obj->Timestamp = 0;
    obj->ReloadValue = 0;
    obj->IsRunning = false;
    obj->Callback = callback;
    obj->Next = NULL;
}

void TimerStart( TimerEvent_t *obj )
{
    TimerTime_t curTime;
    //__disable_irq( );

    if( ( obj == NULL ) || ( TimerExists( obj ) == true ) )
    {
        //__enable_irq( );
        return;
    }

    curTime = TimerGetCurrentTime( );

    obj->Timestamp = curTime + obj->ReloadValue;
    obj->IsRunning = false;

    if( TimerListHead == NULL )
    {
        TimerInsertNewHeadTimer( obj, obj->Timestamp );
    }
    else 
    {
        if( obj->Timestamp < TimerListHead->Timestamp )
        {
            TimerInsertNewHeadTimer( obj, obj->Timestamp );
        }
        else
        {
             TimerInsertTimer( obj, obj->Timestamp );
        }
    }
    //__enable_irq( );
}

static void TimerInsertTimer( TimerEvent_t *obj, uint32_t remainingTime )
{
    TimerEvent_t* prev = TimerListHead;
    TimerEvent_t* cur = TimerListHead->Next;

    if( cur == NULL )
    { // obj comes just after the head
        prev->Next = obj;
        obj->Next = NULL;
    }
    else
    {
        while( prev != NULL )
        {
            if( cur->Timestamp > obj->Timestamp )      /* 如果新加Timer的时间戳在N-1与N Timer之间 */
            {
                prev->Next = obj;
                obj->Next = cur;
                break;
            }
            else
            {
                prev = cur;
                cur = cur->Next;
                if( cur == NULL )
                { // obj comes at the end of the list
                    prev->Next = obj;
                    obj->Next = NULL;
                    break;
                }
            }
        }
    }
}

static void TimerInsertNewHeadTimer( TimerEvent_t *obj, uint32_t remainingTime )
{
    TimerEvent_t* cur = TimerListHead;

    if( TimerListHead != NULL )
    {
        TimerListHead->IsRunning = false;
    }

    obj->Next = TimerListHead;
    obj->IsRunning = true;
    TimerListHead = obj;
    TimerSetTimeout( TimerListHead );
}

void TimerIrqHandler( void )
{
    uint32_t elapsedTime = 0;
    TimerTime_t curTime = 0;
    TimerEvent_t* elapsedTimer = NULL;
    TimerEvent_t *tail = NULL;
    TimerHighEvent_t *pHighEvt = NULL;
 
    while (NULL != TimerHighEventHead ){
        __disable_irq();
        pHighEvt = TimerHighEventHead;
        TimerHighEventHead = TimerHighEventHead->Next;
        __enable_irq();

        if ( NULL != pHighEvt->Callback ) {
            pHighEvt->Callback();
        }
    }


    TimerFlushIsrReq();

    if( LowPowerModeEnable == false )
    {
        if( TimerListHead == NULL )
        {
            return;  // Only necessary when the standard timer is used as a time base
        }
    }

    curTime = TimerGetCurrentTime( );

    /* not timeout yet */
    if( curTime < TimerListHead->Timestamp )
    {
        return ;
    }

    // save TimerListHead
    elapsedTimer = TimerListHead;

    // remove all the expired object from the list
    while( TimerListHead != NULL )
    {
        if ( curTime >= TimerListHead->Timestamp ){
            tail = TimerListHead;
            if( TimerListHead->Next != NULL )
            {
                TimerListHead = TimerListHead->Next;
            }
            else
            {
                TimerListHead = NULL;
            }
        }else{
            
            break;
        }
    }
    
    if( NULL != tail ){
        tail->Next = NULL;
    }

    // execute the callbacks of all the expired objects
    // this is to avoid potential issues between the callback and the object list
    while( elapsedTimer != NULL  )
    {
        TimerEvent_t* obj = elapsedTimer;
        elapsedTimer = elapsedTimer->Next;
        if( obj->Callback != NULL )
        {
            obj->Callback( );
        }

    }

    // start the next TimerListHead if it exists
    if( TimerListHead != NULL )
    {    
        TimerListHead->IsRunning = true;
        TimerSetTimeout( TimerListHead );
    } 
}

void TimerStop( TimerEvent_t *obj ) 
{
    //__disable_irq( );

    TimerEvent_t* prev = TimerListHead;
    TimerEvent_t* cur = TimerListHead->Next;

    // List is empty or the Obj to stop does not exist 
    if( ( TimerListHead == NULL ) || ( obj == NULL ) )
    {
        //__enable_irq( );
        return;
    }

    if( TimerListHead == obj ) // Stop the Head                                    
    {
        if( TimerListHead->IsRunning == true ) // The head is already running 
        {
            if( TimerListHead->Next != NULL )
            {
                TimerListHead->IsRunning = false;
                TimerListHead = TimerListHead->Next;
                TimerListHead->IsRunning = true;
                TimerSetTimeout( TimerListHead );
            }
            else
            {
                TimerListHead = NULL;
            }
        }
        else // Stop the head before it is started
        {     
            if( TimerListHead->Next != NULL )     
            {
                TimerListHead = TimerListHead->Next;
            }
            else
            {
                TimerListHead = NULL;
            }
        }
    }
    else // Stop an object within the list
    {    
        while( cur != NULL )
        {
            if( cur == obj )
            {
                prev->Next = cur->Next;
                break;
            }
            else
            {
                prev = cur;
                cur = cur->Next;
            }
        }
    }
    //__enable_irq( );
}    
    
static bool TimerExists( TimerEvent_t *obj )
{
    TimerEvent_t* cur = TimerListHead;

    while( cur != NULL )
    {
        if( cur == obj )
        {
            return true;
        }
        cur = cur->Next;
    }
    return false;
}

void TimerReset( TimerEvent_t *obj )
{
    TimerStop( obj );
    TimerStart( obj );
}

void TimerSetValue( TimerEvent_t *obj, uint32_t value )
{
    uint32_t minValue = 0;

    TimerStop( obj );

    value = RtcTimeToTick(value);

    minValue = RtcGetMinimumTimeoutTick( );
    
    if( value < minValue )
    {
        value = minValue;
    }

    obj->Timestamp = value;
    obj->ReloadValue = value;
}

uint32_t TimerGetValue( void )
{
    return RtcGetTimerElapsedTime( );
}

TimerTime_t TimerGetCurrentTime( void )
{
    return RtcGetTimerValue( );
}

static void TimerSetTimeout( TimerEvent_t *obj )
{
    HasLoopedThroughMain = 0;

    RtcSetTimeout( obj->Timestamp );
}

void TimerLowPowerHandler( void )
{
    if(( NULL == TimerWaitListHead ) && ( TimerListHead != NULL ) && ( TimerListHead->IsRunning == true ) )
    {
        if( HasLoopedThroughMain < 5 )
        {
            HasLoopedThroughMain++;
        }
        else
        {
            HasLoopedThroughMain = 0;
    
            /* TODO */
            if( LowPowerModeEnable == true )
            {
                RtcEnterLowPowerStopMode( );
            }
            else
            {
                //TimerHwEnterLowPowerStopMode( );
            }
        }
    }
}

void TimerStartInIsr( TimerEvent_t *obj )
{
    TimerEvent_t *TimerAddPosition = TimerWaitListHead;
    if ( NULL != obj ){
        obj->IsStart = true;
        if ( NULL == TimerWaitListHead ){
            TimerWaitListHead = obj;
            obj->IsrNext = NULL;
        }else {
            while ( NULL != TimerAddPosition->IsrNext ) {
                TimerAddPosition = TimerAddPosition->IsrNext;
                if( obj == TimerAddPosition ){  /* 如果已经存在，则直接退出 */
                    break;
                }
            }

            if ( obj != TimerAddPosition ){ /* 如果不存在，则说明是list tail */
                TimerAddPosition->IsrNext = obj;
                obj->IsrNext = NULL;
            }

        }


    }
}


void TimerStopInIsr( TimerEvent_t *obj )
{
    TimerEvent_t *TimerAddPosition = TimerWaitListHead;
    if ( NULL != obj ){
        obj->IsStart = false;
        if ( NULL == TimerWaitListHead ){
            TimerWaitListHead = obj;
            obj->IsrNext = NULL;
        }else {
            while ( NULL != TimerAddPosition->IsrNext ) {
                TimerAddPosition = TimerAddPosition->IsrNext;
                if( obj == TimerAddPosition ){  /* 如果已经存在，则直接退出 */
                    break;
                }
            }

            if ( obj != TimerAddPosition ){ /* 如果不存在，则说明是list tail */
                TimerAddPosition->IsrNext = obj;
                obj->IsrNext = NULL;
            }

        }


    }
}

static inline void TimerFlushIsrReq( void )
{
    TimerEvent_t *obj;

    while ( NULL != TimerWaitListHead ){
        obj = TimerWaitListHead;
        TimerWaitListHead = obj->IsrNext;

        if ( true == obj->IsStart ){
            TimerStart( obj );
        }else {
            TimerStop( obj );
        }
    }
}

void TimerAddHighEvent( TimerHighEvent_t *obj )
{
    TimerHighEvent_t *next = NULL;

    obj->Next = NULL;

    __disable_irq();
    if (NULL == TimerHighEventHead ){
        TimerHighEventHead = obj;
    }else{
        next = TimerHighEventHead->Next;
        while( NULL != next ){
            next = next->Next;
        }
        next = obj;
    }
    __enable_irq();
}
