#include "tests/threads/tests.h"
#include <debug.h>
#include <string.h>
#include <stdio.h>

struct test
{
  const char *name;
  test_func *function;
};

static const struct test tests[] =
    {
        {"alarm-single", test_alarm_single},                           //c
        {"alarm-multiple", test_alarm_multiple},                       //c
        {"alarm-simultaneous", test_alarm_simultaneous},               //c
        {"alarm-priority", test_alarm_priority},                       //c
        {"alarm-zero", test_alarm_zero},                               //c
        {"alarm-negative", test_alarm_negative},                       //c
        {"priority-change", test_priority_change},                     //c
        {"priority-donate-one", test_priority_donate_one},             //c
        {"priority-donate-multiple", test_priority_donate_multiple},   //c
        {"priority-donate-multiple2", test_priority_donate_multiple2}, //c
        {"priority-donate-nest", test_priority_donate_nest},           //c
        {"priority-donate-sema", test_priority_donate_sema},           //c
        {"priority-donate-lower", test_priority_donate_lower},         //c
        {"priority-donate-chain", test_priority_donate_chain},         //c
        {"priority-fifo", test_priority_fifo},                         //c
        {"priority-preempt", test_priority_preempt},                   //c
        {"priority-sema", test_priority_sema},                         //c
        {"priority-condvar", test_priority_condvar},                   //c
        {"mlfqs-load-1", test_mlfqs_load_1},                           //c
        {"mlfqs-load-60", test_mlfqs_load_60},                         //c
        {"mlfqs-load-avg", test_mlfqs_load_avg},
        {"mlfqs-recent-1", test_mlfqs_recent_1},                       //c
        {"mlfqs-fair-2", test_mlfqs_fair_2},
        {"mlfqs-fair-20", test_mlfqs_fair_20},
        {"mlfqs-nice-2", test_mlfqs_nice_2},
        {"mlfqs-nice-10", test_mlfqs_nice_10},
        {"mlfqs-block", test_mlfqs_block},
};

static const char *test_name;

/* Runs the test named NAME. */
void run_test(const char *name)
{
  const struct test *t;

  for (t = tests; t < tests + sizeof tests / sizeof *tests; t++)
    if (!strcmp(name, t->name))
    {
      test_name = name;
      msg("begin");
      t->function();
      msg("end");
      return;
    }
  PANIC("no test named \"%s\"", name);
}

/* Prints FORMAT as if with printf(),
   prefixing the output by the name of the test
   and following it with a new-line character. */
void msg(const char *format, ...)
{
  va_list args;

  printf("(%s) ", test_name);
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  putchar('\n');
}

/* Prints failure message FORMAT as if with printf(),
   prefixing the output by the name of the test and FAIL:
   and following it with a new-line character,
   and then panics the kernel. */
void fail(const char *format, ...)
{
  va_list args;

  printf("(%s) FAIL: ", test_name);
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  putchar('\n');

  PANIC("test failed");
}

/* Prints a message indicating the current test passed. */
void pass(void)
{
  printf("(%s) PASS\n", test_name);
}
