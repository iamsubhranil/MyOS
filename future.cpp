#pragma once

#include "future.h"
#include "scheduler.h"

FutureBase::FutureBase() {
	isAvailable  = false;
	lock         = SpinLock();
	waitingTasks = NULL;
}

void FutureBase::awakeAllNoLock() {
	isAvailable = true;
	while(waitingTasks) {
		Task *nextTask = waitingTasks->next;
		Scheduler::appendTask(waitingTasks);
		waitingTasks = nextTask;
	}
}

void FutureBase::waitTillAvailable() {
	lock.lock();
	if(!isAvailable) {
		Task * s         = (Task *)Scheduler::getCurrentTask();
		Task **insertPos = &waitingTasks;
		while(*insertPos) {
			insertPos = &(*insertPos)->next;
		}
		*insertPos = (Task *)s;
		lock.unlock();
		Scheduler::unschedule(); // the task will return from here,
		                         // already unlocked
	} else {
		lock.unlock();
	}
}
