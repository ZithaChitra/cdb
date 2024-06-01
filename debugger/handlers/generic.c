#include "stdio.h"
#include "generic.h"
#include "sys/types.h"
#include "json.h"


JSON *init_handler_resp(pid_t *pid)
{
    JSON *resp = json_init("empty", NULL);
    json_add_val(resp, "number", "pid", pid);
    json_add_val(resp, "string", "status", "succuess");
    return resp;
}

int attach_process(JSON *args)
{
    printf("\n---------------------\n");
    printf("attach_process handler running\n");
    printf("---------------------\n\n");
    return 0;
}

int get_regs()
{
    printf("-------------------------------\n");
    printf("generic: action handler: get_regs\n");
    pid_t pid = 20;
    JSON *resp = init_handler_resp(&pid);

    if(resp == NULL)
    {
        printf("error: could not init response\n");
        return -1;
    }

    json_print(resp);
    json_delete(resp);

    printf("-------------------------------\n");
    return 0;
}

int no_action()
{
    return 0;
}

