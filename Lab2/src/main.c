#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    double real;
    double imag;
} Complex;

typedef struct {
    Complex **A;
    Complex **B;
    Complex **C;
    int row;
    int col;
    int n;
} ThreadData;

HANDLE semaphore;

Complex complex_multiply(Complex a, Complex b) {
    Complex result;
    result.real = a.real * b.real - a.imag * b.imag;
    result.imag = a.real * b.imag + a.imag * b.real;
    return result;
}

DWORD WINAPI multiply_row_col(LPVOID param) {
    ThreadData *data = (ThreadData *)param;
    Complex sum = {0, 0};
    for (int k = 0; k < data->n; k++) {
        Complex product = complex_multiply(data->A[data->row][k], data->B[k][data->col]);
        sum.real += product.real;
        sum.imag += product.imag;
    }
    data->C[data->row][data->col] = sum;
    ReleaseSemaphore(semaphore, 1, NULL);
    return 0;
}

void multiply_matrices(Complex **A, Complex **B, Complex **C, int n, int max_threads) {
    semaphore = CreateSemaphore(NULL, max_threads, max_threads, NULL);
    HANDLE *threads = (HANDLE *)malloc(n * n * sizeof(HANDLE));
    ThreadData *data = (ThreadData *)malloc(n * n * sizeof(ThreadData));

    int thread_count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            WaitForSingleObject(semaphore, INFINITE);
            data[thread_count] = (ThreadData){A, B, C, i, j, n};
            threads[thread_count] = CreateThread(NULL, 0, multiply_row_col, &data[thread_count], 0, NULL);
            if (threads[thread_count] == NULL) {
                fprintf(stderr, "Error creating thread\n");
                exit(1);
            }
            thread_count++;
        }
    }

    WaitForMultipleObjects(thread_count, threads, TRUE, INFINITE);
    for (int i = 0; i < thread_count; i++) {
        CloseHandle(threads[i]);
    }
    CloseHandle(semaphore);
    free(threads);
    free(data);
}

void read_matrices(const char *filename, Complex ***A, Complex ***B, int *n) {
    HANDLE file = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening file\n");
        exit(1);
    }

    DWORD bytesRead;
    char buffer[4096];
    ReadFile(file, buffer, sizeof(buffer), &bytesRead, NULL);
    buffer[bytesRead] = '\0';

    char *ptr = buffer;

    sscanf(ptr, "%d", n);
    while (*ptr && *ptr != '\n') ptr++;
    if (*ptr) ptr++;

    *A = (Complex **)malloc(*n * sizeof(Complex *));
    *B = (Complex **)malloc(*n * sizeof(Complex *));
    for (int i = 0; i < *n; i++) {
        (*A)[i] = (Complex *)malloc(*n * sizeof(Complex));
        (*B)[i] = (Complex *)malloc(*n * sizeof(Complex));
    }

    for (int i = 0; i < *n; i++) {
        for (int j = 0; j < *n; j++) {
            sscanf(ptr, "%lf %lf", &(*A)[i][j].real, &(*A)[i][j].imag);
            while (*ptr && *ptr != ' ' && *ptr != '\n') ptr++;
            if (*ptr) ptr++;
        }
    }

    for (int i = 0; i < *n; i++) {
        for (int j = 0; j < *n; j++) {
            sscanf(ptr, "%lf %lf", &(*B)[i][j].real, &(*B)[i][j].imag);
            while (*ptr && *ptr != ' ' && *ptr != '\n') ptr++;
            if (*ptr) ptr++;
        }
    }

    CloseHandle(file);
}

void free_matrix(Complex **matrix, int n) {
    for (int i = 0; i < n; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

void print_matrix(Complex **matrix, int n) {
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    char buffer[256];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int len = sprintf(buffer, "(%.2f + %.2fi) ", matrix[i][j].real, matrix[i][j].imag);
            WriteConsoleA(console, buffer, len, &written, NULL);
        }
        WriteConsoleA(console, "\n", 1, &written, NULL);
    }
}

double get_time() {
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    return (double)start.QuadPart / frequency.QuadPart;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <max_threads>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int max_threads = atoi(argv[2]);
    if (max_threads <= 0) {
        fprintf(stderr, "Invalid number of threads\n");
        return 1;
    }

    printf("Reading matrices from file: %s\n", filename);

    int n;
    Complex **A, **B, **C;

    read_matrices(filename, &A, &B, &n);

    printf("Matrix size: %d x %d\n", n, n);

    C = (Complex **)malloc(n * sizeof(Complex *));
    for (int i = 0; i < n; i++) {
        C[i] = (Complex *)malloc(n * sizeof(Complex));
    }

    printf("Multiplying matrices...\n");

    double start_time = get_time();
    multiply_matrices(A, B, C, n, max_threads);
    double end_time = get_time();

    printf("Multiplication complete. Result matrix:\n");

    print_matrix(C, n);

    printf("Time taken: %.6f seconds\n", end_time - start_time);

    free_matrix(A, n);
    free_matrix(B, n);
    free_matrix(C, n);

    printf("Memory freed, program complete.\n");

    return 0;
}
