
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    int pipe1[2];
    int pipe2[2];
    pid_t pid;

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    // pipe[0] - чтение, pipe[1] - запись

    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        dup2(pipe1[0], STDIN_FILENO); // Перенаправляем стандартный ввод на pipe1[0]
        dup2(pipe2[1], STDOUT_FILENO); // Перенаправляем стандартный вывод на pipe2[1]

        close(pipe1[0]);
        close(pipe2[1]);

        execlp("./child", "child", NULL);
        perror("execlp failed");
        exit(1);
    }
    else {
        char msg[100];
        char read_msg[100];

        close(pipe1[0]);
        close(pipe2[1]);

        printf("Write a doc name\n");
        fgets(msg, sizeof(msg), stdin);
        write(pipe1[1], msg, strlen(msg) + 1);

        while (1) {
            printf("Enter a message (exit for exit)\n");
            fgets(msg, sizeof(msg), stdin);
            if (strncmp(msg, "exit", 4) == 0) {
                break;
            }
            write(pipe1[1], msg, strlen(msg) + 1);
            read(pipe2[0], read_msg, sizeof(read_msg));
            printf("Status: %s\n", read_msg);
        }

        close(pipe1[1]);
        close(pipe2[0]);
        waitpid(pid, NULL, 0);
    }

    return 0;
}
