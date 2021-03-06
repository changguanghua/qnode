/*
 * See Copyright Notice in qnode.h
 */

#ifndef __QCONFIG_H__
#define __QCONFIG_H__

#include "qcore.h"
#include "qstring.h"

struct qconfig_t {
  int         thread_num;
  qstring_t   script_path;
  qstring_t   log_path;
  qstring_t   log_level;
  qstring_t   log_handler;
  qmem_pool_t *pool;
};

int  qconfig_init(qconfig_t *config, const char *filename);
void qconfig_free(qconfig_t *config);

#endif  /* __QCONFIG_H__ */
