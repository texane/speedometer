#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


/* string */

typedef struct string
{
  const char* s;
  size_t len;
} string_t;

static void string_init(string_t* s, const char* dat, size_t len)
{
  s->s = dat;
  s->len = len;
}

__attribute__((unused))
static unsigned int string_is_empty(string_t* s)
{
  return s->len == 0;
}

static unsigned int string_is_char(string_t* s, char c)
{
  return (s->len) && (*s->s == c);
}

static unsigned int string_is_char_null(string_t* s)
{
  return *s->s == 0;
}

static int string_get_double(string_t* s, size_t n, double* x)
{
  char buf[32];
  char* e;
  size_t i;

  if (n == 0) n = s->len;
  if (n >= sizeof(buf)) n = sizeof(buf) - 1;
  if (n > s->len) n = s->len;

  for (i = 0; i != n; ++i) buf[i] = s->s[i];
  buf[i] = 0;

  errno = 0;
  *x = strtod(buf, &e);
  if (errno) return -1;

  i = e - buf;
  s->s += i;
  s->len -= i;

  return 0;
}

static int string_get_uint32(string_t* s, size_t n, uint32_t* x)
{
  double d;
  if (string_get_double(s, n, &d)) return -1;
  *x = (uint32_t)d;
  return 0;
}

static int string_get_size(string_t* s, size_t n, size_t* x)
{
  uint32_t xx;
  if (string_get_uint32(s, n, &xx)) return -1;
  *x = (size_t)xx;
  return 0;
}

__attribute__((unused))
static int string_skip_one(string_t* s)
{
  if (s->len == 0) return -1;
  ++s->s;
  --s->len;
  return 0;
}

static int string_skip_char(string_t* s, char c, size_t n)
{
  for (; n && s->len; ++s->s, --s->len)
  {
    if (*s->s == c) --n;
  }

  return (n == 0) ? 0 : -1;
}

__attribute__((unused))
static void string_print(string_t* s)
{
  size_t i;
  for (i = 0; i != s->len; ++i) printf("%c", s->s[i]);
  printf("\n");
}


/* command line options */

typedef struct
{
#define OPT_FLAG_IPATH (1 << 0)
#define OPT_FLAG_OPATH (1 << 1)
#define OPT_FLAG_FIRST_LINE (1 << 2)
#define OPT_FLAG_LAST_LINE (1 << 3)
#define OPT_FLAG_DATE (1 << 4)
#define OPT_FLAG_DETECT_WAVES (1 << 5)
  uint32_t flags;
  const char* ipath;
  const char* opath;
  size_t first_line;
  size_t last_line;
  uint32_t date[3];
  const char* wave_dir;
} opt_t;

static void opt_init(opt_t* opt)
{
  opt->flags = 0;
  opt->ipath = NULL;
  opt->opath = NULL;
  opt->first_line = 0;
  opt->last_line = 0;
  opt->wave_dir = NULL;
}

static int opt_parse(opt_t* opt, int ac, const char** av)
{
  int err = -1;

  opt_init(opt);

  if ((ac % 2)) goto on_error_0;

  for (; ac; av += 2, ac -= 2)
  {
    const char* const k = av[0];
    const char* const v = av[1];

    if (strcmp(k, "-ipath") == 0)
    {
      opt->flags |= OPT_FLAG_IPATH;
      opt->ipath = v;
    }
    else if (strcmp(k, "-opath") == 0)
    {
      opt->flags |= OPT_FLAG_OPATH;
      opt->opath = v;
    }
    else if (strcmp(k, "-date") == 0)
    {
      string_t s;
      opt->flags |= OPT_FLAG_DATE;
      string_init(&s, v, strlen(v));
      if (string_get_uint32(&s, 2, &opt->date[0])) goto on_error_0;
      if (string_get_uint32(&s, 2, &opt->date[1])) goto on_error_0;
      if (string_get_uint32(&s, 2, &opt->date[2])) goto on_error_0;
    }
    else if (strcmp(k, "-first_line") == 0)
    {
      string_t s;
      opt->flags |= OPT_FLAG_FIRST_LINE;
      string_init(&s, v, strlen(v));
      if (string_get_size(&s, 0, &opt->first_line)) goto on_error_0;
    }
    else if (strcmp(k, "-last_line") == 0)
    {
      string_t s;
      opt->flags |= OPT_FLAG_LAST_LINE;
      string_init(&s, v, strlen(v));
      if (string_get_size(&s, 0, &opt->last_line)) goto on_error_0;
    }
    else if (strcmp(k, "-detect_waves") == 0)
    {
      opt->flags |= OPT_FLAG_DETECT_WAVES;
      opt->wave_dir = v;
    }
    else
    {
      goto on_error_0;
    }
  }

  err = 0;

 on_error_0:
  return err;
}


/* gps */

typedef struct gps_item
{
#define GPS_FLAG_DATE (1 << 0)
#define GPS_FLAG_COORD (1 << 1)
#define GPS_FLAG_SPEED (1 << 2)
  uint32_t flags;

  /* hh/mm/ss/dd/mm/yy */
  uint32_t date[6];
  /* lat lng in decimal degrees */
  double coord[2];
  double nmea_coord[2];
  /* in kmh */
  double speed;

  struct gps_item* prev;
  struct gps_item* next;

} gps_item_t;


__attribute__((unused))
static void gps_print_item(const gps_item_t* it)
{
  size_t i;
  for (i = 0; i != 6; ++i) printf("%02u", it->date[i]);
  printf(" %lf %lf", it->coord[0], it->coord[1]);
  printf(" %lf", it->speed);
  printf("\n");
}

__attribute__((unused))
static void gps_print_list(gps_item_t* li)
{
  gps_item_t* it;
  printf("xxx\n");
  for (it = li; it != NULL; it = it->next) gps_print_item(it);
}

static int gps_load_dat(gps_item_t** li, const char* path)
{
  int fd;
  int err = -1;
#define GPS_LINE_SIZE 64
  char buf[GPS_LINE_SIZE];
  string_t line;
  gps_item_t* prev;
  gps_item_t* it;

  fd = open(path, O_RDONLY);
  if (fd == -1) goto on_error_0;

  *li = NULL;
  prev = NULL;

  while (1)
  {
    const ssize_t n = read(fd, buf, GPS_LINE_SIZE);
    if (n < 0) goto on_error_1;
    if (n == 0) break;
    if (n != GPS_LINE_SIZE) goto on_error_1;

    string_init(&line, buf, GPS_LINE_SIZE);

    /* end of file */
    if (string_is_char_null(&line)) break ;

    it = malloc(sizeof(gps_item_t));
    if (it == NULL) goto on_error_1;

    it->flags = 0;
    it->date[0] = 0;
    it->date[1] = 0;
    it->date[2] = 0;
    it->date[3] = 0;
    it->date[4] = 0;
    it->date[5] = 0;
    it->coord[0] = 0.0;
    it->coord[1] = 0.0;
    it->speed = 0.0;
    it->next = NULL;

    if (prev != NULL) prev->next = it;
    else *li = it;
    it->prev = prev;
    prev = it;

    /* printf("a: "); string_print(&line); */

    if (string_is_char(&line, ',') == 0)
    {
      it->flags |= GPS_FLAG_DATE;
      if (string_get_uint32(&line, 2, &it->date[0])) goto on_error_1;
      if (string_get_uint32(&line, 2, &it->date[1])) goto on_error_1;
      if (string_get_uint32(&line, 2, &it->date[2])) goto on_error_1;
    }

    if (string_skip_char(&line, ',', 2)) goto on_error_1;

    /* printf("b: "); string_print(&line); */

    if (string_is_char(&line, ',') == 0)
    {
      /* latitude, dms to decimal degrees */

      uint32_t ui;
      double d;
      it->flags |= GPS_FLAG_COORD;
      if (string_get_uint32(&line, 2, &ui)) goto on_error_1;
      if (string_get_double(&line, 0, &d)) goto on_error_1;
      it->coord[0] = (double)ui + d / 60.0;
      it->nmea_coord[0] = (double)ui * 100.0 + d;
      if (string_skip_char(&line, ',', 1)) goto on_error_1;
      if (string_is_char(&line, 'S')) it->coord[0] *= -1.0;
    }
    else
    {
      if (string_skip_char(&line, ',', 1)) goto on_error_1;
    }

    if (string_skip_char(&line, ',', 1)) goto on_error_1;

    /* printf("c: "); string_print(&line); */

    if (string_is_char(&line, ',') == 0)
    {
      /* longitude, dms to decimal degrees */

      uint32_t ui;
      double d;
      it->flags |= GPS_FLAG_COORD;
      if (string_get_uint32(&line, 3, &ui)) goto on_error_1;
      if (string_get_double(&line, 0, &d)) goto on_error_1;
      it->coord[1] = (double)ui + d / 60.0;
      it->nmea_coord[1] = (double)ui * 100.0 + d;
      if (string_skip_char(&line, ',', 1)) goto on_error_1;
      if (string_is_char(&line, 'W')) it->coord[1] *= -1.0;
    }
    else
    {
      if (string_skip_char(&line, ',', 2)) goto on_error_1;
    }

    if (string_skip_char(&line, ',', 1)) goto on_error_1;

    /* printf("d: "); string_print(&line); */

    if (string_is_char(&line, ',') == 0)
    {
      /* speed, knot to kmh */
      double d;
      it->flags |= GPS_FLAG_SPEED;
      if (string_get_double(&line, 0, &d)) goto on_error_1;
      it->speed = d * 1.852;
    }

    if (string_skip_char(&line, ',', 1)) goto on_error_1;

    /* printf("e: "); string_print(&line); */

    if (string_is_char(&line, ',') == 0)
    {
      it->flags |= GPS_FLAG_DATE;
      if (string_get_uint32(&line, 2, &it->date[3])) goto on_error_1;
      if (string_get_uint32(&line, 2, &it->date[4])) goto on_error_1;
      if (string_get_uint32(&line, 2, &it->date[5])) goto on_error_1;
    }

    /* gps_print_item(it); */
  }

  err = 0;

 on_error_1:
  if (err)
  {
    /* TODO */
    /* free_gps_list(*li); */
    /* *li = NULL; */
  }
  close(fd);
 on_error_0:
  return err;
}


/* writer */

typedef struct writer
{
  int (*header)(struct writer*);
  int (*item)(struct writer*, const gps_item_t*);
  int (*footer)(struct writer*);
  FILE* file;
} writer_t;

#define writer_printf(__w, __fmt, ...) \
 fprintf((__w)->file, __fmt, ##__VA_ARGS__)

static int gpx_header(struct writer* w)
{
  writer_printf
  (
   w,
   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
   "<gpx"
   " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
   " xmlns=\"http://www.topografix.com/GPX/1/0\""
   " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0/gpx.xsd\""
   " version=\"1.0\""
   " creator=\"gpx.py -- https://github.com/tkrajina/gpxpy\""
   ">\n"
   "<trk>"
   "<trkseg>\n"
  );

  return 0;
}

static int gpx_item(struct writer* w, const gps_item_t* i)
{
  if ((i->flags & GPS_FLAG_COORD) == 0) return 0;

  writer_printf
  (
   w,
   "<trkpt lat=\"%lf\" lon=\"%lf\"></trkpt>\n",
   i->coord[0], i->coord[1]
  );

  return 0;
}

static int gpx_footer(struct writer* w)
{
  writer_printf(w, "</trkseg></trk></gpx>\n");

  return 0;
}

static int plt_header(struct writer* w)
{
  return 0;
}

static int plt_item(struct writer* w, const gps_item_t* i)
{
  writer_printf(w, "%lf\n", i->speed);
  return 0;
}

static int plt_footer(struct writer* w)
{
  return 0;
}

static int nmea_header(struct writer* w)
{
  return 0;
}

static int nmea_item(struct writer* w, const gps_item_t* i)
{
  /* GPRMC line format */

  size_t len;

  len = 0;

  if (i->flags & GPS_FLAG_DATE)
  {
    len += (size_t)writer_printf
      (w, "%02u%02u%02u,", i->date[0], i->date[1], i->date[2]);
  }
  else
  {
    len += (size_t)writer_printf(w, ",");
  }

  len += (size_t)writer_printf(w, "A,");

  if (i->flags & GPS_FLAG_COORD)
  {
    /* convert in nmea format */

    char c;
    double ui;
    double d;
  
    if (i->coord[0] < 0) c = 'S';
    else c = 'N';
    ui = trunc(i->nmea_coord[0]);
    d = (i->nmea_coord[0] - ui) * 100000.0;
    len += (size_t)writer_printf(w, "%04u.%05u,%c,", (unsigned int)ui, (unsigned int)d, c);

    if (i->coord[1] < 0) c = 'W';
    else c = 'E';
    ui = trunc(i->nmea_coord[1]);
    d = (i->nmea_coord[1] - ui) * 100000.0;
    len += (size_t)writer_printf(w, "%05u.%05u,%c,", (unsigned int)ui, (unsigned int)d, c);
  }
  else
  {
    len += (size_t)writer_printf(w, ",,,,");
  }

  if (i->flags & GPS_FLAG_SPEED)
  {
    /* kmh to knot */
    double d;
    d = i->speed / 1.852;
    len += (size_t)writer_printf(w, "%.3lf,", d);
  }
  else
  {
    len += (size_t)writer_printf(w, ",");
  }

  if (i->flags & GPS_FLAG_DATE)
  {
    len += (size_t)writer_printf
      (w, "%02u%02u%02u", i->date[3], i->date[4], i->date[5]);
  }
  else
  {
    len += (size_t)writer_printf(w, ",");
  }

  len += (size_t)writer_printf(w, ",,");

  len += (size_t)writer_printf(w, "\n");

  for (; len != GPS_LINE_SIZE; ++len)
  {
    writer_printf(w, " ");
  }

  return 0;
}

static int nmea_footer(struct writer* w)
{
  return 0;
}

static int writer_open(writer_t* w, const char* path)
{
  size_t i;

  i = strlen(path);
  if (i == 0) return -1;
  for (i -= 1; i; --i)
  {
    if (path[i] == '.') break ;
  }
  if (i == 0) return -1;

  if (strcmp(path + i, ".gpx") == 0)
  {
    w->header = gpx_header;
    w->item = gpx_item;
    w->footer = gpx_footer;
  }
  else if (strcmp(path + i, ".plt") == 0)
  {
    w->header = plt_header;
    w->item = plt_item;
    w->footer = plt_footer;
  }
  else if (strcmp(path + i, ".nmea") == 0)
  {
    w->header = nmea_header;
    w->item = nmea_item;
    w->footer = nmea_footer;
  }
  else
  {
    return -1;
  }

  w->file = fopen(path, "w+");
  if (w->file == NULL) return -1;

  return 0;
}

static int writer_close(writer_t* w)
{
  fclose(w->file);
  return 0;
}

static int writer_write
(writer_t* w, const gps_item_t* first, const gps_item_t* last, const opt_t* opt)
{
  size_t i;
  size_t n;

  if (w->header(w)) goto on_error_0;

  i = 0;
  n = 0;
  for (; first != last; first = first->next)
  {
    if ((opt->flags & OPT_FLAG_FIRST_LINE) && (i < opt->first_line)) goto skip_line;
    if ((opt->flags & OPT_FLAG_LAST_LINE) && (i > opt->last_line)) goto skip_line;

    if (w->item(w, first)) goto on_error_0;

    ++n;

  skip_line:
    ++i;
  }
  
  if (w->footer(w)) goto on_error_0;

  return 0;

 on_error_0:
  return -1;
}

__attribute__((unused))
static size_t gps_sec_to_nline(double x)
{
  /* fsampl = 1Hz */
  return (size_t)ceil(x);
}


/* wave detection algorithm */

static int detect_waves(const gps_item_t* i, const opt_t* o)
{
  static const double threshold = 15.0;
  const gps_item_t* first;
  const gps_item_t* last;
  unsigned int n;
  int err = -1;

  errno = 0;
  if (mkdir(o->wave_dir, 0755))
  {
    if (errno != EEXIST)
    {
      goto on_error_0;
    }
  }

  n = 0;

  first = NULL;
  last = NULL;

  for (; i != NULL; i = i->next)
  {
    if ((i->flags & GPS_FLAG_SPEED) == 0) continue ;

    if (first == NULL)
    {
      if (i->speed >= threshold)
      {
	/* TODO: take a few points before */
	printf("new_wave\n");
	first = i;
      }
    }
    else
    {
      if (i->speed < threshold)
      {
	writer_t w;
	char path[64];
	opt_t oo;

	/* TODO: take a few points after */

	/* write wave in nmea format */

	snprintf(path, sizeof(path), "%s/%04x.nmea", o->wave_dir, n);

	if (writer_open(&w, path))
	{
	  goto on_error_1;
	}

	opt_init(&oo);
	writer_write(&w, first, last, &oo);
	writer_close(&w);

	++n;
	first = NULL;
	last = NULL;
      }
    }
  }

  err = 0;

 on_error_1:
 on_error_0:
  return err;
}


/* main */

int main(int ac, char** av)
{
  gps_item_t* li;
  opt_t opt;
  writer_t w;
  int err = -1;

  if (opt_parse(&opt, ac - 1, (const char**)(av + 1)))
  {
    goto on_error_0;
  }

  if ((opt.flags & OPT_FLAG_IPATH) == 0)
  {
    goto on_error_0;
  }

  if (gps_load_dat(&li, opt.ipath))
  {
    goto on_error_1;
  }

  if (opt.flags & OPT_FLAG_DETECT_WAVES)
  {
    if (detect_waves(li, &opt))
    {
      goto on_error_1;
    }
  }
  else
  {
    if ((opt.flags & OPT_FLAG_OPATH) == 0)
    {
      goto on_error_0;
    }

    if (writer_open(&w, opt.opath))
    {
      goto on_error_0;
    }

    err = writer_write(&w, li, NULL, &opt);

    writer_close(&w);

    if (err)
    {
      goto on_error_1;
    }
  }

  err = 0;
 on_error_1:
  /* TODO: gps_free_list */
 on_error_0:
  return err;
}
