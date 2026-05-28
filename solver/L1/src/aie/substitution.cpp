#include "substitution.hpp"
#include "substitution_utils.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/aie.hpp"
#include "kernel_api_utils.hpp"


namespace xf::solver::aie::substitution {

using namespace adf;

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV>
INLINE_DECL void
substitution_base<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV>::substitution_main(
    fbsub_input_params<TT_DATA>& input_params,
    fbsub_output_params<TT_DATA>& output_params) {

  //TP_DIM_SIZE is the DIM_SIZE of this kernel, not the overall.
  static constexpr int kStartCol = TP_DIM_SIZE * TP_X;    //the actual first column
  static constexpr int kEndCol = kStartCol + TP_DIM_SIZE; //one more than the last column - which makes loops easier
  static constexpr int kStartRow = TP_DIM_SIZE * TP_Y;
  static constexpr int kEndRow = kStartRow + TP_DIM_SIZE;


  using acc_t = accTypeFbsub_t<TT_DATA>::type;
  using acc_vect_t = ::aie::accum<acc_t, m_kvecSampleNum>;
  using vect_t = ::aie::vector<TT_DATA, m_kvecSampleNum>;
  using invvect_t = ::aie::vector<tFbsubBaseType<TT_DATA>, m_kvecSampleNum>;

  static constexpr int kVecPerRow = CEIL(TP_DIM_SIZE,m_kvecSampleNum) /m_kvecSampleNum;

  vect_t* __restrict pL = (vect_t*)input_params.in_L; //L values are only ever read. They may be read as samples or vectors.
  vect_t* __restrict pLlocal = (vect_t*)input_params.in_L; //a local L pointer is used to speed up a loop, while pL remains the anchor to the start of the iobuffer.
  TT_DATA* __restrict pLsamp = (TT_DATA*)input_params.in_L; //used to retrieve the special diagonal values.
  vect_t* __restrict pXv = (vect_t*)output_params.out_x;//actually not unique to region, but chess_memory_fence protects
  vect_t lVec; //a vector of values from  L (triangular input matrix)
  vect_t xVec; //a vector of values from x, read in from a higher kernel.
  vect_t xCache; //register vector of X values used as a cache to reduce the number of memory accesses.
  TT_DATA newX; //the value of X calculated in each iteration of this algorithm.
  acc_vect_t acc; //The dot product accumulation of a row of L multiplied by the already calculated X vals.
  vect_t* __restrict pYv = (vect_t*)input_params.in_y;
  vect_t sumv; //the register form of acc, so float rather than accfloat, say.
  vect_t yVect; //a vector of ys, not to be confused with a crash of rhinos or a bloat of hippos
  TT_DATA Lii; //A diagonal element
  TT_DATA yj, sumj; //The calculation of each X is ultimately a scalar equation, needing scalar members of yvect and sum
  invvect_t* __restrict invXv = (invvect_t*)m_invX;
  tFbsubBaseType<TT_DATA> myInv; //diagonal element, reciprocated.
  invvect_t myInvs; //a vector of reciprocated diagonal elements.
  vect_t* xStore = (vect_t*)xStoreSamp; //used to store X vectors from the highest kernel in a column.




  //------------------------
  //Invert diagonal elements if necessary and collect them into a vector cache
  //------------------------
  for (int frame = 0; frame < TP_NUM_FRAMES; frame++) {
    //Only kernels on the diagonal create an X output and only these need to invert the diagonal elements coming in.
    if constexpr (TP_X == TP_Y) {
      //zero output
#pragma unroll kVecPerRow
      for (int i = kStartRow/m_kvecSampleNum; i < kEndRow/m_kvecSampleNum; i++) {
        pXv[i-kStartRow/m_kvecSampleNum] = ::aie::zeros<TT_DATA,m_kvecSampleNum>();
      }

      //Create a vector of pre-inverted diagonal elements.
      //or, for pre-inverted diagonal elements, collect them together so they may later be accessed as a vector
      for (int i = kStartRow; i< kEndRow; i++) {
        if constexpr(TP_DIAG_INV==0) {
          this->m_invX[i-kStartRow] = fnMyInv<TT_DATA>(pLsamp[(i-kStartRow)*(TP_DIM_SIZE+1)]);
        } else {
          if constexpr (isComplex<TT_DATA>()) {
            this->m_invX[i-kStartRow] = pLsamp[(i-kStartRow)*(TP_DIM_SIZE+1)].real;
          } else {
            this->m_invX[i-kStartRow] = pLsamp[(i-kStartRow)*(TP_DIM_SIZE+1)];
          }
        }
      }
      chess_memory_fence(); //ensure that the m_invX values have been stored before retrieving them.
    } //if TP_X == TP_Y

    //--------------------
    //FORWARD SUBSTITUTION
    //--------------------
    if constexpr (TP_SUBST_TYPE == 0) { // forward substitution

      for (int i = kStartRow/m_kvecSampleNum; i < kEndRow/m_kvecSampleNum; i++) {//for all vertical chunks (each chunk is m_kvecSampleNum high)
        if constexpr(kStartCol == 0) {
          acc = ::aie::zeros<acc_t,m_kvecSampleNum>();
        } else {
          acc = readincr_v<m_kvecSampleNum, acc_t>(input_params.in_pp);
        }

        int looplim;
        if constexpr (TP_X==TP_Y) {
          looplim = i;
        } else {
          looplim = kEndCol/m_kvecSampleNum;
        }
        for (int k= kStartCol/m_kvecSampleNum; k < looplim; k++) { //for each column chunk in the vertical chunk
          if (k<kEndCol/m_kvecSampleNum) {
            pLlocal = &pL[((k-kStartCol/m_kvecSampleNum)*m_kvecSampleNum)*kVecPerRow+(i-kStartRow/m_kvecSampleNum)];
            if ((k<kStartRow/m_kvecSampleNum) && (i == kStartRow/m_kvecSampleNum)) { //x from higher kernel
              xVec = readincr_v<m_kvecSampleNum,aie_stream_resource_in::a>(input_params.in_x);
              xStore[k-kStartCol/m_kvecSampleNum] = xVec;
            } else {
              //x from local output
              if constexpr(TP_X == TP_Y) {
                xVec = pXv[k-kStartCol/m_kvecSampleNum];
              } else {
                xVec = xStore[k-kStartCol/m_kvecSampleNum];
              }
            }
#pragma unroll m_kvecSampleNum
            for (int p = 0; p<m_kvecSampleNum; p++)  { //row within a block. chess pipelining and chess_loop_range(m_kvecSampleNum,) is slower than unroll here
              lVec = *pLlocal;
              pLlocal += kVecPerRow;
              acc = ::aie::mac(acc, lVec, xVec.get(p));
            }
          }
          if constexpr(TP_X < TP_Y) {
            if (k == kEndCol/m_kvecSampleNum-1) {
              writeincr<acc_t, m_kvecSampleNum>(output_params.out_pp, acc);
            }
          } //if TP_X<TP_Y
        } // k  loop

        //diagonal handling
        if constexpr(TP_X == TP_Y) {
          xCache = ::aie::zeros<TT_DATA,m_kvecSampleNum>();
          yVect = pYv[i-kStartRow/m_kvecSampleNum];
          sumv = acc.template to_vector<TT_DATA>();

          myInvs = invXv[i-kStartRow/m_kvecSampleNum];
          for (int j = 0; j < m_kvecSampleNum; j++) {
            if constexpr (TP_DIM_SIZE % m_kvecSampleNum != 0) {
              int z = i * m_kvecSampleNum + j; // Current row/element to resolve
              if (z >= TP_DIM_SIZE) continue;
            }
            lVec = pL[i-kStartRow/m_kvecSampleNum + (i-kStartRow/m_kvecSampleNum)*m_kvecSampleNum*kVecPerRow + j*kVecPerRow];
            myInv = myInvs.get(j);//[i * m_kvecSampleNum + j];
            yj = yVect.get(j);
            sumj = sumv.get(j);
            newX = (yj - sumj)*myInv;
            xCache.set(newX,j);
            acc = ::aie::mac(acc, lVec, newX);
            sumv = acc.template to_vector<TT_DATA>();
          } //end of j loop
          pXv[i-kStartCol/m_kvecSampleNum] = xCache; //write cache to memory
          if constexpr (TP_Y < TP_GRID_DIM-1) { //i.e. not a bottom kernel
            writeincr<aie_stream_resource_out::a, TT_DATA,m_kvecSampleNum>(output_params.out_x_strm, xCache);
          }
          chess_memory_fence(); //and allow that to complete before reading.
        } //TP_X==TP_Y i.e. diagonal handling
      } // i loop end of block loop
    } else { // backward substitution

      //---------------------
      //BACKWARD SUBSTITUTION
      //---------------------
      // In backward substitution, the logical processing of the matrix is from bottom-right to top-left.
      // Due to the L-port mapping in the graph, the physical kernel grid operates on this remapped matrix.
      // The physical data flow remains the same as forward substitution:
      // - Partial products (pp) cascade from left-to-right along physical rows.
      // - Solved x-values stream down physical columns from diagonal kernels.
      // An off-diagonal kernel (TP_X < TP_Y) receives a block of x-values from the diagonal kernel
      // in its column, (TP_X, TP_X), stores them locally, and uses them to compute partial products.
      // The x values are read from the stream on the first pass of the 'i' loop and stored locally.
      // Subsequent passes use the stored values. This mimics the forward substitution implementation.

      for (int i = (kEndRow/m_kvecSampleNum) - 1; i+1 >= kStartRow/m_kvecSampleNum+1; i--) { //for all vertical chunks, backwards
        if constexpr(TP_X == 0) { //Kernels in the first physical column are the start of a pp chain.
          acc = ::aie::zeros<acc_t,m_kvecSampleNum>();
        } else {
          acc = readincr_v<m_kvecSampleNum, acc_t>(input_params.in_pp);
        }
        for (int k = (kEndCol/m_kvecSampleNum)-1; k+1 >= kStartCol/m_kvecSampleNum+1; k--) { //for each column chunk in the vertical chunk, backwards
          if (TP_X < TP_Y || k > i) { // Off-diagonal kernels process the whole rectangle, diagonal kernels process the upper triangle.
            pLlocal = &pL[((k-kStartCol/m_kvecSampleNum)*m_kvecSampleNum)*kVecPerRow+(i-kStartRow/m_kvecSampleNum)];

            if constexpr (TP_X < TP_Y) { // This kernel is off-diagonal, it consumes x from stream
              if (i == (kEndRow/m_kvecSampleNum) - 1) { // First pass of the i-loop, read from stream
                xVec = readincr_v<m_kvecSampleNum, aie_stream_resource_in::a>(input_params.in_x);
                xStore[k-kStartCol/m_kvecSampleNum] = xVec;
              } else { // Subsequent passes, read from local store
                xVec = xStore[k-kStartCol/m_kvecSampleNum];
              }
            } else { // This is a diagonal kernel, it produces x
              xVec = pXv[k-kStartCol/m_kvecSampleNum];
            }

#pragma unroll m_kvecSampleNum
            for (int p = 0; p<m_kvecSampleNum; p++)  { //row within a block.
              lVec = *pLlocal;
              pLlocal += kVecPerRow;
              if constexpr (isComplex<TT_DATA>()) {
                acc = ::aie::mac(acc, ::aie::op_conj(lVec), xVec.get(p));
              } else {
                acc = ::aie::mac(acc, lVec, xVec.get(p));
              }
            } //p loop
          } //TP_X < TP_Y || k > i
        } //k loop
        if constexpr(TP_X < TP_Y) { // Off-diagonal kernels write to the pp-cascade.
          writeincr<acc_t, m_kvecSampleNum>(output_params.out_pp, acc);
        }

        //diagonal handling
        if constexpr(TP_X == TP_Y) {
          xCache = ::aie::zeros<TT_DATA,m_kvecSampleNum>();
          yVect = pYv[i-kStartRow/m_kvecSampleNum];
          sumv = acc.template to_vector<TT_DATA>();

          myInvs = invXv[i-kStartRow/m_kvecSampleNum];
          for (int j = m_kvecSampleNum - 1; j+1 >= 0+1; j--) {
            if constexpr (TP_DIM_SIZE % m_kvecSampleNum != 0) {
              int z = i * m_kvecSampleNum + j; // Current row/element to resolve
              if (z >= TP_DIM_SIZE) continue;
            }
            lVec = pL[i-kStartRow/m_kvecSampleNum + (i-kStartRow/m_kvecSampleNum)*m_kvecSampleNum*kVecPerRow + j*kVecPerRow];
            myInv = myInvs.get(j);
            yj = yVect.get(j);
            sumj = sumv.get(j);
            newX = (yj - sumj)*myInv;
            xCache.set(newX,j);
            if constexpr (isComplex<TT_DATA>()) {
              acc = ::aie::mac(acc, ::aie::op_conj(lVec), newX);
            } else {
              acc = ::aie::mac(acc, lVec, newX);
            }
            sumv = acc.template to_vector<TT_DATA>();
          } //end of j loop
          pXv[i-kStartCol/m_kvecSampleNum] = xCache; //write cache to memory
          if constexpr (TP_Y < TP_GRID_DIM-1) { //i.e. not a bottom kernel
            writeincr<aie_stream_resource_out::a, TT_DATA,m_kvecSampleNum>(output_params.out_x_strm, xCache);
          }
          chess_memory_fence(); //and allow that to complete before reading.
        } //TP_X==TP_Y
      } //end of block loop
    } //end of backwards substitution
    pL += TP_DIM_SIZE*TP_DIM_SIZE/m_kvecSampleNum;
    pLlocal = pL;
    pLsamp = (TT_DATA*)pL;
    pXv += TP_DIM_SIZE/m_kvecSampleNum;
    pYv += TP_DIM_SIZE/m_kvecSampleNum;
  }//frame loop
}

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>::substitution_single(
    input_buffer<TT_IO_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    output_buffer<TT_DATA>& out_x) {
    fbsub_input_params<TT_DATA> input_params = {(TT_DATA*)in_L.data(), in_y.data(), nullptr, nullptr};
    fbsub_output_params<TT_DATA> output_params = {out_x.data(), nullptr, nullptr};
    this->substitution_main(input_params, output_params);
}

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>::substitution_top(
    input_buffer<TT_IO_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    output_buffer<TT_DATA>& __restrict out_x,
    output_stream<TT_DATA>* __restrict out_x_strm) {
    fbsub_input_params<TT_DATA> input_params = {(TT_DATA*)in_L.data(), in_y.data(), nullptr, nullptr};
    fbsub_output_params<TT_DATA> output_params = {out_x.data(), out_x_strm, nullptr};
    this->substitution_main(input_params, output_params);
}

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>::substitution_first(
    input_buffer<TT_IO_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    input_stream<TT_DATA>* __restrict in_x,
    output_cascade<accTypeFbsub<TT_DATA>>* __restrict out_pp) {
    fbsub_input_params<TT_DATA> input_params = {(TT_DATA*)in_L.data(), in_y.data(), in_x, nullptr};
    fbsub_output_params<TT_DATA> output_params = {nullptr, nullptr, out_pp};
    this->substitution_main(input_params, output_params);
}

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>::substitution_mid(
    input_buffer<TT_IO_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    input_stream<TT_DATA>* __restrict in_x,
    input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp,
    output_cascade<accTypeFbsub<TT_DATA>>* __restrict out_pp) {
    fbsub_input_params<TT_DATA> input_params = {(TT_DATA*)in_L.data(), in_y.data(), in_x, in_pp};
    fbsub_output_params<TT_DATA> output_params = {nullptr, nullptr, out_pp};
    this->substitution_main(input_params, output_params);
}

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>::substitution_last(
    input_buffer<TT_IO_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp,
    output_buffer<TT_DATA>& __restrict out_x,
    output_stream<TT_DATA>* __restrict out_x_strm) {
    fbsub_input_params<TT_DATA> input_params = {(TT_DATA*)in_L.data(), in_y.data(), nullptr, in_pp};
    fbsub_output_params<TT_DATA> output_params = {out_x.data(), out_x_strm, nullptr};
    this->substitution_main(input_params, output_params);
}

template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV,
          typename TT_IO_DATA>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>::substitution_last_bottom(
    input_buffer<TT_IO_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    input_cascade<accTypeFbsub<TT_DATA>>* __restrict in_pp,
    output_buffer<TT_DATA>& __restrict out_x) {
    fbsub_input_params<TT_DATA> input_params = {(TT_DATA*)in_L.data(), in_y.data(), nullptr, in_pp};
    fbsub_output_params<TT_DATA> output_params = {out_x.data(), nullptr, nullptr};
    this->substitution_main(input_params, output_params);
}
  /*
//This is the original code kept for reference.
template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM>
NOINLINE_DECL void
substitution<TT_DATA, TP_DIM_SIZE, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM>::substitution_single_ai(
    input_buffer<TT_DATA>& __restrict in_L,
    input_buffer<TT_DATA>& __restrict in_y,
    output_buffer<TT_DATA>& __restrict out_x) {
    tFbsubBaseType<TT_DATA> myInv;
    TT_DATA* p_L = in_L.data();
    TT_DATA* p_y = in_y.data();
    TT_DATA* p_x = out_x.data();
    using acc_t = accTypeFbsub_t<TT_DATA>::type;
    using acc_vect_t = ::aie::accum<acc_t, m_kvecSampleNum>;
    using vect_t = ::aie::vector<TT_DATA, m_kvecSampleNum>;
    if constexpr (TP_SUBST_TYPE == 0) { // 0 for forward, 1 for backward
        // Forward substitution: L*x = y
        for (int i = 0; i < TP_DIM_SIZE; ++i) {
            acc_vect_t acc = ::aie::zeros<acc_t, m_kvecSampleNum>();
            // Vectorized dot product for the sum
            for (int j = 0; j < i / m_kvecSampleNum; ++j) chess_prepare_for_pipelining {
                vect_t l_vec;
                l_vec = ::aie::load_v<m_kvecSampleNum>(&p_L[i * TP_DIM_SIZE + j * m_kvecSampleNum]);
                vect_t x_vec = ::aie::load_v<m_kvecSampleNum>(&p_x[j * m_kvecSampleNum]);
                acc = ::aie::mac(acc, l_vec, x_vec);
            }
            // Scalar part for remainder
            TT_DATA sum = ::aie::reduce_add(acc.template to_vector<TT_DATA>());
            for (int j = (i / m_kvecSampleNum) * m_kvecSampleNum; j < i; ++j) {
              sum += p_L[i * TP_DIM_SIZE + j] * p_x[j];
            }
            myInv = fnMyInv<TT_DATA>(p_L[i * (TP_DIM_SIZE + 1)]); // reciprocal of diagonal element
            p_x[i] = (p_y[i] - sum) * myInv;
        }
    } else { // backward substitution
        // Backward substitution: U*x = y
        for (int i = TP_DIM_SIZE - 1; i >= 0; --i) {
            acc_vect_t acc = ::aie::zeros<acc_t, m_kvecSampleNum>();
            TT_DATA sum = nullElem<TT_DATA>();
            int j = i + 1;
            // Scalar part for initial unaligned elements
            for (; (j % m_kvecSampleNum != 0) && (j < TP_DIM_SIZE); j++) {
              sum += p_L[i * TP_DIM_SIZE + j] * p_x[j];
            }
            // Vectorized part
            for (; j + m_kvecSampleNum <= TP_DIM_SIZE; j += m_kvecSampleNum) chess_prepare_for_pipelining {
                vect_t l_vec;
                l_vec = ::aie::load_v<m_kvecSampleNum>(&p_L[i * TP_DIM_SIZE + j]);
                vect_t x_vec = ::aie::load_v<m_kvecSampleNum>(&p_x[j]);
                acc = ::aie::mac(acc, l_vec, x_vec);
            }
            sum += ::aie::reduce_add(acc.template to_vector<TT_DATA>());
            // Scalar part for remaining elements
            for (; j < TP_DIM_SIZE; j++) {
              sum += p_L[i * TP_DIM_SIZE + j] * p_x[j];
            }
            myInv = fnMyInv<TT_DATA>(p_L[i * (TP_DIM_SIZE + 1)]); // reciprocal of diagonal element
            p_x[i] = (p_y[i] - sum) * myInv;
        }
    }
}
  */

} // namespace xf::solver::aie::substitution
