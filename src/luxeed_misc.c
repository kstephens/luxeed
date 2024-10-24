#include "luxeed.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h> /* getpid() */
#include <stdarg.h> /* va_list */

const char *luxeed_error_action;
struct luxeed_options luxeed_options;

const char *luxeed_progname = "";
int luxeed_log_level = 0;
void luxeed_main_argv(int argc, char** argv) {
  luxeed_options.progname = luxeed_progname = argv[0];
  const char *s = getenv("LUXEED_LOG_LEVEL");
  if ( s && s[0] )
    luxeed_options.debug = luxeed_log_level = atoi(s);
  // luxeed_log("luxeed_options.debug = %d", luxeed_log_level);
  if ( luxeed_log_level >= 5 )
    luxeed_options.debug_syscall = luxeed_log_level - 5;
  // luxeed_log("luxeed_options.debug_syscall = %d", luxeed_options.debug_syscall);
}
void luxeed_logv_(const char* file, int line, const char* function, const char* fmt, va_list* vap) {
  fprintf(stderr, "luxeed : %s : pid %d : %20s:%-5d : %s() : ", luxeed_progname, getpid(), file, line, function);
  vfprintf(stderr, fmt, *vap);
  fprintf(stderr, "\n");
}
void luxeed_log_(const char* file, int line, const char* function, const char* fmt, ...) {
  va_list va;
  va_start(va, fmt);
  luxeed_logv_(file, line, function, fmt, &va);
  va_end(va);
}
