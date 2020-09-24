#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>
#include <stdint.h>

#include "syscall.h"
#include "util.h"

extern void init(void);
extern int sys_print(char *str, int len);

static void sighnd(int sig, siginfo_t* info, void* ctx) 
{
	ucontext_t* uc = (ucontext_t*)ctx;
	greg_t* regs = uc->uc_mcontext.gregs;

	// just get registers 
	unsigned long reg_rax= regs[REG_RAX];
	char* reg_rbx = regs[REG_RBX];
	unsigned long reg_rcx = regs[REG_RCX];

	uint8_t* instr = (uint8_t*)regs[REG_RIP];

	// get 3rd instruction adress
	uint8_t* next_instr = &instr[2];

	/* interruption instruction in insrt[0],
	interruption code on instr[1],
	syscall number in reg_rax */

	if(instr[0] != 0xCD || instr[1] != 0x81 || reg_rax != os_syscall_nr_print) 
	{
		abort();
	}

	sys_print(reg_rbx, reg_rcx);

	// jump to 3rd instruction to avoid looping
	regs[REG_RIP] = (unsigned long)next_instr; 
}

int main(int argc, char* argv[]) 
{
	struct sigaction act = 
	{
		.sa_sigaction = sighnd,
		.sa_flags = SA_RESTART,
	};

	sigemptyset(&act.sa_mask);

	if (-1 == sigaction(SIGSEGV, &act, NULL))
	{
		perror("signal set failed");
		return (1);
	}

	init();

	return (0);
}