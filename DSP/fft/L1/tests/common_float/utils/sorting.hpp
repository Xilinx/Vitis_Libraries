#ifndef _SORTING_H_1028567
#define _SORTING_H_1028567
#include <algorithm> // std::sort
#include <vector>    // std::vector

template <typename T>
bool compare_nums(T i, T j) {
    return i < j;
}
template <typename T, int size>
void sort(T data[]) {
    std::vector<T> vectorData(data, data + size);
    std::sort(vectorData.begin(), vectorData.end(), compare_nums<T>);
    int i = 0;
    for (typename std::vector<T>::iterator iter = vectorData.begin(); iter != vectorData.end(); iter++) {
        data[i] = *iter;
        i++;
    }
}

#endif
