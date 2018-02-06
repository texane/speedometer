static void gps_setup(void)
{
}


static void gps_unit(void)
{
  static uint8_t buf[16];
  uint8_t i;
  uint8_t x;

 redo_line:

  i = 0;

  while (1)
  {
    uart_read_uint8(&x);
    if (x == '\n') break ;
    if (i == sizeof(buf)) continue ;
    buf[i++] = x;
  }

  uart_write(buf, i);
  uart_write_rn();

  goto redo_line;
}
