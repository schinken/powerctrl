#include "main.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define USART_BAUD 9600

static inline int uart_putc (const uint8_t c)
{
    while (!(UCSRA & (1 << UDRE)));
    UDR = c;
    return 1;
}
void uart_puts (const char *s){
    do{
        uart_putc (*s);
    }while (*s++);
}

#define SEND_NEWLINE uart_putc(0x0A); uart_putc(0x0D);


#define STAGE_FLOOR     0
#define STAGE_SET       1
#define STAGE_SINGLE    2
#define STAGE_ALL       3
#define STAGE_PORT      4
#define STAGE_DOT       5
#define STAGE_VALUE     6
#define STAGE_FINAL     7

volatile uint8_t  stage = STAGE_FLOOR
                 ,port = 0
                 ,val = 0
                 ,tmp = 0;

///////////////////////////////////////////
// PORTMAPPING
//
// PIN 0 - pd3
// PIN 1 - pd2
// PIN 2 - pc5
// PIN 3 - pc4
// PIN 4 - pc3
// PIN 5 - pc2
// PIN 6 - pc1
// PIN 7 - pc0

const uint8_t portmap[8] = {PIN3,PIN2,PIN5,PIN4,PIN3,PIN2,PIN1,PIN0};

void switch_port( uint8_t port, uint8_t value ) {
    
    volatile uint8_t *pport = 0;

    // PIN 0 and 1 is on a different port
    if( port == 0 || port == 1 ) {
        pport = &PORTD;    
    } else {
        pport = &PORTC;    
    }

    uint8_t tmp = portmap[port];
    
    if( val == 0 ) {
        *pport &= ~(1<<tmp);    
    } else {
        *pport |= (1<<tmp);    
    }
}

ISR( USART_RXC_vect ){

    uint8_t data = UDR;

    uart_putc(data);

    switch( stage ) {
        case STAGE_FLOOR:
            port = 0;
            if( data == 's' ) {
                stage = STAGE_SET;
            }
            break;

        case STAGE_SET:
            if( data == 'p' ) {
                stage = STAGE_SINGLE;
            } else if ( data == 'a' ) {
                stage = STAGE_ALL;
            } else {
                stage = STAGE_FLOOR;
            }
        
            break;

        case STAGE_SINGLE:
            tmp = data-48;
            if( tmp >= 0 || tmp <= 8 ) {
                port = tmp;
                stage = STAGE_DOT;           
            } else {
                stage = STAGE_FLOOR;    
            }
            break;

        case STAGE_ALL:
            port = 0xFF;
            tmp = data-48;
            if( tmp == 0 || tmp == 1 ) {
                val = tmp;
                stage = STAGE_FINAL;
            } else {
                stage = STAGE_FLOOR;    
            }
            break;

        case STAGE_DOT:
            if( data == '.' ) {
                stage = STAGE_VALUE;
            } else {
                stage = STAGE_FLOOR;
            }        
            break;

        case STAGE_VALUE:
            tmp = data-48;
            if( tmp == 0 || tmp == 1 ) {
                val = tmp;
                stage = STAGE_FINAL;
            } else {
                stage = STAGE_FLOOR;    
            }
            break;

        case STAGE_FINAL:
            if( data == '\r' ) {

                PORTD |= (1<<PIN5);

                if( port == 0xFF ) {
                    for( tmp = 0; tmp < 8;tmp++ ) {
                        switch_port( tmp, val );
                    }
                } else {
                    switch_port( port, val );
                }

                SEND_NEWLINE;

                if( port == 0xFF ){
                    if( val == 0 ) {
                        uart_puts("Setting all ports 0!");
                    } else {
                        uart_puts("Setting all ports 1!");
                    }
                } else {
                    switch( port ) {
                        case 0: uart_puts("Set port 0"); break;
                        case 1: uart_puts("Set port 1"); break;
                        case 2: uart_puts("Set port 2"); break;
                        case 3: uart_puts("Set port 3"); break;
                        case 4: uart_puts("Set port 4"); break;
                        case 5: uart_puts("Set port 5"); break;
                        case 6: uart_puts("Set port 6"); break;
                        case 7: uart_puts("Set port 7"); break;
                        default:
                            uart_puts("unknown wtf...");
                            break;
                    }

                    if( val == 1 ) {
                        uart_puts(" to 1");
                    } else {
                        uart_puts(" to 0");
                    }
                }
                SEND_NEWLINE;
                stage = STAGE_FLOOR;
            } else {
                uart_puts("UNKNOWN NEWLINE");    
                stage = STAGE_FLOOR;
            }

            break;
    }

}

int main (void) {            

//    asm("NOP;");

    // INIT DMX
    UCSRC = (1<<USBS) | (1<<UCSZ1) | (1<<UCSZ0); // 2 stop bits, 8 data bits
    UCSRB = (1<<RXEN) | (1<<RXCIE) | (1<<TXEN);               // enable transmit, receive and interrupts,

    uint16_t ubrr = (uint16_t) ((uint32_t) F_CPU/(16UL*USART_BAUD) - 1);

    UBRRH = (uint8_t) (ubrr>>8);
    UBRRL = (uint8_t) (ubrr);

    DDRD |= (1<<PIN5) | (1<<PIN3) | (1<<PIN2);
    DDRC |= (1<<PIN5) | (1<<PIN4) | (1<<PIN3) | (1<<PIN2) | (1<<PIN1) | (1<<PIN0);

    sei();

    while(1) {
        PORTD &= ~(1<<PIN5);
        _delay_ms(1000);
    }

    return 0;                 

}
