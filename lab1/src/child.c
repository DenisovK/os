#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


int main() {
    char read_msg[100];
    char response_msg[100];

    // Читаем имя файла из stdin
    read(STDIN_FILENO, read_msg, sizeof(read_msg));
    FILE *fp = fopen(read_msg, "w");

    if (!fp) {
        perror("file error");
        return -1;
    }

    while (1) {
        int bytes_read = read(STDIN_FILENO, read_msg, sizeof(read_msg));
        if (bytes_read <= 0) {
            break;
        }
        int len = strlen(read_msg);
        if (len > 0 && (read_msg[len - 2] == ';' || read_msg[len - 2] == '.')) {
            fputs(read_msg, fp);
            strcpy(response_msg, "Success");
        } 
        else {
            strcpy(response_msg, "Not over in ';' or '.'");
        }
        // Пишем данные в stdout
        write(STDOUT_FILENO, response_msg, strlen(response_msg) + 1);
    }
    fclose(fp);

    return 0;
}
