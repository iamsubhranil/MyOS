#include "spinlock.h"
#include "scheduler.h"

void SpinLock::assignAcquired() {
	acquired = Scheduler::CurrentTask;
}
