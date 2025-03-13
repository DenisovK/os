
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>

#define SHARED_MEMORY_SIZE 256

int main() {
    const char *shared_memory_name = "/shared_memory";
    int fd = shm_open(shared_memory_name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, SHARED_MEMORY_SIZE);

    char *shared_memory = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }
    if (pid == 0) {
        execlp("./child", "./child", shared_memory_name);
        perror("execlp");
        return 1;
    } 
    else {
        printf("Write a doc name\n");
        fgets(shared_memory, SHARED_MEMORY_SIZE, stdin);
        shared_memory[strcspn(shared_memory, "\n")] = 0;

        // Даём время дочернему процессу открыть файл
        sleep(1);

        while (1) {
            printf("Enter a message (exit for exit)\n");
            fgets(shared_memory, SHARED_MEMORY_SIZE, stdin);
            if (strncmp(shared_memory, "exit", 4) == 0) {
                break;
            }
            
            // Ждем достаточное время для обработки дочерним процессом
            sleep(1);
            printf("Status: %s\n", shared_memory + 128);
        }

        waitpid(pid, NULL, 0);
        shm_unlink(shared_memory_name);
    }

    return 0;
}
