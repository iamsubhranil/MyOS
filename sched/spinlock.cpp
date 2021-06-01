#include <sched/scheduler.h>
#include <sched/spinlock.h>

void SpinLock::assignAcquired() {
	acquired = Scheduler::CurrentTask;
}
