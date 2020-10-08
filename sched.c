#include "sched.h"
#include <stdio.h>
#include <stdlib.h>

struct task
{
	void (*entry)(void *ctx);
	void *ctx;
	int id;
	int priority;
	int deadline;
	int time;

	struct task* next_run;
	struct task* next_wait;
};

static struct task taskpool[16];
static int taskpool_n;
static int sched_time;

static struct task* run_list;
static struct task* wait_list;

static int (*policy_cmp)(struct task* t1, struct task* t2);

static int fifo_cmp(struct task* t1, struct task* t2) 
{
	return t1->id - t2->id;
}

static int priority_cmp(struct task* t1, struct task* t2)
{
	int delta = t2->priority - t1->priority;

	if (delta) 
	{
		return delta;
	}

	return fifo_cmp(t1, t2);
}

static int deadline_cmp(struct task* t1, struct task* t2) 
{
	int delta = t1->deadline - t2->deadline;

	if (delta)
	{
		return delta;
	}

	return priority_cmp(t1, t2);
}

void sched_new(void (*entrypoint)(void *aspace),
		void *aspace,
		int priority,
		int deadline) 
{
	struct task *t = &taskpool[taskpool_n];
	t->entry = entrypoint;
	t->ctx = aspace;
	t->id = taskpool_n;
	t->priority = priority;
	t->deadline = deadline > 0 ? deadline : 0;
	t->time = 0;

	taskpool_n++;
}

static void any_insert(struct task* tmp, struct task* task)
{
	if(policy_cmp == deadline_cmp && (tmp->deadline == 0 || task->deadline == 0))
	{
		if(task->deadline == 0)
		{
			struct task* tmp2 = run_list;

			while(tmp2)
			{
				if(!tmp2->next_run)
				{
					break;
				}

				tmp2 = tmp2->next_run;
			}

			tmp2->next_run = task;
			task->next_run = NULL;
		}
		else if(tmp->deadline == 0)
		{
			task->next_run = tmp;
			run_list = task;
		}
	}
	else
	{
		while(tmp && policy_cmp(tmp, task) < 0)
		{
			if(!tmp->next_run)
			{
				break;
			}

			tmp = tmp->next_run;
		}
	}
	
	struct task* tmp2 = run_list;

	if(tmp2 != tmp)
	{
		while(tmp2->next_run)
		{
			if(tmp2->next_run == tmp)
			{
				break;
			}

			tmp2 = tmp2->next_run;
		}

		tmp2->next_run = task;
		task->next_run = tmp;
	}
	else
	{
		if(policy_cmp == fifo_cmp)
		{
			struct task* tmp3 = tmp2->next_run;

			tmp2->next_run = task;
			task->next_run = tmp3;
		}
		else if(policy_cmp == priority_cmp)
		{
			if(task->priority > tmp->priority)
			{
				task->next_run = tmp;
				run_list = task;
			}
			else if(task->priority < tmp->priority)
			{
				tmp->next_run = task;
				task->next_run = NULL;
			}
		}
	}	
}

static void priority_insert(struct task* tmp, struct task* task)
{
	if(tmp->priority - task->priority == 0)
	{
		policy_cmp = fifo_cmp;
		any_insert(tmp, task);
	}
	else
	{
		any_insert(tmp, task);
	}
}

static void deadline_insert(struct task* tmp, struct task* task)
{
	if(tmp->deadline - task->deadline == 0)
	{
		policy_cmp = priority_cmp;
		priority_insert(tmp, task);
	}			
	else
	{
		any_insert(tmp, task);
	}
}

static void insert_run_list(struct task* task)
{
	if(run_list)
	{
		struct task* tmp = run_list;

		if(policy_cmp == fifo_cmp)
		{
			any_insert(tmp, task);
		}
		else if(policy_cmp == priority_cmp)
		{
			priority_insert(tmp, task);
		}
		else if(policy_cmp == deadline_cmp)
		{
			deadline_insert(tmp, task);
		}
	}
	else
	{
		run_list = task;
	}
	
}

void sched_cont(void (*entrypoint)(void *aspace),
		void *aspace,
		int timeout)
{
	if(timeout > 0)
	{
		for(int i = 0; i < taskpool_n; i++)
		{
			if(taskpool[i].ctx == aspace)
			{
				taskpool[i].time = sched_time + timeout;

				if(!wait_list)
				{
					wait_list = &taskpool[i];
					wait_list->next_run = NULL;
				}
				else
				{
					struct task* tmp = &taskpool[i];
					tmp->next_run = NULL;
					struct task* tmp2 = wait_list;

					while(1)
					{
						if(!tmp2->next_wait)
						{
							break;
						}

						tmp2 = tmp2->next_wait;
					}

					tmp2->next_wait = tmp;
				}

				break;
			}
		}
	}
	else
	{
		for(int i = 0; i < taskpool_n; i++)
		{
			if(taskpool[i].ctx == aspace)
			{
				insert_run_list(&taskpool[i]);

				break;
			}
		}
	}
}

void sched_time_elapsed(unsigned amount) 
{
	sched_time += (int)amount;

	struct task* tmp = wait_list;

	if(wait_list)
	{
		while(1)
		{
			if(tmp->time <= sched_time)
			{
				insert_run_list(tmp);

				struct task* tmp2 = wait_list;

				while(tmp2->next_wait)
				{
					if(tmp2->next_wait == tmp)
					{
						break;
					}

					tmp2 = tmp2->next_wait;
				}

				if(tmp2 == tmp)
				{
					wait_list = NULL;
				}
				else
				{
					tmp2->next_wait = tmp->next_wait;
				}
			}

			if(!tmp->next_wait)
			{
				break;
			}

			tmp = tmp->next_wait;
		}
	}
}

void sched_set_policy(enum policy policy)
{
	switch (policy)
	{
		case POLICY_FIFO:
		{
			policy_cmp = fifo_cmp;

			break;
		}
		case POLICY_PRIO:
		{
			policy_cmp = priority_cmp;

			break;
		}
		case POLICY_DEADLINE:
		{
			policy_cmp = deadline_cmp;

			break;
		}
		default:
		{
			printf("Incorrect policy.");

			break;
		}
	}
}

static void policy_sort()
{
	qsort(taskpool, taskpool_n, sizeof(struct task), policy_cmp);

	if(policy_cmp == deadline_cmp)
	{
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
}

static void make_run_list()
{
	for(int i = 0; i < taskpool_n - 1; i++)
	{
		taskpool[i].next_run = &taskpool[i + 1];
	}

	run_list = &taskpool[0];
}

void sched_run(void) 
{
	policy_sort();
	make_run_list();

	while(run_list)
	{
		struct task* run_list_head = run_list;

		run_list = run_list->next_run;
		run_list_head->entry(run_list_head->ctx);
	}
}