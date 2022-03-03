/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void sema_init(struct semaphore *sema, unsigned value)
{
  ASSERT(sema != NULL);

  sema->value = value;
  // sema->highest_priority_waiter = NULL;
  list_init(&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void sema_down(struct semaphore *sema)
{
  enum intr_level old_level;

  ASSERT(sema != NULL);
  ASSERT(!intr_context());

  struct thread *current = thread_current();

  old_level = intr_disable();
  while (sema->value == 0)
  {
    //等待信号量，把自己加入等待队列
    list_push_back(&sema->waiters, &current->wait_elem);
    thread_block();
  }
  sema->value--;
  intr_set_level(old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema)
{
  enum intr_level old_level;
  bool success;

  ASSERT(sema != NULL);

  old_level = intr_disable();
  if (sema->value > 0)
  {
    sema->value--;
    success = true;
  }
  else
    success = false;
  intr_set_level(old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.
   This function may be called from an interrupt handler. */
void sema_up(struct semaphore *sema)
{
  enum intr_level old_level;
  ASSERT(sema != NULL);

  old_level = intr_disable();
  struct list_elem *e;
  struct list_elem *highest_priority_e = NULL;
  int highest_priority = PRI_MIN - 1;

  if (!list_empty(&sema->waiters))
  {
    //优先级调度，找到等待队列里优先级最高的线程
    for (e = list_begin(&sema->waiters); e != list_end(&sema->waiters); e = list_next(e))
    {
      struct thread *t = list_entry(e, struct thread, wait_elem);
      if (t->priority > highest_priority)
      {
        highest_priority = t->priority;
        highest_priority_e = e;
      }
    }
    //将其移除等待队列 并且停止阻塞
    list_remove(highest_priority_e);
    thread_unblock(list_entry(highest_priority_e, struct thread, wait_elem));
  }

  sema->value++;

  if (!thread_mlfqs)
  {
    //确认是否有人捐赠，如果有，那么这个semaphore 应该是被封装在lcok里面起作用
    if (thread_current()->donate_count > 0 && highest_priority == thread_current()->priority)
    {
      //确认已经有人捐赠过,让捐赠人运行一会儿，如果捐赠数不变了，那么就没有捐赠人了
      int donate_count;
      do
      {
        donate_count = thread_current()->donate_count;
        thread_yield();
      } while (donate_count != thread_current()->donate_count);

      //此时捐赠已经完成，如果在这期间重新设置了自己的优先级，应该在此时生效
      if (thread_current()->priority_after_donation != -1)
      {
        int set_priority = thread_current()->priority_after_donation;
        thread_current()->priority_after_donation = -1;
        thread_set_priority(set_priority);
      }
    }
  }

  //如果没有人捐赠，而且优先级高，此时用的semaphore应该就是semaphore本身，此时应该yield
  if (highest_priority > thread_current()->priority)
    thread_yield();

  //重新维护中断状态
  intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void sema_self_test(void)
{
  struct semaphore sema[2];
  int i;

  printf("Testing semaphores...");
  sema_init(&sema[0], 0);
  sema_init(&sema[1], 0);
  thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++)
  {
    sema_up(&sema[0]);
    sema_down(&sema[1]);
  }
  printf("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper(void *sema_)
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++)
  {
    sema_down(&sema[0]);
    sema_up(&sema[1]);
  }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock)
{
  ASSERT(lock != NULL);

  lock->holder = NULL;
  sema_init(&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void lock_acquire(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(!lock_held_by_current_thread(lock));
  int old_priority;
  struct thread *holder = NULL;
  enum intr_level old_level;
  bool have_donated = false;

  old_level = intr_disable();
  //之前我们已经ASSERT确保了没有锁，而还没有开始获取锁，所以下文在 sema_down() 之前都没有锁
  //在当前位置,锁没有主人时可以拿到锁,否则一定会失败（在单核cpu下,当前只有此线程在运行)
  //为了保证内核调度时，拿到锁的线程的优先级不低于当前线程的优先级，进行优先级捐赠
  //关中断，访问临界资源 priority
  if (lock->holder != NULL)
  {
    holder = lock->holder;
    struct thread *current = holder;

    if (!thread_mlfqs)
    {
      //如果锁的拥有者优先级低，进行捐赠
      if (lock->holder->priority < thread_current()->priority)
      {
        have_donated = true;

        //对当前要获取的锁 的拥有人进行捐赠
        old_priority = holder->priority;
        int donate_priority = thread_current()->priority;
        holder->priority = donate_priority;
        holder->donate_count++;

        //此时被捐赠人可能也已经捐赠了别人
        //因为被捐赠人捐赠了这个关系上的所有人
        //所以这个关系上的优先级应该都等于被捐赠人，不用再进行判断
        //并且重新捐赠所有人
        while (current->lock_waiting_for != NULL)
        {
          ASSERT(current->lock_waiting_for->holder != NULL);
          current->lock_waiting_for->holder->priority = donate_priority;
          current->lock_waiting_for->holder->donate_count++;
          current = current->lock_waiting_for->holder;
        }
      }
    }
  }

  //等待锁
  thread_current()->lock_waiting_for = lock;
  sema_down(&lock->semaphore);
  thread_current()->lock_waiting_for = NULL;

  //OK 已经拿到锁，阻塞的线程已经完成，之前如果进行了捐赠，捐赠的线程已经完成了任务，恢复其优先级
  //此时被捐赠线程的优先级和当前线程的优先级一样，但是当前线程捐赠的线程 yield 了自己，给了当前线程机会运行，要在时间片内恢复被捐赠人的优先级
  //被捐赠人已经脱离了整个的捐赠嵌套关系，恢复捐被赠人的优先级即可
  //但依旧以防万一，还不能恢复中断
  if (!thread_mlfqs)
  {
    if (have_donated == true)
    {
      holder->priority = old_priority;
      holder->donate_count--;
    }
  }
  lock->holder = thread_current();
  //此时重新维护中断状态
  intr_set_level(old_level);
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool lock_try_acquire(struct lock *lock)
{
  bool success;

  ASSERT(lock != NULL);
  ASSERT(!lock_held_by_current_thread(lock));

  success = sema_try_down(&lock->semaphore);
  if (success)
    lock->holder = thread_current();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void lock_release(struct lock *lock)
{
  ASSERT(lock != NULL);
  ASSERT(lock_held_by_current_thread(lock));

  lock->holder = NULL;
  sema_up(&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be rac
   y.) */
bool lock_held_by_current_thread(const struct lock *lock)
{
  ASSERT(lock != NULL);

  return lock->holder == thread_current();
}

/* One semaphore in a list. */
struct semaphore_elem
{
  struct list_elem elem;      /* List element. */
  struct semaphore semaphore; /* This semaphore. */
  //增加的属性，拥有这个信号量的线程
  struct thread *t;
};

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void cond_init(struct condition *cond)
{
  ASSERT(cond != NULL);

  list_init(&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock)
{
  struct semaphore_elem waiter;

  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  sema_init(&waiter.semaphore, 0);
  //初始化waiter 的持有者，在semaphore 中追踪当前进程
  waiter.t = thread_current();
  list_push_back(&cond->waiters, &waiter.elem);
  lock_release(lock);
  sema_down(&waiter.semaphore);
  lock_acquire(lock);
}

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_signal(struct condition *cond, struct lock *lock UNUSED)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);
  ASSERT(!intr_context());
  ASSERT(lock_held_by_current_thread(lock));

  if (!list_empty(&cond->waiters))
  {
    struct semaphore *sema_to_up = NULL;
    struct list_elem *e_high_priority = NULL;
    int highest_priority = PRI_MIN - 1;
    for (struct list_elem *e = list_front(&cond->waiters); e != list_end(&cond->waiters); e = list_next(e))
    {
      struct thread *thread = list_entry(e, struct semaphore_elem, elem)->t;
      struct semaphore *sema = &list_entry(e, struct semaphore_elem, elem)->semaphore;
      if (thread->priority > highest_priority)
      {
        highest_priority = thread->priority;
        sema_to_up = sema;
        e_high_priority = e;
      }
    }
    list_remove(e_high_priority);
    sema_up(sema_to_up);
  }
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void cond_broadcast(struct condition *cond, struct lock *lock)
{
  ASSERT(cond != NULL);
  ASSERT(lock != NULL);

  while (!list_empty(&cond->waiters))
    cond_signal(cond, lock);
}
