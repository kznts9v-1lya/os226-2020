#include "sched.h"
#include <stdio.h>

struct task {
	void (*entry)(void *ctx);
	void *ctx;
	int priority;
	int deadline;
	int id;
	int timer;
};

static struct task taskpool[16];
static int taskpool_n;
static enum policy currentPolicy;
static int priorityCounters[11];

void sched_new(void (*entrypoint)(void *aspace), void *aspace, int priority, int deadline) 
{
	struct task *t = &taskpool[taskpool_n];
	
	t->entry = entrypoint;
	t->ctx = aspace;
	t->priority = priority;
	t->deadline = deadline <= 0? 0 : deadline;
	t->id = taskpool_n;
	t->timer = 0;

	taskpool_n++;
}

void sched_cont(void (*entrypoint)(void *aspace), void *aspace, int timeout)
{
	for (int i = 0; i < taskpool_n; i++) 
	{
		if (taskpool[i].ctx == aspace) 
		{
			taskpool[i].timer = timeout;
			break;
		}
	}
}

void sched_time_elapsed(int amount) 
{
	for(int i = 0; i < taskpool_n; i++)
	{
		if(taskpool[i].timer > 0)
		{
			taskpool[i].timer -= amount;

			if (taskpool[i].timer < 0) 
			{
				printf("Undefined behavior.");
			}
		}
		
	}
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
}

static void policyFIFO(int start, int end)
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

/*static void policyPriority(int start, int end)
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
}*/

static void policyPriority(int start, int end)
{
	struct task tmp;
	tmp.timer = -1;
	tmp.ctx = (void*)-1;
	struct task* taskWithTimer = &tmp;
	int timerOff = 1;

	int i = start;

	while(i < end)
	{
		if(taskWithTimer->timer > 0 || timerOff == 1)
		{
			if(priorityCounters[10 - taskpool[i].priority] > 1)
			{
				policyFIFO(i, i + priorityCounters[10 - taskpool[i].priority]);

				i += priorityCounters[10 - taskpool[i].priority];
			}
			else
			{
				while (*((int*)taskpool[i].ctx) >= 0) 
				{
					taskpool[i].entry(taskpool[i].ctx);

					if(taskpool[i].timer > 0)
					{
						taskWithTimer = &taskpool[i];
						timerOff = 0;
						break;
					}

					if(taskWithTimer->timer == 0)
					{
						i -= 2;
						break;
					}
				}

				i++;
			}	
		}
		else if(taskWithTimer->timer == 0 && *((int*)taskWithTimer[i].ctx) >= 0)
		{
			taskWithTimer->entry(taskWithTimer->ctx);

			if(*((int*)taskWithTimer[i].ctx) == -1)
			{
				timerOff = 1;
			}

			i++;
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