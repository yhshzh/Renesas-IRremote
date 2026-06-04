#include <errno.h>
#include <signal.h>
#include <sys/types.h>

void _exit(int status);
int _kill(pid_t pid, int sig);
pid_t _getpid(void);

void _exit(int status)
{
    (void) status;

    while (1)
    {
        __asm volatile ("bkpt #0");
    }
}

int _kill(pid_t pid, int sig)
{
    (void) pid;
    (void) sig;
    errno = EINVAL;
    return -1;
}

pid_t _getpid(void)
{
    return 1;
}
