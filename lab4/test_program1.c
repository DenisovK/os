#include <stdio.h>

// Объявление функции из библиотеки
extern float E(int x);

int main() {
    int command;
    int x;
    while (1) {
        printf("Введите команду (1 x для расчета, 0 для выхода): ");
        scanf("%d", &command);
        if (command == 0) {
            break;
        } else if (command == 1) {
            scanf("%d", &x);
            printf("Результат: %f\n", E(x));
        } else {
            printf("Неизвестная команда\n");
        }
    }
    return 0;
}
