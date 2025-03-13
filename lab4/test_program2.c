#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>

int main() {
    void *handle;
    float (*E)(int);
    char *error;
    int command;
    int x;
    int impl = 1; // Текущая реализация

    handle = dlopen("./lib_e_impl1.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    E = dlsym(handle, "E");
    if ((error = dlerror()) != NULL) {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }

    while (1) {
        printf("Введите команду (0 для смены реализации, 1 x для расчета, 2 для выхода): ");
        scanf("%d", &command);
        if (command == 0) {
            dlclose(handle);
            if (impl == 1) {
                handle = dlopen("./lib_e_impl2.so", RTLD_LAZY);
                impl = 2;
            } else {
                handle = dlopen("./lib_e_impl1.so", RTLD_LAZY);
                impl = 1;
            }
            if (!handle) {
                fprintf(stderr, "%s\n", dlerror());
                exit(1);
            }
            E = dlsym(handle, "E");
            if ((error = dlerror()) != NULL) {
                fprintf(stderr, "%s\n", error);
                exit(1);
            }
            printf("Реализация изменена на %d\n", impl);
        } else if (command == 1) {
            scanf("%d", &x);
            printf("Результат: %f\n", E(x));
        } else if (command == 2) {
            break;
        } else {
            printf("Неизвестная команда\n");
        }
    }

    dlclose(handle);
    return 0;
}
