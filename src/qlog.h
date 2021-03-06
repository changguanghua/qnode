/*
 * See Copyright Notice in qnode.h
 */

#ifndef __QLOG_H__
#define __QLOG_H__

#include <stdarg.h>
#include <string.h>
#include "qcore.h"
#include "qlist.h"

#define QLOG_STDERR      0
#define QLOG_EMERG       1
#define QLOG_ALERT       2
#define QLOG_CRIT        3
#define QLOG_ERR         4
#define QLOG_WARN        5
#define QLOG_NOTICE      6
#define QLOG_INFO        7
#define QLOG_DEBUG       8

#define QMAX_LOG_SIZE    1000
#define QMAX_FORMAT_SIZE 1000
#define QMAX_FILE_SIZE   200

struct qlog_t {
  qlist_t entry;
  int     level;
  char    file[QMAX_FILE_SIZE];
  size_t  file_len;
  int     line;
  char    buff[QMAX_LOG_SIZE];
  int     n;
  int     idx;
  char    format[QMAX_FORMAT_SIZE];
  size_t  fmt_len;
  va_list args;
};

extern int g_log_level;
void qlog(int level, const char* file, long line, const char *format, ...);

#define qerror(args...) qlog(QLOG_ERR,   __FILE__, __LINE__, args)
#define qinfo(args...)  qlog(QLOG_INFO,  __FILE__, __LINE__, args)
#define qdebug(args...) qlog(QLOG_DEBUG, __FILE__, __LINE__, args)

#endif  /* __QLOG_H__ */
