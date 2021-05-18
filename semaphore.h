#pragma once

#include "task.h"

struct Semaphore {
	i32 value;
	// list of tasks which are waiting on this semaphore
	Task *taskList;

	Semaphore();
	void acquire();
	void release();
};
