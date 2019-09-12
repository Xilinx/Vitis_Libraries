#include <iostream>
#include <cstdlib>

using namespace std;

extern void uut_top(
    uint32_t p_m, uint32_t p_n, uint32_t p_k, uint32_t p_r, BLAS_dataType* p_a, BLAS_dataType* p_b, BLAS_dataType* p_c);
void mmult(uint32_t p_m, uint32_t p_n, uint32_t p_k, BLAS_dataType* p_a, BLAS_dataType* p_b, BLAS_dataType* p_c) {
    for (int i = 0; i < p_m; i++)
        for (int j = 0; j < p_n; j++) {
            BLAS_dataType tmp = 0;
            for (int k = 0; k < p_k; k++) tmp += p_a[k * p_m + i] * p_b[k * p_n + j];
            p_c[i * p_n + j] = tmp;
        }
}

int main() {
    BLAS_dataType p_a[BLAS_r * BLAS_m * BLAS_k];
    for (int i = 0; i < BLAS_r * BLAS_m * BLAS_k; i++) p_a[i] = i + 1;
    BLAS_dataType p_b[BLAS_r * BLAS_n * BLAS_k];
    for (int i = 0; i < BLAS_r * BLAS_n * BLAS_k; i++) p_b[i] = i << 1;
    BLAS_dataType p_c[BLAS_r * BLAS_m * BLAS_n];
    BLAS_dataType p_cref[BLAS_r * BLAS_m * BLAS_n];
    uut_top(BLAS_m, BLAS_n, BLAS_k, BLAS_r, p_a, p_b, p_c);
    for (int i = 0; i < BLAS_r; i++)
        mmult(BLAS_m, BLAS_n, BLAS_k, p_a + i * BLAS_m * BLAS_k, p_b + i * BLAS_n * BLAS_k,
              p_cref + i * BLAS_m * BLAS_n);
    int err = 0;
    for (int i = 0; i < BLAS_r * BLAS_n * BLAS_m; i++) {
        if (p_c[i] != p_cref[i]) {
            err++;
            cout << "p_c[" << i << "]=" << p_c[i] << '\t' << "p_cref[" << i << "]=" << p_cref[i] << endl;
        }
    }
    if (err == 0)
        return 0;
    else {
        cout << "There are in total " << err << " errors." << endl;
        return -1;
    }
}
