/*
 * See Copyright Notice in qnode.h
 */

#include <unistd.h>
#include <stdio.h>
#include "qassert.h"
#include "qactor.h"
#include "qconfig.h"
#include "qengine.h"
#include "qdefines.h"
#include "qlist.h"
#include "qlog.h"
#include "qluautil.h"
#include "qmailbox.h"
#include "qmempool.h"
#include "qserver.h"
#include "qthread.h"
#include "qthread_log.h"

static void server_box_func(int fd, int flags, void *data) {
  UNUSED(fd);
  UNUSED(flags);
  qinfo("server box");

  qmsg_t      *msg;
  qlist_t     *list;
  qlist_t     *pos, *next;
  qthread_t   *thread;

  thread = (qthread_t*)data;
  qmailbox_get(thread->in_box[0], &list);
  for (pos = list->next; pos != list; ) {
    msg = qlist_entry(pos, qmsg_t, entry);
    qassert(msg);
    next = pos->next;
    qlist_del_init(&(msg->entry));
    if (msg == NULL) {
      qinfo("msg NULL");
      goto next;
    }
    qinfo("handle %d msg %p", msg->type, msg);
    if (!qmsg_is_smsg(msg)) {
      qerror("msg %d , flag %d is not server msg", msg->type, msg->flag);
      goto next;
    }
    if (qmsg_invalid_type(msg->type)) {
      qerror("msg %d is not valid msg type", msg->type);
      goto next;
    }
    qinfo("handle %d msg", msg->type);
    (g_thread_msg_handlers[msg->type])(thread, msg);

next:
    msg->handled = 1;
    qmsg_destroy(msg);
    pos = next;
  }
}

static void thread_box_func(int fd, int flags, void *data) {
  qinfo("in thread_box_func");
  UNUSED(fd);
  UNUSED(flags);

  qlist_t       *list, *pos, *next;
  qmailbox_t    *box;
  qmsg_t        *msg;
  qthread_box_t *thread_box;
  qthread_t     *thread;

  thread_box = (qthread_box_t*)data;
  thread = thread_box->thread;
  box = thread_box->box;
  qmailbox_get(box, &list);

  for (pos = list->next; pos != list; ) {
    msg = qlist_entry(pos, qmsg_t, entry);
    qassert(msg);
    next = pos->next;
    qlist_del_init(&(msg->entry));
    if (msg == NULL) {
      qinfo("msg NULL");
      goto next;
    }
    qinfo("handle %d msg %p", msg->type, msg);
    if (!qmsg_is_smsg(msg)) {
      qerror("msg %d , flag %d is not server msg", msg->type, msg->flag);
      goto next;
    }
    if (qmsg_invalid_type(msg->type)) {
      qerror("msg %d is not valid msg type", msg->type);
      goto next;
    }
    qinfo("handle %d msg", msg->type);
    (g_thread_msg_handlers[msg->type])(thread, msg);

next:
    msg->handled = 1;
    qmsg_destroy(msg);
    pos = next;
  }
}

static void* main_loop(void *arg) {
  qthread_t *thread;

  thread = (qthread_t*)arg;
  g_server->thread_log[thread->tid] = qthread_log_init(thread->engine, thread->tid);
  thread->started = 1;
  while (!thread->stop && qengine_loop(thread->engine) == 0) {
  }
  return NULL;
}

qthread_t* qthread_new(struct qserver_t *server, qtid_t tid) {
  int           i, thread_num, result;
  qthread_t    *thread;
  qmem_pool_t  *pool;

  pool = qmem_pool_create();
  if (pool == NULL) {
    return NULL;
  }

  thread_num = server->config->thread_num;
  thread = qcalloc(pool, sizeof(qthread_t));
  if (thread == NULL) {
    qerror("create thread error");
    return NULL;
  }
  thread->engine = qengine_new(pool);
  if (thread->engine == NULL) {
    qerror("create thread engine error");
    return NULL;
  }
  thread->tid = tid;

  thread->thread_box = qcalloc(pool, (thread_num + 1) * sizeof(qthread_box_t*));
  if (thread->thread_box == NULL) {
    return NULL;
  }
  thread->in_box  = qcalloc(pool, (thread_num + 1) * sizeof(qmailbox_t*));
  if (thread->in_box == NULL) {
    return NULL;
  }
  thread->out_box = qcalloc(pool, (thread_num + 1) * sizeof(qmailbox_t*));
  if (thread->out_box == NULL) {
    return NULL;
  }
  for (i = 0; i < thread_num + 1; ++i) {
    thread->out_box[i] = NULL;
    if (i == (int)tid) {
      thread->in_box[i] = NULL;
      continue;
    }
    if (i == 0) {
      /* communicate with main thread */
      thread->in_box[i] = qmailbox_new(pool, server_box_func, thread);
      qassert(thread->in_box[i]->signal);
    } else {
      /* communicate with other worker thread */
      thread->thread_box[i] = qcalloc(pool, sizeof(qthread_box_t));
      thread->in_box[i] = qmailbox_new(pool, thread_box_func, thread->thread_box[i]);

      thread->thread_box[i]->thread = thread;
      qassert((char*)(thread->in_box[i]) != (char*)thread);

      thread->thread_box[i]->box = thread->in_box[i];
      qassert(thread->in_box[i]->signal);
    }
    if (thread->in_box[i] == NULL) {
      return NULL;
    }
    qmailbox_active(thread->engine, thread->in_box[i]);
  }

  thread->state = qlua_new_state();
  qlist_entry_init(&(thread->actor_list));
  result = pthread_create(&thread->id, NULL, main_loop, thread);
  qassert(result == 0);
  /* ugly, but works */
  while (thread->started == 0) {
    usleep(100);
  }
  return thread;
}

void qthread_destroy(qthread_t *thread) {
  qmsg_t *msg;

  msg = qmsg_new(0, thread->tid);
  qmsg_init_sstop(msg);
  qmsg_send(msg);

  pthread_join(thread->id, NULL);
}
