/**
	ADA600 printer
		
	FWL.
	@author	tchan@TSoft
	@date	2015/08/25
*/
#ifndef	__ADA600_H__
#define	__ADA600_H__


// Printer Align
#define ALIGN_LEFT 		0
#define ALIGN_CENTER	1
#define ALIGN_RIGHT 	2

typedef struct _printInfo
{
	char *cur_date; // 날짜				YYYY-MM-DD
	char *mode; 	// 운전모드			2-CYCLE
	char *t_start; 	// 시작시간			PM 13:22
	char *s_init; 	// 초기단계			_00 min 00 sec
	char *s_run; 	// 운전단계			_00 min 00 sec
	char *s_last; 	// 최종단계			_00 min 00 sec
	char *t_finish; // 종료시간			PM 13:47
	char *t_sum; 	// 총 소요시간		_00 min 00 sec

	uint8 isError;	// 에러 여부,  0-complete, 1-error

} PrintInfo;


/**
	
*/
int ada_init();


void ada_print(char *str);

/**
	print a line
	note) no CR/LF at end of str.
*/
int ada_print_line(char *str);



#if 0

/**
	print

	@usage
		PrintInfo pInfo;

		pInfo.cur_date 	= "2015-09-09";
		pInfo.mode 		= "2 STAGE";
		pInfo.t_start 	= "PM 03:10";
		pInfo.s_init	= "999min 55sec";
		pInfo.s_run		= "888min 44sec";
		pInfo.s_last	= "777min 33sec";
		pInfo.t_finish	= "PM 03:47";
		pInfo.t_sum		= "03hr 55min";
		pInfo.isError 	= 0; // 0-complete, 1-error
*/
int ada_print(PrintInfo *pInfo);

/**
	print
*/
int ada_print0(char *cur_date, char *mode, char *t_start, char *s_init, char *s_run, char *s_last, char *t_finish, char *t_sum, int isError);
#endif

/**
	Initialize the printer
		- The print buffer is cleared.
		- Reset the param to default value.
		- Return to standard mode.
		- Delete user_defined characters.
*/
int ada_init_printer();

/**
	align printer
	0 - left
	1 - center
	2 - right
*/
int ada_align_mode(uint8 align);

/**
	left space

	1 => 0.125mm
*/
int ada_left_space(uint8 space);

/**
	font enLarge
*/
int ada_font_enLarge(uint8 large);

/**
	font bold
		0 - normal
		1 - bold
*/
int ada_font_bold(uint8 bold);

/**
	line spacing(0 ~ 255)
		default : 32
*/
int ada_line_spacing(uint8 space);



#endif
//__ADA600_H__
