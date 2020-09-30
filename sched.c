#include "sched.h"
#include <stdlib.h>

struct task {
	void (*entry)(void *ctx);
	void *ctx;
	int priority;
	int deadline;
	int id;
};

static struct task taskpool[16];
static int taskpool_n;
static enum policy currentPolicy;
static int priorityCounters[11];
static int deadlineMax;
static int* deadlineCounters;

void sched_new(void (*entrypoint)(void *aspace),
		void *aspace,
		int priority,
		int deadline) {
	struct task *t = &taskpool[taskpool_n];
	
	t->entry = entrypoint;
	t->ctx = aspace;
	t->priority = priority;
	t->deadline = deadline <= 0? 0 : deadline;

	if(deadline > deadlineMax)
	{
		deadlineMax = deadline;
	}

	t->id = taskpool_n;

	priorityCounters[10 - priority]++;
	taskpool_n++;
}



void sched_cont(void (*entrypoint)(void *aspace),
		void *aspace,
		int timeout) {
	
}

void sched_time_elapsed(unsigned amount) {
	// ...
}

void sched_set_policy(enum policy policy) {
	currentPolicy = policy;
}

static deadlineCmp(const struct task* task1, const struct task* task2)
{
	return(task1->deadline - task2->deadline);
}

static int priorityCmp(const struct task* task1, const struct task* task2)
{
	return(task2->priority - task1->priority);
}

static int idCmp(const struct task* task1, const struct task* task2)
{
	return(task1->id - task2->id);
}

static void IDSort(struct task* taskpoolArr)
{
	for(int n = 0; n < 11; n++)
	{
		if(priorityCounters[n] > 1)
		{
			struct task taskpoolID[priorityCounters[n]];
			int m = 0; // Number of tasks before n

			for(int j = 0; j < n; j++)
			{
				m += priorityCounters[j];
			}

			for(int i = 0; i < priorityCounters[n]; i++)
			{
				taskpoolID[i] = taskpoolArr[m + i];
			}

			qsort(taskpoolID, priorityCounters[n], sizeof(struct task), (int(*)(const void*, const void*)) idCmp);

			for(int i = 0; i < priorityCounters[n]; i++)
			{
				taskpoolArr[m + i] = taskpoolID[i];
			}
		}
	}
}

static void prioritySort(struct task* taskpoolArr, int size)
{
	qsort(taskpoolArr, size, sizeof(struct task), (int(*)(const void*, const void*)) priorityCmp);
	IDSort(taskpoolArr);
}

static void deadlineSort()
{
	qsort(taskpool, taskpool_n, sizeof(struct task), (int(*)(const void*, const void*)) deadlineCmp);
	
	while(taskpool[0].deadline == 0)
	{
		struct task tmp = taskpool[0];
		
		for(int i = 0; i < taskpool_n - 1; i++)
		{
			taskpool[i] = taskpool[i + 1];
		}

		taskpool[taskpool_n] = tmp;
	}

	
}

void policyFIFO(int start, int end)
{
	int tasksLeft = end - start;

	while(tasksLeft != 0)
	{
		for(int i = start; i < end && i < taskpool_n; i++)
		{
			if (*((int*)taskpool[i].ctx) >= 0)
			{
				taskpool[i].entry(taskpool[i].ctx);

				if(*((int*)taskpool[i].ctx) == -1)
				{
					tasksLeft--;
				}
			}		
		}
	}
}

void sched_run(void) 
{
	switch (currentPolicy)
	{
		case POLICY_FIFO:
		{
			policyFIFO(0, taskpool_n);

			break;
		}
		case POLICY_PRIO:
		{
			prioritySort(taskpool, taskpool_n);
			
			for(int i = 0; i < taskpool_n; i++)
			{
				if(priorityCounters[10 - taskpool[i].priority] > 1)
				{
					policyFIFO(i, i + priorityCounters[10 - taskpool[i].priority]);

					i += (priorityCounters[10 - taskpool[i].priority] - 1);
				}
				else
				{
					while (*((int*)taskpool[i].ctx) >= 0) 
					{
						taskpool[i].entry(taskpool[i].ctx);
					}
				}								
			}

			break;
		}
		case POLICY_DEADLINE:
		{
			deadlineCounters = (int*)calloc(deadlineMax, sizeof(int));

			for(int i = 0; i < taskpool_n; i++)
			{
				deadlineCounters[taskpool[i].deadline]++;
			}

			for(int i = 0; i < taskpool_n; i++)
			{
				for(int j = 0; j < deadlineMax; j++)
				{
					if(taskpool[i].deadline == j)
					{
						struct task taskpoolDeadline[deadlineCounters[j]];

						for(int m = 0; m < deadlineCounters[j]; m++)
						{
							taskpoolDeadline[m] = taskpool[i + m];
						}

						//prioritySort();
					}
				}
			}


			while (taskpool_n != 0)
			{
				
			}

			break;
		}
		default:
		{
			printf("Non-existent policy selected.");
			break;
		}
	}
}