#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <util/delay_basic.h>

#define CONFIG_UART
#ifdef CONFIG_UART
#include "./uart.c"
#endif /* CONFIG_UART */

#define CONFIG_SD
#ifdef CONFIG_SD
#include "./spi.c"
#include "./sd.c"
#endif /* CONFIG_SD */


/* main */

int main(void)
{
#ifdef CONFIG_UART
  uart_setup();
#endif /* CONFIG_UART */

#ifdef CONFIG_SD
  spi_setup_master();
  sd_setup();
#endif /* CONFIG_SD */

  sei();

#ifdef CONFIG_SD
  sd_unit();
#endif /* CONFIG_SD */

  while (1)
  {
    _delay_ms(250);
    _delay_ms(250);
    _delay_ms(250);
    _delay_ms(250);

    UART_WRITE_STRING("delay");
    uart_write_rn();
  }

  return 0;
}
