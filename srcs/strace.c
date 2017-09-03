/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jhalford <jack@crans.org>                  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/04/22 14:10:24 by jhalford          #+#    #+#             */
/*   Updated: 2017/04/23 18:18:41 by jhalford         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "strace.h"

extern char **environ;

extern t_syscall	g_syscalls[];

#define LOAD_PARAMS(params, regs) do { \
	ft_memcpy(params + 0, &regs->rdi, 8); \
	ft_memcpy(params + 1, &regs->rsi, 8); \
	ft_memcpy(params + 2, &regs->rdx, 8); \
	ft_memcpy(params + 3, &regs->rcx, 8); \
	ft_memcpy(params + 4, &regs->r8, 8); \
	ft_memcpy(params + 5, &regs->r9, 8); \
} while (0)

void	print_syscall_params(t_syscall syscall, struct user_regs_struct *regs)
{
	enum e_param type;
	unsigned long long	params[7];

	LOAD_PARAMS(params, regs);
	for (int i = 0; syscall.params[i]; i++) {
		type = syscall.params[i];
		if (i != 0)
			printf(", ");
		if (type == PARAM_NUMBER)
			printf("%d", (int)params[i]);
		if (type == PARAM_STRING)
		{
			if (params[i] == 0)
				printf("NULL");
			/* else */
			/* 	printf("\"%s\"", (char *)params[i]); */
		}
		if (type == PARAM_ADDR)
		{
			if (params[i] == 0)
				printf("NULL");
			else
				printf("%#llx", params[i]);
		}
	}
}

void	print_syscall_ret(unsigned long long ret)
{
	printf(") = ");
	if ((signed long long)ret < 0)
		printf("-1 (errno %lld)", -ret);
	else if (ret >> 16)
		printf("%#llx", ret);
	else
		printf("%lld", ret);
	printf("\n");
}

int		main(int ac, char **av)
{
	int							child;
	int							status;
	unsigned long				old;
	struct user_regs_struct		regs;

	(void)ac;
	if ((child = fork()) == 0) {
		/* ptrace(PTRACE_TRACEME, child, 0, 0); */
		raise(SIGSTOP);
		execve(av[1], av + 1, environ);
	}
	ptrace(PTRACE_SEIZE, child, 0, (void*)(PTRACE_O_TRACESYSGOOD));
	ptrace(PTRACE_INTERRUPT, child, 0, 0);
	wait(&status);
	/* ptrace(PTRACE_SETOPTIONS, child, NULL, PTRACE_O_TRACEEXEC); */
	while (1) {
		ptrace(PTRACE_SYSCALL, child, NULL, NULL);
		waitpid(child, &status, 0);
		if (WIFEXITED(status))
		{
			if (old != 0)
				printf(") = ?\n+++ exited with %d +++\n", WEXITSTATUS(status));
			break ;
		}
		else if (!(WIFSTOPPED(status) && WSTOPSIG(status) & 0x80))
			continue ;

		ptrace(PTRACE_GETREGS, child, NULL, &regs);
		if (old != regs.rip) {
			printf("%s(", g_syscalls[regs.orig_rax].name);
			print_syscall_params(g_syscalls[regs.orig_rax], &regs);
			old = regs.rip;	
		}
		else
		{
			print_syscall_ret(regs.rax);
			old = 0;
		}
	}
	return (0);
}
