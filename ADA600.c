#include <stdio.h>
#include <string.h>
#include "uart.h"

#include "ADA600.h"

// Printer Align
#define LEFT 0
#define CENTER 1
#define RIGHT 2

#define	PUTC(c) while (uart_putchar(c) == 0)


/**
	Init printer connection
*/
int ada_init()
{
	uart_init(BAUD_9600);

	return 0;
}


/**
	write string to printer

	note) no end of line char needed.
*/
void ada_print(char *str)
{
	char *p = str;

	while (*p)
	{
		PUTC(*p);		// until ok(1).   note) 0:fail.
		p++;
	}
}

/**
	write a line of log

	note) no end of line char needed.
*/
int ada_print_line(char *str)
{
	char *p = str;

	while (*p)
	{
		//if ((*p == '\r') || (*p == '\n')) break;

		PUTC(*p);		// until ok(1).   note) 0:fail.
		p++;
	}

	// put "end of line : <LF>" char.
	//uart_putchar('\n');

	return 0;
}

#if 0
/**
	print
*/
int ada_print(PrintInfo *pInfo)
{
	// align-center, font enLarge, bold
	ada_font_enLarge(25);
	ada_font_bold(1);	
	ada_align_mode(CENTER);

	// 제품번호
	ada_print_line("LEADGENEX-LX20");
	PUTC('\n');
	PUTC('\n');

	// font enLarge = 0, normal, align left
	ada_font_enLarge(0);
	ada_font_bold(0);	
	ada_align_mode(LEFT);

	// 모델번호
	ada_print_line("S/N : LX20-201-CX01");
	PUTC('\n');

	// 줄간격
	ada_line_spacing(50); // 32 default

	// 날짜
	ada_print_line("DATE : ");
	ada_print_line(pInfo->cur_date);
	PUTC('\n');

	// 운전모드
	ada_print_line("RUNNING MODE : ");
	ada_print_line(pInfo->mode);
	PUTC('\n');

	// 시작시간
	ada_print_line("START TIME : ");
	ada_print_line(pInfo->t_start);
	PUTC('\n');

	// left space
	ada_left_space(25); // 25 * 0.125mm

	// 초기단계
	ada_print_line("1.INIT STAGE : ");
	ada_print_line(pInfo->s_init);
	PUTC('\n');

	// left space
	ada_left_space(25); // 25 * 0.125mm

	// 운전단계
	ada_print_line("2.RUN STAGE  : ");
	ada_print_line(pInfo->s_run);
	PUTC('\n');

	// left space
	ada_left_space(25); // 25 * 0.125mm

	// 최종단계
	ada_print_line("3.LAST STAGE : ");
	ada_print_line(pInfo->s_last);
	PUTC('\n');

	// 종료시간
	ada_print_line("FINISH TIME : ");
	ada_print_line(pInfo->t_finish);
	PUTC('\n');

	// 총소요시간
	ada_print_line("TOTAL LEAD TIME : ");
	ada_print_line(pInfo->t_sum);
	PUTC('\n');

	// align-center, font enLarge, bold
	ada_font_enLarge(25);
	ada_font_bold(1);	
	ada_align_mode(CENTER);

	// ERROR 여부
	if(pInfo->isError)
	{
		ada_print_line("ERROR");
	}
	else
	{
		ada_print_line("COMPLETE");
	}

	PUTC('\n');

	// font enLarge = 0, normal, align left
	ada_font_enLarge(0);
	ada_font_bold(0);	
	ada_align_mode(LEFT);

	ada_print_line("------------------------------");
	PUTC('\n');
	PUTC('\n');

	return 0;
}
#endif

#if 0
int ada_print0(char *cur_date, char *mode, char *t_start, char *s_init, char *s_run, char *s_last, char *t_finish, char *t_sum, int isError)
{

	// align-center, font enLarge, bold
	ada_font_enLarge(25);
	ada_font_bold(1);	
	ada_align_mode(CENTER);

	// 제품번호
	ada_print_line("LEADGENEX-LX20");
	PUTC('\n');
	PUTC('\n');

	// font enLarge = 0, normal, align left
	ada_font_enLarge(0);
	ada_font_bold(0);	
	ada_align_mode(LEFT);

	// 모델번호
	ada_print_line("S/N  : LX20-201-CX01");
	PUTC('\n');

	// 줄간격
	ada_line_spacing(50); // 32 default

	// 날짜
	ada_print_line("DATE : ");
	ada_print_line(cur_date);
	PUTC('\n');

	// 운전모드
//	ada_print_line("WORKING MODE : ");
//	ada_print_line("OPERATING MODE : ");
	ada_print_line("RUNNING MODE : ");
	ada_print_line(mode);
	PUTC('\n');

	// 시작시간
	ada_print_line("START TIME : ");
	ada_print_line(t_start);
	PUTC('\n');


	// left space
	ada_left_space(25); // 25 * 0.125mm

	// 초기단계
	ada_print_line("1.INIT STAGE : ");
	ada_print_line(s_init);
	PUTC('\n');


	// left space
	ada_left_space(25); // 25 * 0.125mm

	// 운전단계
	ada_print_line("2.RUN STAGE  : ");
	ada_print_line(s_run);
	PUTC('\n');

	// left space
	ada_left_space(25); // 25 * 0.125mm

	// 최종단계
	ada_print_line("3.LAST STAGE : ");
	ada_print_line(s_last);
	PUTC('\n');

	// 종료시간
	ada_print_line("FINISH TIME : ");
	ada_print_line(t_finish);
	PUTC('\n');

	// 총소요시간
	ada_print_line("TOTAL LEAD TIME : ");
	ada_print_line(t_sum);
	PUTC('\n');

	// align-center, font enLarge, bold
	ada_font_enLarge(25);
	ada_font_bold(1);	
	ada_align_mode(CENTER);

	// ERROR 여부
	if(isError)
	{
		ada_print_line("ERROR");
	}
	else
	{
		ada_print_line("COMPLETE");
	}

	PUTC('\n');

	// font enLarge = 0, normal, align left
	ada_font_enLarge(0);
	ada_font_bold(0);	
	ada_align_mode(LEFT);

	ada_print_line("------------------------------");
	PUTC('\n');
	PUTC('\n');

	return 0;
}
#endif

/**
	Initialize the printer
		- The print buffer is cleared.
		- Reset the param to default value.
		- Return to standard mode.
		- Delete user_defined characters.
*/
int ada_init_printer()
{
	PUTC(0x1B); 
	PUTC(0x40);
	 
	return 0;
}

/**
	align printer
	0 - left
	1 - center
	2 - right
*/
int ada_align_mode(uint8 align)
{
	PUTC(0x1B); 
	PUTC(0x61);

	PUTC(align); 

	return 0;
}

/**
	left space

	1 => 0.125mm
*/
int ada_left_space(uint8 space)
{
	PUTC(0x1B); 
	PUTC(0x24);

	PUTC(space);
	PUTC(0x00);

	return 0;
}

/**
	font enLarge
*/
int ada_font_enLarge(uint8 large)
{
	PUTC(0x1D); 
	PUTC(0x21);

	PUTC(large);

	return 0;
}

/**
	font bold
		0 - normal
		1 - bold
*/
int ada_font_bold(uint8 bold)
{
	PUTC(0x1B); 
	PUTC(0x45);

	PUTC(bold);

	return 0;
}

/**
	line spacing(0 ~ 255)
		default : 32
*/
int ada_line_spacing(uint8 space)
{
	PUTC(0x1B); 
	PUTC(0x33);

	PUTC(space);

	return 0;
}



int ada_print_error(char *str, int errorNum)
{
	char *p = str;



	while (*p)
	{
		if ((*p == '\r') || (*p == '\n')) break;

		uart_putchar(*p);
		p++;
	}

	// put "end of line : <LF>" char.
	uart_putchar('\n');

	return 0;
}
