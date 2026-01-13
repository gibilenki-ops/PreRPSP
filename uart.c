/**
	STR 

	@project	201506-LGX
	@author	tchan@TSoft
	@date	2015/08/24
*/
/*
	ATMEGA uart common layer.
	- buffered uart io.
	- HRMS: sensor node ? for 14MHz ?
	
	2009/08/18. note: Only 8 MHz clock system is tested.
*/

#include "avr/io.h"
#include "avr/interrupt.h"

#include "uart.h"

#define	UART0_BUFF_SIZE		64

uint8	uart0_rx_buff[UART0_BUFF_SIZE];
uint8	uart0_rx_front = 0;
uint8	uart0_rx_tail = 0;
uint8	uart0_rx_len = 0;

uint8	uart0_tx_buff[UART0_BUFF_SIZE];
uint8	uart0_tx_front = 0;
uint8	uart0_tx_tail = 0;
uint8	uart0_tx_len = 0;


#define DISABLE_INTERRUPT()     asm("cli")
#define ENABLE_INTERRUPT()      asm("sei")


//
#ifdef	UCSR0A
//#pragma	"error;"
	// ATmega 325,
	//
#define	REG_UCSRA	UCSR0A
#define	REG_UCSRB	UCSR0B
#define	REG_UCSRC	UCSR0C
#define	REG_UBRRH	UBRR0H
#define	REG_UBRRL	UBRR0L
#define	REG_UDR		UDR0
#define	BIT_RXC		RXC0
#define	UART_RX_VECT	USART0_RX_vect	
#define	UART_TX_VECT	USART0_TX_vect	
#define	UART_UDRE_VECT	USART0_UDRE_vect	

#elif defined(UCSRA)
	// ATmega8,16,16x,32

#define	REG_UCSRA	UCSRA
#define	REG_UCSRB	UCSRB
#define	REG_UCSRC	UCSRC
#define	REG_UBRRH	UBRRH
#define	REG_UBRRL	UBRRL
#define	REG_UDR		UDR
#define	BIT_RXC		RXC
#define	UART_RX_VECT	USART_RXC_vect	
#define	UART_TX_VECT	USART_TXC_vect	
#define	UART_UDRE_VECT	USART_UDRE_vect	

#else
#pragma	error "F_CPU definition needed"

#endif

/**
	uart initialization	

	BAUD_9600, BAUD_115200
*/
int  uart_init(uint32 baud)
{
	REG_UCSRA = 0x02;	// U2X = 1.
	REG_UCSRB = 0xD8;	// RXCIE, TXCIE, RXEN, TXEN
	REG_UCSRC = 0x06;
	REG_UBRRH = ((F_CPU/8/baud)-1)>>8;
	REG_UBRRL = (F_CPU/8/baud)-1;		// when U2X==1.

 	return 0;
}

ISR(UART_RX_VECT)
{
	//v10. read and save at buffer.
	char status,data; 

	// wait status ready.
    while (((status=REG_UCSRA) & (1<<BIT_RXC))==0); 
	
	data=REG_UDR;
	
	// save at buffer. : check overflow.
	uart0_rx_buff[uart0_rx_tail] = data;
	uart0_rx_tail = (uart0_rx_tail+1) % UART0_BUFF_SIZE;
	uart0_rx_len ++;
} 


ISR(UART_UDRE_VECT)
{
	//v10. write to uart0.
	uint8 c;

    // do first tx. next will be done by txc interrupt.
	if (uart0_tx_len > 0)
	{
		c = uart0_tx_buff[uart0_tx_front];

	    REG_UDR=c; 

		uart0_tx_front = (uart0_tx_front+1) % UART0_BUFF_SIZE;
		uart0_tx_len--;
	}

	// Clear DRE interrupt.
    REG_UCSRB &= ~0x20;    // RXCIE, UDRIE, RXEN, TXEN
} 

ISR(UART_TX_VECT)
{
	//v10. write to uart0.
	uint8 c;

	if (uart0_tx_len > 0)
	{
		c = uart0_tx_buff[uart0_tx_front];
		//while ((UCSR1A & (1<<UDRE))==0);
		// tx isr.
	    REG_UDR=c; 
		uart0_tx_front = (uart0_tx_front+1) % UART0_BUFF_SIZE;
		uart0_tx_len--;
	}
} 

int uart_getchar(uint8 *c) 
{
	// disable_intr
	DISABLE_INTERRUPT();
	if (uart0_rx_len > 0)
	{
		*c = uart0_rx_buff[uart0_rx_front];

		uart0_rx_front = (uart0_rx_front+1) % UART0_BUFF_SIZE;
		uart0_rx_len--;

		//enable_intr
    	ENABLE_INTERRUPT();
		return 1;
	}
	//enable_intr
   	ENABLE_INTERRUPT();
	return 0;
} 

int uart_putchar(uint8 c) 
{ 
	// disable_intr
	DISABLE_INTERRUPT();
    REG_UCSRB |= 0x20;    // UDRIE set : enable DRE interrupt
    
	if (uart0_tx_len < UART0_BUFF_SIZE)
	{
		uart0_tx_buff[uart0_tx_tail] = c;
		uart0_tx_tail = (uart0_tx_tail+1) % UART0_BUFF_SIZE;
		uart0_tx_len ++;

    	//enable_intr
    	ENABLE_INTERRUPT();

		return 1;
	}
	//enable_intr
   	ENABLE_INTERRUPT();
	return 0;
}
