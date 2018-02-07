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

#define CONFIG_GPS
#ifdef CONFIG_GPS
#include "./gps.c"
#endif /* CONFIG_GPS */


/* main */

int main(void)
{
  uint8_t* buf;
  uint8_t i;
  uint8_t len;
  uint16_t bid;

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

  /* TODO: find first unused block */
  bid = 0;

  while (1)
  {
    buf = sd_block_buf;

    /* warning: i uint8_t, cannot exceed 255 */
#define LOG_BLOCK_SIZE 32
    for (i = 0; i != (SD_BLOCK_SIZE / LOG_BLOCK_SIZE); ++i)
    {
#if 0 /* if GPRMC not available */
      gps_next_gpgll(buf, &len);
      buf[len] = ',';
      gps_next_gpvtg(buf + len + 1);
      len += 1 + 5;
#else
      gps_next_gprmc(buf, &len);
#endif

      buf[len++] = '\n';

#if 1
      uart_write(buf, len - 1);
      uart_write_rn();
#endif

      buf += LOG_BLOCK_SIZE;
    }

    sd_write_block(bid);

    bid += 1;
  }

  return 0;
}
