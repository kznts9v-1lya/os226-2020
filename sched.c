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

void sched_new(void (*entrypoint)(void *aspace),
		void *aspace,
		int priority,
		int deadline) {
	struct task *t = &taskpool[taskpool_n];
	
	t->entry = entrypoint;
	t->ctx = aspace;
	t->priority = priority;
	t->deadline = deadline <= 0? 0 : deadline;
	t->id = taskpool_n;

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

static int deadlineCmp(const struct task* task1, const struct task* task2)
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
			int m = 0;

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
	for(int i = 0; i < size; i++)
	{
		priorityCounters[10 - taskpoolArr[i].priority]++;
	}

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

		taskpool[taskpool_n - 1] = tmp;
	}



	/*deadlineCounters = (int*)calloc(deadlineMax, sizeof(int));

	for(int i = 0; i < taskpool_n; i++)
	{
		deadlineCounters[taskpool[i].deadline]++;
	}

	for(int i = 0; i < taskpool_n; i++)
	{
		for(int j = 1; j < deadlineMax; j++)
		{
			if(taskpool[i].deadline == j && deadlineCounters[j] > 1)
			{
				struct task taskpoolDeadline[deadlineCounters[j]];
			}


		}
	}*/

	/*for(int i = 0; i < taskpool_n; i++)
	{
		for(int j = 0; j < deadlineMax; j++)
		{
			if(taskpool[i].deadline == j && deadlineCounters[j] > 1)
			{
				struct task taskpoolDeadline[deadlineCounters[j]];
				int m = 0;
				
				for(int n = j + 1; n < deadlineMax; n++)
				{
					m += deadlineCounters[n];
				}

				for(int n = deadlineCounters[j]; n > j; n--)
				{
					taskpoolDeadline[deadlineCounters[j] - n] = taskpool[taskpool_n - m - n];
				}

				prioritySort(taskpoolDeadline, deadlineCounters[j]);

				for (int n = deadlineCounters[j]; n > j; n--)
				{
					taskpool[taskpool_n - m - n] = taskpoolDeadline[deadlineCounters[j] - n];
				}
			}
		}
	}*/
}

void policyFIFO(int start, int end)
{
	int tasksLeft = end - start;

	while(tasksLeft != 0)
	{
		for(int i = start; i < end; i++)
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

void policyPriority(int start, int end)
{
	for(int i = start; i < end; i++)
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
			policyPriority(0, taskpool_n);

			break;
		}
		case POLICY_DEADLINE:
		{
			deadlineSort();

			int taskTotal = taskpool_n;

			while (taskpool_n != 0)
			{
				for(int i = 0; i < taskTotal; i++)
				{
					if(taskpool_n == 0)
					{
						break;
					}

					int c = i;
					int j = 0;

					while(taskpool[c].deadline == taskpool[c + 1].deadline && c < taskTotal - 1)
					{
						j++;
						c++;
					}

					j++;

					if(j == 1)
					{
						policyFIFO(i, i + j);

						taskpool_n--;
					}
					else
					{
						prioritySort(taskpool + i, j);
						policyPriority(i, i + j);

						taskpool_n -= j;
					}
				}
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