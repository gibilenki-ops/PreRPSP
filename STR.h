#ifndef	__STR_H__
#define __STR_H__

#include "types.h"
#include "DateTime.h"
/**
	STR control state
*/
enum str_state
{
	ST_NONE,			// power on initial state
	ST_WARMUP,			// initial warming-up
	ST_CHECKING,		// initial checking
	ST_READY,
	ST_STRUN,			// running self-test mode and initial self-test
	ST_SDRYING,			// running drying mode
	ST_PTESTING,		// press test function
	ST_RUN,
	ST_DRYING,
	ST_EFINISH,
	ST_STOPPING,		// Stopping current run by user "stop button"
	ST_COMPLETE,
	ST_ERROR,

	ST_DOPEN			// Door open state (none-error)
};

/**
	Error code for system
*/
#define	ERR_P1		0x01			// low pressure
#define	ERR_P2		0x02			// high pressure
#define	ERR_FLOW	0x03			// no flow
#define	ERR_T1		0x04			// temperature 1
#define	ERR_T2		0x05			// temperature 2
#define	ERR_T3		0x06			// temperature 3
#define	ERR_T4		0x11			// temperature 4 : over temp.
#define	ERR_WL		0x07			// water level low
#define	ERR_D1		0x08			// door open
#define	ERR_PLASMA	0x09			// plasma sensor error
#define	ERR_STOP	0x10			// stop by user
#define	ERR_S0		0x20			// internal error case


/**
	Run mode	@runInfo

*/
#define	MODE_SELF_TEST		0x01
#define	MODE_RUN			0x02
#define	MODE_DRYING			0x03
#define	MODE_PTESTING		0x04

/**
	Run info.working..
*/
typedef struct _runInfo
{
	uint8	mode; 			// 운전모드			MODE_xxxx		note) meaning changed at v1.0b
	uint8	runCycles;

	TDate	runDate;		// 날짜				YYYY-MM-DD
	TTime	startTime;		// 시작시간			PM 13:22

	uint16	firstStepSecs;		// running, drying, self-test
	uint16	secondStepSecs; 	// drying..
	uint16	finishStepSecs;	// 최종단계			_00 min 00 sec

	uint16	totalSeconds;	// total time in seconds

	TTime	endTime;		// 종료시간			PM 13:47

	uint8 errorCode;			// 에러 여부,  0-complete, 1-error
} RunInfo;

/**
	init module / interface
*/
int str_init();

/**
	Reset state
	note) only valid when COMPLETE, STOPPED, ERROR
		else invalid.
*/
int str_reset();

/**
	Go ready for test
*/
int str_goReady();

/**
	Start self test
*/
int str_startSelfTest();

/**
	Start drying mode
*/
int str_startDrying();

/**
	Start pressure test mode
*/
int str_startPTesting();
int str_stopPTesting();

/**
	Get current running state
*/
int str_getState();


/**
	Current/Last run mode
	note) set at str_start***()
*/
int str_getConfigMode();

/**
	Current configured cycle.
	note) valid after start., default is 2.
*/
int str_getConfigCycle();


/**
	Get current running cycle
*/
int str_getRunningCycle();

/**
	Procedure running time in seconds.
	- self-test
	- run n cycles and finishing
*/
unsigned int str_getRunSeconds();

/**
	Get stop cause
	valid when : STOPPED, ERROR, FINISHING
*/
int str_getErrorCode();

/**
	Get error code name
*/
const char *str_getErrorString(int errCode);


/**
	Get running information after complete/stop
*/
RunInfo *str_getRunInfo();

/**
	Start run
*/
int str_start(int cycle);

/**
	Stop run by user key.

	Note) only when ST_RUN state
*/
int str_userStop();

/**
	Stop run

	Note) When error, don't stop.
	error -> stop after, finishing step run.

	2016/10/17. not used anymore. only for test.
*/
//int str_stop();

/**
	internal process to run str.
*/
int str_process();


/**
	Get temperature
*/
int str_getTemperature();

/**
	notify user
	- state change
	- etc.

	@param	completedFlag
		1	: operation complete, needs print or log.
		0	: normal state change event.
*/
extern void str_event(int strState, int completedFlag);

extern void str_step_event(int strState, int runCycle, int step, int totalStep);

//-------------------------------------
// test features
//

/**
	id		: 1..5
	onoff	: 0, 1
*/
int str_CtrlTValve(int id, int onoff);

int str_CtrlTubePump(int onoff);

int str_CtrlFan(int onoff);

/**
	id		: 1..3
	onoff	: 0, 1
*/
int str_CtrlSValve(int id, int onoff);

int str_CtrlHV(int onoff);


void str_printState();

void str_makeReady();

#endif	//__STR_H__
