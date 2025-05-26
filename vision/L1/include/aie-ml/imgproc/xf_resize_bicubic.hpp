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

#ifndef __XF_RESIZE_BICUBIC_
#define __XF_RESIZE_BICUBIC_

#include <adf.h>
#include <aie_api/aie.hpp>
#include <aie_api/utils.hpp>
#include <common/xf_aie_hw_utils.hpp>
#include <type_traits>

namespace xf {
namespace cv {
namespace aie {
template <typename T, int N>
class Resizebicubic {
    int mnFBitsIn;
    int mnFBitsAlpha;
    int mnFBitsBeta;
    int mnFBitsOut;
    uint32_t (&mwtsY)[LUT_DEPTH];

   public:
    float coefficient = -0.75f;
    Resizebicubic(uint32_t (&wtsy)[LUT_DEPTH]) : mwtsY(wtsy) {}

    /*      __attribute__((noinline))
            void compute_wtsy(int row, int img_height_in, int img_height_out, const uint32_t scale_y, const uint32_t*
       weighty, int8_t &Wy1, int8_t &Wy2, int8_t &Wy3, int8_t &Wy4
                          , int &pos1, int &pos2, int &pos3, int &pos4)
        {

                    int32 position = (row * scale_y) + (scale_y >> 1) - ToFixed<int64_t, 16>(0.5f);
                    position = position > 0 ? position : 0;
            uint16_t wt_16 = position;
                    uint8_t wt_8 = (uint8_t)(wt_16>>8);
            //printf("position=%d wt_16=%d wt_8=%d\n", position, wt_16, wt_8) ;

                    uint32_t wtsy = weighty[wt_8];
                    Wy1 = (int8_t)(wtsy >> 24);
                    Wy2 = (int8_t)(wtsy >> 16);
                    Wy3 = (int8_t)(wtsy >> 8);
                    Wy4 = (int8_t)(wtsy);
            int p=(position >> 16);

             //int p=(position >> 16);
            pos1=(p - 1)<0 ? 0: (p - 1);
            pos2=(p) <0 ? 0: p;
            pos3=(p + 1)<0 ? 0: (p + 1);
            pos4=(p + 2)<0 ? 0: (p + 2);

             int pos11=(p - 1)<0 ? 0: (p - 1);
            int pos22=(p) <0 ? 0: p;
            int pos33=(p + 1)<0 ? 0: (p + 1);
            int pos44=(p + 2)<0 ? 0: (p + 2);

            //pos
           // int position= (p-1 > 0) * (p-1) ;
            pos1 = (pos11 < (img_height_in - 1)) * pos11 + (pos11 >= (img_height_in - 1)) * (img_height_in - 1);
            pos2 = (pos22 < (img_height_in - 1)) * pos22 + (pos22 >= (img_height_in - 1)) * (img_height_in - 1);
            pos3 = (pos33 < (img_height_in - 1)) * pos33 + (pos33 >= (img_height_in - 1)) * (img_height_in - 1);
            pos4 = (pos44 < (img_height_in - 1)) * pos44 + (pos44 >= (img_height_in - 1)) * (img_height_in - 1);

        }
     */
    __attribute__((noinline)) void compute_wtsy_f(int row,
                                                  int img_height_in,
                                                  const uint32_t scale_y_fix,
                                                  float scale_y,
                                                  const uint32_t* weighty,
                                                  int8_t& Wy1,
                                                  int8_t& Wy2,
                                                  int8_t& Wy3,
                                                  int8_t& Wy4,
                                                  int& pos1,
                                                  int& pos2,
                                                  int& pos3,
                                                  int& pos4) {
        ::aie::vector<float, 16> _row = ::aie::broadcast<float, 16>((float)row);
        ::aie::vector<float, 16> add_ = ::aie::add(0.5f, _row);
        ::aie::accum<accfloat, 16> acc(::aie::broadcast<float, 16>(-0.5f), 0);
        acc = ::aie::mac(acc, add_, scale_y);
        // pos
        int32 position = (row * scale_y_fix) + (scale_y_fix >> 1) - ToFixed<int64_t, 16>(0.5f);
        int p = (position >> 16);

        ::aie::vector<float, 16> dist = ::aie::sub(acc.to_vector<float>(0), ::aie::broadcast<float, 16>((float)p));
        ::aie::accum<accfloat, 16> mul_Acc = ::aie::mul(dist, ::aie::broadcast<float, 16>(256.0f));
        ::aie::vector<int32_t, 16> idx = ::aie::to_fixed<int32_t, 16>(mul_Acc.to_vector<bfloat16>(), 0);
        //    ::aie::vector<int32_t, 16> idx=::aie::to_fixed<int32_t,16>(dist, 8);
        uint32_t wtsy = weighty[idx[0]];

        Wy1 = (int8_t)(wtsy >> 24);
        Wy2 = (int8_t)(wtsy >> 16);
        Wy3 = (int8_t)(wtsy >> 8);
        Wy4 = (int8_t)(wtsy);

        int pos11 = (p - 1) < 0 ? 0 : (p - 1);
        int pos22 = (p) < 0 ? 0 : p;
        int pos33 = (p + 1) < 0 ? 0 : (p + 1);
        int pos44 = (p + 2) < 0 ? 0 : (p + 2);

        // pos
        // int position= (p-1 > 0) * (p-1) ;
        pos1 = (pos11 < (img_height_in - 1)) * pos11 + (pos11 >= (img_height_in - 1)) * (img_height_in - 1);
        pos2 = (pos22 < (img_height_in - 1)) * pos22 + (pos22 >= (img_height_in - 1)) * (img_height_in - 1);
        pos3 = (pos33 < (img_height_in - 1)) * pos33 + (pos33 >= (img_height_in - 1)) * (img_height_in - 1);
        pos4 = (pos44 < (img_height_in - 1)) * pos44 + (pos44 >= (img_height_in - 1)) * (img_height_in - 1);
    }

    void xf_resize1DV(T* input,
                      T* output,
                      int channels,
                      int start_in_row,
                      int start_out_row,
                      uint32_t scale_y,
                      int img_height_in,
                      int tile_height_out,
                      int tile_width_out,
                      const uint32_t* weighty,
                      float scale_y_f);

    void runImpl(T* input,
                 T* metaData,
                 T* output,
                 int channels,
                 uint32_t scale_y,
                 int img_height_in,
                 int img_height_out,
                 float scale_y_f);
};

template <typename T, int N>
__attribute__((noinline)) void Resizebicubic<T, N>::xf_resize1DV(T* input,
                                                                 T* output,
                                                                 int channels,
                                                                 int start_in_row,
                                                                 int start_out_row,
                                                                 uint32_t scale_y,
                                                                 int img_height_in,
                                                                 int tile_height_out,
                                                                 int tile_width_out,
                                                                 const uint32_t* weighty,
                                                                 float scale_y_f) {
    const uint32_t* wty = weighty;
    int8_t Wy1, Wy2, Wy3, Wy4;
    int pos1, pos2, pos3, pos4; // y-1
    T* restrict img_out_ptr = (T*)output;
    set_rnd(rnd_conv_even);

    ::aie::vector<T, (N * 2)> data_vec1, data_vec2;
    ::aie::vector<int8_t, 64> Wy1_y2;
    ::aie::vector<int8_t, 64> Wy3_y4;
    ::aie::vector<int16_t, 32> Wy1_y2_16;
    ::aie::vector<int16_t, 32> Wy3_y4_16;
    ::aie::accum<acc32, N> acc1;
    ::aie::accum<acc32, N> acc2;
    ::aie::accum<acc64, N> acc11;
    ::aie::accum<acc64, N> acc22;

    for (int i = 0; i < tile_height_out; i++) {
        //        compute_wtsy(start_out_row + i, img_height_in, img_height_out, scale_y, weighty, Wy1, Wy2, Wy3, Wy4,
        //        pos1, pos2,pos3,pos4);
        compute_wtsy_f(start_out_row + i, img_height_in, scale_y, scale_y_f, weighty, Wy1, Wy2, Wy3, Wy4, pos1, pos2,
                       pos3, pos4);
        if
            constexpr(std::is_same_v<T, uint8>) {
                Wy1_y2 = ::aie::concat(::aie::broadcast<int8_t, 32>(Wy1), ::aie::broadcast<int8_t, 32>(Wy2));
                Wy3_y4 = ::aie::concat(::aie::broadcast<int8_t, 32>(Wy3), ::aie::broadcast<int8_t, 32>(Wy4));
            }
        else {
            Wy1_y2_16 = ::aie::concat(::aie::broadcast<int16_t, 16>(Wy1), ::aie::broadcast<int16_t, 16>(Wy2));
            Wy3_y4_16 = ::aie::concat(::aie::broadcast<int16_t, 16>(Wy3), ::aie::broadcast<int16_t, 16>(Wy4));
        }
        chess_report(Wy1_y2_16);
        chess_report(Wy3_y4_16);
        chess_report(pos1);
        chess_report(pos2);
        chess_report(pos3);
        chess_report(pos4);

        int y_idx1 = (pos1 - start_in_row) * (tile_width_out * channels); // y-1
        int y_idx2 = (pos2 - start_in_row) * (tile_width_out * channels);
        int y_idx3 = (pos3 - start_in_row) * (tile_width_out * channels);
        int y_idx4 = (pos4 - start_in_row) * (tile_width_out * channels);
        T* restrict img_in_ptr1 = (T*)(input + y_idx1); // y-1
        T* restrict img_in_ptr2 = (T*)(input + y_idx2); // y
        T* restrict img_in_ptr3 = (T*)(input + y_idx3); // y+1
        T* restrict img_in_ptr4 = (T*)(input + y_idx4); // y+2

        for (int j = 0; j < ((tile_width_out * channels) / N); j++)
            chess_prepare_for_pipelining chess_loop_range(32, ) {
                data_vec1.insert(0, ::aie::load_v<N>(img_in_ptr1));
                data_vec1.insert(1, ::aie::load_v<N>(img_in_ptr2));
                data_vec2.insert(0, ::aie::load_v<N>(img_in_ptr3));
                data_vec2.insert(1, ::aie::load_v<N>(img_in_ptr4));
                img_in_ptr1 += N;
                img_in_ptr2 += N;
                img_in_ptr3 += N;
                img_in_ptr4 += N;

                if
                    constexpr(std::is_same_v<T, uint8>) {
                        acc1 = mul_elem_32_2(data_vec1, Wy1_y2);
                        acc2 = mac_elem_32_2(data_vec2, Wy3_y4, acc1);
                        set_sat();
                        ::aie::store_v(img_out_ptr, acc2.template to_vector<T>(7));
                    }
                else {
                    acc11 = mul_elem_16_2(data_vec1, Wy1_y2_16);
                    acc22 = mac_elem_16_2(data_vec2, Wy3_y4_16, acc11);
                    set_sat();
                    ::aie::store_v(img_out_ptr, acc22.template to_vector<T>(7));
                }
                img_out_ptr += N;
            }
    }
    set_rnd(rnd_floor);
}

/*__attribute__((noinline)) void Resizebicubic::xf_resize1DV(uint8_t* input,
                                                        uint8_t* output,
                                                        int channels,
                                                        int start_in_row,
                                                        int start_out_row,
                                                        uint32_t scale_y,
                                                        int img_height_in,
                                                        int img_height_out,
                                                        int tile_height_out,
                                                        int tile_width_out,
                                                                                                                const
uint32_t* weighty) {
        const uint32_t* wty = weighty;
        int8_t Wy1,Wy2,Wy3,Wy4;
    int pos1, pos2, pos3, pos4;            //y-1
    ::aie::vector<uint8_t, 64> data_vec1, data_vec2;
    ::aie::accum<acc32, 32> acc1;
    ::aie::accum<acc32, 32> acc2;
    int k=0;
//     printf("start_in_row=%d start_out_row=%d\n", start_in_row, start_out_row);
//     printf("tile_height_out=%d tile_width_out=%d\n", tile_height_out, tile_width_out);
    uint8_t* restrict img_out_ptr = (uint8_t*)output;
    set_rnd(rnd_conv_even);

    for (int j = 0; j < tile_width_out; j +=32){
        uint8_t* restrict img_out_ptr = (uint8_t*)(output + j);
        for(int i=0; i<tile_height_out;i++) chess_prepare_for_pipelining{

            compute_wtsy(start_out_row + i, img_height_in, img_height_out,  scale_y, weighty, Wy1, Wy2, Wy3, Wy4, pos1,
pos2,pos3,pos4);
//            printf("pos1=%d pos2=%d pos3=%d pos4=%d\n", pos1, pos2, pos3, pos4);
            ::aie::vector<int8_t, 64> Wy1_y2 = ::aie::concat(::aie::broadcast<int8_t, 32>(Wy1), ::aie::broadcast<int8_t,
32>(Wy2));
            ::aie::vector<int8_t, 64> Wy3_y4 = ::aie::concat(::aie::broadcast<int8_t, 32>(Wy3), ::aie::broadcast<int8_t,
32>(Wy4));
            int y_idx1 = (pos1 - start_in_row) * (tile_width_out*channels);  //y-1
            int y_idx2 = (pos2 - start_in_row) * (tile_width_out*channels);
            int y_idx3 = (pos3 - start_in_row) * (tile_width_out*channels);
            int y_idx4 = (pos4 - start_in_row) * (tile_width_out*channels);
            uint8_t* restrict img_in_ptr1 = (uint8_t*)(input + y_idx1); //y-1
            uint8_t* restrict img_in_ptr2 = (uint8_t*)(input + y_idx2); //y
            uint8_t* restrict img_in_ptr3 = (uint8_t*)(input + y_idx3); //y+1
            uint8_t* restrict img_in_ptr4 = (uint8_t*)(input + y_idx4); //y+2

            data_vec1.insert(0, ::aie::load_v<32>(img_in_ptr1));
            data_vec1.insert(1, ::aie::load_v<32>(img_in_ptr2));
            data_vec2.insert(0, ::aie::load_v<32>(img_in_ptr3));
            data_vec2.insert(1, ::aie::load_v<32>(img_in_ptr4));

            acc1 = mul_elem_32_2(data_vec1, Wy1_y2);
            acc2 = mac_elem_32_2(data_vec2, Wy3_y4, acc1);
               set_sat();
            // chess_report(y_idx1);
            // chess_report(y_idx2);
            // chess_report(y_idx3);
            // chess_report(y_idx4);
            // chess_report(data_vec1);
            // chess_report(Wy1_y2);
            // chess_report(Wy3_y4);
            // chess_report(data_vec2);
            // chess_report(acc2);
            // chess_report(acc2.template to_vector<uint8_t>(7));

           ::aie::store_v(img_out_ptr, acc2.template to_vector<uint8_t>(7));
           clr_sat();

           img_out_ptr+=32;
        }
    }
    set_rnd(rnd_floor);
}
*/
template <typename T, int N>
__attribute__((noinline)) void Resizebicubic<T, N>::runImpl(T* input,
                                                            T* metaData,
                                                            T* output,
                                                            int channels,
                                                            uint32_t scale_y,
                                                            int img_height_in,
                                                            int img_height_out,
                                                            float scale_y_f) {
    int start_out_row = xfGetTileOutPosV(input);
    int start_in_row = xfGetTilePosV(input);
    const int16_t tile_height_out = xfGetTileOutTHeight(input);
    const int16_t tile_width_out = xfGetTileOutTWidth(input);

    // printf("start_in_row=%d start_out_row=%d\n", start_in_row, start_out_row);
    // printf("tile_height_out=%d tile_width_out=%d\n", tile_height_out, tile_width_out);

    /*     int core_id = get_coreid();

        if(core_id==393219)
        {
        int core_col = (uint16_t) (core_id>>16 & 0xff) ;
        int core_row = (uint16_t) ((core_id & 0xff)-2);
        printf("core_id=%d \n", core_id);
        printf("core_col=%d core_row=%d\n", core_col, core_row);
        printf("start_in_row=%d start_out_row=%d\n", start_in_row, start_out_row);
        printf("tile_height_out=%d tile_width_out=%d\n", tile_height_out, tile_width_out);
        //printf("in_posv=%d in_posh=%d\n", in_posv, in_posh);

        }
        if(core_id==393220){
        int core_col = (uint16_t) (core_id>>16 & 0xff) ;
        int core_row = (uint16_t) ((core_id & 0xff)-2);
        printf("core_id=%d \n", core_id);
        printf("core_col=%d core_row=%d\n", core_col, core_row);
        printf("start_in_row=%d start_out_row=%d\n", start_in_row, start_out_row);
        printf("tile_height_out=%d tile_width_out=%d\n", tile_height_out, tile_width_out);
        //printf("in_posv=%d in_posh=%d\n", in_posv, in_posh);
        } */

    // for(int i=0;i<64;i++)
    // {

    //         *metaData++=*input++;
    // }
    //     for(int i=0;i<32832,;i++)
    // {

    //         *output++=*input++;
    // }

    T* ptr_in = (T*)xfGetImgDataPtr(input);

    xf_resize1DV((T*)ptr_in, (T*)output, channels, start_in_row, start_out_row, scale_y, img_height_in, tile_height_out,
                 tile_width_out, mwtsY, scale_y_f);

    xfCopyMetaData(input, metaData);

    // meta data update for transpose
    uint16_t mt_height = tile_width_out; // xfGetTileWidth(out_ptr);
    uint16_t mt_width = tile_height_out; // xfGetTileHeight(out_ptr);

    xfSetTileWidth(metaData, mt_width);
    xfSetTileHeight(metaData, mt_height);
    xfSetTileOutTWidth(metaData, mt_width);
    xfSetTileOutTHeight(metaData, mt_height);
    //    printf("xfGetTileOutPosV(out_ptr)=%d  xfGetTileOutPosH(out_ptr)=%d\n", xfGetTileOutPosV(out_ptr),
    //    xfGetTileOutPosH(out_ptr));
    uint16_t in_posv = xfGetTileOutPosV(metaData);
    uint16_t in_posh = xfGetTileOutPosH(metaData);
    xfSetTileOutPosH(metaData, in_posv);
    xfSetTileOutPosV(metaData, in_posh);
    //    printf("xfGetTileOutPosV(metaData)=%d  xfGetTileOutPosH(metaData)=%d\n", xfGetTileOutPosV(metaData),
    //    xfGetTileOutPosH(metaData));
    int outOffset = (xfGetTileOutPosV(metaData) * img_height_out) + xfGetTileOutPosH(metaData);
    xfSetTileOutOffset_L(metaData, (uint16_t)(outOffset & 0x0000ffff));
    xfSetTileOutOffset_U(metaData, (uint16_t)(outOffset >> 16));
}

} // aie
} // cv
} // xf

#endif