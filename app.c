#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
    pid_t pid = getpid();
    fprintf(stdout, "app started. pid: %d\n", pid);

    while (1)
    {
    }

    return 0;
}