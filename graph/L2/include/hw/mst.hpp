/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file mst.hpp
 *
 */

#ifndef __XF_GRAPH_MST_HPP_
#define __XF_GRAPH_MST_HPP_

namespace xf {
namespace graph {
namespace internal {
namespace mst {

constexpr long INTERFACE_MEMSIZE = 50000;

static bool mstVert[INTERFACE_MEMSIZE];
static unsigned heap_size_ = 0;
static unsigned elems_[INTERFACE_MEMSIZE + 1]; // idx to v
static unsigned idxs_[INTERFACE_MEMSIZE + 1];  // v to idx
static float keys[INTERFACE_MEMSIZE];

inline float e_key(unsigned i) {
#pragma HLS INLINE
    return keys[i];
}

inline unsigned parent(unsigned i) {
#pragma HLS INLINE
    return i >> 1;
}

inline unsigned left(unsigned i) {
#pragma HLS INLINE
    return i << 1;
}

inline unsigned right(unsigned i) {
#pragma HLS INLINE
    return (i << 1) + 1;
}

inline void exchange(unsigned i, unsigned j) {
#pragma HLS INLINE
    unsigned tmp = elems_[i];
    elems_[i] = elems_[j];
    elems_[j] = tmp;
}

inline void exchange_idx(unsigned i, unsigned j) {
#pragma HLS INLINE
    unsigned tmp = idxs_[i];
    idxs_[i] = idxs_[j];
    idxs_[j] = tmp;
}

inline bool empty() {
#pragma HLS INLINE
    return heap_size_ == 0;
}

inline float getLeftWeight(const unsigned cur_idx, unsigned& left_idx) {
#pragma HLS INLINE
    left_idx = left(cur_idx);
    if (left_idx > heap_size_) {
        left_idx = cur_idx;
        return 100.0;
    } else {
        return e_key(elems_[left_idx]);
    }
}

inline float getRightWeight(const unsigned cur_idx, unsigned& right_idx) {
#pragma HLS INLINE
    right_idx = right(cur_idx);
    if (right_idx > heap_size_) {
        right_idx = cur_idx;
        return 100.0;
    } else {
        return e_key(elems_[right_idx]);
    }
}

inline void heapify(unsigned i) {
    unsigned smallest = i;
    unsigned cur_idx = i;
loop6:
    while (cur_idx <= (heap_size_ >> 1)) {
#pragma HLS UNROLL factor = 2
        unsigned left_idx = 0, right_idx = 0;
        float cw = e_key(elems_[cur_idx]);
        float lw = getLeftWeight(cur_idx, left_idx);
        float rw = getRightWeight(cur_idx, right_idx);
        if (cw > lw) {
            smallest = left_idx;
            if (lw > rw) smallest = right_idx;
        } else {
            if (cw > rw) smallest = right_idx;
        }
        if (cur_idx != smallest) {
            exchange_idx(elems_[cur_idx], elems_[smallest]);
            exchange(cur_idx, smallest);
            cur_idx = smallest;
        } else
            break;
    }
}
// return element with minimum key and delete it
inline unsigned pop() {
    unsigned top = elems_[1];
    exchange_idx(elems_[1], elems_[heap_size_]);
    exchange(1, heap_size_--);
    heapify(1);
    return top;
}

inline void push(unsigned elem) {
#pragma HLS INLINE
    elems_[++heap_size_] = elem;
    idxs_[elem] = heap_size_;
    unsigned i = heap_size_;
loop7:
    while (i > 1 && e_key(elems_[parent(i)]) > e_key(elems_[i])) {
#pragma HLS UNROLL factor = 2
        exchange_idx(elems_[i], elems_[parent(i)]);
        exchange(i, parent(i));
        i = parent(i);
    }
}

inline void decrease_key(unsigned elem, float key) {
    keys[elem] = key;
    unsigned i = idxs_[elem];
loop8:
    while (i > 1 && e_key(elems_[parent(i)]) > e_key(elems_[i])) {
#pragma HLS UNROLL factor = 2
        exchange_idx(elems_[i], elems_[parent(i)]);
        exchange(i, parent(i));
        i = parent(i);
    }
}

inline void prim(unsigned int allVert,
                 unsigned int allEdge,
                 unsigned int source,
                 unsigned int* offset,
                 unsigned int* column,
                 float* weight,
                 unsigned* mstRes) {
    unsigned mst_idx = 0;
    unsigned edge_idx = 0;
    bool inQ[INTERFACE_MEMSIZE];
    mstVert[source] = true;
loop0:
    for (unsigned int i = 0; i < allVert; i++) {
#pragma HLS UNROLL factor = 4
        mstVert[i] = false;
        inQ[i] = false;
        keys[i] = 100.0;
    }
    mstRes[source] = source;
    keys[source] = 0;
    push(source);
    inQ[source] = true;
loop2:
    while (!empty()) {
        unsigned tmp = pop();
        mstVert[tmp] = true;
    loop3:
        for (unsigned int i = offset[tmp]; i < offset[tmp + 1]; i++) {
#pragma HLS UNROLL factor = 4
            unsigned to = column[i];
            float tmp_w = weight[i];
            if (mstVert[to] == false && tmp_w < e_key(to)) {
                if (inQ[to] == true) {
                    decrease_key(to, tmp_w);
                    mstRes[to] = tmp;
                } else {
                    keys[to] = tmp_w;
                    push(to);
                    inQ[to] = true;
                    mstRes[to] = tmp;
                }
            }
        }
    }
}

} // namespace mst
} // namespace internal

/**
 * @brief minimum spanning tree based on the Prim algorithm
 *
 * @param allVert vertex number of the input graph
 * @param allEdge edge number of the input graph
 * @param source starting point of the Prim algorithm
 * @param offset row offset of CSR format
 * @param column column index of CSR format
 * @param weight weight value of CSR format
 * @param mstRes the result of the MST. To get the parent vertex of a vertex with ID V in the generated tree,
 * parent=mstRes[V].
 *
 */
inline void mst(unsigned int allVert,
                unsigned int allEdge,
                unsigned int source,
                unsigned int* offset,
                unsigned int* column,
                float* weight,
                unsigned* mstRes) {
    internal::mst::prim(allVert, allEdge, source, offset, column, weight, mstRes);
}

} // namespace graph
} // namespace xf
#endif
