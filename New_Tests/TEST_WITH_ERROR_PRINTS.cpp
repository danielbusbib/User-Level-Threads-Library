#include "uthreads.h"
#include <unistd.h>
#include <iostream>

int a, b, c, d, flag;
#define SECOND 1000
#define ITER_NUM 1000
int quant_c = 0;
int quant_d = 0;

void func_a ()
{
  a = 0;
  for (int i = 0; i < ITER_NUM; i++)
    {
      a += 1;
      usleep (SECOND);
    }
  flag += 1;
  uthread_terminate (uthread_get_tid ());

}
void func_b ()
{
  b = 0;
  for (int i = 0; i < ITER_NUM; i++)
    {
      b += 1;
      usleep (SECOND);
    }
  flag += 1;
  uthread_terminate (uthread_get_tid ());

}

void func_c ()
{
  c = 0;
  for (int i = 0; i < ITER_NUM; i++)
    {
      c += 1;
      usleep (SECOND);
    }
  flag += 1;
  quant_c = uthread_get_quantums (uthread_get_tid ());
  uthread_terminate (uthread_get_tid ());

}

void func_d ()
{
  d = 0;
  for (int i = 0; i < ITER_NUM; i++)
    {
      d += 1;
      usleep (SECOND);
    }
  flag += 1;
  quant_d = uthread_get_quantums (uthread_get_tid ());
  uthread_terminate (uthread_get_tid ());
  uthread_terminate (uthread_get_tid ());
}

int quantoms_before_sleep = 0;
void func_sleep ()
{
  b = 0;
  for (int i = 0; i < ITER_NUM; i++)
    {
      b += 1;
      usleep (SECOND);
      if (i == 100)
        {
          quantoms_before_sleep = uthread_get_total_quantums ();
          uthread_sleep (5);
          int quantoms_after_sleep = uthread_get_total_quantums ();
          if (quantoms_before_sleep + 5 > quantoms_after_sleep)
            {
              std::cout
                  << "sleep doesn't work well - sleeps less quamtoms than necessary\n";
              return;
            }
        }
    }
  flag += 1;
  uthread_terminate (uthread_get_tid ());
}

/**
 * check edge cases of function uthread_init.
 * Should print 2 error message and success at the end.
 */
void test_init_errors ()
{
  int r = uthread_init (-2);
  if (r != -1)
    {
      std::cerr << "init can't get non-positive quantum_usecs\n";
      return;
    }
  int r2 = uthread_init (0);
  if (r2 != -1)
    {
      std::cerr << "init can't get non-positive quantum_usecs\n";
      return;
    }
  std::cout
      << "SUCCESS test_init_errors - This test should print 2 error message\n";
}

/**
 * test simple switch with 3 threads
 * Should print success at the end.
 */
void test_switch ()
{
  flag = 0;
  uthread_init (1);
  uthread_spawn (&func_a);
  uthread_spawn (&func_b);
  while (flag != 2)
    {
    }
  if (a != ITER_NUM or b != ITER_NUM)
    {
      std::cerr << "Switch test - threads didn't finish there run\n";
      return;
    }
  std::cout << "SUCCESS test_switch\n";
  uthread_terminate (0);
}

/**
 * check edge cases of function uthread_spawn.
 * Should print 2 error message and success at the end.
 */
void test_spawn_errors ()
{
  uthread_init (1);
  std::cout << "\n";
  int re = uthread_spawn (nullptr);
  if (re != -1)
    {
      std::cerr << "spawn can't get null entry point\n";
      return;
    }
  int re_t;
  for (int i = 1; i < MAX_THREAD_NUM; i++)
    {
      re_t = uthread_spawn (&func_a);
      if (re_t == -1)
        {
          std::cout << "Spawn send error even it shouldn't\n";
          return;
        }
    }
  int re2 = uthread_spawn (&func_a);
  if (re2 != -1)
    {
      std::cerr << "Spawn can't get more there max thread num\n";
      return;
    }
  std::cout
      << "SUCCESS test_spawn_errors - This test should print 2 error message\n";

  uthread_terminate (0);

}

/**
 * check edge cases of reallocte id.
 * The test create MAX_THREAD_NUM threads, delete two and except that create new thread will get minimum id.
 * Should print success at the end.
 */
void test_reallocte_id ()
{
  uthread_init (1);
  int re_t;
  for (int i = 1; i < MAX_THREAD_NUM; i++)
    {
      re_t = uthread_spawn (&func_a);
      if (re_t == -1)
        {
          std::cout << "Spawn send error even it shouldn't\n";
          return;
        }
    }
  uthread_terminate (50);
  uthread_terminate (25);
  int id = uthread_spawn (&func_a);
  if (id != 25)
    {
      std::cout << "New thread didn't get minimal id\n";
      return;
    }
  std::cout << "SUCCESS test_reallocte_id\n";
  uthread_terminate (0);
}

/**
 * check edge cases of function uthread_terminate.
 * Should print 1 error message and success at the end.
 */
void test_terminate_error ()
{

  uthread_init (1);
  int id = uthread_spawn (&func_a);
  int e = uthread_terminate (id + 5);
  if (e != -1)
    {
      std::cout << "terminate on id that doesn't exists it illegal\n";
      return;
    }
  std::cout
      << "SUCCESS test_terminate_error - this test should print 1 error message\n";
  uthread_terminate (0);
  std::cout
      << "if get to this line this is a error since we did terminate to 0\n";

}
/**
 * check edge cases of function uthread_block.
 * Should print 2 error message and success at the end.
 */
void test_block_errors ()
{
  uthread_init (1);
  int id = uthread_spawn (&func_a);
  uthread_block (id);
  int second_block = uthread_block (id);
  if (second_block == -1)
    {
      std::cout << "re-blocking thread should not be an error\n";
      return;
    }
  int re = uthread_block (id + 5);
  if (re != -1)
    {
      std::cout << "block an id that doesn't exist is illegal\n";
      return;
    }
  int re0 = uthread_block (0);
  if (re0 != -1)
    {
      std::cout << "block id 0 is illegal\n";
      return;
    }
  std::cout
      << "SUCCESS test_block_errors - this test should print 2 error message\n";
}

/**
 * check simaple block and resume
 * Should print:
 * thread 1 is blocked
 * thread 1 is resume
 * SUCCESS
 */
void test_block_and_resume ()
{
  flag = 0;
  a = 0;
  uthread_init (1);
  int id = uthread_spawn (&func_a);
  for (int i = 0; i < ITER_NUM; i++)
    {
      usleep (SECOND);
    }
  uthread_block (id);
  int m = a;
  std::cout << "thread 1 is blocked\n";
  for (int i = 0; i < ITER_NUM * 10; i++)
    {
      if (m != a)
        {
          std::cout << "block didn't work well\n";
          return;
        }
      usleep (SECOND);
    }
  uthread_resume (id);
  std::cout << "thread 1 is resume\n";
  while (flag != 1)
    {}
  if (m != ITER_NUM)
    {
      if (a != ITER_NUM && m >= a)
        {
          std::cout << "resume doesn't work well\n";
          return;
        }
    }
  std::cout << "SUCCESS test_block_and_resume\n";
}

/**
 * check edge cases of function uthread_resume.
 * Should print 1 error message and success at the end.
 */
void test_resume_errors ()
{
  uthread_init (1);
  int id = uthread_spawn (&func_a);
  uthread_block (id);
  int re = uthread_resume (id + 5);
  if (re != -1)
    {
      std::cout << "resume on id that doesn't exists it illegal\n";
      return;
    }
  int re_cur = uthread_resume (uthread_get_tid ());
  if (re_cur == -1)
    {
      std::cout << "resume on runnning id (even 0) should be ok\n";
      return;
    }
  std::cout
      << "SUCCESS test_resume_errors - this test should print 1 error message\n";
}

/**
 * check edge cases of function uthread_sleep.
 * Should print 1 error message and success at the end.
 */
void test_sleep_error ()
{
  uthread_init (1);
  int id = uthread_spawn (&func_a);
  int s = uthread_sleep (5);
  if (s != -1)
    {
      std::cout << "sleep on 0 thread it illegal\n";
      return;
    }
  std::cout
      << "SUCCESS test_sleep_error - this test should print 1 error message\n";
}

/**
 * check simple sleep action.
 * Should print success at the end.
 */
void test_sleep ()
{
  uthread_init (1);
  uthread_spawn (&func_sleep);
  while (flag != 1)
    {}
  std::cout << "SUCCESS test_sleep\n";
}

/**
 * check edge cases of function uthread_get_quantums.
 * @return 0 upon success, 1 otherwise.
 */
int test_uthread_get_quantums ()
{
  if (uthread_get_quantums (-1) != -1)
    {
      std::cerr << "id cant be less then zero" << std::endl;
      return 1;
    }

  if (uthread_get_quantums (101) != -1)
    {
      std::cerr << "id cant be more than 100" << std::endl;;
      return 1;
    }
  std::cout
      << "SUCCESS test_uthread_get_quantums -this test should print 2 error message"
      << std::endl;
  return 0;
}

/**
 * check edge cases of function uthread_get_total_quantums.
 * @return 0 upon success, 1 otherwise.
 */
int test_get_total_quantums ()
{
  uthread_init (20);
  if (uthread_get_total_quantums () != 1)
    {
      std::cerr << "after init num quantums should be 1" << std::endl;;
      return 1;
    }

  int id1 = uthread_spawn (&func_c);
  int id2 = uthread_spawn (&func_d);

  while (flag != 2)
    {

    }
  int quant0 = uthread_get_quantums (0);
  if (uthread_get_total_quantums () != (quant_c + quant_d + quant0))
    {
      std::cerr << "fail" << std::endl;
    }
  std::cout << "SUCCESS test_get_total_quantums" << std::endl;
  return 0;
}

int main (int argc, char *argv[])
{
//  test_init_errors ();
//  test_switch ();
//  test_spawn_errors ();
//  test_reallocte_id ();
//  test_terminate_error ();
//  test_block_errors ();
//  test_block_and_resume ();
//  test_resume_errors ();
//  test_sleep_error ();
//  test_sleep ();
//  test_uthread_get_quantums ();
  test_get_total_quantums ();
  return 0;
}
