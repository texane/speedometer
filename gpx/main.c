#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>


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
  /* in kmh */
  double speed;
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


static int gps_write_gpx(gps_item_t* li, const char* path)
{
  static const char* const gpx_header =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<gpx"
    " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
    " xmlns=\"http://www.topografix.com/GPX/1/0\""
    " xsi:schemaLocation=\"http://www.topografix.com/GPX/1/0/gpx.xsd\""
    " version=\"1.0\""
    " creator=\"gpx.py -- https://github.com/tkrajina/gpxpy\""
    ">\n"
    "<trk>"
    "<trkseg>\n";

  printf("%s", gpx_header);

  for (; li != NULL; li = li->next)
  {
    if ((li->flags & GPS_FLAG_COORD) == 0) continue ;

    printf("<trkpt lat=\"%lf\" lon=\"%lf\">\n", li->coord[0], li->coord[1]);

#if 0
    printf("<extensions>");
    printf("<gpxtpx:TrackPointExtension>\n");
    printf("<gpxtpx:speed>%lf</gpxtpx:speed>\n", li->speed);
    printf("</gpxtpx:TrackPointExtension>\n");
    printf("</extensions>\n");
#endif

    printf("</trkpt>\n");
  }

  printf("</trkseg></trk></gpx>\n");

  return 0;
}


static int gps_write_plot(gps_item_t* li, const char* path)
{
  for (; li != NULL; li = li->next)
  {
    printf("%lf\n", li->speed);
  }

  return 0;
}


static int gps_load_dat(gps_item_t** li, const char* path, size_t nline)
{
  int fd;
  int err = -1;
#define GPS_LINE_SIZE 64
  char buf[GPS_LINE_SIZE];
  string_t line;
  gps_item_t** prev;
  gps_item_t* it;

  *li = NULL;

  fd = open(path, O_RDONLY);
  if (fd == -1) goto on_error_0;

  *li = NULL;
  prev = li;

  while (1)
  {
    const ssize_t n = read(fd, buf, GPS_LINE_SIZE);
    if (n < 0) goto on_error_1;
    if (n == 0) break;
    if (n != GPS_LINE_SIZE) goto on_error_1;

    string_init(&line, buf, GPS_LINE_SIZE);

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

    *prev = it;
    prev = &it->next;

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

    if ((nline != 0) && ((--nline) == 0)) break ;
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


__attribute__((unused))
static size_t gps_sec_to_nline(double x)
{
  /* fsampl = 1Hz */
  return (size_t)ceil(x);
}


/* command line options */

typedef struct
{
#define OPT_FLAG_IPATH (1 << 0)
#define OPT_FLAG_OPATH (1 << 1)
#define OPT_FLAG_OFMT (1 << 2)
#define OPT_FLAG_NLINE (1 << 3)
  uint32_t flags;
  const char* ipath;
  const char* opath;
  const char* ofmt;
  size_t nline;
} opt_t;


static int opt_parse(opt_t* opt, int ac, const char** av)
{
  int err = -1;

  opt->flags = 0;
  opt->ipath = NULL;
  opt->opath = NULL;
  opt->ofmt = NULL;
  opt->nline = 0;

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
    else if (strcmp(k, "-ofmt") == 0)
    {
      opt->flags |= OPT_FLAG_OFMT;
      opt->ofmt = v;
    }
    else if (strcmp(k, "-nline") == 0)
    {
      char buf[32];
      strncpy(buf, v, sizeof(buf) - 1);
      buf[sizeof(buf) - 1] = 0;
      opt->flags |= OPT_FLAG_NLINE;
      opt->nline = (size_t)strtoul(buf, NULL, 10);
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


/* main */

int main(int ac, char** av)
{
#define PATH "../dat/road_140218."
/* #define PATH "../dat/road_082018." */
/* #define PATH "../dat/siouville_082018." */
#define DAT_PATH PATH "dat"
#define GPX_PATH PATH "gpx"

  gps_item_t* li;
  opt_t opt;
  int err = -1;

  if (opt_parse(&opt, ac - 1, (const char**)(av + 1)))
  {
    goto on_error_0;
  }

  if ((opt.flags & OPT_FLAG_IPATH) == 0)
  {
    goto on_error_0;
  }

  if ((opt.flags & OPT_FLAG_NLINE) == 0)
  {
    opt.flags |= OPT_FLAG_NLINE;
    opt.nline = 0;
  }

  if ((opt.flags & OPT_FLAG_OFMT) == 0)
  {
    opt.flags |= OPT_FLAG_OFMT;
    opt.ofmt = "gpx";
  }

  if (gps_load_dat(&li, opt.ipath, opt.nline))
  {
    goto on_error_0;
  }

  /* gps_print_list(li); */

  if (strcmp(opt.ofmt, "gpx") == 0)
  {
    if (gps_write_gpx(li, opt.opath)) goto on_error_1;
  }
  else if (strcmp(opt.ofmt, "plt") == 0)
  {
    if (gps_write_plot(li, opt.opath)) goto on_error_1;
  }

  err = 0;
 on_error_1:
  /* TODO: gps_free_list */
 on_error_0:
  return err;
}
