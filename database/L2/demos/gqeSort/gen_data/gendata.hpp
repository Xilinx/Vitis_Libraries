#ifndef GEN_DATA_HPP
#define GEN_DATA_HPP
#include <cstdlib>
#include <cstdint>

inline int gen_dat(const std::string& file_path, size_t n) {
    uint64_t* data = (uint64_t*)malloc(sizeof(uint64_t) * n);
    if (!data) {
        return -1;
    }

    for (size_t j = 0; j < n; j++) {
        int32_t tmp = rand();
        uint64_t tmp2 = tmp + 1;
        tmp2 <<= 32;
        tmp2 |= tmp;
        data[j] = tmp2;
    }

    FILE* f = fopen(file_path.c_str(), "wb");
    if (!f) {
        std::cerr << "ERROR: " << file_path << " cannot be opened for binary read." << std::endl;
    }
    size_t cnt = fwrite((void*)data, sizeof(uint64_t), n, f);
    fclose(f);
    if (cnt != n) {
        std::cerr << "ERROR: " << cnt << " entries read from " << file_path << ", " << n << " entries required."
                  << std::endl;
        return -1;
    }

    free(data);
    return 0;
}

#endif
