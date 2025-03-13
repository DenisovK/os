float E(int x) {
    float sum = 0;
    float factorial = 1;
    for (int n = 0; n <= x; n++) {
        if (n > 0) {
            factorial *= n;
        }
        sum += 1.0 / factorial;
    }
    return sum;
}
