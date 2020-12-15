/*
 File: scheduler.C
 
 Author:
 Date  :
 
 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "simple_keyboard.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler() {
  size = 0;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield() {
  if (size == 0) Console::puts("No threads available, cannot yield\n");
  else {
      size--;
      Thread* dispatch_thread = ready.dequeue();
      Thread::dispatch_to(dispatch_thread);
  }
}

void Scheduler::resume(Thread * _thread) {
  ready.enqueue(_thread);
  size++;
}

void Scheduler::add(Thread * _thread) {
  ready.enqueue(_thread);
  size++;
}

void Scheduler::terminate(Thread * _thread) {
  for(int i = 0; i < size; i++) {
      Thread* top = ready.dequeue();
      if (_thread->ThreadId() == top->ThreadId()) size--;
      else ready.enqueue(top);
  }
}
