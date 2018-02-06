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

#if 0 /* TODO */
#define CONFIG_SD
#ifdef CONFIG_SD
#include "./spi.c"
#include "./sd.c"
#endif /* CONFIG_SD */
#endif /* 0 */

#define CONFIG_GPS
#ifdef CONFIG_GPS
#include "./gps.c"
#endif /* CONFIG_GPS */


/* main */

int main(void)
{
#ifdef CONFIG_UART
  uart_setup();
#endif /* CONFIG_UART */

#ifdef CONFIG_GPS
  gps_setup();
#endif /* CONFIG_SD */

#ifdef CONFIG_SD
  spi_setup_master();
  sd_setup();
#endif /* CONFIG_SD */

  sei();

#ifdef CONFIG_SD
  sd_unit();
#endif /* CONFIG_SD */

#ifdef CONFIG_GPS
  gps_unit();
#endif /* CONFIG_GPS */

  while (1)
  {
  }

  return 0;
}
