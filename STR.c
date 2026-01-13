/**
	STR 
		- control the device
		- engine part.  note) not separate part.

	@project	HANSO INC.
	@author	tchan@TSoft
	@date	2019/12/24

*/
#ifdef	TEST_SIM				// for test configuration : without device.
#define		FT_TEST
#define		FT_NOERROR				// disable error det. for test.
#endif

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "putil.h"
#include "timer.h"
#include "STR.h"
#include "STRCBOptions.h"
#include "ds1307.h"
//#include "TempNTC.h"		//----------------------------------------------------

#define			TWVALVE1_ON()		_SET(PORTA, 7)		// high active
#define			TWVALVE1_OFF()		_CLR(PORTA, 7)
#define			TWVALVE2_ON()		_SET(PORTA, 6)
#define			TWVALVE2_OFF()		_CLR(PORTA, 6)
#define			TWVALVE3_ON()		_SET(PORTA, 5)
#define			TWVALVE3_OFF()		_CLR(PORTA, 5)
#define			TWVALVE4_ON()		_SET(PORTA, 4)
#define			TWVALVE4_OFF()		_CLR(PORTA, 4)

#define			DLOCK_ON()			_SET(PORTA, 3)
#define			DLOCK_OFF()			_CLR(PORTA, 3)

//#define			TWVALVE5_ON()		_SET(PORTA, 3)
//#define			TWVALVE5_OFF()		_CLR(PORTA, 3)

#if 0	// at type4.
#define			SVALVE1_ON()		_SET(PORTD, 5)
#define			SVALVE1_OFF()		_CLR(PORTD, 5)
#else
	// temporary use TWVALVE4 as SVALVE1. (2016/06/14)
#define			SVALVE1_ON()		TWVALVE4_ON()
#define			SVALVE1_OFF()		TWVALVE4_OFF()
#endif

#define			SVALVE2_ON()		_SET(PORTD, 6)
#define			SVALVE2_OFF()		_CLR(PORTD, 6)
#define			SVALVE3_ON()		_SET(PORTD, 7)
#define			SVALVE3_OFF()		_CLR(PORTD, 7)

#define			PUMP_ON()			_SET(PORTE, 5)
#define			PUMP_OFF()			_CLR(PORTE, 5)

#define			FAN_ON()			_SET(PORTE, 6)
#define			FAN_OFF()			_CLR(PORTE, 6)

#define			HV_ON()				_SET(PORTF, 7)
#define			HV_OFF()			_CLR(PORTF, 7)


// sensor input
#ifdef	HW_REV03
	// since HW_REV 3.0
	// DOOR moved to PG3 <- PB7.
#define		IS_DOOR_OPEN()	((PING & 0x08)!=0x00)
#else
	// pre HW_REV 3.0
#define		IS_DOOR_OPEN()	((PINB & 0x80)!=0x00)
#endif

#define		IS_P1_ON()		((PINB & 0x40)==0x00)		// low press sensor, ok
#define		IS_P2_ON()		((PINB & 0x20)==0x00)		// high press sensor, ok

#define		IS_WL_ON()		((PING & 0x02)==0x00)		// water level sensor, ok

#define		IS_FL_ON()		((PIND & 0x02)==0x00)		// flow sensor, ok

// 2017.01. plasma port change to PC6. SPARE1. low normal.
#define		IS_PLASMA_OK()	((PINC & 0x40)==0x00)		// plasma sensor is ok

//		if ((PINF & 0x04) == 4)				// ok.

// temp4 ?

#define		BUZZ_ON()				_CLR(PORTG, 0)
#define		BUZZ_OFF()				_SET(PORTG, 0)




//#ifdef	HW_REV03
#define		RUN_LED_ON()			_CLR(PORTE, 7)
#define		RUN_LED_OFF()			_SET(PORTE, 7)

#define		STOP_LED_ON()			_CLR(PORTD, 0)
#define		STOP_LED_OFF()			_SET(PORTD, 0)

#define		ERROR_LED_ON()			_CLR(PORTD, 4)
#define		ERROR_LED_OFF()			_SET(PORTD, 4)
//#endif


// power on, or door-close delay.
#define		INIT_DELAY				500UL				// 0.5 sec
//#define		WARMING_UP_DELAY		10000UL			// test 10 sec
#define		WARMING_UP_DELAY		500UL				// 0.5 sec 
#define		CHECKING_DELAY			500UL				// 0.5 sec


// call asap. : 2016/02/29.
static void ste_checkError();

// to check multiple state of door sensor
static void checkDoorSensor();
static int isDoorOpen();


//-----------------------------------------------------------------------------
// new engine.
//
typedef struct _device_control 
{
	unsigned int runTimeHms;	// 100ms runtime count. ex.10 = 1 sec. max 60000 = 6000 sec = 100 min.
	unsigned int sv2 : 1;		// SSR-02
	unsigned int sv3 : 1;		// SSR-03
	unsigned int hv : 1;		// SSR-04

	unsigned int tv1 : 1;		// SSR-05
	unsigned int tv2 : 1;		// SSR-06

	unsigned int tv3 : 1;		// SSR-07
	unsigned int sv1 : 1;		// SSR-08
	unsigned int pump : 1;		// SSR-09 water pump

	unsigned int sv1t : 1;		// sv1 toggle (was sv3)
	unsigned int fan : 1;		// fan motor

	unsigned int fs : 1;		// flow sensor
	unsigned int ps1 : 1;		// pressure sensor1 - Low Pressure sensor
	unsigned int ps2 : 2;		// pressure sensor1 - High Pressure sensor
	unsigned int wls : 1;		// liquid level sensor
	unsigned int ts : 1;		// temperature sensor
	unsigned int ts2 : 1;		// temperature sensor
	unsigned int ts3 : 1;		// temperature sensor

	unsigned int pls : 1;		// plasma sensor
	unsigned int rsv : 7;		// reserved (7)
}
DeviceControl;


typedef struct _ctrl_proc
{
	int numStep;
	DeviceControl pDC[];		// note) list of control data is defined only statically.
}
ControlProc;


/** Mode
	test start.
	test stop
*/

static int strState;
static int isTestCompleted = 0;	// self test completed ?
		// note) self-test completed : then no more self-test when start.

/**
	Current(last) mode

#define	MODE_SELF_TEST		0x01
#define	MODE_RUN			0x02
#define	MODE_DRYING			0x03
*/
static int configMode = MODE_RUN;		// default running-cycle.
static int configCycle = 2;		// default running-cycle.
static int runCycle = 0;		// current running cycle in progress

// v2016.08, update
static int configDryingCycle = 1;
static int runDryingCycle = 0;


//-- procedure run time counting variables.
static int prevTotalSeonds;	
static int totalRunSeconds;			// procedure running time in progress.

/**
	Run information
*/
RunInfo		runInfo;

/**
	See "Error code"
*/
static int errorCode;			// cause of process stop
static int currErrorCode;		// current state of error

static int tid_t1;				// general timer. -> start timer.
// usage) at ST_NONE, T error timeout (5 min)

static int tid_buzz;			// buzz timer.
static int tid_bto;				// buzz time out.

static int tid_op;				// timer 1 sec.

//static int currFlowCount = 0;	// flow sensor / sec.  previous value.
//static int flowCount = 0;		// flow sensor / sec. by intr.

//--- procedure running engine ----
static int engState = 0;
static int pTestFlag = 0;
static int tid_ct;				// engine control and stopping control
static int tid_s1ct;
static int tid_chk;				// sensor check timer

static ControlProc *currentProc = 0;
static DeviceControl currentCtrl;
static int nextRunStep = 0;

//
//static int currTemperature = 0;		//----------------------------------------------------

//--- error checking variables
static uint8 ps1ErrorTime = 0;					// error count in seconds
static uint8 ps2ErrorTime = 0;
static uint8 flowErrorTime = 0;
static uint8 plsErrorTime = 0;					// plasma sensor
static uint8 edCountWL = 0;						// error detect count for water level.
static uint8 edCountTemp1 = 0;					// error detect count for temperature
static uint8 edCountTemp2 = 0;
static uint8 edCountTemp3 = 0;

//--- buzzer control var.
#define	BUZZ_ERROR				0x90000000		// b_b_______
#define	BUZZ_COMPLETE			0x80000000		// b_________
#define	BUZZ_DOPEN				0xA8000000		// bbb_______

uint32 buzzProfile = 0;		// on-off bits in 100 ms interval. max 3.2 sec.
uint8	buzzIndex = 0;			// 0..31

//------------------------------------------------------------------------------


ControlProc selfTestProc =
{
	2,		//--------------------------------------------------제어 라인 숫자 확인!!!												
	{
	// Total run time :  min
	//   	  /-------Control---------/---Sensors------/
 	// SSR-No. M,D X, 2,3, 3,X,X,   
	// Control P,P,X  V,V  V,X,X, 0,0, F,L,H,W,T,   ,P,r\n");
	//   -No.  1,2,3, 4,5, 6,X,X, 0,0, s,p,p,s,s,2,3,s,r\n");
		{   5, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 1. 시작  
		{   5, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 2. 종료 (Time : 1 sec ) 
	}
};

ControlProc runProc =
{
	22,		//--------------------------------------------------제어 라인 숫자 확인!!!
	{
	// Total run time :  min
	//   	  /-------Control---------/---Sensors------/
 	// SSR-No. M,D X, 2,3, 3,X,X,   
	// Control P,P,X  V,V  V,X,X, 0,0, F,L,H,W,T,   ,P,r\n");
	//   -No.  1,2,3, 4,5, 6,X,X, 0,0, s,p,p,s,s,2,3,s,r\n");
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 1.  ----준  비-----
		{  30, 0,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 2.  ---상수 공급---     분사 노즐-1,2
		{ 250, 1,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 3.  ---상수 공급---     분사 노즐-1,2
		{  30, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 4.  ---상수 공급---     노즐-1/2 --> 3/4
		{ 250, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 5.  ---상수 공급---     분사 노들-3,4)
		{  30, 1,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 6.  ---------------
		{  10, 1,1,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 7.  ---------------
		{  10, 1,1,0, 1,1, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 8.  --세정제 공급--
		{ 150, 1,1,0, 1,1, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 9.  --분사 노즐-1,2)
		{  30, 1,1,0, 1,1, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 10. ---밸브 회전---
		{ 150, 1,1,0, 1,1, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 11. --분사 노즐-3,4)
		{  10, 1,0,0, 0,1, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 12. ---------------
		{  10, 0,0,0, 0,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 13. ---------------
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 14. ---------------
		{ 350, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 15. ---------------
		{  30, 0,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 16. ---------------
		{ 250, 1,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 17. 세척(Line-1,2)
		{  30, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 18. ---밸브 회전---
		{ 250, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 19. 세척(Line-3,4)
		{  10, 0,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 20. ---------------
		{  10, 0,0,0, 0,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 21. ---------------
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 22. 종료 (Time : 200s / 03:20) 
	}
};


ControlProc dryingProc =
{
	10,		//--------------------------------------------------제어 라인 숫자 확인!!!
	{
	// Total run time :  min
	//   	  /-------Control---------/---Sensors------/
 	// SSR-No. M,D X, 2,3, 3,X,X,   
	// Control P,P,X  V,V  V,X,X, 0,0, F,L,H,W,T,   ,P,r\n");
	//   -No.  1,2,3, 4,5, 6,X,X, 0,0, s,p,p,s,s,2,3,s,r\n");
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 1.--------------	
		{ 500, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 2.--------------	
		{  30, 0,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 3.--------------
		{ 250, 1,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 4. 세척(Line-1,2)
		{  30, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 5.--------------
		{ 250, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 6. 세척(Line-3,4)
		{  30, 1,0,0, 0,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 7. --------------
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 8. --------------
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 9. --------------
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 10. 종료 	(Time : 130s / 02:10 ) 
	}  //    , 3,1,0, 6,2, 8,0,0,------------------------------동작횟수
};                      
 

                        
ControlProc errFinishProc =
{  
	8,
	{
	// Total run time :  min
	//   	  /-------Control---------/---Sensors------/
 	// SSR-No. M,D X, 2,3, 3,X,X,   
	// Control P,P,X  V,V  V,X,X, 0,0, F,L,H,W,T,   ,P,r\n");
	//   -No.  1,2,3, 4,5, 6,X,X, 0,0, s,p,p,s,s,2,3,s,r\n");
		{  50, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 1.--------------	
		{  30, 0,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 2. --------------
		{ 150, 1,0,0, 1,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 3. 세척(Line-1,2)
		{  30, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 4.--------------
		{ 150, 1,0,0, 1,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 5. 세척(Line-3,4)
		{  30, 0,0,0, 0,0, 1,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 6. --------------
		{  20, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 7. --------------
		{  10, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0},	// 8. 종료 	(Time  75s / 01:15 ) 	
	}
};
                    
 
                     
// Press Test Control : only TV1, doorlock and door sensor.
                    

							// Total run time :  min
							//   	  /-------Control---------/---Sensors------/
 							// SSR-No. 2,3 4, 5,6, 7,8,10,   
							// Control S,S,S  S,S  V,P,W, 0,0, F,L,H,W,T,   ,P,r\n");
							//   -No.  1,2,3, 4,5, P,L,P, 0,0, s,p,p,s,s,2,3,s,r\n");
							// step 1.
DeviceControl	pTestControl =  { 600, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0};
 
DeviceControl	pTestStop	 = 	{   1, 0,0,0, 0,0, 0,0,0, 0,0, 0,0,0,0,0,0,0,0};

//------------------------------------------------------------------------------


int str_ControlDev(DeviceControl dc)
{
//	DPRINTF("--: **** TV[]      SV  P,F,H, F,s,s\n");
//	DPRINTF("--: **** 1,2,3,4,5 1,2 P,F,H, F,1,2\n");
//	DPRINTF("--: **** 1,2,3, 4,5,6, 7,8,9  F,1,2\n");
	DPRINTF("--: %4dHms %d,%d,%d, %d,%d, %d,%d,%d, %d,%d,%d\n",
		dc.runTimeHms, 
		dc.sv2, dc.sv3, dc.hv,
		dc.tv1, dc.tv2, 
		dc.tv3, dc.sv1, dc.pump, 
		dc.sv1t, dc.fan,  
		dc.fs, dc.ps1, dc.ps2);


	if (dc.tv1)	TWVALVE1_ON();
	else		TWVALVE1_OFF();

	if (dc.tv2)	TWVALVE2_ON();
	else		TWVALVE2_OFF();

	if (dc.tv3)	TWVALVE3_ON();
	else		TWVALVE3_OFF();

//	if (dc.tv4)	TWVALVE4_ON();
//	else		TWVALVE4_OFF();

//	if (dc.tv5)	TWVALVE5_ON();
//	else		TWVALVE5_OFF();

	if (dc.sv1)	SVALVE1_ON();
	else		SVALVE1_OFF();

	if (dc.sv2)	SVALVE2_ON();
	else		SVALVE2_OFF();

	if (dc.sv3)	SVALVE3_ON();
	else		SVALVE3_OFF();

	if (dc.pump)	PUMP_ON();
	else			PUMP_OFF();

	if (dc.fan)		FAN_ON();
	else			FAN_OFF();

	if (dc.hv)		HV_ON();
	else			HV_OFF();

	// copy current control to check.
	// note) used at stopping procedure.
	currentCtrl = dc;

	return 0;
}


//-----------------------------------------------------------------------------
// procedure engine part
//

/**
	Reset Proc engine
	- when completed, init for next procedure run.
*/
static void ste_reset()
{
	engState = 0;			// reset engine state
	currErrorCode = 0;

	// should clear error checking state.
	ps1ErrorTime = 0;
	ps2ErrorTime = 0;
	flowErrorTime = 0;
}

/**
	Engine is completed current proc ?
*/
static int ste_isComplete()
{
	return (engState == 4);
}


/**
	@param	state	engine state
	@param	step	current step
	@param	totalStep	procedure total step
*/
static void ste_event(int state, int step, int totalStep)
{
	//
	DPRINTF("EST:%d %d/%d\n", state, step, totalStep);

	// notify user
	str_step_event(strState, runCycle, step, totalStep);
}

static int ste_start(ControlProc *proc)
{
	if (engState == 0)	// ready
	{
		currentProc = proc;
		nextRunStep = 0;

		DPRINTF("eng start\n");
		engState = 1;
		
		//runSeconds = 0;		=> moved to str.

//		DPRINTF("--: ****    TV[]       SV[]   P,F,H, F,s,s\n"); (변경됨)
//		DPRINTF("--: ****Hms 1,2,3,4,5, 1,2,3, P,F,H, F,1,2\n");
		return 0;
	}
	else
	{
		return -1;
	}
}

/**
	Just stop engine.
	other (control) is handled by str_.

	note) don't clear error code.
*/
static void ste_stop()
{
	timer_clear(tid_ct);
	engState = 0;

	currentProc = 0;	// dummy.
	nextRunStep = 0;	// dummy.
}

static int ste_process()
{
	switch (engState)
	{
	case 0 : // ready
		//ste_event(engState, nextRunStep, currentProc->numStep);
		break;

	case 1 : // running : start fetch and run
		{
			// event.
			ste_event(engState, nextRunStep, currentProc->numStep);

			str_ControlDev(currentProc->pDC[nextRunStep]);
			timer_set(tid_ct, currentProc->pDC[nextRunStep].runTimeHms * 100L);

			nextRunStep++;
			engState = 2;
		}
		break;

	case 2 : // waiting
		{
			//-- SolValve #2 : toggling process (사용안함)
			static int s1tFlag = 0;
			//--

			if (timer_isfired(tid_ct))
			{
				timer_clear(tid_ct);
				engState = 3;

				s1tFlag = 0;		// for next.

				if (currentCtrl.sv1t == 1)
				{
					SVALVE1_OFF();	// make sure sv1 off when exit.
					DPRINTF("S1 OFF Exit\n");
				}
			}
			else
			{
				//------------------------
				// SolValve #2 : toggling process
				//
				if (currentCtrl.sv1t == 1)
				{
					if (s1tFlag == 0)		// non-ctrl state
					{
						// start toggling after 500 ms.
						s1tFlag = 2;
						timer_set(tid_s1ct, 500);
					}
					else if (s1tFlag == 1)		// off state
					{
						if (timer_isfired(tid_s1ct))
						{
							SVALVE1_ON();
							// on
							s1tFlag = 2;

							timer_set(tid_s1ct, 1000);		// 200 ms on.
							DPRINTF("S1 ON");
						}
					}
					else	// s1tFlag == 2.
					{
						if (timer_isfired(tid_s1ct))
						{
							SVALVE1_OFF();
							s1tFlag = 1;	// off.

							timer_set(tid_s1ct, 5000);		// wait 1800 ms to on.
							DPRINTF(" OFF\n");
						}
					}
				}
				else
				{
					s1tFlag = 0;		// for next.
				}
			}

			// check error. do at str_process()
		}
		break;

	case 3 : // go next or end
		{
			if (nextRunStep < currentProc->numStep)
			{
				engState = 1;
			}
			else
			{
				engState = 4;
				ste_event(engState, nextRunStep, currentProc->numStep);
			}
		}
		break;

	case 4 : // completed
		{
			// wait STR module clear completed state, using ste_reset()
		}
		break;
	}
	//end switch.

	if (timer_isfired(tid_op))
	{
		timer_set(tid_op, 1000);

		// run timer.
		//if ((engState > 1) && (engState < 4))
		if ((engState == 2) || (engState == 3) || (pTestFlag == 1))
		{
			totalRunSeconds++;
		}
	}

	// call asap : internally control timing.
	ste_checkError();

	return 0;
}

/**
	Check error while running.

	- temperature sensor error
	- flow alaram
	- pressure alarm

	note) assumed to call 1 sec interval
	note) only single error detection..
*/
static void ste_checkError()
{
	static int tcnt = 0;

	// always : check water level
	currErrorCode = 0;

#ifdef FT_NOERROR
	return;
#endif

	if (timer_isfired(tid_chk))
	{
		timer_set(tid_chk, 100);

		tcnt++;		// 0..9
		if (tcnt > 9) tcnt = 0;
	}
	else
	{
		return;
	}

	// below runs per 1 sec

	if (currentCtrl.wls)
	{
		if (IS_WL_ON())		// is error.
		{
			if (edCountWL < 7)
			{
				edCountWL++;
			}
			else	// if (edCountWL >= 7)
			{
				currErrorCode |= ERR_WL;

				return;
			}
		}
		else
		{
		//	currErrorCode &=~ERR_WL;
			edCountWL = 0;
		}
	}

	// sensor check : 1 sec interval - block.
	// : ps1,ps2,fs.

	if (tcnt == 0)
	{
		if (currentCtrl.ps1)	// low pressure sensor
		{
			// 7 sec. margin to check vaccum..
			if (IS_P1_ON())		// low pressure (ok)
			{
			//	currErrorCode &=~ERR_P1;
				ps1ErrorTime = 0;
			}
			else
			{
				// pressure not low for 7 seconds then error.
				if (ps1ErrorTime < 7)
				{
					ps1ErrorTime++;
				}
				else	// if (ps1ErrorTime >= 7)
				{
					currErrorCode |= ERR_P1;
				}
			}
		}
		else
		{
			ps1ErrorTime = 0;
		}

		if (currentCtrl.ps2)	// high pressure sensor
		{
			if (currentCtrl.ps2 == 1)
			{
				// 7 sec. margin to check pressure
				if (IS_P2_ON())		// high pressure (ok)
				{
				//	currErrorCode &=~ERR_P2;
					ps2ErrorTime = 0;
				}
				else
				{
					// pressure not high for 7 seconds then error.
					if (ps2ErrorTime < 7)
					{
						ps2ErrorTime++;
					}
					else	// if (ps2ErrorTime >= 7)
					{
						currErrorCode |= ERR_P2;
						return;
					}
				}
			}
			else if (currentCtrl.ps2 == 2)	// error if high pressure sensor off.
			{
				// 7 sec. margin to check pressure
				if (!IS_P2_ON())		// high pressure (ok)
				{
					ps2ErrorTime = 0;
				}
				else
				{
					// pressure not high for 7 seconds then error.
					if (ps2ErrorTime < 7)
					{
						ps2ErrorTime++;
					}
					else	// if (ps2ErrorTime >= 7)
					{
						currErrorCode |= ERR_P2;
						return;
					}
				}
			}
		}
		else
		{
			ps2ErrorTime = 0;
		}


		if (currentCtrl.fs)
		{
			// 7 sec. margin to check flow..
			if (IS_FL_ON())						// flow is ok.
			{
				flowErrorTime = 0;
				//currErrorCode &=~ERR_P2;
			}
			else
			{
				if (flowErrorTime < 5)
				{
					flowErrorTime++;
				}
				else	// if (flowErrorTime >= 7)
				{
					currErrorCode |= ERR_FLOW;
					return;
				}
			}
		}
		else
		{
			flowErrorTime = 0;
		}


		if (currentCtrl.pls)
		{
			// 7 sec. margin to check flow..
			if (IS_PLASMA_OK())
			{
				plsErrorTime = 0;
			}
			else
			{
				if (plsErrorTime < 5)
				{
					plsErrorTime++;
				}
				else
				{
					currErrorCode |= ERR_PLASMA;
					return;
				}
			}
		}
		else
		{
			plsErrorTime = 0;
		}
	}
	// end of 1 sec block.

	if (currentCtrl.ts)
	{
		if ((PINF & 0x01) == 1)				// ok.
		{
			edCountTemp1 = 0;
		}
		else
		{
			if (edCountTemp1 < 5)
			{
				edCountTemp1++;
			}
			else
			{
				currErrorCode |= ERR_T1;
				return;
			}
		}

		if ((PINF & 0x02) == 2)				// ok.
		{
			edCountTemp2 = 0;
		}
		else
		{
			if (edCountTemp2 < 5)
			{
				edCountTemp2++;
			}
			else
			{
				currErrorCode |= ERR_T2;
				return;
			}
		}

		if ((PINF & 0x04) == 4)				// ok.
		{
			edCountTemp3 = 0;
		}
		else
		{
			if (edCountTemp3 < 5)
			{
				edCountTemp3++;
			}
			else
			{
				currErrorCode |= ERR_T3;
				return;
			}
		}
	}
	else
	{
		edCountTemp1 = 0;
		edCountTemp2 = 0;
		edCountTemp3 = 0;
	}	


	// 
//	currTemperature = ntc_getTemperature();		//---------------------------------

//	if ((currTemperature < -20) || (currTemperature > 70))
//	{
//		currErrorCode = ERR_T4;
//	}


	// check temperature sensor alarm ...
}
// end engine
//-----------------------------------------------------------------------------

// to check multiple state of door sensor
static uint8 doorState = 0;		// 1 : open, 0: closed.

static void checkDoorSensor()
{
	static uint8 count = 0;

	if (doorState == 0)		// closed..
	{
		if (IS_DOOR_OPEN())
		{
			if (count++ > 5)
			{
				doorState = 1;	// open.
				count = 0;
			}
		}
		else
		{
			count = 0;
		}
	}
	else	// open.
	{
		if (IS_DOOR_OPEN())
		{
			count = 0;
		}
		else
		{
			if (count++ > 5)
			{
				doorState = 0;	// open.
				count = 0;
			}
		}
	}
}

static int isDoorOpen()
{
	return doorState;
}


/**
	init module / interface

	note) HW_REV03 only, 2016/06/14.
*/
int str_init()
{
	// PA7,6,5,4,3 - 3WAY Valve 1,2,3,4,5
	DDRA |= 0xF8;		// high active

	// PE5 - TUBE_PUMP	- high active
	// PE6 - FAN_MOTOR	- high active
	// PE7 - RUN_LED	- low active
	DDRE |= 0xE0;

	// PD1 - FLOW INPUT

	// PD5,6,7 - SOL 1,2,3
	DDRD |= 0xE0;		// high active
	// PD5,6,7 - SOL 1,2,3; high active
	// PD4 - ERROR_LED
	// PD0 - STOP_LED
	DDRD |= 0xF1;

	// PB7 - DOOR_INPUT
	// PB6,5 - PRESS SENSOR - 1,2.

	// PC0,1 - CONTACT 1,2  - low active => to Key.

	// PC[3-7] => LCD.

	// PF0,1,2 - TEMP 1,2,3

	// Plasma sensor : PC6.
	//PORTC |= 0x40;		// internal pull-up. 20170220 -> cancel. don't pull-up.
	DDRC &=~0x40;

	// PF4,5 - TIME_SDA,SDL => DS1307.

	// PF7 - HV: H.P.G_CNT
	DDRF |= 0x80;

	// PG0 - buzz
	PORTG |= 0x01;	// buzz off.
	DDRG |= 0x01;
	// PG1,3 - input. water-in, door-input.

	// adc init. for PF0,1,2.
	//ADCSRA	= 0x87;// ADEN(7)	// only enable:, ADSC(6), ADFR(5).

	RUN_LED_OFF();
	STOP_LED_OFF();
	ERROR_LED_OFF();

	tid_t1 = timer_alloc();		// process general timer

	tid_buzz = timer_alloc();
	timer_set(tid_buzz, 1000);
	tid_bto = timer_alloc();

	tid_ct = timer_alloc();			// engine control timer
	tid_s1ct = timer_alloc();		// engine control timer for sv1 toggle
	tid_op = timer_alloc();			// engine timer <- flow sensor check
	tid_chk = timer_alloc();		// sensor check

	timer_set(tid_op, 1000);
	timer_set(tid_chk, 100);


	currentCtrl.ps1 = 0;
	currentCtrl.ps2 = 0;
	currentCtrl.fs = 0;

	// after warming up, check wls and temp sensor.
	strState = ST_NONE;
	timer_set(tid_t1, INIT_DELAY);	// 3 sec delay

	return 0;
}

/**
	Go ready for test

	note) test only
*/
int str_goReady()
{
	if (strState == ST_NONE)
	{
		strState = ST_READY;
		isTestCompleted = 1;
	}
	else if (strState == ST_COMPLETE)
	{
		strState = ST_READY;
		isTestCompleted = 1;

		// stopping buzz.
		buzzProfile = 0;
		buzzIndex = 0;
	}
	return 0;
}


/**
	Reset state

	note) only valid when COMPLETE, STOPPED, ERROR
*/
int str_reset()
{
	// error,
	if ( (strState == ST_COMPLETE)
		|| (strState == ST_ERROR) )
	{
		// clear error code
		errorCode = 0;

		prevTotalSeonds = 0;
		totalRunSeconds = 0;

		runInfo.runDate.year = 0;

		ste_reset();		// dummy, engine must be stopped already.

		currentCtrl.wls = 1;
		currentCtrl.ts = 1;
		timer_set(tid_t1, 10000UL);		// check tempearture again...
		
		// clear buzzer.
		buzzProfile = 0;		// buzz none.
		buzzIndex = 0;

		strState = ST_NONE;

		return 0;
	}
	else
	{
		return -1;			// invalid call.
	}
}

/**
	Start self test

	note) At READY state, for test only.
*/
int str_startSelfTest()
{
	if ( (strState == ST_NONE)
		|| (strState == ST_READY) )
	{
		if (isDoorOpen())
		{
			return -1;			// error cannot start.
		}

		// initiate self-test.
		if (ste_start(&selfTestProc) == 0)
		{
			//DPRINTF("engine start ok\n");
			strState = ST_STRUN;

			configMode = MODE_SELF_TEST;
			configCycle = 1;

			totalRunSeconds = 0;		// reset run seconds.
			prevTotalSeonds = 0;

			runInfo.mode = MODE_SELF_TEST;
			runInfo.runCycles = 0;

			runInfo.firstStepSecs = 0;
			runInfo.secondStepSecs = 0;
			runInfo.finishStepSecs = 0;
			runInfo.totalSeconds = 0;

			DS1307_date ddate;
			ds1307_read_date(&ddate);
			runInfo.runDate.year	 = ddate.year;
			runInfo.runDate.month 	 = ddate.month;
			runInfo.runDate.date	 = ddate.day;

			DS1307_time dtime;
			ds1307_read_time(&dtime);
			runInfo.startTime.hour	 = dtime.hour;
			runInfo.startTime.minute = dtime.minute;
			runInfo.startTime.second = dtime.second;

			runInfo.errorCode = 0;

			DLOCK_ON();
		}
		else
		{
			DPRINTF("ste_start error\n");
			return -1;
		}
	}
	else
	{
		return -2;
	}
	return 0;
}

/**
	Start finishing/drying mode only
*/
int str_startDrying()
{
	if ( (strState == ST_NONE)
		|| (strState == ST_READY) )
	{
		if (isDoorOpen())
		{
			return -1;			// error cannot start.
		}

		// initiate finishing mode
		if (ste_start(&dryingProc) == 0)
		{
			//DPRINTF("engine start ok\n");
			strState = ST_SDRYING;

			configMode = MODE_DRYING;
			configCycle = 1;

			totalRunSeconds = 0;		// reset run seconds.
			prevTotalSeonds = 0;

			runInfo.mode = MODE_DRYING;		// finishing
			runInfo.runCycles = 1;

			runInfo.firstStepSecs = 0;
			runInfo.secondStepSecs = 0;
			runInfo.finishStepSecs = 0;
			runInfo.totalSeconds = 0;

			DS1307_date ddate;
			ds1307_read_date(&ddate);
			runInfo.runDate.year	 = ddate.year;
			runInfo.runDate.month 	 = ddate.month;
			runInfo.runDate.date	 = ddate.day;

			DS1307_time dtime;
			ds1307_read_time(&dtime);
			runInfo.startTime.hour	 = dtime.hour;
			runInfo.startTime.minute = dtime.minute;
			runInfo.startTime.second = dtime.second;

			runInfo.errorCode = 0;

			DLOCK_ON();
		}
		else
		{
			DPRINTF("ste_start error\n");
			return -1;
		}
	}
	else
	{
		return -2;
	}
	return 0;
}


/**
	Start pressure test mode
*/
int str_startPTesting()
{
	if ( (strState == ST_NONE)
		|| (strState == ST_WARMUP)
		|| (strState == ST_CHECKING)
		|| (strState == ST_READY) )
	{
		configMode = MODE_PTESTING;
		configCycle = 1;

		str_ControlDev(pTestControl);

		pTestFlag = 1;
		totalRunSeconds = 0;

		strState = ST_PTESTING;

		DLOCK_OFF();
	}
	else
	{
		return -2;
	}
	return 0;
}

/**
	Stop run
	note) v1.1, 10/17. only for PTESTING.
*/
int str_stopPTesting()
{
	if (strState == ST_PTESTING)
	{
		// clear device state...
		errorCode = ERR_STOP;			//		; stop by user.
		runInfo.errorCode = errorCode;

		strState = ST_STOPPING;
		str_event(strState, 0);

		pTestFlag = 0;
		return 0;
	}
	return -1;
}

/**
	Get current running state
*/
int str_getState()
{
	return strState;
}

/**
	Current/Last run mode
*/
int str_getConfigMode()
{
	return configMode;
}

/**
	Current configured cycle.
	note) valid after start., default is 2.
*/
int str_getConfigCycle()
{
	return configCycle;
}


/**
	Get current running cycle

	1..n
*/
int str_getRunningCycle()
{
	return runCycle;
}

/**
	Procedure running time in seconds.
	60000.
*/
unsigned int str_getRunSeconds()
{
	return totalRunSeconds;
}

/**
	Error code
	note) Error code caused to error-finishing or stop
*/
int str_getErrorCode()
{
	return errorCode;
}


/**
	Get current running state

	#define	ERR_P1		0x01			// low pressure
	#define	ERR_P2		0x02			// high pressure
	#define	ERR_FLOW	0x03			// no flow
	#define	ERR_T1		0x04			// temperature 1
	#define	ERR_T2		0x05			// temperature 2
	#define	ERR_T3		0x06			// temperature 3
	#define	ERR_D1		0x07			// door open
	#define	ERR_STOP	0x10			// stop by user

	#define	ERR_S0		0x20			// internal error case


	note) max 10 char.
*/
const char *str_getErrorString(int errCode)
{
	switch (errCode)
	{
	case	0x01 : return "LOW PRES";	// low pressure
	case	0x02 : return "HIGH PRESS";	// high pressure
	case	0x03 : return "FLOW";		// no flow
	case	0x04 : return "TEMP1";		// temperature 1
	case	0x05 : return "TEMP2";		// temperature 2
	case	0x06 : return "TEMP3";		// temperature 3
//	case	0x11 : return "OVER TEMP";	// temperature 4		-------------------------------
	case	0x07 : return "H.P.L-LOW";	// water level low
	case	0x08 : return "DOOR OPEN";	// door open
	case	0x09 : return "PLASMA";		// plasma error
	case	0x10 : return "USER STOP";	// stop by user
	default:
		return "N/A";	// not available
	}
}

RunInfo *str_getRunInfo()
{
	return &runInfo;
}

/**
	Test only.
*/
void str_makeReady()
{
	if (strState == ST_NONE)
	{
		strState = ST_READY;
	}
}

/**
	Start run

	@param	cycle	2,4,6
		1 Level : 2-cycle mode, 40 min
		2 Level : 4-cycle mode, 70 min
		3 Level : 6-cycle mode, 100 min
*/
int str_start(int cycle)
{
	if ((cycle < 1) || (cycle > 12))
	{
	//	DPRINTF("invalid cycle : %d\n", cycle);
		return -1;
	}

	if (strState != ST_READY)
	{
		return -1;
	}

	configMode = MODE_RUN;
	configCycle = cycle;

	// 2016.08 update
	if (configCycle <= 8)	configDryingCycle = 1;
	else					configDryingCycle = 2;

	if (ste_start(&runProc) == 0)
	{
		//DPRINTF("engine start ok\n");
		strState = ST_RUN;

		runCycle = 1;
		totalRunSeconds = 0;		// reset run seconds.
		prevTotalSeonds = 0;

		// save run info.
		runInfo.mode = MODE_RUN;
		runInfo.runCycles = configCycle;

		runInfo.firstStepSecs = 0;
		runInfo.secondStepSecs = 0;
		runInfo.finishStepSecs = 0;
		runInfo.totalSeconds = 0;

		DS1307_date ddate;
		ds1307_read_date(&ddate);
		runInfo.runDate.year	 = ddate.year;
		runInfo.runDate.month 	 = ddate.month;
		runInfo.runDate.date	 = ddate.day;

		DS1307_time dtime;
		ds1307_read_time(&dtime);

		runInfo.startTime.hour	 = dtime.hour;
		runInfo.startTime.minute = dtime.minute;
		runInfo.startTime.second = dtime.second;

		runInfo.errorCode = 0;

		DLOCK_ON();
	}
	else
	{
		DPRINTF("ste_start error\n");
		return -1;
	}

	return 0;
}



/**
	For dummy error processing.
*/
static void dummyErrorStop()
{
	errorCode = 0x20;	// internal error : not possible
	runInfo.errorCode = errorCode;

	ste_stop();
	ste_reset();					// clear error checking var.
	strState = ST_STOPPING;
}

/**
	Stop run by user key.

	Note) only when ST_RUN state
*/
int str_userStop()
{
	if (strState == ST_RUN)
	{
		// go efinish by user-stop
		errorCode = ERR_STOP;
		runInfo.errorCode = errorCode;

		currErrorCode = 0;				// clear error code.

		ste_stop();
		ste_reset();					// clear error checking var.

		runInfo.firstStepSecs = totalRunSeconds;
		prevTotalSeonds = totalRunSeconds;

		// start error finishing process
		if (ste_start(&errFinishProc) == 0)
		{
			DPRINTF("go finish by error(%04u)\n", errorCode);
			strState = ST_EFINISH;

			str_event(strState, 0);
		}
		else
		{
			dummyErrorStop();

			return -1;
		}
	}

	return 0;
}


/**
	param 1 : as firstStep..
		
*/
static void updateRunInfoEndTime(int step)
{

	// end of single Self-Test mode run.
	if (step == 1)
	{
		runInfo.firstStepSecs = totalRunSeconds;
		prevTotalSeonds = totalRunSeconds;
	}
	else if (step == 2)
	{
		runInfo.secondStepSecs = totalRunSeconds - prevTotalSeonds;
		prevTotalSeonds = totalRunSeconds;	
	}
	else
	{
		runInfo.finishStepSecs = totalRunSeconds - prevTotalSeonds;
		prevTotalSeonds = totalRunSeconds;	
	}

	runInfo.totalSeconds = totalRunSeconds;

	// end time.
	DS1307_time dtime;
	ds1307_read_time(&dtime);

	runInfo.endTime.hour   = dtime.hour;
	runInfo.endTime.minute = dtime.minute;
	runInfo.endTime.second = dtime.second;
}

static void goErrorFinishing(int step)
{
	// go efinish by error.
	errorCode = currErrorCode;
	runInfo.errorCode = errorCode;

	currErrorCode = 0;				// clear error code.

	ste_stop();
	ste_reset();					// clear error checking var.


	if (step == 1)
	{
		// 1st step go error finishing
		runInfo.firstStepSecs = totalRunSeconds;
		prevTotalSeonds = totalRunSeconds;
	}
	else	// if step == 2
	{
		// 2nd step go error finishing
		runInfo.secondStepSecs = totalRunSeconds - prevTotalSeonds;
		prevTotalSeonds = totalRunSeconds;
	}

	// start error finishing process
	if (ste_start(&errFinishProc) == 0)
	{
		//DPRINTF("go finish by error(%04u)\n", errorCode);
		strState = ST_EFINISH;

		str_event(strState, 0);
	}
	else
	{
		dummyErrorStop();
	}
}

/**
	// go stopping by door error
*/
static void goErrorStopping(int step, int err)
{
	errorCode = err;
	runInfo.errorCode = errorCode;

	buzzProfile = BUZZ_ERROR;
	buzzIndex = 0;

	if (step == 1)		runInfo.firstStepSecs = totalRunSeconds;
	else if (step == 2)	runInfo.secondStepSecs = totalRunSeconds - prevTotalSeonds;
	else				runInfo.finishStepSecs = totalRunSeconds - prevTotalSeonds;

	ste_stop();
	ste_reset();					// clear error checking var.

	strState = ST_STOPPING;
}


// call by 100ms interval.
//
static void updateStateLed()
{
	static int cnt = 0;

	if (cnt < 9)	cnt++;
	else			cnt = 0;

	switch (strState)
	{
	case ST_NONE:
	case ST_WARMUP:
	case ST_CHECKING:
	case ST_READY:
	case ST_COMPLETE:
		{
			RUN_LED_OFF();
			STOP_LED_OFF();
			ERROR_LED_OFF();
		}
		break;

	case ST_STRUN:			// self-test running
	case ST_RUN:
	case ST_SDRYING:
	case ST_DRYING:
	case ST_PTESTING:
		{
			RUN_LED_ON();
			STOP_LED_OFF();
			ERROR_LED_OFF();
		}
		break;
	case ST_EFINISH:
		{
			STOP_LED_ON();
		}
		break;

	case ST_ERROR:
	case ST_DOPEN:
		{
			RUN_LED_OFF();
			STOP_LED_OFF();
			if (cnt < 5)	ERROR_LED_ON();
			else			ERROR_LED_OFF();
		}
		break;
	default:
		{
			RUN_LED_ON();
			STOP_LED_OFF();
			ERROR_LED_OFF();
		}
		break;
	}
	// end case.
}


int str_process()
{
	// engine process
	ste_process();

	switch (strState)
	{
	case ST_NONE:	// check if ready. (TEMP1,2,3 is ready ?)
		{
			if (timer_isfired(tid_t1))
			{
				timer_clear(tid_t1);

				if (isDoorOpen())
				{
					strState = ST_DOPEN;

					buzzProfile = BUZZ_DOPEN;
					buzzIndex = 0;
				}
				else
				{
					strState = ST_WARMUP;

					DLOCK_ON();
					timer_set(tid_t1, WARMING_UP_DELAY);		// 5 min.
				}
			}
		}
		break;

	case ST_WARMUP:	// initial warming-up, no error while warming-up.
		{
			if (timer_isfired(tid_t1))					// after WARMING_UP_DELAY
			{
				// check temperature before auto self-test.
				currentCtrl.wls = 1;
				currentCtrl.ts = 1;

				strState = ST_CHECKING;
				timer_set(tid_t1, CHECKING_DELAY);		// 30 seconds.
			}
		}
		break;

	case ST_CHECKING:	// initial checking
		{
			if (timer_isfired(tid_t1))					// after CHECKING_DELAY
			{
				timer_clear(tid_t1);

				strState = ST_READY;	// go STRUN directly.

				// go start self-test automatically.
				str_startSelfTest();

				// after self test : DLOCK is off.
			}
			else if (currErrorCode != 0)	// check error.
			{
				timer_clear(tid_t1);

				errorCode = currErrorCode;	// check water level sensor (by init)

				strState = ST_ERROR;

				buzzProfile = BUZZ_ERROR;
				buzzIndex = 0;				
			}
		}
		break;

	case ST_READY:
		{
			if (isDoorOpen())
			{
				strState = ST_DOPEN;

				buzzProfile = BUZZ_DOPEN;
				buzzIndex = 0;
			}
			else if (currErrorCode != 0)	// check error.
			{
				// unexpected error.
				errorCode = currErrorCode;

				strState = ST_ERROR;

				buzzProfile = BUZZ_ERROR;
				buzzIndex = 0;
			}
		}
		break;
	
	case ST_STRUN:	// self test running
		{
			//
			if (ste_isComplete())		// check engine complete the proc.
			{
				DPRINTF(" self test done -> ready\n");

				ste_reset();

				currentCtrl.wls = 1;
				currentCtrl.ts = 1;

				strState = ST_READY;
				isTestCompleted = 1;

				updateRunInfoEndTime(1);	// 1 step end

				// self test completed.
				str_event(strState, 1);

				DLOCK_OFF();
			}
			else if (isDoorOpen())
			{
				goErrorStopping(1, ERR_D1);
			}
			else if (currErrorCode != 0)	// check error.
			{
				// 06/28. do not self-test again..
				isTestCompleted = 1;		// do not self-test again.

				ste_stop();

				// clear device state...
				errorCode = currErrorCode;
				runInfo.errorCode = errorCode;

				// 1st step go error finishing
				runInfo.firstStepSecs = totalRunSeconds;
				prevTotalSeonds = totalRunSeconds;

				strState = ST_STOPPING;
				str_event(strState, 0);
			}
		}
		break;

	case ST_RUN:	// running
		{
			if (ste_isComplete())		// check engine complete the proc.
			{
				ste_reset();		// clear engine state.

				if (runCycle < configCycle)
				{
					// restart runProc to 2nd... cycle
					if (ste_start(&runProc) == 0)
					{
						//DPRINTF("engine %d cycle start ok\n", runCycle);
						strState = ST_RUN;

						runCycle++;
					}
					else
					{
						dummyErrorStop();
					}
				}
				else
				{
					// end of running cycles.
					runInfo.firstStepSecs = totalRunSeconds;
					prevTotalSeonds = totalRunSeconds;

					DPRINTF(" run done -> go drying\n");

					if (ste_start(&dryingProc) == 0)
					{
						//DPRINTF("engine drying proc start ok\n");
						strState = ST_DRYING;

						runDryingCycle = 1;

						str_event(strState, 0);
					}
					else
					{
						dummyErrorStop();
					}
				}
			}
			else if (isDoorOpen())
			{
				goErrorStopping(1, ERR_D1);
			}
			else if (currErrorCode != 0)	// check error.
			{
				goErrorFinishing(1);		// 1st step error, go finishing
			}
		}
		break;

	case ST_DRYING:	// old finishing after run cycles..
		{
			if (ste_isComplete())
			{
				ste_reset();		// clear engine state.

				if (runDryingCycle < configDryingCycle)
				{
					// restart runProc to 2nd... cycle
					if (ste_start(&dryingProc) == 0)
					{
						//DPRINTF("engine %d cycle start ok\n", runCycle);
						strState = ST_DRYING;

						runDryingCycle++;
					}
					else
					{
						dummyErrorStop();
					}
				}
				else
				{
					engState = 0;	// make ready.

					strState = ST_COMPLETE;

					buzzProfile = BUZZ_COMPLETE;
					buzzIndex = 0;

					timer_set(tid_bto, 60000UL);		// buzz timeout 1 min.

					updateRunInfoEndTime(2);	// 2 step end

					// completed or stopped.
					str_event(strState, 1);

					DLOCK_OFF();
				}
			}
			else if (isDoorOpen())
			{
				goErrorStopping(2, ERR_D1);		// door error at 2nd step
			}
			else if (currErrorCode != 0)	// check error.
			{
				goErrorFinishing(2);		// 2nd step error, go finishing
			}
		}
		break;

	case ST_SDRYING:	// individual drying mode running..
		{
			if (ste_isComplete())
			{
				engState = 0;	// make ready.

				strState = ST_COMPLETE;

				buzzProfile = BUZZ_COMPLETE;
				buzzIndex = 0;

				timer_set(tid_bto, 60000UL);		// buzz timeout 1 min.

				updateRunInfoEndTime(1);	// 1 step end

				// completed or stopped.
				str_event(strState, 1);

				DLOCK_OFF();
			}
			else if (isDoorOpen())
			{
				goErrorStopping(1, ERR_D1);		// door error at first step
			}
			else if (currErrorCode != 0)	// check error.
			{
				goErrorFinishing(1);		// 1st step error, go finishing
			}
		}
		break;

	case ST_PTESTING:	// test mode, // not using engine.
		{
			/*
			if (isDoorOpen())
			{
				goErrorStopping(1, ERR_D1);		// door error at first step
				// ??????
			}
			*/
		}
		break;

	case ST_EFINISH:	// error finishing (since v.b1)
		{
			if (ste_isComplete())
			{
				engState = 0;	// make ready.

				// note) always error code exist.
				strState = ST_ERROR;		// by error.

				buzzProfile = BUZZ_ERROR;
				buzzIndex = 0;

				//timer_set(tid_t1, 60000UL);		// buzz timeout 1 min.

				updateRunInfoEndTime(3);	// finish step end

				// completed or stopped.
				str_event(strState, 1);

				DLOCK_OFF();
			}
			else if (isDoorOpen())
			{
				goErrorStopping(3, ERR_D1);		// door error at finish step
			}
			else
			{
				// just continue..
			}
		}
		break;

	case ST_STOPPING:
		{
			// to control stopping step.
			static int stoppingMode = 0;
	
			if (stoppingMode == 0)	// start stopping
			{
				// at first, clear sensor.
				currentCtrl.fs = 0;
				currentCtrl.ps1 = 0;
				currentCtrl.ps2 = 0;

				stoppingMode = 1;
				timer_set(tid_ct, 1000);
			}
			else if (stoppingMode == 1)
			{
				if (timer_isfired(tid_ct))
				{
					// check and stop : one by one.

					timer_set(tid_ct, 1000);

					if (currentCtrl.sv1 == 1)		{ currentCtrl.sv1 = 0;	SVALVE1_OFF();	}
					else if (currentCtrl.sv2 == 1)	{ currentCtrl.sv2 = 0;	SVALVE2_OFF();	}
					else if (currentCtrl.sv3 == 1)	{ currentCtrl.sv3 = 0;	SVALVE3_OFF();	}
					else if (currentCtrl.sv1t == 1)	{ currentCtrl.sv1t = 0;	SVALVE1_OFF();	}
					else if (currentCtrl.hv == 1)	{ currentCtrl.hv = 0;	HV_OFF();		}
					else if (currentCtrl.pump == 1)	{ currentCtrl.pump = 0;	PUMP_OFF();		}
					else if (currentCtrl.fan == 1)	{ currentCtrl.fan = 0;	FAN_OFF();		}
					else if (currentCtrl.tv1 == 1)	{ currentCtrl.tv1 = 0;	TWVALVE1_OFF();	}
					else if (currentCtrl.tv2 == 1)	{ currentCtrl.tv2 = 0;	TWVALVE2_OFF();	}
					else if (currentCtrl.tv3 == 1)	{ currentCtrl.tv3 = 0;	TWVALVE3_OFF();	}
					else
					{
						// done, all off state.
						stoppingMode = 2;	
					}
				}
			}
			else if (stoppingMode == 2)
			{
				if (timer_isfired(tid_ct))
				{

					if (configMode == MODE_PTESTING)
					{
						timer_clear(tid_ct);
						stoppingMode = 0;

						if (errorCode == ERR_STOP)		// stop by user.
						{
							// go ready.
							str_event(strState, 0);
							DLOCK_OFF();

							currentCtrl.wls = 1;
							currentCtrl.ts = 1;
							strState = ST_READY;
						}
						else							// only case : errorCode == DOOR_OPEN
						{
							DLOCK_OFF();
							strState = ST_DOPEN;

							buzzProfile = BUZZ_DOPEN;
							buzzIndex = 0;
						}
					}
					else
					{
						timer_clear(tid_ct);
						stoppingMode = 0;

						DS1307_time dtime;
						ds1307_read_time(&dtime);

						runInfo.endTime.hour   = dtime.hour;
						runInfo.endTime.minute = dtime.minute;
						runInfo.endTime.second = dtime.second;

						// stopping procedure done.
						strState = ST_ERROR;

						buzzProfile = BUZZ_ERROR;
						buzzIndex = 0;

						runInfo.totalSeconds = totalRunSeconds;
						// check.. : needs to print ?
						str_event(strState, 1);

						DLOCK_OFF();
					}
				}
			}
		}
		break;

	case ST_COMPLETE:
		{
			// note) printing, logging is done at str_event(*, 1)
			//----------------------------------------
			if (timer_isfired(tid_bto))
			{
				timer_clear(tid_bto);

				// stopping buzz.
				buzzProfile = 0;
			}

			// if door is open/close => then reset state.
			if (isDoorOpen())
			{
				strState = ST_DOPEN;

				buzzProfile = BUZZ_DOPEN;
				buzzIndex = 0;
			}
		}
		break;

	case ST_ERROR:		// unhandled error. stopped.
		{
			// TODO: error clearing procedure needed.
			//----------------------------------------

			// if door is open/close => then reset state.
			if (isDoorOpen())
			{
				strState = ST_DOPEN;

				buzzProfile = BUZZ_DOPEN;
				buzzIndex = 0;
			}
		}
		break;

	case ST_DOPEN:
		{
			if (isDoorOpen())
			{
				// wait..
			}
			else
			{
				if (! isTestCompleted)
				{
					// if not warm-up.. then go warmup.
					strState = ST_NONE;
					timer_set(tid_t1, INIT_DELAY);	// 3 sec delay
					currentCtrl.wls = 0;
					currentCtrl.ts = 0;

					// stopping buzz.
					buzzProfile = 0x00000000;
					buzzIndex = 0;

					// clear error code
					errorCode = 0;
					totalRunSeconds = 0;

					ste_reset();		// dummy, engine must be stopped already.
				}
				else
				{
					// enable wls and ts at ready state
					currentCtrl.wls = 1;
					currentCtrl.ts = 1;

					strState = ST_READY;

					// stopping buzz.
					buzzProfile = 0x00000000;
					buzzIndex = 0;

					// clear error code
					errorCode = 0;
					totalRunSeconds = 0;

					ste_reset();		// dummy, engine must be stopped already.
				}

			}
		}
		break;

	default:	// no case.
		break;
	}
	//end switch.


	// buzzer. run
	if (timer_isfired(tid_buzz))
	{
		timer_set(tid_buzz, 100);
#ifdef FT_NOERROR
#else
		if ((buzzProfile >> buzzIndex) & 0x01)	BUZZ_ON();
		else									BUZZ_OFF();
#endif
		buzzIndex++;
		if (buzzIndex > 31) buzzIndex = 0;


		// check door sensor state
		checkDoorSensor();

		updateStateLed();
	}

	return 0;
}


/**
	Get temperature
*/
//int str_getTemperature()		//--------------------------------------
//{
	// currTemperature is udpated at str_process()
//	return currTemperature;
//}



//-----------------------------------------------------------------------------
// test/debug feature
//
#ifdef	CONSOLE_DEBUG	// test only.
/**
	id		: 1..5
	onoff	: 0, 1
*/
int str_CtrlTValve(int id, int onoff)
{
	switch(id)
	{
	case 1: if (onoff) TWVALVE1_ON(); else TWVALVE1_OFF(); break;	
	case 2: if (onoff) TWVALVE2_ON(); else TWVALVE2_OFF(); break;	
	case 3: if (onoff) TWVALVE3_ON(); else TWVALVE3_OFF(); break;	
	case 4: if (onoff) TWVALVE4_ON(); else TWVALVE4_OFF(); break;	
	//case 5: if (onoff) TWVALVE5_ON(); else TWVALVE5_OFF(); break;	
	default:
		return -1;
	}
	return 0;
}

int str_CtrlTubePump(int onoff)
{
	if (onoff)	PUMP_ON(); 
	else 		PUMP_OFF();

	return 0;
}

int str_CtrlFan(int onoff)
{
	if (onoff)	FAN_ON(); 
	else 		FAN_OFF();

	return 0;
}

/**
	id		: 1..3
	onoff	: 0, 1
*/
int str_CtrlSValve(int id, int onoff)
{
	switch(id)
	{
	case 1: if (onoff) SVALVE1_ON(); else SVALVE1_OFF(); break;	
	case 2: if (onoff) SVALVE2_ON(); else SVALVE2_OFF(); break;	
	case 3: if (onoff) SVALVE3_ON(); else SVALVE3_OFF(); break;	
	default:
		return -1;
	}
	return 0;
}

int str_CtrlHV(int onoff)
{
	if (onoff)	HV_ON(); 
	else 		HV_OFF();

	return 0;
}

#endif

/**
	Print overall state
	- Pressuresw 1,2
	- Flow sensor
	- Temp sensor (3)

	not used anymore, v1.1
*/
void str_printState()
{
#ifdef	CONSOLE_DEBUG
	DPRINTF("str state : %d\n", strState);

	DPRINTF("Temp = %d\n", ntc_getTemperature());

	DPRINTF("DOOR = %s\n", IS_DOOR_OPEN()?"OPEN":"CLOSED");
	//DPRINTF("FLOW = %d\n", currFlowCount);

	// press in 1, 2
	DPRINTF("PL:%d, PH:%d\n", (PINB&0x20)?1:0, (PINB&0x40)?1:0);
#endif
}
