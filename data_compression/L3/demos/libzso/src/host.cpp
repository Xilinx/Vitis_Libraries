#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <assert.h>
#define CHUNK 16384

void zlib_compress(char* inFile) {
    std::string outFile = inFile;
    outFile = outFile + ".zlib";
    uint64_t compress_len;

    FILE* fptr;
    fptr = fopen(inFile, "rb");
    fseek(fptr, 0, SEEK_END);
    uint32_t input_size = ftell(fptr);
    fclose(fptr);
    uint32_t insize_print = (input_size / 1000000);
    uint8_t* input = (uint8_t*)calloc(input_size, 1);
    uint8_t* compress_out = (uint8_t*)calloc(input_size * 2, 1);

    fptr = fopen(inFile, "rb");
    fread(input, 1, input_size, fptr);
    fclose(fptr);

    std::chrono::duration<double, std::milli> compress_API_time_ms_1(0);
    auto compress_API_start = std::chrono::high_resolution_clock::now();

    int err = compress(compress_out, &compress_len, input, input_size);

    auto compress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(compress_API_end - compress_API_start);
    compress_API_time_ms_1 = duration;

    std::cout << "Time: " << std::fixed << std::setprecision(2) << compress_API_time_ms_1.count() << " ms" << std::endl;
    ;
    std::cout << "Input File: " << inFile << " (" << insize_print << " MB)" << std::endl;
    std::cout << "Output File: " << outFile.c_str() << " (" << (compress_len / 1000000) << " MB)" << std::endl;
    std::cout << "Compression Ratio: " << std::setprecision(2) << ((float)input_size / (float)compress_len)
              << std::endl;

    fptr = fopen(outFile.c_str(), "wb");
    fwrite(compress_out, 1, compress_len, fptr);
    fclose(fptr);
}

void zlib_uncompress(char* inFile) {
    std::string outFile = inFile;
    outFile = outFile + ".orig";
    uint64_t uncompress_len;

    FILE* fptr;
    fptr = fopen(inFile, "rb");
    fseek(fptr, 0, SEEK_END);
    int input_size = ftell(fptr);
    fclose(fptr);
    uint32_t insize_print = (input_size / 1000000);
    uint8_t* input = (uint8_t*)calloc(input_size, 1);
    uint8_t* uncompress_out = (uint8_t*)calloc(input_size * 20, 1);

    fptr = fopen(inFile, "rb");
    fread(input, 1, input_size, fptr);
    fclose(fptr);

    std::chrono::duration<double, std::milli> compress_API_time_ms_1(0);
    auto compress_API_start = std::chrono::high_resolution_clock::now();

    int err = uncompress(uncompress_out, &uncompress_len, input, input_size);

    auto compress_API_end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(compress_API_end - compress_API_start);
    compress_API_time_ms_1 = duration;

    std::cout << "Time: " << std::fixed << std::setprecision(2) << compress_API_time_ms_1.count() << " ms" << std::endl;
    ;
    std::cout << "Input File: " << inFile << " (" << insize_print << " MB)" << std::endl;
    std::cout << "Output File: " << outFile.c_str() << " (" << (uncompress_len / 1000000) << " MB)" << std::endl;

    // printf("uncompress_len %d err %d \n", uncompress_len, err);

    fptr = fopen(outFile.c_str(), "wb");
    fwrite(uncompress_out, 1, uncompress_len, fptr);
    fclose(fptr);
}

int main(int argc, char* argv[]) {
    char* c_opt = "-c";
    char* d_opt = "-d";
    char* v_opt = "-v";
    char* cu_opt = "-cu";
    char* h_opt = "-h";
    char* cu_id = "0";
    char* inFile_name = argv[2];
    int ret;

help:

    if (argc < 3 || !strcmp(h_opt, argv[2])) {
        std::cout << "\n";
        std::cout << "-c  --> Compress"
                  << "\n";
        std::cout << "-d  --> DeCompress"
                  << "\n";
        std::cout << "-v  --> Validate"
                  << "\n";
        std::cout << "-cu --> Compute Unit"
                  << "\n";
        std::cout << "\n";
        std::cout << "[Example]: zlib.exe -v <input_file> -cu <0/1>" << std::endl;
        std::cout << "\n";
        exit(1);
    }

    if (argc > 3) {
        if (!strcmp(cu_opt, argv[3])) cu_id = argv[4];
    }

    setenv("XILINX_CU_ID", cu_id, 1);

    if (!strcmp(c_opt, argv[1])) {
        printf("\n");
        printf("ZLIB Compression - LIBZSO \n");
        zlib_compress(inFile_name);
    }

    if (!strcmp(d_opt, argv[1])) {
        printf("\n");
        printf("ZLIB DeCompression - LIBZSO \n");
        zlib_uncompress(inFile_name);
    }

    if (!strcmp(v_opt, argv[1])) {
        printf("\n");
        printf("ZLIB Compression - LIBZSO \n\n");

        // Compress
        zlib_compress(inFile_name);

        printf("\n");
        printf("ZLIB DeCompression - LIBZSO \n\n");
        std::string outFile = inFile_name;
        outFile = outFile + ".zlib";

        // Decompress
        zlib_uncompress((char*)outFile.c_str());

        std::string in = inFile_name;
        std::string out = outFile + ".orig";
        std::string command = "cmp " + in + " " + out;

        // Compare input/output
        int ret = system(command.c_str());

        printf("\n");
        if (ret == 0)
            std::cout << "TEST PASSED" << std::endl;
        else
            std::cout << "TEST FAILED" << std::endl;
    }
}
