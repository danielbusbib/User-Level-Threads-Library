/*
 * test1.cpp
 *
 *	test suspends and resume
 *
 *  Created on: Apr 6, 2015
 *      Author: roigreenberg
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <deque>
#include <list>
#include <assert.h>
#include "uthreads.h"
//#include "libuthread.a"
#include <iostream>

using namespace std;

void f (void)
{
  int i = 1;
  int j = 0;
  while (1)
    {
      if (i == uthread_get_quantums (uthread_get_tid ()))
        {
          cout << "f" << "  q:  " << i << endl;
          if (i == 3 && j == 0)
            {
              j++;
              cout << "          f suspend by f" << endl;
              uthread_block (uthread_get_tid ());
            }
          if (i == 6 && j == 1)
            {
              j++;
              cout << "          g resume by f" << endl;
              uthread_resume (2);
            }
          if (i == 8 && j == 2)
            {
              j++;
              cout << "          **f end**" << endl;
              uthread_terminate (uthread_get_tid ());
              return;
            }
          i++;
        }
    }
}

void g (void)
{
  int i = 1;
  int j = 0;
  while (1)
    {
      if (i == uthread_get_quantums (uthread_get_tid ()))
        {
          cout << "g" << "  q:  " << i << endl;
          if (i == 11 && j == 0)
            {
              j++;
              cout << "          **g end**" << endl;
              uthread_terminate (uthread_get_tid ());
              return;
            }
          i++;
        }
    }
}

int main (void)
{
  int q[2] = {10, 20};
  if (uthread_init (2) == -1)
    {
      return 0;
    }

  int i = 1;
  int j = 0;

  while (1)
    {
      //int a = uthread_get_quantums(uthread_get_tid());
      //cout<<"quantums of thread number " << uthread_get_tid()<<" is " <<a<<std::endl;
      if (i == uthread_get_quantums (uthread_get_tid ()))
        {
          cout << "m" << "  q:  " << i << endl;
          if (i == 3 && j == 0)
            {
              j++;
              cout << "          spawn f at (1) " << uthread_spawn (f) << endl;
              cout << "          spawn g at (2) " << uthread_spawn (g) << endl;
            }

          if (i == 6 && j == 1)
            {
              j++;
              cout << "          g suspend by main" << endl;
              uthread_block (2);
              cout << "          g suspend again by main" << endl;
              uthread_block (2);
            }
          if (i == 9 && j == 2)
            {
              j++;
              cout << "          f resume by main" << endl;
              uthread_resume (1);
              cout << "          f resume again by main" << endl;
              uthread_resume (1);
            }
          if (i == 13 && j == 3)
            {
              j++;
              cout << "          spawn f at (1) " << uthread_spawn (f) << endl;
              cout << "          f suspend by main" << endl;
              uthread_block (1);
            }
          if (i == 17 && j == 4)
            {
              j++;
              cout << "          spawn g at (2) " << uthread_spawn (g) << endl;
              cout << "          f terminate by main" << endl;
              uthread_terminate (1);
              cout << "          spawn f at (1) " << uthread_spawn (f) << endl;
              cout << "          f suspend by main" << endl;
              uthread_block (1);
            }
          if (i == 20 && j == 5)
            {
              j++;
              //cout << "i: " << i << endl;
              cout << "          ******end******" << endl;
              cout << "total quantums:  " << uthread_get_total_quantums () << endl;
              uthread_terminate (0);
              return 0;
            }
          i++;
        }
    }
  cout << "end" << endl;
  return 0;
}




