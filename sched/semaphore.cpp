#include <sched/scheduler.h>
#include <sched/scopedlock.h>
#include <sched/semaphore.h>

void Semaphore::acquire() {
	lock.lock();
	if(value == 0) {
		Task  *s       = (Task *)Scheduler::getCurrentTask();
		Task **waiters = &taskList;
		while(*waiters) {
			waiters = &(*waiters)->nextInList;
		}
		*waiters      = s;
		s->nextInList = NULL;
		Scheduler::unschedule(lock);
	} else {
		value--;
		lock.unlock();
	}
}

void Semaphore::release(u32 times, bool ensureLock) {
	if(ensureLock) {
		lock.lock();
	}
	value += times;
	while(taskList && value > 0) {
		Task *n              = taskList->nextInList;
		taskList->nextInList = NULL;
		Scheduler::appendTask(taskList);
		taskList = n;
		value--;
	}
	if(ensureLock) {
		lock.unlock();
	}
}
