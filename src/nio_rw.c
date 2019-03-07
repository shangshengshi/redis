#include "nio_rw.h"
#include "server.h"

extern struct redisServer server;

static void* nio_read_proc(void* arg) {
    
}

int32_t start_nio_read_thread() 
{
    if (server.nio_rtid > 0) 
    {
        serverLog(LL_WARNING, "nio read thread already start!");
        return -1;
    }

    int32_t ret = 0;
    if (ret = pthread_create(&server.nio_rtid, NULL, &nio_read_proc, NULL) != 0) 
    {
        serverLog(LL_WARNING, "create nio read thread error! ret[%d]", ret);
        return -1;
    }

    return 0;
}
