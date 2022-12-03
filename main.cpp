#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "defines.h"

typedef struct _thread_data
{
    int number;
    int is_running;
    char cmd[256];
} thread_data;

void parse_arguments(char *argv[], char* cmdfile, int* num_threads, int* num_counters, int* log_enabled)
{
    if(NULL==cmdfile)
    {
        printf("ERROR: Unable to allocate memory for filename\n");
        exit(1);
    }
    strcpy(cmdfile, argv[1]);
    if(sscanf(argv[2], "%d", num_threads)!=1)
    {
        printf("ERROR: Unable to read the number of threads\n");
        exit(1);
    }
    if(sscanf(argv[3], "%d", num_counters)!=1)
    {
        printf("ERROR: Unable to read the number of counters\n");
        exit(1);
    }
    if(sscanf(argv[4], "%d", log_enabled)!=1)
    {
        printf("ERROR: Unable to read log enabled\n");
        exit(1);
    }

}

void init_ctr_files(int num_counters)
{
    //name is of format "counterxx.txt" 11 characters
    char filename[12]; 
    FILE *f;
    for (long long i =0; i<num_counters; i++)
    {
        sprintf(filename, "counter%lld.txt", i);
        f = fopen(filename, "w");
        if(NULL!=f)
        {
            fputs("0\n", f);
            fclose(f);
        }
        else
        {
            printf("ERROR: unable to open file %s for writring\n", filename);
            exit(1);
        }
    }
}

void *init_threads_routine(void *data)
{
    thread_data* td = (thread_data*)data;
    printf("I'm here! Thread id %d\n", td->number);
    for(;;)
    {
        if(strlen(td->cmd)==0)
        {
            printf("THREAD: %d has no work to do\n", td->number);
            sleep(1);
        }
        else
        {
            td->is_running = true;
            printf("THREAD: %d is active! yay!\n", td->number);
            // command handler
            if (strncmp(td->cmd, "msleep ", 7)==0)
            {
                int sleeptime;
                if (1==sscanf(td->cmd + 7, "%d", &sleeptime))
                {
                    printf("THREAD: %d is SLEEPING!\n", td->number);
                    sleep(sleeptime / 1000);
                }
                td->is_running = false;
            }
            else if (strncmp(td->cmd, "increment ", 10)==0)
            {
                long long file_id;
                char val[0];
                if (1 == sscanf(td->cmd+10, "%lld", &file_id))
                {
                    FILE *f;
                    char filename[12];
                    sprintf(filename, "counter%lld.txt", file_id);
                    printf("THREAD: %d is INCREMENTING file %s!\n", td->number, filename);
                    f = fopen(filename, "rt");
                    if(NULL!=f)
                    {
                        val[0] = fgetc(f) + 1;
                        fclose(f);
                    }
                    else
                    {
                        printf("ERROR: unable to open file %s for writring\n", filename);
                        exit(1);
                    }
                    f = fopen(filename, "wt");
                    if(NULL!=f)
                    {
                        fputs(val, f);
                        fclose(f);
                    }
                    else
                    {
                        printf("ERROR: unable to open file %s for writring\n", filename);
                        exit(1);
                    }
                }
            }
        }
    }
}

void init_threads(pthread_t* pool, thread_data *td, int num_threads)
{
    int status, i;

    for (i=0; i<num_threads; i++)
    {
        strcpy(td[i].cmd, "");
        td[i].is_running = false;
        td[i].number = i;
        status = pthread_create(pool + i, NULL, init_threads_routine, (void *)(&td[i]));
        if(status !=0)
        {
            printf("ERROR: Uanble to create thread pool\n");
            exit(1);
        }
    }
}

int thread_available(thread_data*td, int numt)
{
    for(int i=0; i< numt; i++)
    {
        if (td[i].is_running == false)
            return i;
    }
    return -1;
}

void execute_cmd(thread_data* td, char *cmd, int numt)
{
    int avail_thread = -1;
    while(avail_thread == -1)
    {
        avail_thread = thread_available(td, numt);
        printf("EXEC: no threads available\n");
        sleep(1);
    }
    printf("EXEC: available thread %d\n", avail_thread);
    strcpy(td[avail_thread].cmd, cmd);
}

int main(int argc, char* argv[])
{
    int num_threads, num_counters, log_enabled;
    char* cmdfile, cmd[MAX_LINE_WIDTH];
    pthread_t* thread_pool;
    thread_data *tdata;

    if (argc!= 5)
    {
        printf(USAGE);
        return 1;
    }

    cmdfile = (char*)malloc(strlen(argv[1])+1);
    parse_arguments(argv, cmdfile, &num_threads, &num_counters, &log_enabled);
    
    thread_pool = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    tdata = (thread_data*)malloc(num_threads * sizeof(thread_data));

    init_ctr_files(num_counters);
    init_threads(thread_pool, tdata, num_threads);

    // read commands from file
    FILE *f;
    f = fopen(argv[1], "rt");
    if (NULL!=f)
    {
        while(!feof(f))
        {
            fgets(cmd, MAX_LINE_WIDTH, f);
            printf("MAIN: %s\n", cmd);
            execute_cmd(tdata, cmd, num_threads);
        }
    }
    
    // cleanup
    free(cmdfile);
    free(thread_pool);
    free(tdata);
}