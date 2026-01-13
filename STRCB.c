// 2017.12.27
//  --> total run cycle 2000--> 20,000 수정 

/**
	Main controller of STR.


	- str logic is done at STR module
	- this module
		.do GUI display.
		.printing
		.SD card logging.
*/
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "putil.h"
#include "types.h"
#include "uart.h"
#include "uart1.h"
#include "timer.h"
#include "key2.h"
#include "ds1307.h"
#include "TDisplay.h"
#include "TFont.h"
#include "ADA600.h"
#include "SDL.h"

#include "STR.h"
#include "STRCBOptions.h"


// for gui editing
enum _time_field
{
	TF_NONE,
	TF_YEAR,
	TF_MONTH,
	TF_DAY,
	TF_HOUR,
	TF_MIN,
	TF_SEC
};

typedef struct _my_time
{
	uint8	year;		// 20xx	0..99
	uint8	month;
	uint8	day;
	uint8	hour;
	uint8	minute;
	uint8	second;
} DateTime;

// gui display.

typedef struct _str_state_info 
{
	//str_state
	/**
		0 : runMode view - normal.
		1 : runMode edit
		2 : time show
		3 : time edit
	*/
	int uiMode;

	int timeEditField;		// 0..6 : _time_field

	/**
		STR run mode : 1..5
			(1)   Standard (3-cycle)
			(2)   Advanced (6-cycle)
			(3)   Special (10-cycle)
			(4)   Self-Test
			(5)   Drying
	*/
	int runMode;

	// temporary str state
	int state;

	// error code

	// runtime
//	int min;
//	int sec;


	// set 1 if gui needs refresh.
	int uiUpdateFlag;

} StrRunState;


#define	CFG_VALIDATION_CODE	0xA1

/**
	Total run cycles
*/
typedef struct _lxRunInfo
{
	uint8	validationCode;

	uint16	runCycles;			// total run cycles
} StatisticsInfo;


static void gui_display();
static int printRunInfo(RunInfo *pInfo);
//static void testPrintRunInfo(RunInfo *pInfo);		// test

#ifndef	CONSOLE_DEBUG
static int logRunInfo(RunInfo *pInfo);
#endif



//-- config/run info
static void loadStatInfo(StatisticsInfo *pInfo, int loc);
static void saveStatInfo(StatisticsInfo *pInfo, int loc);


/**
	@param loc
		0	: read from default position
		1	: read from backup position
*/
static void loadStatInfo(StatisticsInfo *pInfo, int loc)
{
	asm("cli");
	if (loc==0) eeprom_read_block(pInfo, (const void *)1024, sizeof(StatisticsInfo));
	else        eeprom_read_block(pInfo, (const void *)3096, sizeof(StatisticsInfo));
	asm("sei");
}

/**
	@param loc
		0	: write to default position
		1	: write to backup position
*/
static void saveStatInfo(StatisticsInfo *pInfo, int loc)
{
	asm("cli");
	if (loc==0) eeprom_write_block(pInfo, (void *)1024, sizeof(StatisticsInfo));
	else        eeprom_write_block(pInfo, (void *)3096, sizeof(StatisticsInfo));

	asm("sei");
}


static int runInfoIsValid(StatisticsInfo *pInfo)
{
	if (pInfo->validationCode != CFG_VALIDATION_CODE) return 0;	// not valid
	return 1;
}



//-----------------------------------------------------------------------------
// config command
//

/**
	serial number string : AF001
*/
typedef struct _snInfo
{
	uint8	validationCode;

	char	sno[8+1];			// "AF001"
} SNInfo;

SNInfo	dsn;


/**
	Load serial no. at 0.
*/
static void loadSNInfo(SNInfo *pInfo)
{
	asm("cli");
	eeprom_read_block(pInfo, (const void *)0, sizeof(SNInfo));
	asm("sei");
}

/**
	@param loc
		0	: write to default position
		1	: write to backup position
*/
static void saveSNInfo(SNInfo *pInfo)
{
	pInfo->validationCode = CFG_VALIDATION_CODE;

	asm("cli");
	eeprom_write_block(pInfo, (void *)0, sizeof(SNInfo));
	asm("sei");
}

static int isSNInfoValid(SNInfo *pInfo)
{
	if (pInfo->validationCode != CFG_VALIDATION_CODE) return 0;	// not valid
	return 1;
}

// call back from SDLs.
// to set serial no.
void handle_config_command(const char *scmdBuff)
{
	if (strncmp(scmdBuff, "SET SN=", 7) == 0)
	{
		int len = strlen(scmdBuff);

		// ignore other than 5 to 8 char sno.
		if ( (len < (7+5)) || (len > (7+8)) ) return;

		// serial number : "AF001", max 8 char string.
		strcpy(dsn.sno, scmdBuff+7);

		sdl_write("Device serial no : ");
		sdl_write(dsn.sno);
		sdl_write(" ok\n");

		// save new serial no.
		saveSNInfo(&dsn);
	}
}


//-----------------------------------------------------------------------------
//-- console util.
#ifdef	CONSOLE_DEBUG
static int std_putchar(char c,FILE *stream);
static int std_getchar(FILE *stream);

static int std_putchar(char c,FILE *stream)                     
{
	if (c == '\n')
	{
		while (!uart1_putchar('\r'));
	}
	else {}

	while (!uart1_putchar(c));

	return 0;
}

static int std_getchar(FILE *stream)
{
	unsigned char c;
	while (uart1_getchar(&c) == 0)
	{
		asm("nop");
	};
	return c;
}


//
static char dbgCmdBuff[32];
static int dbgCmdLen = 0;
static int getDebugCommandLine(char *poutBuff);
#endif
//-----------------------------------------------------------------------------
// total run cycles.
StatisticsInfo statInfo;

static StrRunState  runState;
static DateTime editTime;

static uint8	testKeyMode = 0;


static int tid_run;		// led
static int tid_kto;		// key input timeout
static int tid_key;

int main()
{
	// init
	runState.uiMode = 0;
	runState.timeEditField = 0;

	runState.runMode = 1;
	runState.state = 0;


	runState.uiUpdateFlag = 1;

	// init ports.
	DDRE |= 0x08;	// PE3, LED.

#ifdef	CONSOLE_DEBUG
	uart1_init(BAUD_115200);
	fdevopen(std_putchar, std_getchar);
#else
	// sd card log interface.
	sdl_init();
#endif

	ds1307_init();
	timer_init();
	str_init();
	ada_init();

	// init
	td_init();
extern TFont tFont;
	td_setEFont(&tFont);

	tid_run = timer_alloc();
	timer_set(tid_run, 500);

	tid_kto = timer_alloc();

	tid_key = timer_alloc();		// key scan interval check timer.
	timer_set(tid_key, 100);


	sei();

	// init read time.
	DS1307_date date;
	DS1307_time time;

	ds1307_read_date(&date);
	ds1307_read_time(&time);

	editTime.year = date.year;
	editTime.month = date.month;
	editTime.day = date.day;

	editTime.hour = time.hour;
	editTime.minute = time.minute;
	editTime.second = time.second;

	DPRINTF("LX$");

	// load sn info.
	loadSNInfo(&dsn);
	if (!isSNInfoValid(&dsn))
	{
		strcpy(dsn.sno, "AF001");

		saveSNInfo(&dsn);
	}

	//test to log:
	sdl_write("SN=");
	sdl_write(dsn.sno);
	sdl_write("\n");

	// load run info.
	loadStatInfo(&statInfo, 0);

	if (!runInfoIsValid(&statInfo))
	{
		DPRINTF("runinfo 0 fail -> loading 1\n");
		// test load from backup.
		loadStatInfo(&statInfo, 1);

		if (runInfoIsValid(&statInfo))
		{
			DPRINTF("restore runinfo from 1\n");
			saveStatInfo(&statInfo, 0);
		}
		else
		{
			DPRINTF("runinfo 1 fail -> init 0,1\n");

			statInfo.validationCode = CFG_VALIDATION_CODE;
			statInfo.runCycles	= 0;

			saveStatInfo(&statInfo, 0);
			saveStatInfo(&statInfo, 1);
		}
	}
	else
	{
		DPRINTF("backup runinfo to 1\n");
		// save as backup.
		saveStatInfo(&statInfo, 1);

		// test:
		StatisticsInfo testInfo;

		loadStatInfo(&testInfo, 1);
		if (runInfoIsValid(&testInfo))
		{
			DPRINTF(" -> backup ok\n");
		}
		else
		{
			DPRINTF(" -> backup fail !!!!!!!\n");
		}
	}

  while(1)
  {
	// process STR controller.
	str_process();

	// update str state.
	runState.state = str_getState();

	// running led.
	if (timer_isfired(tid_run))
	{
		timer_set(tid_run, 500);

		_XOR(PORTE, 3);

		// update periodically : run time update and other.
		runState.uiUpdateFlag = 1;
	}

	// process key
	int scanCode = 0;

	if (timer_isfired(tid_key))
	{
		timer_set(tid_key, 2);			// 2 : 20 ms duration for key input.
		scanCode = key_getKey();


#ifndef	CONSOLE_DEBUG
		sdl_process();		// may run at low speed
#endif
	}

	if (IS_KEY_PRESSED(scanCode))
	{
		char key = KEY_CODE(scanCode);

		timer_set(tid_kto, 7000);		// key timeout.

		if (runState.state == ST_READY)
		{
			if (runState.uiMode == 0)
			{
				if (key == KEY_START)
				{
					if (testKeyMode == 5)
					{
						str_startPTesting();
						testKeyMode = 0;
					}
					else
					{
						if (runState.runMode == 1)			str_start(1);
						else if (runState.runMode == 2)		str_start(2);
						//else if (runState.runMode == 3)		str_start(4);
						//else if (runState.runMode == 2)		str_start(5);
						else if (runState.runMode == 3)		str_startSelfTest();
						//else if (runState.runMode == 6)		str_startDrying();
						//else if (runState.runMode == 5)		str_start(1);
					}
				}
				else if (key == KEY_MODE)
				{
					runState.uiMode = 1;
				}
				// testKeyMode : [STOP]-[UP]-[DOWN]-[UP]-[DOWN]-[START]

				else if (key == KEY_STOP)
				{
					testKeyMode = 1;							// start seq.
				}
				else if (key == KEY_UP)
				{
					if (testKeyMode == 1) testKeyMode = 2;
					else if (testKeyMode == 3) testKeyMode = 4;
					else testKeyMode = 0;						// invalid seq.
				}
				else if (key == KEY_DOWN)
				{
					if (testKeyMode == 2) testKeyMode = 3;
					else if (testKeyMode == 4) testKeyMode = 5;
					else testKeyMode = 0;						// invalid seq.
				}
			}
			else if (runState.uiMode == 1)
			{
				if (key == KEY_START)
				{
					runState.uiMode = 0;		// back to ready state.

					if (runState.runMode == 1)			str_start(1);
					else if (runState.runMode == 2)		str_start(2);
					//else if (runState.runMode == 3)		str_start(4);
					//else if (runState.runMode == 2)		str_start(5);
					else if (runState.runMode == 3)		str_startSelfTest();
					//else if (runState.runMode == 6)		str_startDrying();
				}
				else if (key == KEY_MODE)
				{
					runState.uiMode = 2;
				}
				else if (key == KEY_UP)
				{
					if (runState.runMode < 3) runState.runMode++;
					else if (runState.runMode == 3) runState.runMode = 1;
				}
				else if (key == KEY_DOWN)
				{
					if (runState.runMode > 1) runState.runMode--;
					else if (runState.runMode == 1) runState.runMode = 3;
				}
			}
			else if (runState.uiMode == 2)		// view time
			{
				if (key == KEY_MODE)
				{
					runState.uiMode = 4;
				}
				else if (key == KEY_DOWN)
				{
					runState.uiMode = 3;
					runState.timeEditField = 1;
				}
			}
			else if (runState.uiMode == 3)		// time editing mode.
			{
				if (key == KEY_MODE)
				{
					if (runState.timeEditField < 6)
					{
						runState.timeEditField++;
					}
					else
					{
						// save time.
						ds1307_set_datetime(0,
							editTime.minute,
							editTime.hour,
							editTime.day,
							editTime.month,
							editTime.year);

						runState.uiMode = 2;
					}
				}
				else if (key == KEY_UP)
				{
					switch (runState.timeEditField)
					{
					case TF_NONE: break;
					case TF_YEAR:
						if (editTime.year < 99) editTime.year++;

						break;
					case TF_MONTH:
						editTime.month++;
						if (editTime.month > 12) editTime.month = 1;
						break;
					case TF_DAY:
						editTime.day++;
						if (editTime.day > 31) editTime.day = 1;
						break;
					case TF_HOUR:
						editTime.hour++;
						if (editTime.hour > 23) editTime.hour = 0;
						break;
					case TF_MIN:
						editTime.minute++;
						if (editTime.minute > 59) editTime.minute = 0;
						break;
					case TF_SEC:
						editTime.second++;
						if (editTime.second > 59) editTime.second = 0;
						break;
					}
				}
				else if (key == KEY_DOWN)
				{
					switch (runState.timeEditField)
					{
					case TF_NONE: break;
					case TF_YEAR:
						if (editTime.year > 0) editTime.year--;
						break;
					case TF_MONTH:
						editTime.month--;
						if (editTime.month < 1) editTime.month = 12;
						else if (editTime.hour > 12) editTime.hour = 12;
						break;
					case TF_DAY:
						editTime.day--;
						if (editTime.day < 1) editTime.day = 31;
						else if (editTime.hour > 31) editTime.hour = 31;
						break;
					case TF_HOUR:
						editTime.hour--;
						if (editTime.hour < 0) editTime.hour = 23;
						else if (editTime.hour > 23) editTime.hour = 23;
						break;
					case TF_MIN:
						editTime.minute--;
						if (editTime.minute < 0) editTime.minute = 59;
						else if (editTime.minute > 59) editTime.minute = 59;
						break;
					case TF_SEC:
						editTime.second++;
						if (editTime.second < 0) editTime.second = 59;
						else if (editTime.second > 59) editTime.second = 59;
						break;
					}
				}
				//
			}//end of edit time.
			else if (runState.uiMode == 4)
			{
				if (key == KEY_MODE) runState.uiMode = 0;
				else if (key == KEY_DOWN)
				{
					runState.uiMode = 5;
				}
			}
			else if (runState.uiMode == 5)
			{
				if (key == KEY_MODE) runState.uiMode = 0;
				else if (key == KEY_UP)	// clear/reset stat
				{
					statInfo.runCycles = 0;
					saveStatInfo(&statInfo, 0);
					saveStatInfo(&statInfo, 1);
					runState.uiMode = 0;
				}
				else if (key == KEY_DOWN)	// cancel
				{
					runState.uiMode = 0;
				}
			}
		}
		else if (runState.state == ST_NONE)
		{
			// TODO: remove at release.
			if (key == KEY_MODE)
			{
				str_goReady();
			}
		}
		else if (runState.state == ST_RUN)
		{
			// can stop only RUNning state.

			if (key == KEY_STOP)
			{
				DPRINTF("Stop key\n");

				if (str_userStop() == 0)
				{
					runState.state = str_getState();
					DPRINTF("Stopped\n");
				}
				else
				{
					DPRINTF("Stop fail\n");
				}
			}
		}
		else if (runState.state == ST_PTESTING)
		{
			if (key == KEY_STOP)
			{
				if (str_stopPTesting() == 0)
				{
					runState.state = str_getState();
					DPRINTF("Stopped\n");
				}
				else
				{
					DPRINTF("Stop fail\n");
				}
			}
		}
		else if (runState.state == ST_COMPLETE)
		{
		// test only
		//	if (key == KEY_MODE)		// for test
		//	{
		//		str_goReady();
		//	}
		}
		else if (runState.state == ST_ERROR)
		{
		// test only
		//	if (key == KEY_STOP)
		//	{
		//		str_reset();
		//	}
		}


#if 0
		// press test mode control.
		if ( (runState.state == ST_NONE)
			|| (runState.state == ST_WARMUP)
			|| (runState.state == ST_CHECKING)
			)
		{
			if (key == KEY_START)
			{
				if (testKeyMode == 5)
				{
					str_startPTesting();
					testKeyMode = 0;
				}
			}
			else if (key == KEY_STOP)
			{
				testKeyMode = 1;							// start seq.
			}
			else if (key == KEY_UP)
			{
				if (testKeyMode == 1) testKeyMode = 2;
				else if (testKeyMode == 3) testKeyMode = 4;
				else testKeyMode = 0;						// invalid seq.
			}
			else if (key == KEY_DOWN)
			{
				if (testKeyMode == 2) testKeyMode = 3;
				else if (testKeyMode == 4) testKeyMode = 5;
				else testKeyMode = 0;						// invalid seq.
			}
		}
#endif
		// refresh ui.
		runState.uiUpdateFlag = 1;

		// end of key-press
	}
	else if (timer_isfired(tid_kto))
	{
		timer_clear(tid_kto);

		// clear mode to normal.
		runState.uiMode = 0;			// normal mode
		runState.timeEditField = 0;

		runState.uiUpdateFlag = 1;
	}
	//end of key.

	// operation control action by key.
	

	// display if changed.
	if (runState.uiUpdateFlag)
	{
		gui_display();
		runState.uiUpdateFlag = 0;
	}


#ifdef	CONSOLE_DEBUG
	
	//---------
	// main command buffer.
	static int cmdLen;
	static char cmdBuff[32];
	//---------
	//if (getCommandLine() > 0)
	if ((cmdLen=getDebugCommandLine(cmdBuff)) > 0)
	{
		// 기본 명령들..
		if (strcmp(cmdBuff, "st") == 0)
		{
			str_printState();
		}
		else if (strncmp(cmdBuff, "t?", 2) == 0)
		{
			// init read time.
			DS1307_date date;
			DS1307_time time;

			ds1307_read_date(&date);
			ds1307_read_time(&time);

			editTime.year = date.year;
			editTime.month = date.month;
			editTime.day = date.day;

			editTime.hour = time.hour;
			editTime.minute = time.minute;
			editTime.second = time.second;


			DPRINTF("Date %4d/%02d/%02d %02d:%02d:%02d\n",
				editTime.year, editTime.month, editTime.day,
				editTime.hour, editTime.minute, editTime.second);
		}
		else if (strncmp(cmdBuff, "tw", 2) == 0)
		{
			//void ds1307_initial_config(uint8 sec, uint8 min, uint8 hour, uint8 dow, uint8 date, uint8 month, uint8 year );

			ds1307_initial_config(0, 1, 16, 1, 7, 9, 15);	// 2015..
		}
		else if (strcmp(cmdBuff, "runT") == 0)
		{
			DPRINTF("starting SELF_TEST procedure");

			str_startSelfTest();
		}
		else if (strcmp(cmdBuff, "start") == 0)
		{
			if (str_getState() == ST_NONE)
			{
				str_makeReady();
			}
			str_start(2);
			DPRINTF("starting procedure(2)\n");
		}
		else if (strcmp(cmdBuff, "stop") == 0)
		{
			str_userStop();
		}
		else if (strcmp(cmdBuff, "PR") == 0)
		{
			RunInfo *pInfo = str_getRunInfo();
			printRunInfo(pInfo);
		}
		else if (strcmp(cmdBuff, "PT") == 0)
		{
			ada_print("test print\n");
		}

		/**
			Command format.
			TV[i]=0,1		;; 0 or 1.
			SV[i]=0,1.
			TP=0,1

		*/
		else if (strncmp(cmdBuff, "TV[", 3) == 0)
		{
			if (cmdLen == 7)
			{
				int i = cmdBuff[3] - '0';		// 1..5
				int ctrl = cmdBuff[6] - '0';	// 0 or 1.
				
				str_CtrlTValve(i, ctrl);
			}
			else
			{
				DPRINTF("invalid cmd : %s", cmdBuff);
			}
		}
		else if (strncmp(cmdBuff, "SV[", 3) == 0)
		{
			if (cmdLen == 7)
			{
				int i = cmdBuff[3] - '0';		// 1..3
				int ctrl = cmdBuff[6] - '0';	// 0 or 1.
				
				str_CtrlSValve(i, ctrl);
			}
			else
			{
				DPRINTF("invalid cmd : %s", cmdBuff);
			}
			//
		}
		else if (strncmp(cmdBuff, "TP=", 3) == 0)
		{
			if (cmdLen == 4)
			{
				int ctrl = cmdBuff[3] - '0';	// 0 or 1.

				str_CtrlTubePump(ctrl);
			}
			else
			{
				DPRINTF("invalid cmd : %s", cmdBuff);
			}
		}
		else if (strncmp(cmdBuff, "FAN=", 4) == 0)
		{
			if (cmdLen == 5)
			{
				int ctrl = cmdBuff[4] - '0';	// 0 or 1.

				str_CtrlFan(ctrl);
			}
			else
			{
				DPRINTF("invalid cmd : %s", cmdBuff);
			}
		}
		else if (strncmp(cmdBuff, "HV=", 3) == 0)
		{
			if (cmdLen == 4)
			{
				int ctrl = cmdBuff[3] - '0';	// 0 or 1.

				str_CtrlHV(ctrl);
			}
			else
			{
				DPRINTF("invalid cmd : %s", cmdBuff);
			}
		}
		else if (strcmp(cmdBuff, "help") ==0 )
		{
			DPRINTF("  TV[i]=0,1	; TV[1..5]\n");
			DPRINTF("  SV[i]=0,1	; SV[1..3]\n");
			DPRINTF("  TP=0,1	; Tube Pump\n");
			DPRINTF("  FAN=0,1	; FAN\n");
			DPRINTF("  HV=0,1	; High Volt.\n");
		}
		else
		{
			DPRINTF("LX$ ");
		}
	}
	else if (cmdLen == 0)
	{
		DPRINTF("LX$ ");
	}
	// end command line
#endif	//CONSOLE_DEBUG

  }
  //end while
}
//end of main.

// callback from STR module
void str_event(int strState, int completedFlag)
{
	// needs update gui.
	runState.uiUpdateFlag = 1;

	if (completedFlag)
	{
		// update and save run info.
		RunInfo *pInfo = str_getRunInfo();
		statInfo.runCycles += pInfo->runCycles;

		saveStatInfo(&statInfo, 0);
		saveStatInfo(&statInfo, 1);

		printRunInfo(pInfo);


#ifdef	CONSOLE_DEBUG
		// needs print.
		DPRINTF("Needs print : %d\n", strState);

#else
		logRunInfo(pInfo);
#endif
	}
	// else just update gui.
}

/**
	informational event from STR.
	current step / cycles.
*/
void str_step_event(int strState, int runCycle, int step, int totalStep)
{
	// needs update gui.
	runState.uiUpdateFlag = 1;
}

char uiBuff[4][24];


static void bFill(char *pBuff, int index, const char *str)
{
	int i=index;
	const char *p = str;
	
	while (*p)
	{
		pBuff[i++] = *p++;
	}
}

/**
	Make LCD gui display content from state
*/
static void gui_display()
{
	// STR state :
	char *str = "";
	char tBuff[7];		// "T:100C"

	int runSeconds = str_getRunSeconds();
	int min = runSeconds / 60;
	int sec = runSeconds % 60;

	// fill.
	// fill.           "12345678901234567890"
	sprintf(uiBuff[0], "                    ");
	sprintf(uiBuff[1], "                    ");
	sprintf(uiBuff[2], "                    ");
	sprintf(uiBuff[3], "                    ");

	// fill Tbuff.
//	int t0 = str_getTemperature();		//--------------------------------------
//	if (t0 < -10)			sprintf(tBuff, "T: E1 ");
//	else if (t0 > 200)		sprintf(tBuff, "T: E2 ");
//	else					sprintf(tBuff, "T:%3dC", t0);

	switch (runState.state)			// str state by str_getState()
	{
		case ST_NONE:
			{
				// "01234567890123456789"
				// "HS-B20      Starting"
				// "Warming up..        "
				// "HS-B20 HANSO INC.   "
				bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 12, "Starting");
				bFill(uiBuff[1], 0, "Warming up..");
				bFill(uiBuff[2], 0, "HANSO INC.");
			}
			break;

		case ST_WARMUP:
			{
				// "01234567890123456789"
				// "HS-B20     WarmingUp"
				// "Warming up..        "
				// "HS-B20 HANSO INC.   "
				bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 10, "Warming up");
				bFill(uiBuff[1], 0, "Wait warming up..");
				bFill(uiBuff[2], 0, "HANSO INC.");
			}
			break;

		case ST_CHECKING:
			{
				// "01234567890123456789"
				// "HS-B20     WarmingUp"
				// "Checking..          "
				// "HS-B20 HANSO INC.   "
				bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 10, "Warming up");
				bFill(uiBuff[1], 0, "Checking..");
				bFill(uiBuff[2], 0, "HANSO INC.");
			}
			break;

		case ST_READY:
			{
				if (runState.uiMode == 0)
				{
					// "01234567890123456789"
					// "HS-B20 r       READY"
					// "READY  r      T:100C"
					// "Ready to run        "
					// "Standard (3-Cycle)  "
//					sprintf(uiBuff[0], "HS-B20 %c       READY", sdl_getState());

					bFill(uiBuff[0], 0, "READY");
					uiBuff[0][7] = sdl_getState();
					bFill(uiBuff[0], 14, tBuff);

					bFill(uiBuff[1], 0, "Ready to run");
//					sprintf(uiBuff[1], "RUN MODE            ");	// normal.
					if (runState.runMode == 1)			sprintf(uiBuff[2], "Standard (2-Cycle)  ");
				//	else if (runState.runMode == 2)		sprintf(uiBuff[2], "Standard (2-Cycle)  ");
				//  else if (runState.runMode == 3)		sprintf(uiBuff[2], "Advanced (4-Cycle)  ");
				    else if (runState.runMode == 2)		sprintf(uiBuff[2], "Advanced (5-Cycle)  ");
					else if (runState.runMode == 3)		sprintf(uiBuff[2], "Self-Test           ");
				//	else if (runState.runMode == 6)		sprintf(uiBuff[2], "Drying mode         ");
				//	else if (runState.runMode == 6)		sprintf(uiBuff[2], "  1-CYCLE (test)    ");
				}
				else if (runState.uiMode == 1)
				{
					// "01234567890123456789"
					// "HS-B20         SETUP"
					// "Select run mode..   "
					// "  3-CYCLE (34 min)  "-----------------------------------------------------
					bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 15, "SETUP");
					bFill(uiBuff[1], 0, "Select run mode..");

					if (runState.runMode == 1)			sprintf(uiBuff[2], "Standard (2-Cycle)  ");
				//	else if (runState.runMode == 2)		sprintf(uiBuff[2], "Standard (3-Cycle)  ");
				//	else if (runState.runMode == 3)		sprintf(uiBuff[2], "Advanced (4-Cycle)  ");
					else if (runState.runMode == 2)		sprintf(uiBuff[2], "Advanced (5-Cycle)  ");
					else if (runState.runMode == 3)		sprintf(uiBuff[2], "Self-Test           ");
				//	else if (runState.runMode == 6)		sprintf(uiBuff[2], "Drying mode         ");
				//	else if (runState.runMode == 6)		sprintf(uiBuff[2], "  1-CYCLE (test)    ");
				}
				else if (runState.uiMode == 2)
				{
					// "01234567890123456789"
					// "HS-B20          INFO"
					// "TIME                "
					// "  3-CYCLE (34 min)  "
					DS1307_date date;
					DS1307_time time;
					ds1307_read_date(&date);
					ds1307_read_time(&time);

					editTime.year = date.year;
					editTime.month = date.month;
					editTime.day = date.day;

					editTime.hour = time.hour;
					editTime.minute = time.minute;
					editTime.second = time.second;

					// "01234567890123456789"
					// "HS-B20          INFO"
					// " 2015/10/12 23:32:34 ---------------------------------------------------------
					bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 16, "INFO");
					bFill(uiBuff[1], 0, "TIME");

					sprintf(uiBuff[2], " %04d/%02d/%02d %02d:%02d:%02d",
						2000+editTime.year,
						editTime.month,
						editTime.day,
						editTime.hour,
						editTime.minute,
						editTime.second
					);
				}
				else if (runState.uiMode == 3)
				{
					// "01234567890123456789"
					// "HS-B20         SETUP"
					// "Set time..          "
					// " 2015/10/12 23:32:34"
					bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 15, "SETUP");
					bFill(uiBuff[1], 0, "Set time..");

					sprintf(uiBuff[2], " %04d/%02d/%02d %02d:%02d:%02d",
						2000+editTime.year,
						editTime.month,
						editTime.day,
						editTime.hour,
						editTime.minute,
						editTime.second
					);

					// "01234567890123456789"
					// " YYYY/MM/DD hh:mm:ss"
					switch (runState.timeEditField)
					{
					case TF_NONE:	break;			

					case TF_YEAR:	bFill(uiBuff[3], 1, "----"); break;
					case TF_MONTH:	bFill(uiBuff[3], 6, "--"); break;
					case TF_DAY:	bFill(uiBuff[3], 9, "--"); break;
					case TF_HOUR:	bFill(uiBuff[3], 12,"--"); break;
					case TF_MIN:	bFill(uiBuff[3], 15,"--"); break;
					case TF_SEC:	bFill(uiBuff[3], 18,"--"); break;
					}
				}
				else if (runState.uiMode == 4)		// show total run cycles.
				{
					bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 10, "STATISTICS");
					bFill(uiBuff[1], 0, "Total run cycles");

							//		   " 99999 / 99999      "
					sprintf(uiBuff[2], " %5d / 20,000       ", statInfo.runCycles);
				}
				else if (runState.uiMode == 5)		// show total run cycles.
				{
					// "01234567890123456789"
					// "HS-B20         SETUP"
					// "Reset statistics ?  "
					// " [YES]     [NO]     "
					bFill(uiBuff[0], 0, "HS-VC20");	bFill(uiBuff[0], 15, "SETUP");
					bFill(uiBuff[1], 1, "Reset statistics ?");
					sprintf(uiBuff[2], "  [YES]     [NO]   ");
				}
			}
			break;

		case ST_STRUN:
			{
//doing
				// "01234567890123456789"
				// "HS-B20     Self-Test"
				// "Self-Test     T:100C"

				// "Running self-test.. "
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 11, "Self-Test");
				bFill(uiBuff[0], 0, "Self-Test");
				bFill(uiBuff[0], 14, tBuff);

				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);

				bFill(uiBuff[0], 0, "Self-Test");

				bFill(uiBuff[1], 0, "Running self-test..");
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
			}
			break;

		case ST_RUN:
			{
				// "01234567890123456789"
				// "HS-B20       Running"
				// " 00-cycle / 00-cycle"
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 13, "Running");
				bFill(uiBuff[0], 0, "Running");
				bFill(uiBuff[0], 14, tBuff);

				if (str_getErrorCode() == 0) 
				{
					sprintf(uiBuff[1], " %2d-cycle / %2d-cycle", str_getRunningCycle(), str_getConfigCycle());
				}
				else
				{
					sprintf(uiBuff[1], "Error:%-10s   ", str_getErrorString(str_getErrorCode()));
				}

				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
			}
			break;

		case ST_DRYING:
			{
				// "01234567890123456789"
				// "HS-B20        Drying"
				// "00-cycle finishing  "
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 14, "Drying");
				bFill(uiBuff[0], 0, "Drying");
				bFill(uiBuff[0], 14, tBuff);

				if (str_getErrorCode() == 0)	// normal finishing
				{
					sprintf(uiBuff[1], "%2d-cycle finishing  ", str_getConfigCycle());
				}
				else
				{
					sprintf(uiBuff[1], "Error:%-10s    ", str_getErrorString(str_getErrorCode()));
				}
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
				break;

			}
			break;

		case ST_EFINISH:
			{
				// "01234567890123456789"
				// "HS-B20 Err Finishing"
				// "Err Finishing T:100C"
				// "00-cycle finishing  "
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20 Err Finishing");
				bFill(uiBuff[0], 0, "Err Finishing");
				bFill(uiBuff[0], 14, tBuff);

				if (str_getErrorCode() == 0)	// normal finishing
				{
					sprintf(uiBuff[1], "%2d-cycle finishing  ", str_getConfigCycle());
				}
				else
				{
					sprintf(uiBuff[1], "Error:%-10s    ", str_getErrorString(str_getErrorCode()));
				}
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
				break;
			}
			break;

		case ST_SDRYING:
			{
				// "01234567890123456789"
				// "HS-B20        Drying"
				// "Running drying..    "
				// "Time: 000 min 00 sec"

//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 14, "Drying");
				bFill(uiBuff[0], 0, "Drying");
				bFill(uiBuff[0], 14, tBuff);

				if (str_getErrorCode() == 0)	// normal finishing
				{
					bFill(uiBuff[1], 0, "Running drying..");
				}
				else
				{
					sprintf(uiBuff[1], "Error:%-10s    ", str_getErrorString(str_getErrorCode()));
				}
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
				break;

			}
			break;

		case ST_PTESTING:
			{
				// "01234567890123456789"
				// "HS-B20       Testing"
				// "Running testing..   "
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 13, "Testing");
				bFill(uiBuff[0], 0, "Testing");
				bFill(uiBuff[0], 14, tBuff);

				bFill(uiBuff[1], 0, "Press testing..");
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
			}
			break;

		case ST_COMPLETE:
			{
				// "01234567890123456789"
				// "HS-B20      Complete"
				// "Self-Test completed "		...
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 12, "Complete");
				bFill(uiBuff[0], 0, "Complete");
				bFill(uiBuff[0], 14, tBuff);

				int runMode = str_getConfigMode();

				if (runMode == MODE_SELF_TEST)		sprintf(uiBuff[1], "Self-Test completed ");
				else if (runMode == MODE_DRYING)	sprintf(uiBuff[1], "Drying completed    ");
				else								sprintf(uiBuff[1], "%2d-cycle completed  ", str_getConfigCycle());
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
			}
			break;

		case ST_STOPPING:
			{
				// "01234567890123456789"
				// "HS-B20      Stopping"
				// "Error:**********    "
				// "Time: 000 min 00 sec"

//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 12, "Stopping");
				bFill(uiBuff[0], 0, "Stopping");
				bFill(uiBuff[0], 14, tBuff);

				sprintf(uiBuff[1], "Error:%-10s    ", str_getErrorString(str_getErrorCode()));
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
			}
			break;

		case ST_ERROR:
			{
				// "01234567890123456789"
				// "HS-B20         ERROR"
				// "Error:**********    "
				// "Time: 000 min 00 sec"

//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 15, "ERROR");
				bFill(uiBuff[0], 0, "ERROR");
				bFill(uiBuff[0], 14, tBuff);

				sprintf(uiBuff[1], "Error:%-10s    ", str_getErrorString(str_getErrorCode()));
				sprintf(uiBuff[2], "Time: %3d min %2d sec", min, sec);
			}
			break;

		case ST_DOPEN:
			{
				// "01234567890123456789"
				// "HS-B20       WARNING"
				// "Error:**********    "
				// "Time: 000 min 00 sec"
//				bFill(uiBuff[0], 0, "HS-B20");	bFill(uiBuff[0], 13, "WARNING");
				bFill(uiBuff[0], 0, "WARNING");
				bFill(uiBuff[0], 14, tBuff);

				bFill(uiBuff[1], 0, "DOOR OPEN");
				bFill(uiBuff[2], 0, "Close the door..");
			}
			break;
		default:
			{
				str = "  Unknown";	
			}
			break;
	}


	// display to..
	td_text(8, 0, uiBuff[0]);
	td_text(8, 8, uiBuff[1]);
	td_text(8,16, uiBuff[2]);
	td_text(8,24, uiBuff[3]);
}
//-----------------------------------------------------------------------------
// printing
//

#define	PR_DELAY()		_delay_ms(10)

/**
	print
*/
static int printRunInfo(RunInfo *pInfo)
{
	char buff[64];
	// align-center, font enLarge, bold
	ada_font_enLarge(25);
	ada_font_bold(1);	
	ada_align_mode(ALIGN_CENTER);

	// 모델 번호----------------------------------------------------------------------> 확인 !!!1
	ada_print("HS-VC20\n\n");

	// font enLarge = 0, normal, align left
	ada_font_enLarge(0);
	ada_font_bold(0);	
	ada_align_mode(ALIGN_LEFT);

	// 제품 번호
	//ada_print("S/N : B20-AF001\n");----------------------------------------------------> 확인 !!!1
	ada_print("S/N : VC20-BJ002\n");
	PR_DELAY();

	// 줄간격
	ada_line_spacing(50); // 32 default

	// 날짜
	sprintf(buff, "DATE : %04d-%02d-%02d\n",
		2000+pInfo->runDate.year, pInfo->runDate.month, pInfo->runDate.date);
	ada_print(buff);

	// 운전모드
	if (pInfo->mode == MODE_SELF_TEST)
	{
		sprintf(buff, "RUNNING MODE : SELF-TEST\n");
	}
	else if (pInfo->mode == MODE_DRYING)
	{
		sprintf(buff, "RUNNING MODE : DRYING\n");
	}
	else if (pInfo->mode == MODE_RUN)
	{
		sprintf(buff, "RUNNING MODE : %d-CYCLE\n",
			pInfo->runCycles);
	}
	//no else case.
	ada_print(buff);
	ada_print("\n");
	PR_DELAY();

	// 시작시간
	sprintf(buff, "START TIME : %02d:%02d:%02d\n",
		pInfo->startTime.hour, pInfo->startTime.minute, pInfo->startTime.second);
	ada_print(buff);
	PR_DELAY();

	// left space
//	ada_left_space(25); // 25 * 0.125mm

	// 초기단계
//	sprintf(buff, "1.INIT STAGE : %02d min %02d sec\n",
//		pInfo->initSeconds/60, pInfo->initSeconds%60);
//	ada_print(buff);

	// left space
	ada_left_space(25); // 25 * 0.125mm

	if (pInfo->mode == MODE_SELF_TEST)
	{
		// 운전단계
		sprintf(buff, "1.SELF-TEST : %2d min %02d sec\n",
			pInfo->firstStepSecs/60, pInfo->firstStepSecs%60);
		ada_print(buff);
	}
	else if (pInfo->mode == MODE_DRYING)
	{
		sprintf(buff, "1.DRYING : %2d min %02d sec\n",
			pInfo->firstStepSecs/60, pInfo->firstStepSecs%60);
		ada_print(buff);
	}
	else	// normal RUN.
	{
		// 운전단계
		sprintf(buff, "1.STERILIZE : %2d min %02d sec\n",
			pInfo->firstStepSecs/60, pInfo->firstStepSecs%60);
		ada_print(buff);

		// left space
		ada_left_space(25); // 25 * 0.125mm

		// 최종단계
		sprintf(buff, "2.DRYING  : %2d min %02d sec\n",
			pInfo->secondStepSecs/60, pInfo->secondStepSecs%60);
		ada_print(buff);
	}
	PR_DELAY();

	// 오류.
	if (pInfo->errorCode != 0)
	{
		if (pInfo->finishStepSecs != 0)
		{
			// error finishing
			sprintf(buff, "* ERR-FINISHING : %2d min %02d sec\n",
				pInfo->finishStepSecs/60, pInfo->finishStepSecs%60);
			ada_print(buff);
		}
		// note) else stop by door-open

		sprintf(buff, "* ERROR : %s\n", str_getErrorString(pInfo->errorCode));
		ada_print(buff);
	}

	// 종료시간
	sprintf(buff, "END TIME : %02d:%02d:%02d\n",
		pInfo->endTime.hour, pInfo->endTime.minute, pInfo->endTime.second);
	ada_print(buff);
	ada_print("\n");

	sprintf(buff, "TOTAL LEAD TIME : %2d min %02d sec\n",
		pInfo->totalSeconds/60, pInfo->totalSeconds%60);
	ada_print(buff);


	sprintf(buff, "STAT: %2d / 20,000 cycles\n",
		statInfo.runCycles);
	ada_print(buff);
	ada_print("\n");

	// align-center, font enLarge, bold
	ada_font_enLarge(25);
	ada_font_bold(1);	
	ada_align_mode(ALIGN_CENTER);

	// ERROR 여부
	if(pInfo->errorCode == 0)
	{
		ada_print("COMPLETE\n");
	}
	else
	{
		ada_print("ERROR\n");
	}

	// font enLarge = 0, normal, align left
	ada_font_enLarge(0);
	ada_font_bold(0);	
	ada_align_mode(ALIGN_LEFT);

	ada_print("------------------------------\n\n");

	return 0;
}


#ifndef	CONSOLE_DEBUG

#define	SDL_DELAY()	_delay_ms(10);
/**
	log
*/
static int logRunInfo(RunInfo *pInfo)
{
	char buff[64];

	// make file name: long format from runDate.
	// Filename:
	// ex) 15ab1234.log
	//	Lyymmdd-hhMMssS.log			-- self test
	//	Lyymmdd-hhMMssR.log			-- running
	//	Lyymmdd-hhMMssD.log			-- drying
	char cmode = 'N';
	if (pInfo->mode == MODE_SELF_TEST)		cmode = 'S';
	else if (pInfo->mode == MODE_RUN)		cmode = 'R';
	else if (pInfo->mode == MODE_DRYING)	cmode = 'D';

	sprintf(buff, "L%02d%02d%02d-%02d%02d%02d%c.log",
		pInfo->runDate.year,
		pInfo->runDate.month,
		pInfo->runDate.date,
		pInfo->endTime.hour,
		pInfo->endTime.minute,
		pInfo->endTime.second,
		cmode);

	// create log file.
	sdl_create(buff);

	SDL_DELAY();
	SDL_DELAY();


	// 모델 번호----------------------------------------------------> 확인 !!!1
	sdl_write("HS-VC20\n\n");
	SDL_DELAY();

	// 제품 번호----------------------------------------------------> 확인 !!!1
	//sdl_write("S/N : B20-AF001\n");
	sdl_write("S/N : VC20-BJ003\n");
	SDL_DELAY();

	// 날짜
	sprintf(buff, "DATE : %04d-%02d-%02d\n",
		2000+pInfo->runDate.year, pInfo->runDate.month, pInfo->runDate.date);
	sdl_write(buff);
	SDL_DELAY();

	// 운전모드
	if (pInfo->mode == MODE_SELF_TEST)
	{
		sprintf(buff, "RUNNING MODE : SELF-TEST\n");
	}
	else if (pInfo->mode == MODE_DRYING)
	{
		sprintf(buff, "RUNNING MODE : DRYING\n");
	}
	else if (pInfo->mode == MODE_RUN)
	{
		sprintf(buff, "RUNNING MODE : %d-CYCLE\n",
			pInfo->runCycles);
	}
	//no else case.
	sdl_write(buff);
	SDL_DELAY();

	// 시작시간
	sprintf(buff, "START TIME : %02d:%02d:%02d\n",
		pInfo->startTime.hour, pInfo->startTime.minute, pInfo->startTime.second);
	sdl_write(buff);
	SDL_DELAY();

/*
	// 운전단계
	sprintf(buff, "1.RUN STAGE : %2d min %02d sec\n",
		pInfo->runStepSecs/60, pInfo->runStepSecs%60);
	sdl_write(buff);
	SDL_DELAY();

	// 최종단계
	sprintf(buff, "2.FIN STAGE : %2d min %02d sec\n",
		pInfo->finStepSecs/60, pInfo->finStepSecs%60);
	sdl_write(buff);
	SDL_DELAY();
*/
	if (pInfo->mode == MODE_SELF_TEST)
	{
		// 운전단계
		sprintf(buff, "1.SELF-TEST : %2d min %02d sec\n",
			pInfo->firstStepSecs/60, pInfo->firstStepSecs%60);
		sdl_write(buff);
	}
	else if (pInfo->mode == MODE_DRYING)
	{
		sprintf(buff, "1.DRYING : %2d min %02d sec\n",
			pInfo->firstStepSecs/60, pInfo->firstStepSecs%60);
		sdl_write(buff);
	}
	else	// normal RUN.
	{
		// 운전단계
		sprintf(buff, "1.STERILIZE : %2d min %02d sec\n",
			pInfo->firstStepSecs/60, pInfo->firstStepSecs%60);
		sdl_write(buff);

		// 최종단계
		sprintf(buff, "2.DRYING  : %2d min %02d sec\n",
			pInfo->secondStepSecs/60, pInfo->secondStepSecs%60);
		sdl_write(buff);
	}

	// 오류.
	if (pInfo->errorCode != 0)
	{
		if (pInfo->finishStepSecs != 0)
		{
			// error finishing
			sprintf(buff, "*.FINISHING : %2d min %02d sec\n",
				pInfo->finishStepSecs/60, pInfo->finishStepSecs%60);
			sdl_write(buff);
		}
		// note) else stop by door-open

		sprintf(buff, "* ERROR : %s\n", str_getErrorString(pInfo->errorCode));
		sdl_write(buff);
	}

	// 종료시간
	sprintf(buff, "END TIME : %02d:%02d:%02d\n",
		pInfo->endTime.hour, pInfo->endTime.minute, pInfo->endTime.second);
	sdl_write(buff);
	SDL_DELAY();

	sprintf(buff, "TOTAL LEAD TIME : %2d min %02d sec\n",
		pInfo->totalSeconds/60, pInfo->totalSeconds%60);
	sdl_write(buff);
	SDL_DELAY();

	sprintf(buff, "STAT: %2d / 2000 cycles\n",
		statInfo.runCycles);
	sdl_write(buff);
	SDL_DELAY();

	// ERROR 여부
	if(pInfo->errorCode == 0)
	{
		sdl_write("COMPLETE\n");
	}
	else
	{
		sdl_write("ERROR\n");
	}
	SDL_DELAY();

	// close log file.
	sdl_close();
	SDL_DELAY();

	return 0;
}

#endif



//-----------------------------------------------------------------------------
#ifdef	CONSOLE_DEBUG

//int getDebugCommandLine(char *poutBuff);

/**

	@return	bufferedcommand length.

	note) user SHOULD clear cmdLength.
*/
static int getDebugCommandLine(char *poutBuff)
{
		// 디버그 모드 : 콘솔 인터페이스를 이용한 디버그 모드
	char c;

	// data count from PC to FPGA
	if (uart1_getchar((unsigned char *)&c) == 1)
	{
		if (c == '\r')
		{
			int len = dbgCmdLen;

			strcpy(poutBuff, dbgCmdBuff);	//, len);
			dbgCmdLen = 0;

			uart1_putchar(c);
			uart1_putchar('\n');

			return len;


			//return dbgCmdLen;
		}
		else
		{
			if (dbgCmdLen < 63)
			{
				// echo.
				uart1_putchar(c);

				//putchar(cmd);
				dbgCmdBuff[dbgCmdLen] = c;
				dbgCmdBuff[++dbgCmdLen] = 0;
			}
			//else no more command..

			return -1;
		}
	}
	
	return -1;
}
#endif
