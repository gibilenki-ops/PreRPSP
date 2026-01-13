/*
	ATMEGA uart common layer.
	- buffered uart io.
	- HRMS: sensor node ? for 14MHz ?
	
	2009/08/18. note: Only 8 MHz clock system is tested.
*/
#include "avr/io.h"
#include "avr/interrupt.h"

#include "uart1.h"


#define	UART1_BUFF_SIZE		64

uint8	uart1_rx_buff[UART1_BUFF_SIZE];
uint8	uart1_rx_front = 0;
uint8	uart1_rx_tail = 0;
uint8	uart1_rx_len = 0;

    // only used when uart1 tx interrupt is used.
uint8	uart1_tx_buff[UART1_BUFF_SIZE];
uint8	uart1_tx_front = 0;
uint8	uart1_tx_tail = 0;
uint8	uart1_tx_len = 0;


#define DISABLE_INTERRUPT()     asm("cli")
#define ENABLE_INTERRUPT()      asm("sei")


/*
	Using 8MHz clock.
*/
int  uart1_init(uint32 baud)
{
	UCSR1A = 0x02;	// U2X = 1.
	UCSR1B = 0xD8;	// RXCIE, TXCIE, RXEN, TXEN
	UCSR1C = 0x06;
	UBRR1H = ((F_CPU/8/baud)-1)>>8;
	UBRR1L = (F_CPU/8/baud)-1;		// when U2X==1.

 	return 0;
}


//-----------------------------------------------------------------------------
                      
ISR(USART1_RX_vect)
{
	//v10. read and save at buffer.
	char status,data; 

	// wait status ready.
    while (((status=UCSR1A) & (1<<RXC1))==0); 
	
	data=UDR1;
	
	// save at buffer. : check overflow.
	uart1_rx_buff[uart1_rx_tail] = data;
	uart1_rx_tail = (uart1_rx_tail+1) % UART1_BUFF_SIZE;
	uart1_rx_len ++;
} 

ISR(USART1_UDRE_vect)
{
	//v10. write to uart1.
	uint8 c;

    // do first tx. next will be done by txc interrupt.
	if (uart1_tx_len > 0)
	{
		c = uart1_tx_buff[uart1_tx_front];

	    UDR1=c; 

		uart1_tx_front = (uart1_tx_front+1) % UART1_BUFF_SIZE;
		uart1_tx_len--;
	}

	// Clear DRE interrupt.
    UCSR1B &= ~0x20;    // RXCIE, UDRIE, RXEN, TXEN
} 

ISR(USART1_TX_vect)
{
	//v10. write to uart1.
	uint8 c;

	if (uart1_tx_len > 0)
	{
		c = uart1_tx_buff[uart1_tx_front];
		//while ((UCSR1A & (1<<UDRE))==0);
		// tx isr.
	    UDR1=c; 
		uart1_tx_front = (uart1_tx_front+1) % UART1_BUFF_SIZE;
		uart1_tx_len--;
	}
} 

int uart1_getchar(uint8 *c) 
{
	// disable_intr
	DISABLE_INTERRUPT();
	if (uart1_rx_len > 0)
	{
		*c = uart1_rx_buff[uart1_rx_front];

		uart1_rx_front = (uart1_rx_front+1) % UART1_BUFF_SIZE;
		uart1_rx_len--;

		//enable_intr
    	ENABLE_INTERRUPT();
		return 1;
	}
	//enable_intr
   	ENABLE_INTERRUPT();
	return 0;
} 


int uart1_putchar(uint8 c) 
{ 
	// disable_intr
	DISABLE_INTERRUPT();
    UCSR1B |= 0x20;    // UDRIE set : enable DRE interrupt
    
	if (uart1_tx_len < UART1_BUFF_SIZE)
	{
		uart1_tx_buff[uart1_tx_tail] = c;
		uart1_tx_tail = (uart1_tx_tail+1) % UART1_BUFF_SIZE;
		uart1_tx_len ++;

    	//enable_intr
    	ENABLE_INTERRUPT();

		return 1;
	}
	//enable_intr
   	ENABLE_INTERRUPT();
	return 0;
}

