#include <zmq.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

void *heartbit_thread(void *arg)
{
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_SUB);
    zmq_connect(socket, "tcp://localhost:5555");
    zmq_setsockopt(socket, ZMQ_SUBSCRIBE, "", 0);

    int time = *(int *)arg;
    while (1)
    {
        char buffer[256];
        zmq_recv(socket, buffer, 256, 0);
        usleep(time * 1000);
    }
    zmq_close(socket);
    zmq_ctx_destroy(context);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <id>\n", argv[0]);
        return 1;
    }

    int id = atoi(argv[1]);
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_REP);
    char address[256];
    snprintf(address, sizeof(address), "tcp://*:5557%d", id);
    zmq_bind(socket, address);

    time_t start_time = 0;
    int timer_running = 0;

    char buffer[256];
    while (1)
    {
        zmq_recv(socket, buffer, 256, 0);
        if (strncmp(buffer, "start", 5) == 0)
        {
            if (!timer_running)
            {
                start_time = time(NULL);
                timer_running = 1;
                zmq_send(socket, "Ok", 2, 0);
            }
            else
            {
                zmq_send(socket, "Error", 5, 0);
            }
        }
        else if (strncmp(buffer, "stop", 4) == 0)
        {
            if (timer_running)
            {
                timer_running = 0;
                zmq_send(socket, "Ok", 2, 0);
            }
            else
                zmq_send(socket, "Error", 5, 0);
        }
        else if (strncmp(buffer, "time", 4) == 0)
        {
            if (timer_running)
            {
                char response[256];
                snprintf(response, sizeof(response), "%ld", time(NULL) - start_time);
                zmq_send(socket, response, strlen(response), 0);
            }
            else
                zmq_send(socket, "Error", 5, 0);
        }
        else if (strncmp(buffer, "ping", 4) == 0)
        {
            if (timer_running)
                zmq_send(socket, "Error", 5, 0);
            else
            {
                zmq_send(socket, "Pong", 4, 0);
            }
        }
        else
            zmq_send(socket, "Error", 5, 0);
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);
    return 0;
}