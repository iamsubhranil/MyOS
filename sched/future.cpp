#include <drivers/terminal.h>
#include <sched/future.h>
#include <sched/scheduler.h>

FutureBase::FutureBase() {
	isAvailable  = false;
	lock         = SpinLock();
	waitingTasks = NULL;
}

void FutureBase::awakeAllNoLock() {
	isAvailable = true;
	// Terminal::write("Future :", (void *)this,
	//                " Scheduling: ", (void *)waitingTasks, "\n");
	// Terminal::write("Scheduling ", (void *)waitingTasks, "\n");
	while(waitingTasks) {
		Task *nextTask           = waitingTasks->nextInList;
		waitingTasks->nextInList = NULL;
		Scheduler::appendTask(waitingTasks);
		waitingTasks = nextTask;
	}
}

void FutureBase::waitTillAvailable() {
	lock.lock();
	if(!isAvailable) {
		Task  *s         = (Task *)Scheduler::getCurrentTask();
		Task **insertPos = &waitingTasks;
		while(*insertPos) {
			insertPos = &(*insertPos)->nextInList;
		}
		*insertPos = (Task *)s;
		// Terminal::write("Future :", (void *)this,
		//                " Unscheduling: ", (void *)waitingTasks, "\n");
		Scheduler::unschedule(lock); // the task will return from here,
		                             // already unlocked
	} else {
		lock.unlock();
	}
}
