#include <iostream>
#include "uthreads.h"
#include <unordered_map>

int a;

void wait_one_quantum ()
{
  a = a + 1;
  for (int j = 0; j < 8000000; ++j)
    {
      a = 1;
    }
}

void wait_for_test_end ()
{
  for (int i = 0; i < 200; ++i)
    {
      wait_one_quantum ();
    }
}

void empty_func ()
{
  while (true)
    {}
}

void thread1 ()
{
  for (int i = 0; i < 5; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-1: round-" << i << ", quantums-" << uthread_get_quantums (1) <<
                std::endl;
    }
  uthread_terminate (1);
}

void thread2 ()
{
  for (int i = 0; i < 5; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-2: round-" << i << ", quantums-" << uthread_get_quantums (2) <<
                std::endl;
    }
  uthread_terminate (2);
}

void thread3 ()
{
  for (int i = 0; i < 5; ++i)
    {
      wait_one_quantum ();
      wait_one_quantum ();
      std::cout << "thread-3: round-" << i << ", quantums-" << uthread_get_quantums (3) <<
                std::endl;
    }
  uthread_terminate (3);
}

void thread4 ()
{
  for (int i = 0; i < 5; ++i)
    {
      wait_one_quantum ();
      wait_one_quantum ();
      std::cout << "thread-4: round-" << i << ", quantums-" << uthread_get_quantums (4) <<
                std::endl;
    }
  uthread_terminate (4);
}

void thread5 ()
{
  for (int i = 0; i < 5; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-5: round-" << i << ", quantums-" << uthread_get_quantums (5) <<
                std::endl;
    }
  uthread_terminate (5);
}

/**
 * method to check the correct operation of switching between 2 threads.
 * each thread will print a message 5 times.
 */
void check_two_threads_running ()
{
  std::cout << "thread 1 and 2 will print 'thread-i' 5 times each" <<
            std::endl;
  int id1 = uthread_spawn (&thread1);
  int id2 = uthread_spawn (&thread2);
  if (id1 != 1 || id2 != 2)
    {
      std::cout << "threads ids are not 1 and 2, but instead: " << id1
                << " and " << id2 << std::endl;
      exit (1);
    }
  wait_for_test_end ();
  std::cout << "threads 1 and 2 finished successfully" << std::endl;
}

void thread1_block ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-1: round-" << i << ", quantums-" << uthread_get_quantums (1) <<
                std::endl;
      if (i == 3)
        {
          std::cout << "blocking thread 2!!!" << std::endl;
          uthread_block (2);
        }
    }
  std::cout << "resuming thread 2!!!" << std::endl;
  uthread_resume (2);
  uthread_terminate (1);
}

void block_threads_test ()
{
  std::cout << "thread 1 will count to 8 and thread 2 to 5." << std::endl;
  int id1 = uthread_spawn (&thread1_block);
  int id2 = uthread_spawn (&thread2);
  if (id1 != 1 || id2 != 2)
    {
      std::cout << "threads ids are not 1 and 2, but instead: " << id1
                << " and " << id2 << std::endl;
      exit (1);
    }
  wait_for_test_end ();
  std::cout << "threads 1 and 2 finished successfully" << std::endl;
}

void thread1_sleep ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-1: round-" << i << ", quantums-" << uthread_get_quantums (1) <<
                std::endl;
      if (i == 3)
        {
          std::cout << "putting thread 1 to sleep for 5 quantums." << std::endl;
          uthread_sleep (5);
        }
    }
  uthread_terminate (1);
}

void thread2_sleep ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-2: round-" << i << ", quantums-" << uthread_get_quantums (2) <<
                std::endl;
    }
  uthread_terminate (2);
}

/**
 * method to test the sleep for threads.
 */
void sleep_threads_test ()
{
  std::cout << "thread 1 and 2 will print 'thread-i' 8 times each" <<
            std::endl;
  int id1 = uthread_spawn (&thread1_sleep);
  int id2 = uthread_spawn (&thread2_sleep);
  if (id1 != 1 || id2 != 2)
    {
      std::cout << "threads ids are not 1 and 2, but instead: " << id1
                << " and " << id2 << std::endl;
      exit (1);
    }
  wait_for_test_end ();
  std::cout << "threads 1 and 2 finished successfully" << std::endl;
}

void thread1_sleep_block_before_wakeup ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-1: round-" << i << ", quantums-" << uthread_get_quantums (1) << std::endl;
      if (i == 3)
        {
          std::cout << "putting thread 1 to sleep for 20 quantums" <<
                    std::endl;
          int before = uthread_get_total_quantums ();
          uthread_sleep (20);
          std::cout << "thread 1 is awake, it was out for " << uthread_get_total_quantums () - before << " quantums"
                    << std::endl;
        }
    }
  uthread_terminate (1);
}

/**
 * resuming thread 1 before it wakes up from sleep
 */
void thread2_sleep_block_before_wakeup ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-2: round-" << i << ", quantums-" << uthread_get_quantums (1) << std::endl;
      if (i == 4)
        {
          std::cout << "blocking thread 1" << std::endl;
          uthread_block (1);
        }
    }
  std::cout << "resuming thread 1" << std::endl;
  uthread_resume (1);
  uthread_terminate (2);
}

/**
 * test blocking threads while they are asleep, resuming before thread wakes up
 */
void test_block_sleeping_before_wakeup ()
{
  std::cout << "thread 1 and 2 will print 'thread-i' 8 times each" << std::endl;
  std::cout << "thread 1 should be blocked and resumed before waking up from its sleep" << std::endl;
  int id1 = uthread_spawn (&thread1_sleep_block_before_wakeup);
  int id2 = uthread_spawn (&thread2_sleep_block_before_wakeup);
  if (id1 != 1 || id2 != 2)
    {
      std::cout << "threads ids are not 1 and 2, but instead: " << id1
                << " and " << id2 << std::endl;
      exit (1);
    }
  wait_for_test_end ();
  std::cout << "threads 1 and 2 finished successfully" << std::endl;
}

void thread1_sleep_block_after_wakeup ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-1: round-" << i << ", quantums-" << uthread_get_quantums (1) << std::endl;
      if (i == 3)
        {
          std::cout << "putting thread 1 to sleep for 10 quantums" << std::endl;
          int before = uthread_get_total_quantums ();
          uthread_sleep (10);
          std::cout << "thread 1 is awake, it was out for " << uthread_get_total_quantums () - before << " quantums"
                    << std::endl;
        }
    }
  uthread_terminate (1);
}

/**
 * resuming thread 1 after it has woken up
 */
void thread2_sleep_block_after_wakeup ()
{
  for (int i = 0; i < 8; ++i)
    {
      wait_one_quantum ();
      std::cout << "thread-2: round-" << i << ", quantum-" << uthread_get_quantums (1) <<
                std::endl;
      if (i == 4)
        {
          std::cout << "blocking thread 1" << std::endl;
          uthread_block (1);
        }
    }
  uthread_sleep (10); //sleeping some more to make sure thread 1 will finish sleep before resuming
  std::cout << "resuming thread 1" << std::endl;
  uthread_resume (1);
  uthread_terminate (2);
}

/**
 * test blocking threads while they are asleep, resuming after thread wakes up
 */
void test_block_sleeping_after_wakeup ()
{
  std::cout << "thread 1 and 2 will print 'thread-i' 8 times each" << std::endl;
  std::cout << "thread 1 should be blocked while sleeping, and resumed when is awake" << std::endl;
  int id1 = uthread_spawn (&thread1_sleep_block_after_wakeup);
  int id2 = uthread_spawn (&thread2_sleep_block_after_wakeup);
  if (id1 != 1 || id2 != 2)
    {
      std::cout << "threads ids are not 1 and 2, but instead: " << id1
                << " and " << id2 << std::endl;
      exit (1);
    }
  wait_for_test_end ();
  std::cout << "threads 1 and 2 finished successfully" << std::endl;
}

void sleep_100 ()
{
  uthread_sleep (500);
  while (true)
    {}
}

void test_100_threads ()
{
  for (int i = 0; i < 100; ++i)
    {
      int id = uthread_spawn (&sleep_100);
      if (i == 99)
        {
          if (id != -1)
            {
              std::cout << "thread id is not -1" << std::endl;
              exit (1);
            }
          else
            {
              std::cout << "max limit reached - this is good!" << std::endl;
            }
        }
      else if (id != i + 1)
        {
          std::cout << "wrong id, expected: " << i + 1 << ", got: " << id <<
                    std::endl;
          exit (1);
        }
    }
  wait_for_test_end ();
  for (int i = 1; i < 100; ++i)
    {
      uthread_terminate (i);
    }

  std::cout << "finished spawning and terminating 100 threads successfully."
            << std::endl;
}

/**
 * test to check ids are allocated correctly
 */
void test_thread_id_management ()
{
  std::cout << "checking allocation of ids." << std::endl;
  int id1 = uthread_spawn (&empty_func);
  int id2 = uthread_spawn (&empty_func);
  int id3 = uthread_spawn (&empty_func);
  if (id1 != 1 || id2 != 2 || id3 != 3)
    {
      std::cout << "initial threads ids are not correct" << std::endl;
      exit (1);
    }

  uthread_terminate (2);
  int new_id = uthread_spawn (&empty_func);
  if (new_id != 2)
    {
      std::cout << "terminated thread 2 and created a new one instead, it did not receive id=2" << std::endl;
      exit (1);
    }

  uthread_terminate (3);
  new_id = uthread_spawn (&empty_func);
  if (new_id != 3)
    {
      std::cout << "terminated thread 3 and created a new one instead, it did not receive id=3" << std::endl;
      exit (1);
    }

  uthread_terminate (1);
  uthread_terminate (2);
  uthread_terminate (3);
  std::cout << "Success" << std::endl;

}

/**
 * testing switching between 5 different threads.
 */
void test_5_threads ()
{
  std::cout << "Spawning 5 threads, each thread will print thread-i 5 times" <<
            std::endl;
  int id1 = uthread_spawn (&thread1);
  int id2 = uthread_spawn (&thread2);
  int id3 = uthread_spawn (&thread3);
  int id4 = uthread_spawn (&thread4);
  int id5 = uthread_spawn (&thread5);
  if (id1 != 1 || id2 != 2 || id3 != 3 || id4 != 4 || id5 != 5)
    {
      std::cout << "threads ids are not correct" << std::endl;
      exit (1);
    }
  wait_for_test_end ();
  std::cout << "Success" << std::endl;
}

int main ()
{
  //init uthread class
  uthread_init (1);

  std::cout << std::endl << "Test 1" << std::endl;
  check_two_threads_running ();

  std::cout << std::endl << "Test 2" << std::endl;
  block_threads_test ();

  std::cout << std::endl << "Test 3" << std::endl;
  sleep_threads_test ();

  std::cout << std::endl << "Test 4" << std::endl;
  test_block_sleeping_before_wakeup ();

  std::cout << std::endl << "Test 5" << std::endl;
  test_block_sleeping_after_wakeup ();

  std::cout << std::endl << "Test 6" << std::endl;
  test_thread_id_management ();

  std::cout << std::endl << "Test 7" << std::endl;
  test_5_threads ();

  std::cout << std::endl << "Test 8" << std::endl;
  test_100_threads ();

  uthread_terminate (0);
  std::cout << "ERR: this message should not be printed!!!!!!!" << std::endl;

  return 0;
}

