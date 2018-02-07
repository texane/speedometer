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


__attribute__((unused))
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
  if (buf[0] == ',')
  {
    buf[0] = '0';
    buf[1] = '0';
    buf[2] = '0';
    buf[3] = '.';
    buf[4] = '0';
    return ;
  }

  for (i = 1; i != 5; ++i) uart_read_uint8(&buf[i]);
}


__attribute__((unused))
static void gps_next_gpgll(uint8_t* buf, uint8_t* len)
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
  if (c != 'G') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'L') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'L') goto redo_line;

  gps_skip_until(',');

  *len = 0;
  i = 0;
  while (1)
  {
    uart_read_uint8(&c);

    if (c == ',')
    {
      if ((++i) == 3) break ;
    }

    buf[(*len)++] = c;
  }

  uart_read_uint8(&c);
  buf[(*len)++] = c;
}


__attribute__((unused))
static void gps_next_gprmc(uint8_t* buf, uint8_t* len)
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
  if (c != 'R') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'M') goto redo_line;
  uart_read_uint8(&c);
  if (c != 'C') goto redo_line;

  gps_skip_until(',');
  gps_skip_until(',');
  gps_skip_until(',');

  *len = 0;
  i = 0;
  while (1)
  {
    uart_read_uint8(&c);

    if (c == ',')
    {
      if ((++i) == 5) break ;
    }

    buf[(*len)++] = c;
  }
}


__attribute__((unused))
static void gps_unit(void)
{
  static uint8_t buf[32];
  uint8_t i;

  while (1)
  {
    for (i = 0; i != sizeof(buf); ++i) uart_read_uint8(buf + i);
    uart_write(buf, sizeof(buf));
  }
}
