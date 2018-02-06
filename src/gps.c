static void gps_setup(void)
{
}


static void gps_skip_until(uint8_t c)
{
  uint8_t x;

  while (1)
  {
    if (uart_read_uint8(&x)) continue ;
    if (x == c) break ;
  }
}


static void gps_next_gpvtg(uint8_t* buf)
{
  uint8_t i;
  uint8_t c;

 redo_line:
  gps_skip_until('$');

  uart_read_uint8(&c);
  if (c != 'G') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'P') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'V') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'T') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'G') goto redo_line;

  for (i = 0; i != 7; ++i) gps_skip_until(',');

  uart_read_uint8(&buf[0]);
  if (buf[0] == ',') goto redo_line;
  for (i = 1; i != 5; ++i) uart_read_uint8(&buf[i]);
}


static void gps_unit(void)
{
  static uint8_t buf[5];

  while (1)
  {
    gps_next_gpvtg(buf);
    uart_write(buf, 5);
    uart_write_rn();
  }
}
