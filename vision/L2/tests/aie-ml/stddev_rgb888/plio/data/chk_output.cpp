#include <iostream>
#include <cmath>
#include <fstream>
#include "./../config.h"
#include <cstring>  

void read_file_int8(FILE* fpout, uint8_t* in_buffer, int n) {
    for (int i = 0; i < n; i++) {
        fscanf(fpout, "%hhd \n", (in_buffer));
        in_buffer += 1;
    }
}

void read_file_float(FILE* fpout, float* in_buffer, int n) {
    for (int i = 0; i < n; i++) {
        fscanf(fpout, "%f \n", (in_buffer));
        in_buffer += 1;
    }
}

int main(int argc, char** argv) {
    uint8_t* aie_out = (uint8_t*)std::malloc(TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS * sizeof(float) + 64);
    float* ref_out = (float*)std::malloc(TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS * sizeof(float));

    FILE* fp_aie = fopen("output_Y1.txt", "r");
    if (fp_aie == NULL) {
        printf("Failure opening file %s for reading!!\n", "output_Y1.txt");
        return -1;
    } else {
        printf("Opened file for reading: %s!!\n", " AIE output in output_Y1.txt");
        read_file_int8(fp_aie, aie_out, TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS * sizeof(float) + 64);
    }

    FILE* fp_ref = fopen("output_ref.txt", "r");
    if (fp_ref == NULL) {
        printf("Failure opening file %s for reading!!\n", "output_ref.txt");
        return -1;
    } else {
        printf("Opened file for reading: %s!!\n", "output_ref.txt");
        read_file_float(fp_ref, ref_out, TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS);
    }

    // convert to float
    int ERR_CNT = 0;
    aie_out+=64;
    for (int i = 0; i < 3; i+=1) {
        float value;
        std::memcpy(&value, &aie_out[4*i], sizeof(float));
        std::cout << " AIE out = " << value << " ; ref_out = " << ref_out[i] << std::endl;

        if (abs(value - ref_out[i]) > 4) {
            ERR_CNT += 1;
        }
    }

    if (ERR_CNT == 0) {
        std::cout << " TEST PASSED " << std::endl;
    } else {
        std::cout << " TEST FAILED  with ERR_CNT = " << ERR_CNT << std::endl;
    }

    fclose(fp_aie);
    fclose(fp_ref);
}
