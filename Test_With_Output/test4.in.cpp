/*
 * test4.cpp
 *
 *  Created on: Apr 8, 2015
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
  while (1)
    {
//		cout << "f" << endl;
      uthread_block (1);
    }
}

void g (void)
{
  while (1)
    {
//		cout << "g" << endl;
      uthread_block (2);
    }
}

void h (void)
{
  while (1)
    {
//		cout << "h" << endl;
      uthread_block (3);
    }
}

int main (void)
{
  int q[2] = {10, 20};
  if (uthread_init (200000) == -1)
    {
      return 0;
    }

  uthread_spawn (f);
  uthread_spawn (g);
  uthread_spawn (h);

  while (uthread_get_total_quantums () < 10)
    {
      uthread_resume (1);
      uthread_resume (2);
      uthread_resume (3);
    }

  cout << uthread_get_quantums (0) << " + " << endl;
  cout << uthread_get_quantums (1) << " + " << endl;
  cout << uthread_get_quantums (2) << " + " << endl;
  cout << uthread_get_quantums (3) << endl;
  cout << " = " << uthread_get_total_quantums () << endl;

  uthread_block (2);

  while (uthread_get_total_quantums () < 20)
    {
      uthread_resume (1);
      uthread_resume (3);
    }

  cout << uthread_get_quantums (0) << " + " << endl;
  cout << uthread_get_quantums (1) << " + " << endl;
  cout << uthread_get_quantums (2) << " + " << endl;
  cout << uthread_get_quantums (3) << endl;
  cout << " = " << uthread_get_total_quantums () << endl;

  uthread_resume (2);

  while (uthread_get_total_quantums () < 30)
    {
      uthread_resume (1);
      uthread_resume (2);
      uthread_resume (3);
    }

  cout << uthread_get_quantums (0) << " + " << endl;
  cout << uthread_get_quantums (1) << " + " << endl;
  cout << uthread_get_quantums (2) << " + " << endl;
  cout << uthread_get_quantums (3) << endl;
  cout << " = " << uthread_get_total_quantums () << endl;

  uthread_terminate (0);
  return 0;
}




