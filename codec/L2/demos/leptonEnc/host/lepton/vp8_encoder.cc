/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#include "../../vp8/util/memory.hh"
#include <string>
#include <cassert>
#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <fcntl.h>
#endif
#include "bitops.hh"
#include "component_info.hh"
#include "uncompressed_components.hh"
#include "jpgcoder.hh"
#include "vp8_encoder.hh"

#include "bool_encoder.hh"
#include "model.hh"
#include "numeric.hh"

#include "../vp8/model/model.hh"
#include "../vp8/encoder/encoder.hh"
#include "../io/MuxReader.hh"
#include "loop_stt.h"

#include <ap_int.h>
#include <hls_stream.h>

//#include "../lepton/XModified.h"
extern LoopNodeFactory g_loops;
// void process_row_range3(
//		WD_AXI*         axi_coeff,
//		LeptonInput&    lepp,
//      struct_arith&   arith_enc,
//		uint8_t*        res
//       );
using namespace std;
typedef Sirikata::MuxReader::ResizableByteBuffer ResizableByteBuffer;
void printContext(FILE* fp) {
#ifdef ANNOTATION_ENABLED
    for (int cm = 0; cm < 3; ++cm) {
        for (int y = 0; y < Context::H / 8; ++y) {
            for (int x = 0; x < Context::W / 8; ++x) {
                for (int by = 0; by < 8; ++by) {
                    for (int bx = 0; bx < 8; ++bx) {
                        for (int ctx = 0; ctx < NUMCONTEXT; ++ctx) {
                            for (int dim = 0; dim < 3; ++dim) {
                                int val = 0;
                                val = gctx->p[cm][y][x][by][bx][ctx][dim];
                                const char* nam = "UNKNOWN";
                                switch (ctx) {
                                    case ZDSTSCAN:
                                        nam = "ZDSTSCAN";
                                        break;
                                    case ZEROS7x7:
                                        nam = "ZEROS7x7";
                                        break;
                                    case EXPDC:
                                        nam = "EXPDC";
                                        break;
                                    case RESDC:
                                        nam = "RESDC";
                                        break;
                                    case SIGNDC:
                                        nam = "SIGNDC";
                                        break;
                                    case EXP7x7:
                                        nam = "EXP7x7";
                                        break;
                                    case RES7x7:
                                        nam = "RES7x7";
                                        break;
                                    case SIGN7x7:
                                        nam = "SIGN7x7";
                                        break;
                                    case ZEROS1x8:
                                        nam = "ZEROS1x8";
                                        break;
                                    case ZEROS8x1:
                                        nam = "ZEROS8x1";
                                        break;
                                    case EXP8:
                                        nam = "EXP8";
                                        break;
                                    case THRESH8:
                                        nam = "THRESH8";
                                        break;
                                    case RES8:
                                        nam = "RES8";
                                        break;
                                    case SIGN8:
                                        nam = "SI#include " emmintrin.h "GN8";
                                        break;
                                    default:
                                        break;
                                }
                                if (val != -1 && ctx != ZDSTSCAN) {
                                    fprintf(fp, "col[%02d] y[%02d]x[%02d] by[%02d]x[%02d] [%s][%d] = %d\n", cm, y, x,
                                            by, bx, nam, dim, val);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
#endif
}

VP8ComponentEncoder::VP8ComponentEncoder(bool do_threading) : LeptonCodec(do_threading) {}

CodingReturnValue VP8ComponentEncoder::encode_chunk(const UncompressedComponents* input,
                                                    IOUtil::FileWriter* output,
                                                    const ThreadHandoff* selected_splits,
                                                    unsigned int num_selected_splits) {
    /// clock_t begin = 0, end = 1;
    // begin = clock();
    g_loops.START("PROC:encode_chunk", PROC);
    g_loops.CNT();
    return vp8_full_encoder(input, output, selected_splits, num_selected_splits);
    g_loops.END();
    // end = clock();
    // printf("%d, %d, %d \n", begin, end, end-begin);
}

template <class Left, class Middle, class Right>
void VP8ComponentEncoder::process_row(
    ProbabilityTablesBase& pt,
    Left& left_model,
    Middle& middle_model,
    Right& right_model,
    int curr_y,
    const UncompressedComponents* const colldata,
    Sirikata::Array1d<ConstBlockContext, (uint32_t)ColorChannel::NumBlockTypes>& context,
    BoolEncoder& bool_encoder) {
    uint32_t block_width = colldata->full_component_nosync((int)middle_model.COLOR).block_width();

    if (block_width > 0) {
        g_loops.START("BRCH:process_row, bw>0", BRCH);
        g_loops.CNT();
        ConstBlockContext state = context.at((int)middle_model.COLOR);
        const AlignedBlock& block = state.here();
#ifdef ANNOTATION_ENABLED
        gctx->cur_cmp = component; // for debug purposes only, not to be used in production
        gctx->cur_jpeg_x = 0;
        gctx->cur_jpeg_y = curr_y;
#endif
        state.num_nonzeros_here->set_num_nonzeros(block.recalculate_coded_length());
        serialize_tokens(state, bool_encoder, left_model, pt);
        uint32_t offset = colldata->full_component_nosync((int)middle_model.COLOR).next(state, true, curr_y);
        context.at((int)middle_model.COLOR) = state;
        if (offset >= colldata->component_size_in_blocks(middle_model.COLOR)) {
            return;
        }
    }
    g_loops.END();
    g_loops.START("LOOP:process_row, jpeg_x:1 to bw-2", FOR);
    for (unsigned int jpeg_x = 1; jpeg_x + 1 < block_width; jpeg_x++) {
        g_loops.CNT();
        ConstBlockContext state = context.at((int)middle_model.COLOR);
        const AlignedBlock& block = state.here();
#ifdef ANNOTATION_ENABLED
        gctx->cur_cmp = component; // for debug purposes only, not to be used in production
        gctx->cur_jpeg_x = jpeg_x;
        gctx->cur_jpeg_y = curr_y;
#endif
        state.num_nonzeros_here->set_num_nonzeros(block.recalculate_coded_length()); // FIXME set edge pixels too
        serialize_tokens(state, bool_encoder, middle_model, pt);
        uint32_t offset = colldata->full_component_nosync((int)middle_model.COLOR).next(state, true, curr_y);
        context.at((int)middle_model.COLOR) = state;
        if (offset >= colldata->component_size_in_blocks(middle_model.COLOR)) {
            return;
        }
    }
    g_loops.END();
    g_loops.START("BRCH:process_row, jpeg_x==bw-1", BRCH);
    if (block_width > 1) {
        g_loops.CNT();
        ConstBlockContext state = context.at((int)middle_model.COLOR);
        const AlignedBlock& block = state.here();
#ifdef ANNOTATION_ENABLED
        gctx->cur_cmp = middle_model.COLOR; // for debug purposes only, not to be used in production
        gctx->cur_jpeg_x = block_width - 1;
        gctx->cur_jpeg_y = curr_y;
#endif
        state.num_nonzeros_here->set_num_nonzeros(block.recalculate_coded_length());
        serialize_tokens(state, bool_encoder, right_model, pt);
        colldata->full_component_nosync((int)middle_model.COLOR).next(state, false, curr_y);
        context.at((int)middle_model.COLOR) = state;
    }
    g_loops.END();
}
uint32_t aligned_block_cost(const AlignedBlock& block) {
#ifdef __SSE2__ /* SSE2 or higher instruction set available { */
    const __m128i zero = _mm_setzero_si128();
    __m128i v_cost;
    for (int i = 0; i < 64; i += 8) {
        __m128i val = _mm_abs_epi16(_mm_load_si128((const __m128i*)(const char*)(block.raw_data() + i)));
        v_cost = _mm_set1_epi16(0);
#ifndef __SSE4_1__
        while (_mm_movemask_epi8(_mm_cmpeq_epi32(val, zero)) != 0xFFFF)
#else
        while (!_mm_test_all_zeros(val, val))
#endif
        {
            __m128i mask = _mm_cmpgt_epi16(val, zero);
            v_cost = _mm_add_epi16(v_cost, _mm_and_si128(mask, _mm_set1_epi16(2)));
            val = _mm_srli_epi16(val, 1);
        }
        v_cost = _mm_add_epi16(v_cost, _mm_srli_si128(v_cost, 8));
        v_cost = _mm_add_epi16(v_cost, _mm_srli_si128(v_cost, 4));
        v_cost = _mm_add_epi16(v_cost, _mm_srli_si128(v_cost, 2));
    }
    return 16 + _mm_extract_epi16(v_cost, 0);
#else  /* } No SSE2 instructions { */
    uint32_t scost = 0;
    for (int i = 0; i < 64; ++i) {
        scost += 1 + 2 * uint16bit_length(abs(block.raw_data()[i]));
    }
    return scost;
#endif /* } */
}

#ifdef ALLOW_FOUR_COLORS
#define ProbabilityTablesTuple(left, above, right)                      \
    ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR0>,     \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR1>, \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR2>, \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR3>
#define EACH_BLOCK_TYPE(left, above, right)                                                                \
    ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR0>(BlockType::Y, left, above, right),      \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR1>(BlockType::Cb, left, above, right), \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR2>(BlockType::Cr, left, above, right), \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR3>(BlockType::Ck, left, above, right)
#else
#define ProbabilityTablesTuple(left, above, right)                      \
    ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR0>,     \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR1>, \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR2>
#define EACH_BLOCK_TYPE(left, above, right)                                                                \
    ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR0>(BlockType::Y, left, above, right),      \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR1>(BlockType::Cb, left, above, right), \
        ProbabilityTables<left && above && right, TEMPLATE_ARG_COLOR2>(BlockType::Cr, left, above, right)
#endif

tuple<ProbabilityTablesTuple(false, false, false)> corner(EACH_BLOCK_TYPE(false, false, false));
tuple<ProbabilityTablesTuple(true, false, false)> top(EACH_BLOCK_TYPE(true, false, false));
tuple<ProbabilityTablesTuple(false, true, true)> midleft(EACH_BLOCK_TYPE(false, true, true));
tuple<ProbabilityTablesTuple(true, true, true)> middle(EACH_BLOCK_TYPE(true, true, true));
tuple<ProbabilityTablesTuple(true, true, false)> midright(EACH_BLOCK_TYPE(true, true, false));
tuple<ProbabilityTablesTuple(false, true, false)> width_one(EACH_BLOCK_TYPE(false, true, false));

enum IDX_PTB { COR = 0, TOP, MLF, MMD, MRT, ONE }; // Xilinx
/*
void VP8ComponentEncoder::process_row_range(unsigned int thread_id,
                                            const UncompressedComponents * const colldata,
                                            int min_y,
                                            int max_y,
                                            ResizableByteBuffer *stream,
                                            BoolEncoder *bool_encoder,
                                            Sirikata::Array1d<std::vector<NeighborSummary>,
                                                              (uint32_t)ColorChannel::NumBlockTypes
                                                              > *num_nonzeros) {

    TimingHarness::timing[thread_id][TimingHarness::TS_ARITH_STARTED] = TimingHarness::get_time_us();
    using namespace Sirikata;
    Array1d<ConstBlockContext, (uint32_t)ColorChannel::NumBlockTypes> context;
    for (size_t i = 0; i < context.size(); ++i) {
        context[i] = colldata->full_component_nosync(i).begin(num_nonzeros->at(i).begin());
    }
    uint8_t is_top_row[(uint32_t)ColorChannel::NumBlockTypes];
    memset(is_top_row, true, sizeof(is_top_row));
    ProbabilityTablesBase *model = nullptr;
    if (do_threading_) {
        reset_thread_model_state(thread_id);
        model = &thread_state_[thread_id]->model_;
    } else {
        reset_thread_model_state(0);
        model = &thread_state_[0]->model_;
    }
    KBlockBasedImagePerChannel<false> image_data;
    for (int i = 0; i < colldata->get_num_components(); ++i) {
        image_data[i] = &colldata->full_component_nosync((int)i);
    }
    uint32_t encode_index = 0;
    Array1d<uint32_t, (uint32_t)ColorChannel::NumBlockTypes> max_coded_heights = colldata->get_max_coded_heights();
    g_loops.START("LOOP:process_row_range, while", WHILE);
    while(true) {
        RowSpec cur_row = row_spec_from_index(encode_index++,
                                              image_data,
                                              colldata->get_mcu_count_vertical(),
                                              max_coded_heights);
        if(cur_row.done) {
            break;
        }
        if (cur_row.luma_y >= max_y && thread_id + 1 != NUM_THREADS) {
            break;
        }
        if (cur_row.skip) {
            continue;
        }
        if (cur_row.luma_y < min_y) {
            continue;
        }
        g_loops.CNT();
        context[cur_row.component]
            = image_data.at(cur_row.component)->off_y(cur_row.curr_y,
                                                      num_nonzeros->at(cur_row.component).begin());
        // DEBUG only fprintf(stderr, "Thread %d min_y %d - max_y %d cmp[%d] y = %d\n", thread_id, min_y, max_y,
(int)component, curr_y);
        int block_width = image_data.at(cur_row.component)->block_width();
        if (is_top_row[cur_row.component]) {
                g_loops.START("Brch:process_row_range,while, top_row", BRCH);g_loops.CNT();//g_loops.END();
            is_top_row[cur_row.component] = false;
            switch((BlockType)cur_row.component) {
                case BlockType::Y:
//                	g_loops.START("BRCH:process_row_range, while, top_row, Y",BRCH);g_loops.CNT();
                        STTBRCH("BRCH:process_row_range, while, top_row, Y");STTCNT;
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Y,
                                                        block_width,
                                                        true,
                                                        false,
                           // std::get<(int)BlockType::Y>(corner),
                          //  std::get<(int)BlockType::Y>(top),
                           // std::get<(int)BlockType::Y>(top),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);g_loops.END();
                    break;
                case BlockType::Cb:
                        g_loops.START("BRCH:process_row_range, while, top_row, Cb",BRCH); g_loops.CNT();
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Cb,
                                                        block_width,
                                                        true,
                                                        false,
                          //  std::get<(int)BlockType::Cb>(corner),
                           // std::get<(int)BlockType::Cb>(top),
                           // std::get<(int)BlockType::Cb>(top),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);g_loops.END();
                    break;
                case BlockType::Cr:
                        g_loops.START("BRCH:process_row_range, while, top_row, Cr",BRCH);g_loops.CNT();
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Cr,
                                                        block_width,
                                                        true,
                                                        false,
                         //   std::get<(int)BlockType::Cr>(corner),
                         //   std::get<(int)BlockType::Cr>(top),
                         //   std::get<(int)BlockType::Cr>(top),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);g_loops.END();
                    break;
#ifdef ALLOW_FOUR_COLORS
                case BlockType::Ck:
                    process_row(*model,
                            std::get<(int)BlockType::Ck>(corner),
                            std::get<(int)BlockType::Ck>(top),
                            std::get<(int)BlockType::Ck>(top),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    break;
#endif
                    }
             g_loops.END();//g_loops.START("process_row_top");g_loops.CNT();//
        } else if (block_width > 1) {
                g_loops.START("BRCH:process_row_range, while, !top&bw>1",BRCH);g_loops.CNT();//g_loops.END();//
            switch((BlockType)cur_row.component) {
                case BlockType::Y:
                        g_loops.START("BRCH:process_row_range, while, !top&bw>1,
Y",BRCH);g_loops.CNT();//g_loops.END();//
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Y,
                                                        block_width,
                                                        false,
                                                        false,
                          //  std::get<(int)BlockType::Y>(midleft),
                         //   std::get<(int)BlockType::Y>(middle),
                         //   std::get<(int)BlockType::Y>(midright),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    g_loops.END();
                    break;
                case BlockType::Cb:
                        g_loops.START("BRCH:process_row_range, while, !top&bw>1,
Cb",BRCH);g_loops.CNT();//g_loops.END();//
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Cb,
                                                        block_width,
                                                        false,
                                                        false,
                          //  std::get<(int)BlockType::Cb>(midleft),
                         //   std::get<(int)BlockType::Cb>(middle),
                          //  std::get<(int)BlockType::Cb>(midright),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    g_loops.END();
                    break;
                case BlockType::Cr:
                        g_loops.START("BRCH:process_row_range,while,!top&bw>1,Cr",BRCH);g_loops.CNT();//g_loops.END();//
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Cr,
                                                        block_width,
                                                        false,
                                                        false,
                         //   std::get<(int)BlockType::Cr>(midleft),
                          //  std::get<(int)BlockType::Cr>(middle),
                           // std::get<(int)BlockType::Cr>(midright),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    g_loops.END();
                    break;
#ifdef ALLOW_FOUR_COLORS
                case BlockType::Ck:
                    process_row(*model,
                            std::get<(int)BlockType::Ck>(midleft),
                            std::get<(int)BlockType::Ck>(middle),
                            std::get<(int)BlockType::Ck>(midright),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    break;
#endif
            }
             g_loops.END();//g_loops.START("process_row_top_!top&bw>1");g_loops.CNT();//
        } else {
                g_loops.START("BRCH:process_row_range, while, !top&bw==1", BRCH);g_loops.CNT();//g_loops.END();//
            always_assert(block_width == 1);
            switch((BlockType)cur_row.component) {
                case BlockType::Y:
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Y,
                                                        block_width,
                                                        false,
                                                        true,//Only One
                          //  std::get<(int)BlockType::Y>(width_one),
                          //  std::get<(int)BlockType::Y>(width_one),
                          //  std::get<(int)BlockType::Y>(width_one),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    break;
                case BlockType::Cb:
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Cb,
                                                        block_width,
                                                        false,
                                                        true,//Only One
                         //   std::get<(int)BlockType::Cb>(width_one),
                         //   std::get<(int)BlockType::Cb>(width_one),
                         //   std::get<(int)BlockType::Cb>(width_one),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                break;
                case BlockType::Cr:
                    process_row2(*model,
                                (BlockType)cur_row.component,//BlockType::Cr,
                                                        block_width,
                                                        false,
                                                        true,//Only One
                         //   std::get<(int)BlockType::Cr>(width_one),
                         //   std::get<(int)BlockType::Cr>(width_one),
                         //   std::get<(int)BlockType::Cr>(width_one),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    break;
#ifdef ALLOW_FOUR_COLORS
                case BlockType::Ck:
                    process_row(*model,
                            std::get<(int)BlockType::Ck>(width_one),
                            std::get<(int)BlockType::Ck>(width_one),
                            std::get<(int)BlockType::Ck>(width_one),
                            cur_row.curr_y,
                            colldata,
                            context,
                            *bool_encoder);
                    break;
#endif
            }
             g_loops.END();//g_loops.START("process_row_top_width_one");g_loops.CNT();//
        }
    }g_loops.END();//("process_row_range_while");

    RowSpec test = row_spec_from_index(encode_index,
                                       image_data,
                                       colldata->get_mcu_count_vertical(),
                                       max_coded_heights);

    if (thread_id == NUM_THREADS - 1 && (test.skip == false || test.done == false)) {
        fprintf(stderr, "Row spec test: cmp %d luma %d item %d skip %d done %d\n",
                test.component, test.luma_y, test.curr_y, test.skip, test.done);
        custom_exit(ExitCode::ASSERTION_FAILURE);
    }
    bool_encoder->finish(*stream);
    TimingHarness::timing[thread_id][TimingHarness::TS_ARITH_FINISHED] = TimingHarness::get_time_us();
}*/

int load_model_file_fd_output() {
    const char* out_model_name = getenv("LEPTON_COMPRESSION_MODEL_OUT");
    if (!out_model_name) {
        return -1;
    }
    return open(out_model_name, O_CREAT | O_TRUNC | O_WRONLY, 0
#ifndef _WIN32
                                                                  | S_IWUSR | S_IRUSR
#endif
                );
}
int model_file_fd = load_model_file_fd_output();
CodingReturnValue VP8ComponentEncoder::vp8_full_encoder(const UncompressedComponents* const colldata,
                                                        IOUtil::FileWriter* str_out,
                                                        const ThreadHandoff* selected_splits,
                                                        unsigned int num_selected_splits) {
    /* cmpc is a global variable with the component count */
    using namespace Sirikata;
    /* get ready to serialize the blocks */

    BoolEncoder bool_encoder_0;
    // bool_encoder_0.output_.resize(MAX_NUM_PIX);
    fprintf(stderr, "Enter a empty process_row_range\n");
    process_row_range2(0, colldata,
                       &(bool_encoder_0.output_), // stream[0],
                       &bool_encoder_0);          // bool_encoder[0]);
    fprintf(stderr, "Done: a empty process_row_range\n");
    static_assert(MAX_NUM_THREADS * SIMD_WIDTH <= MuxReader::MAX_STREAM_ID,
                  "Need to have enough mux streams for all threads and simd width");

    if (do_threading_) {
        for (unsigned int thread_id = 1; thread_id < NUM_THREADS; ++thread_id) {
            TimingHarness::timing[thread_id][TimingHarness::TS_THREAD_WAIT_STARTED] = TimingHarness::get_time_us();
            spin_workers_[thread_id - 1].main_wait_for_done();
            TimingHarness::timing[thread_id][TimingHarness::TS_THREAD_WAIT_FINISHED] = TimingHarness::get_time_us();
        }
    }
    TimingHarness::timing[0][TimingHarness::TS_STREAM_MULTIPLEX_STARTED] = TimingHarness::get_time_us();

    Sirikata::MuxWriter mux_writer(str_out, JpegAllocator<uint8_t>());
    size_t stream_data_offset[MuxReader::MAX_STREAM_ID] = {0};
    bool any_written = true;
    while (any_written) {
        any_written = false;
        // for (int i = 0; i < MuxReader::MAX_STREAM_ID; ++i) {
        for (int i = 0; i < 1; ++i) {
            if (bool_encoder_0.output_.size() > stream_data_offset[i]) {
                any_written = true;
                size_t max_written = 65536;
                if (stream_data_offset[i] == 0) {
                    max_written = 256;
                } else if (stream_data_offset[i] == 256) {
                    max_written = 4096;
                }
                auto to_write = std::min(max_written, bool_encoder_0.output_.size() - stream_data_offset[i]);
                stream_data_offset[i] +=
                    mux_writer.Write(i, &(bool_encoder_0.output_)[stream_data_offset[i]], to_write).first;
            }
        }
    }
    mux_writer.Close();
    write_byte_bill(Billing::DELIMITERS, true, mux_writer.getOverhead());
    // we can probably exit(0) here
    TimingHarness::timing[0][TimingHarness::TS_STREAM_MULTIPLEX_FINISHED] =
        TimingHarness::timing[0][TimingHarness::TS_STREAM_FLUSH_STARTED] = TimingHarness::get_time_us();
    check_decompression_memory_bound_ok(); // this has to happen before last
    // bytes are written
    /* possibly write out new probability model */
    {
        uint32_t out_file_size = str_out->getsize() + 4; // gotta include the final uint32_t
        uint32_t file_size = out_file_size;
        uint8_t out_buffer[sizeof(out_file_size)] = {};
        for (uint8_t i = 0; i < sizeof(out_file_size); ++i) {
            out_buffer[i] = out_file_size & 0xff;
            out_file_size >>= 8;
        }
        str_out->Write(out_buffer, sizeof(out_file_size));
        write_byte_bill(Billing::HEADER, true, sizeof(out_file_size));
        (void)file_size;
        always_assert(str_out->getsize() == file_size);
    }

    if (model_file_fd >= 0) {
        const char* msg = "Writing new compression model...\n";
        while (write(2, msg, strlen(msg)) < 0 && errno == EINTR) {
        }

        std::get<(int)BlockType::Y>(middle).optimize(thread_state_[0]->model_);
        std::get<(int)BlockType::Y>(middle).serialize(thread_state_[0]->model_, model_file_fd);
    }
#ifdef ANNOTATION_ENABLED
    {
        FILE* fp = fopen("/tmp/lepton.ctx", "w");
        printContext(fp);
        fclose(fp);
    }
#endif
    TimingHarness::timing[0][TimingHarness::TS_STREAM_FLUSH_FINISHED] = TimingHarness::get_time_us();
    return CODING_DONE;
}

CodingReturnValue VP8ComponentEncoder::vp8_full_encoder(const UncompressedComponents* const colldata,
                                                        IOUtil::FileWriter* str_out,
                                                        const ThreadHandoff* selected_splits,
                                                        unsigned int num_selected_splits,
                                                        struct_arith& arith,
                                                        uint8_t* res) {
    /* cmpc is a global variable with the component count */
    using namespace Sirikata;
    /* get ready to serialize the blocks */

    BoolEncoder bool_encoder_0;
    // bool_encoder_0.output_.resize(MAX_NUM_PIX);
    fprintf(stderr, "Enter a empty process_row_range\n");
    /*process_row_range2(0,
                      colldata,
                      &(bool_encoder_0.output_),//stream[0],
                      &bool_encoder_0);//bool_encoder[0]);*/
    vpx_writer boolwriter;
    boolwriter.buffer = res;
    boolwriter.lowvalue = arith.value;
    boolwriter.range = arith.range;
    boolwriter.count = arith.count;
    boolwriter.pos = arith.pos;
    boolwriter.run = arith.run;
    boolwriter.isFirst = arith.isFirst;
    vpx_stop_encode(&boolwriter);
    uint32_t pos = boolwriter.pos;
    bool_encoder_0.output_.mSize = pos;
    bool_encoder_0.output_.mReserved = MAX_NUM_PIX;
    bool_encoder_0.output_.mBegin = res;
    fprintf(stderr, "Done: a empty process_row_range\n");

    static_assert(MAX_NUM_THREADS * SIMD_WIDTH <= MuxReader::MAX_STREAM_ID,
                  "Need to have enough mux streams for all threads and simd width");

    if (do_threading_) {
        for (unsigned int thread_id = 1; thread_id < NUM_THREADS; ++thread_id) {
            TimingHarness::timing[thread_id][TimingHarness::TS_THREAD_WAIT_STARTED] = TimingHarness::get_time_us();
            spin_workers_[thread_id - 1].main_wait_for_done();
            TimingHarness::timing[thread_id][TimingHarness::TS_THREAD_WAIT_FINISHED] = TimingHarness::get_time_us();
        }
    }
    TimingHarness::timing[0][TimingHarness::TS_STREAM_MULTIPLEX_STARTED] = TimingHarness::get_time_us();

    Sirikata::MuxWriter mux_writer(str_out, JpegAllocator<uint8_t>());
    size_t stream_data_offset[MuxReader::MAX_STREAM_ID] = {0};
    bool any_written = true;
    while (any_written) {
        any_written = false;
        // for (int i = 0; i < MuxReader::MAX_STREAM_ID; ++i) {
        for (int i = 0; i < 1; ++i) {
            if (bool_encoder_0.output_.size() > stream_data_offset[i]) {
                any_written = true;
                size_t max_written = 65536;
                if (stream_data_offset[i] == 0) {
                    max_written = 256;
                } else if (stream_data_offset[i] == 256) {
                    max_written = 4096;
                }
                auto to_write = std::min(max_written, bool_encoder_0.output_.size() - stream_data_offset[i]);
                stream_data_offset[i] +=
                    mux_writer.Write(i, &(bool_encoder_0.output_)[stream_data_offset[i]], to_write).first;
            }
        }
    }
    mux_writer.Close();
    write_byte_bill(Billing::DELIMITERS, true, mux_writer.getOverhead());
    // we can probably exit(0) here
    TimingHarness::timing[0][TimingHarness::TS_STREAM_MULTIPLEX_FINISHED] =
        TimingHarness::timing[0][TimingHarness::TS_STREAM_FLUSH_STARTED] = TimingHarness::get_time_us();
    check_decompression_memory_bound_ok(); // this has to happen before last
    // bytes are written
    /* possibly write out new probability model */
    {
        uint32_t out_file_size = str_out->getsize() + 4; // gotta include the final uint32_t
        uint32_t file_size = out_file_size;
        uint8_t out_buffer[sizeof(out_file_size)] = {};
        for (uint8_t i = 0; i < sizeof(out_file_size); ++i) {
            out_buffer[i] = out_file_size & 0xff;
            out_file_size >>= 8;
        }
        str_out->Write(out_buffer, sizeof(out_file_size));
        write_byte_bill(Billing::HEADER, true, sizeof(out_file_size));
        (void)file_size;
        always_assert(str_out->getsize() == file_size);
    }

    if (model_file_fd >= 0) {
        const char* msg = "Writing new compression model...\n";
        while (write(2, msg, strlen(msg)) < 0 && errno == EINTR) {
        }

        std::get<(int)BlockType::Y>(middle).optimize(thread_state_[0]->model_);
        std::get<(int)BlockType::Y>(middle).serialize(thread_state_[0]->model_, model_file_fd);
    }
#ifdef ANNOTATION_ENABLED
    {
        FILE* fp = fopen("/tmp/lepton.ctx", "w");
        printContext(fp);
        fclose(fp);
    }
#endif
    TimingHarness::timing[0][TimingHarness::TS_STREAM_FLUSH_FINISHED] = TimingHarness::get_time_us();
    return CODING_DONE;
}

////////////////////////////////////////////////////////////////////////////////////

// template<class Left, class Middle, class Right>
/*void VP8ComponentEncoder:: process_row2(ProbabilityTablesBase &pt,
                                                                                BlockType color,
                                                                                int block_width,
                                                                                bool isTopRow,
                                                                                bool isOnlyOne,
                                      //Left & left_model,
                                      //Middle& middle_model,
                                      //Right& right_model,
                                      int curr_y,
                                      const UncompressedComponents * const colldata,
                                      Sirikata::Array1d<ConstBlockContext,
                                              (uint32_t)ColorChannel::NumBlockTypes> &context,
                                      BoolEncoder &bool_encoder){

    g_loops.START("LOOP:process_row, jpeg_x:=0 to BW", FOR);
    for ( unsigned int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++ ) {
        g_loops.CNT();
        ConstBlockContext state = context.at((int)color);
        const AlignedBlock &block = state.here();
        state.num_nonzeros_here->set_num_nonzeros(block.recalculate_coded_length()); //FIXME set edge pixels too
        bool left        = (jpeg_x==0)             ? false : (block_width>1);
        bool above       = !isTopRow;
        bool above_right = isTopRow                ? false : (jpeg_x < block_width);
        bool has_left    = (jpeg_x+1==block_width) ? false : true;
        serialize_tokens(
                        color,
                                left,
                                above,
                                above_right,
                                state,
                bool_encoder,
                pt);
        uint32_t offset = colldata->full_component_nosync((int)color).next(state, has_left, curr_y);
        context.at((int)color) = state;

    }g_loops.END();

}*/

void next2(ConstBlockContext& it, bool has_left, int width_, int cur_y) {
    it.cur += 1;
    /* if (cur_y==0) {
         it.above = it.cur + width_;

     } else {
         it.above = it.cur - width_;
     }
     ++it.num_nonzeros_here;
     ++it.num_nonzeros_above;
     if (!has_left) {
         bool cur_row_first = (it.num_nonzeros_here < it.num_nonzeros_above);
         if (cur_row_first) {
             it.num_nonzeros_above -= width_;
             it.num_nonzeros_above -= width_;
         } else {
             it.num_nonzeros_here -= width_;
             it.num_nonzeros_here -= width_;
         }
     }*/
}

void pre_scan_tmp(ConstBlockContext context, uint8_t* num_nonzeros_7x7, uint8_t* eob_x, uint8_t* eob_y) {
    *num_nonzeros_7x7 = 0; // context.num_nonzeros_here->num_nonzeros();
    *eob_x = 0;
    *eob_y = 0;
    uint8_t num_nonzeros_left_7x7 = *num_nonzeros_7x7;
    // for (unsigned int zz = 0; zz < 49 && num_nonzeros_left_7x7; ++zz) {
    for (unsigned int zz = 0; zz < 49; ++zz) {
        unsigned int coord = unzigzag49[zz];
        unsigned int b_x = (coord & 7);
        unsigned int b_y = coord >> 3;

        int16_t coef = context.here().coef.at(zz + AlignedBlock::AC_7x7_INDEX);
        if (coef != 0) {
            if (b_x > 0 && b_y > 0) {
                (*num_nonzeros_7x7)++;
            }

            if (b_x > *eob_x) *eob_x = b_x;
            if (b_y > *eob_y) *eob_y = b_y;
        }
    }
}
void pre_scan_tmp(AlignedBlock* context, uint8_t* num_nonzeros_7x7, uint8_t* eob_x, uint8_t* eob_y) {
    *num_nonzeros_7x7 = 0; // context.num_nonzeros_here->num_nonzeros();
    *eob_x = 0;
    *eob_y = 0;
    uint8_t num_nonzeros_left_7x7 = *num_nonzeros_7x7;
    // for (unsigned int zz = 0; zz < 49 && num_nonzeros_left_7x7; ++zz) {
    for (unsigned int zz = 0; zz < 49; ++zz) {
        unsigned int coord = unzigzag49[zz];
        unsigned int b_x = (coord & 7);
        unsigned int b_y = coord >> 3;

        int16_t coef = context->coef.at(zz + AlignedBlock::AC_7x7_INDEX);
        if (coef != 0) {
            if (b_x > 0 && b_y > 0) {
                (*num_nonzeros_7x7)++;
            }

            if (b_x > *eob_x) *eob_x = b_x;
            if (b_y > *eob_y) *eob_y = b_y;
        }
    }
}

void pre_scan_tmp(int16_t* context, uint8_t* num_nonzeros_7x7, uint8_t* eob_x, uint8_t* eob_y) {
    *num_nonzeros_7x7 = 0; // context.num_nonzeros_here->num_nonzeros();
    *eob_x = 0;
    *eob_y = 0;
    uint8_t num_nonzeros_left_7x7 = *num_nonzeros_7x7;
    // for (unsigned int zz = 0; zz < 49 && num_nonzeros_left_7x7; ++zz) {
    for (unsigned int zz = 0; zz < 49; ++zz) {
        unsigned int coord = unzigzag49[zz];
        unsigned int b_x = (coord & 7);
        unsigned int b_y = coord >> 3;

        int16_t coef = context[zz + AlignedBlock::AC_7x7_INDEX];
        if (coef != 0) {
            if (b_x > 0 && b_y > 0) {
                (*num_nonzeros_7x7)++;
            }

            if (b_x > *eob_x) *eob_x = b_x;
            if (b_y > *eob_y) *eob_y = b_y;
        }
    }
}
void pre_scan_tmp_77(ConstBlockContext context, uint8_t* num_nonzeros_7x7, uint8_t* eob_x, uint8_t* eob_y) {
    *num_nonzeros_7x7 = context.num_nonzeros_here->num_nonzeros();
    *eob_x = 0;
    *eob_y = 0;
    uint8_t num_nonzeros_left_7x7 = *num_nonzeros_7x7;
    for (unsigned int zz = 0; zz < 49 && num_nonzeros_left_7x7; ++zz) {
        unsigned int coord = unzigzag49[zz];
        unsigned int b_x = (coord & 7);
        unsigned int b_y = coord >> 3;

        int16_t coef = context.here().coef.at(zz + AlignedBlock::AC_7x7_INDEX);
        if (coef != 0) {
            if (b_x > *eob_x) *eob_x = b_x;
            if (b_y > *eob_y) *eob_y = b_y;
        }
    }
}
void tmp_cp_AlignedBlock(hls_AlignedBlock& des, const AlignedBlock& src) {
    for (int i = 0; i < 64; i++) {
        des.coef[i] = src.coef.at(i);
    }
}
void tmp_cp_AlignedBlock(int16_t des[64], AlignedBlock* src) {
    for (int i = 0; i < 64; i++) {
        des[i] = src->coef.at(i);
    }
}
void tmp_cp_AlignedBlock(int16_t des[64], const AlignedBlock& src) {
    for (int i = 0; i < 64; i++) {
        des[i] = src.coef.at(i);
    }
}
void tmp_cp_Block(int16_t des[64], int16_t src[64]) {
    for (int i = 0; i < 64; i++) {
        des[i] = src[i];
    }
}

void tmp_cp_AlignedBlock(hls_AlignedBlock& des, hls_AlignedBlock& src) {
    for (int i = 0; i < 64; i++) {
        des.coef[i] = src.coef[i];
    }
}

#if 0
//hls::stream<tmp_struct> str_77;
//hls::stream<tmp_struct> str_edges;
//hls::stream<tmp_struct> str_dc;
void tmp_cp_AXI(int16_t des[64], WD_AXI* src)
{
	int16_t* pc = (int16_t*)src;
	for(int i=0; i<64; i++){
		des[i] = pc[i];
		//des[i] = src[i/NUM_COEF_AXI].data[i%NUM_COEF_AXI];
	}
}
void Sim_DDr_InitImage(WD_AXI* axi_coeff, const UncompressedComponents * const colldata,
		//Sirikata::Array1d<std::vector<NeighborSummary>, (uint32_t)ColorChannel::NumBlockTypes> num_nonzeros[1],
		uint16_t        axi_width               [MAX_NUM_COLOR],//colldata->block_width(i);
		uint16_t        axi_height              [MAX_NUM_COLOR],//colldata->block_width(i);
		uint8_t         axi_map_row2cmp         [4], //     AXI                   2,1,0,0 2,1,0
		uint16_t        axi_mcuv,
		uint8_t         axi_num_cmp_mcu
		)
{
    int16_t coef_here[64];

    AlignedBlock* state3[MAX_NUM_COLOR] ={
    		colldata->full_component_nosync(0).image_,
			colldata->full_component_nosync(1).image_,
			colldata->full_component_nosync(2).image_
    };
    WD_AXI* p_axi = axi_coeff;
    for( int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++){
    	for(int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu ; idx_cmp++){
            uint8_t id_cmp        = axi_map_row2cmp[idx_cmp];
            uint16_t block_width  = axi_width [id_cmp];
            uint16_t block_height = axi_height[id_cmp];
            for ( int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++ ) {
            	int16_t* pc = (int16_t*)p_axi;
                tmp_cp_AlignedBlock(pc, state3[id_cmp]);
                p_axi+= (64/NUM_COEF_AXI);
                state3[id_cmp]++;
            }
    	}
    }
}



void kernel_run(
    //input
    WD_AXI          axi_coeff               [MAX_COEF_AXI],
    uint16_t        axi_width               [MAX_NUM_COLOR],//colldata->block_width(i);
    uint16_t        axi_height              [MAX_NUM_COLOR],//colldata->block_width(i);
    uint8_t         axi_map_row2cmp         [4], //     AXI                   2,1,0,0 2,1,0
    uint8_t         min_nois_thld_x         [MAX_NUM_COLOR][64],
    uint8_t         min_nois_thld_y         [MAX_NUM_COLOR][64],
    uint8_t         q_tables                [MAX_NUM_COLOR][8][8],//[64],
    int32_t         idct_q_table_x          [MAX_NUM_COLOR][8][8],
    int32_t         idct_q_table_y          [MAX_NUM_COLOR][8][8],
    int32_t         idct_q_table_l          [MAX_NUM_COLOR][8][8],

    uint16_t        axi_mcuv,
    uint8_t         axi_num_cmp_mcu,
    uint8_t         axi_num_cmp,
    //tmp output
    uint8_t         axi_res                 [MAX_NUM_PIX],
    struct_arith    &axi_arith
){

    bool is_top_row[MAX_NUM_COLOR]= {true, true, true};
    uint16_t cur_y_cmp[MAX_NUM_COLOR] = {0,0,0};

    uint8_t array_num_nonzeros_7x7_above[MAX_NUM_COLOR][MAX_NUM_BLOCK88_W];
    int16_t array_coef_above_77         [MAX_NUM_COLOR][MAX_NUM_BLOCK88_W][64];
    int16_t array_coef_above_edges      [MAX_NUM_COLOR][MAX_NUM_BLOCK88_W][64];
    uint16_t array_edge_above           [MAX_NUM_COLOR][MAX_NUM_BLOCK88_W][8];//2 uram used;

    WD_AXI*  pcoef = axi_coeff;

    hls::stream<bool>    strm_bit;
    hls::stream<uint8_t> strm_prob;
    hls::stream<bool>    strm_e;
    hls::stream<uint8_t> strm_tab_dbg;
    unsigned char    range = 128;//boolwriter.range;
    int              count = -24;//boolwriter.count;
    unsigned int     value = 0;//boolwriter.lowvalue;
    unsigned char pre_byte = 0;//boolwriter.pre_byte;
    unsigned short     run = 0;//boolwriter.run;
    bool           isFirst = 1;//boolwriter.isFirst;
    unsigned int       pos = 0;//boolwriter.pos;
    unsigned int      pos2 = 0;
    
    for( int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++){
        if(i_mcuv==axi_mcuv-1)
                    i_mcuv=axi_mcuv-1;
        for(int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu ; idx_cmp++){
            uint8_t id_cmp        = axi_map_row2cmp[idx_cmp];
            uint16_t block_width  = axi_width [id_cmp];
            uint16_t block_height = axi_height[id_cmp];
            uint16_t cur_y        = cur_y_cmp [id_cmp];
            bool is_top_row_cmp   = is_top_row[id_cmp];

            hls::stream<coeff_64> str_tmp_coeff_here_edges;
            hls::stream<coeff_64> str_tmp_coeff_here_dc;

            ///////////////////////////////////////////////////////////////////////////
            // PRE   //////////////////////////////////////////////////////////////////
    		hls::stream<ap_int<11> >  coef[8];

            hls::stream<ap_int<11> > strm_coef[8];
            hls::stream<ap_int<11> > strm_coef_7x7("coef_7x7");
            hls::stream<ap_int<11> > strm_coef_lft("coef_lft");
            hls::stream<ap_int<11> > strm_coef_abv("coef_abv");
            hls::stream<ap_int<11> > strm_coef_abv_lft("coef_abv_lft");
            hls::stream<ap_int<11> > strm_coef_h[8];
#pragma HLS stream depth = 64 variable = strm_coef_h
            hls::stream<ap_int<11> > strm_coef_above_h[8];
#pragma HLS stream depth = 64 variable = strm_coef_above_h
            hls::stream<bool>        strm_has_left_h;
#pragma HLS stream depth = 64 variable = strm_has_left_h
            hls::stream<bool>        strm_coef_e_h;
#pragma HLS stream depth = 64 variable = strm_coef_e_h

            hls::stream<ap_int<11> > strm_coef_v[8];
#pragma HLS stream depth = 64 variable = strm_coef_v
            hls::stream<ap_int<11> > strm_coef_left_v[8];
#pragma HLS stream depth = 64 variable = strm_coef_left_v
            hls::stream<bool>        strm_has_left_v;
#pragma HLS stream depth = 64 variable = strm_has_left_v
            hls::stream<bool>        strm_coef_e_v;
#pragma HLS stream depth = 64 variable = strm_coef_e_v
            hls::stream<ap_uint<6> > strm_non_zero_cnt("non_zero_cnt");
            hls::stream<ap_uint<6> > strm_non_zero_cnt_lft("non_zero_cnt_lft");
            hls::stream<ap_uint<6> > strm_non_zero_cnt_abv("non_zero_cnt_abv");
            hls::stream<ap_uint<6> > strm_non_zero_7x7("non_zero_7x7");
            hls::stream<ap_uint<6> > strm_non_zero_h("non_zero_h");
            hls::stream<ap_uint<6> > strm_non_zero_v("non_zero_v");
//                hls::stream<ap_uint<3> > strm_coef_cnt_h("coef_cnt_h");
            hls::stream<ap_uint<3> > strm_coef_cnt_exp_h;
            hls::stream<ap_uint<3> > strm_coef_cnt_sign_h;
            hls::stream<ap_uint<3> > strm_coef_cnt_nois_h;
//                hls::stream<ap_uint<3> > strm_coef_cnt_v("coef_cnt_v");
            hls::stream<ap_uint<3> > strm_coef_cnt_exp_v;
            hls::stream<ap_uint<3> > strm_coef_cnt_sign_v;
            hls::stream<ap_uint<3> > strm_coef_cnt_nois_v;
            hls::stream<ap_uint<3> > strm_coef_cnt_h_len("coef_cnt_h_len");
            hls::stream<ap_uint<3> > strm_coef_cnt_v_len("coef_cnt_v_len");
            hls::stream<ap_uint<3> > strm_lane_h;
            hls::stream<ap_uint<3> > strm_lane_v;

            hls::stream<ap_uint<3> > strm_eob_x("eob_x");
            hls::stream<ap_uint<3> > strm_eob_y("eob_y");

            // For DC
            hls::stream< coef_t> str_rast8[8];
            hls::stream< coef_t> str_dc1;
            hls::stream< coef_t> str_dc2;

            //FROM DDR
            //===================================================================
            for ( int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++ ) {
                // Read from DDR
                int16_t coef_here[64];
                tmp_cp_AXI(coef_here, pcoef);
                pcoef += STRIP_COEFF_AXI;

                // For 77
                for(int i=0;i<8;i++){
                    for(int j=0;j<8;j++){
                    	coef[j].write(coef_here[8*i+j]);
                //        std::cout<<coef_here[8*i+j]<<" ";
                    }
                //    std::cout<<std::endl;
                }
                //std::cout<<std::endl;

            }
            //==================================================================

            ap_uint<32> cnt=0;
            ap_int<11> coef_reg[8];
#pragma HLS array_partition variable = coef_reg complete dim = 0

            while(cnt<block_width*8){
#pragma HLS pipeline II = 1

            	coef_reg[0]=coef[0].read();
            	coef_reg[1]=coef[1].read();
            	coef_reg[2]=coef[2].read();
            	coef_reg[3]=coef[3].read();
            	coef_reg[4]=coef[4].read();
            	coef_reg[5]=coef[5].read();
            	coef_reg[6]=coef[6].read();
            	coef_reg[7]=coef[7].read();

            	strm_coef[0].write(coef_reg[0]);
            	strm_coef[1].write(coef_reg[1]);
            	strm_coef[2].write(coef_reg[2]);
            	strm_coef[3].write(coef_reg[3]);
            	strm_coef[4].write(coef_reg[4]);
            	strm_coef[5].write(coef_reg[5]);
            	strm_coef[6].write(coef_reg[6]);
            	strm_coef[7].write(coef_reg[7]);

            	str_rast8[0].write(coef_reg[0]);
            	str_rast8[1].write(coef_reg[1]);
            	str_rast8[2].write(coef_reg[2]);
            	str_rast8[3].write(coef_reg[3]);
            	str_rast8[4].write(coef_reg[4]);
            	str_rast8[5].write(coef_reg[5]);
            	str_rast8[6].write(coef_reg[6]);
            	str_rast8[7].write(coef_reg[7]);

            	if(cnt(2,0)==0){
            		str_dc1.write(coef_reg[0]);
            	}
        		cnt++;
            }

            preprocess(
                block_width,
                id_cmp,
                is_top_row_cmp,
                strm_coef,
                strm_coef_7x7,
                strm_coef_lft,
                strm_coef_abv,
                strm_coef_abv_lft,

                strm_coef_h,
                strm_coef_above_h,
                strm_has_left_h,
                strm_coef_e_h,
                strm_coef_v,
                strm_coef_left_v,
                strm_has_left_v,
                strm_coef_e_v,
                strm_non_zero_cnt,
                strm_non_zero_cnt_lft,
                strm_non_zero_cnt_abv,
                strm_non_zero_7x7,
                strm_non_zero_h,
                strm_coef_cnt_h_len,
                strm_lane_h,
                strm_coef_cnt_v_len,
                strm_lane_v,
                strm_eob_x,
                strm_eob_y
			);
            /// PRE ////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////////


            //////////////////////////////////////////////////////////////////////
            //77//////////////////////////////////////////////////////////////////

        	hls::stream<ap_uint<4>  > strm_sel_tab_77;
        	hls::stream<bool>		  strm_cur_bit_77;
        	hls::stream<bool>		  strm_e_77;
        	hls::stream<ap_uint<16> > strm_addr1_77;
        	hls::stream<ap_uint<16> > strm_addr2_77;
        	hls::stream<ap_uint<16> > strm_addr3_77;
        	hls::stream<ap_uint<16> > strm_addr4_77;

            hls_serialize_tokens_77(
                block_width,
                !is_top_row_cmp,
                
                strm_non_zero_7x7,
                strm_coef_7x7,
                strm_coef_abv,
                strm_coef_lft,
                strm_coef_abv_lft,

                strm_non_zero_cnt,
                strm_non_zero_cnt_abv,
                strm_non_zero_cnt_lft,
        
				strm_sel_tab_77,
				strm_cur_bit_77,
				strm_e_77,
				strm_addr1_77,
				strm_addr2_77,
				strm_addr3_77,
				strm_addr4_77
            );
                


            //77  /////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////////


            ////////////////////////////////////////////////////////////////////////
            //EDGES/////////////////////////////////////////////////////////////////

            hls::stream<bool> strm_cur_bit_h("edge h");
            hls::stream<ap_uint<6> > strm_nz_77_h;
            hls::stream<ap_uint<3> > strm_so_far_h;

            hls::stream<ap_uint<4> > strm_length_exp_h("h_length_exp");
            hls::stream<ap_uint<4> > strm_length_sign_h("h_length_sign");
            hls::stream<ap_uint<4> > strm_length_nois_h("h_length_nois");

            hls::stream<bool> strm_cur_bit_exp_h("edge h exp");
            hls::stream<ap_uint<3> > strm_num_nonzero_bin_h;
            hls::stream<ap_uint<4> > strm_best_prior_exp_h("h_bsr_exp");

            hls::stream<bool> strm_cur_bit_sign_h("edge h sign");
            hls::stream<ap_uint<2> > strm_tri_sign_h;
            hls::stream<ap_uint<4> > strm_best_prior_sign_h("h_bsr_sign");

            hls::stream<bool> strm_cur_bit_nois_h("edge h nois");
            hls::stream<ap_uint<8> > strm_ctx_nois_h;
            hls::stream<ap_uint<8> > strm_min_nois_h;
            hls::stream<ap_uint<8> > strm_so_far_nois_h;
            hls::stream<ap_uint<6> > strm_coord_nois_h;

            hls::stream<bool> strm_cur_bit_v("edge v");
            hls::stream<ap_uint<6> > strm_nz_77_v;
            hls::stream<ap_uint<3> > strm_so_far_v;

            hls::stream<ap_uint<4> > strm_length_exp_v("v_length_exp");
            hls::stream<ap_uint<4> > strm_length_sign_v("v_length_sign");
            hls::stream<ap_uint<4> > strm_length_nois_v("v_length_nois");

            hls::stream<bool> strm_cur_bit_exp_v("edge v exp");
            hls::stream<ap_uint<3> > strm_num_nonzero_bin_v;
            hls::stream<ap_uint<4> > strm_best_prior_exp_v("v_bsr_exp");

            hls::stream<bool> strm_cur_bit_sign_v("edge v_sign");
            hls::stream<ap_uint<2> > strm_tri_sign_v;
            hls::stream<ap_uint<4> > strm_best_prior_sign_v("v_bsr_sign");

            hls::stream<bool> strm_cur_bit_nois_v("edge v_nois");
            hls::stream<ap_uint<8> > strm_ctx_nois_v;
            hls::stream<ap_uint<8> > strm_min_nois_v;
            hls::stream<ap_uint<8> > strm_so_far_nois_v;
            hls::stream<ap_uint<6> > strm_coord_nois_v;

        	hls::stream<ap_uint<4>  > strm_sel_tab_edge("sel_tab");
        	hls::stream<bool>		  strm_cur_bit_edge;
        	hls::stream<bool>		  strm_e_edge;
        	hls::stream<ap_uint<16> > strm_addr1_edge("addr1");
        	hls::stream<ap_uint<16> > strm_addr2_edge;
        	hls::stream<ap_uint<16> > strm_addr3_edge;
        	hls::stream<ap_uint<16> > strm_addr4_edge;


            int16_t coef_left_edges[64];
            bool left        = false;
            bool above       = !is_top_row_cmp;
            bool above_right = false;

            //serializing
            hls_serialize_tokens_edges(
                block_width,
                id_cmp!=0,
                min_nois_thld_x,
                min_nois_thld_y,
                left,
                !is_top_row_cmp,
                above_right,

                strm_non_zero_h,
                strm_coef_cnt_h_len,
                strm_lane_h,

                strm_coef_cnt_v_len,
                strm_lane_v,

                strm_eob_x,
                strm_eob_y,

                idct_q_table_x,
                idct_q_table_y,

                strm_coef_h,
                strm_coef_above_h,
                strm_has_left_h,
                strm_coef_e_h,
                strm_coef_v,
                strm_coef_left_v,
                strm_has_left_v,
                strm_coef_e_v,

				strm_sel_tab_edge,
				strm_cur_bit_edge,
				strm_e_edge,
				strm_addr1_edge,
				strm_addr2_edge,
				strm_addr3_edge,
				strm_addr4_edge
            );



            //EDGES  //////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////////////
            //DC  /////////////////////////////////////////////////////////////////////

                uint8_t q0 = q_tables[id_cmp][0][0];
                hls::stream<uint8_t> strm_length_dc_exp("dc_length_exp");
                hls::stream<uint8_t> strm_length_dc_sign("dc_length_sign");
                hls::stream<uint8_t> strm_length_dc_nois("dc_length_nois");

                hls::stream<bool> strm_dc_cur_bit_exp;
                hls::stream<ap_uint<4> > strm_dc_addr_1_exp;
                hls::stream<ap_uint<5> > strm_dc_addr_2_exp;

                hls::stream<bool> strm_dc_cur_bit_sign;
                hls::stream<ap_uint<4> > strm_dc_addr_0_sign;
                hls::stream<ap_uint<5> > strm_dc_addr_1_sign;

                hls::stream<bool> strm_dc_cur_bit_nois;
                hls::stream<ap_uint<5> > strm_dc_addr_0_nois;

            	hls::stream<ap_uint<4>  > strm_sel_tab_dc;
            	hls::stream<bool>		  strm_cur_bit_dc;
            	hls::stream<bool>		  strm_e_dc;
            	hls::stream<ap_uint<16> > strm_addr1_dc;
            	hls::stream<ap_uint<16> > strm_addr2_dc;
            	hls::stream<ap_uint<16> > strm_addr3_dc;

                hls_serialize_tokens_dc(
                    !is_top_row_cmp,
                    id_cmp,
                    block_width,
                    q_tables,
                    q0,

                    str_rast8,
                    str_dc1,

					strm_sel_tab_dc,
					strm_cur_bit_dc,
					strm_e_dc,
					strm_addr1_dc,
					strm_addr2_dc,
					strm_addr3_dc

                );
            //DC  /////////////////////////////////////////////////////////////////////
            ///////////////////////////////////////////////////////////////////////////

            hls::stream<bool>    strm_num_nonzeros_counts_7x7_bit;
            hls::stream<uint8_t> strm_num_nonzeros_counts_7x7_prob;
            hls::stream<bool>    strm_num_nonzeros_counts_7x7_e;

            hls::stream<bool>    strm_exponent_counts_bit;
            hls::stream<uint8_t> strm_exponent_counts_prob;
            hls::stream<bool>    strm_exponent_counts_e;

            hls::stream<bool>    strm_sign_counts_bit;
            hls::stream<uint8_t> strm_sign_counts_prob;
            hls::stream<bool>    strm_sign_counts_e;

            hls::stream<bool>    strm_residual_noise_counts_bit;
            hls::stream<uint8_t> strm_residual_noise_counts_prob;
            hls::stream<bool>    strm_residual_noise_counts_e;
            hls::stream<bool>    strm_block_e;

            hls::stream<bool>    strm_num_nonzeros_counts_8x1_bit;
            hls::stream<uint8_t> strm_num_nonzeros_counts_8x1_prob;
            hls::stream<bool>    strm_num_nonzeros_counts_8x1_e;
        
            hls::stream<bool>    strm_num_nonzeros_counts_1x8_bit;
            hls::stream<uint8_t> strm_num_nonzeros_counts_1x8_prob;
            hls::stream<bool>    strm_num_nonzeros_counts_1x8_e;

            hls::stream<bool>    strm_exponent_counts_x_bit;
            hls::stream<uint8_t> strm_exponent_counts_x_prob;
            hls::stream<bool>    strm_exponent_counts_x_e;

            hls::stream<bool>    strm_exponent_counts_dc_bit;
            hls::stream<uint8_t> strm_exponent_counts_dc_prob;
            hls::stream<bool>    strm_exponent_counts_dc_e;

            hls::stream<bool>    strm_residual_noise_counts_dc_bit;
            hls::stream<uint8_t> strm_residual_noise_counts_dc_prob;
            hls::stream<bool>    strm_residual_noise_counts_dc_e;

            hls::stream<bool>    strm_7x7_bit;
            hls::stream<uint8_t> strm_7x7_prob;
            hls::stream<bool>    strm_7x7_e;

            hls::stream<bool>    strm_edge_bit;
            hls::stream<uint8_t> strm_edge_prob;
            hls::stream<bool>    strm_edge_e;

            hls::stream<bool>    strm_dc_bit;
            hls::stream<uint8_t> strm_dc_prob;
            hls::stream<bool>    strm_dc_e;
/*
            hls::stream<bool>    strm_bit;
            hls::stream<uint8_t> strm_prob;
            hls::stream<bool>    strm_e;
            hls::stream<uint8_t> strm_tab_dbg;
            */


            ap_uint<1> ap_color = hls_color_index((int)id_cmp);  

		    hls::stream<bool>    strm_bit_77;
		    hls::stream<uint8_t> strm_prob_77;
		    hls::stream<bool>    strm_e_7x7;
		    hls::stream<uint8_t> strm_tab_dbg_77;

			hls::stream<ap_uint<4>  > strm_sel_tab;
			hls::stream<bool>		  strm_cur_bit("res_bit");
			hls::stream<bool>		  strm_e_in("res_e");
			hls::stream<ap_uint<16> > strm_addr1;
			hls::stream<ap_uint<16> > strm_addr2;
			hls::stream<ap_uint<16> > strm_addr3;
			hls::stream<ap_uint<16> > strm_addr4;

			collect(
                block_width,

				strm_sel_tab_77,
				strm_cur_bit_77,
				strm_e_77,
				strm_addr1_77,
				strm_addr2_77,
				strm_addr3_77,
				strm_addr4_77,

				strm_sel_tab_edge,
				strm_cur_bit_edge,
				strm_e_edge,
				strm_addr1_edge,
				strm_addr2_edge,
				strm_addr3_edge,
				strm_addr4_edge,

				strm_sel_tab_dc,
				strm_cur_bit_dc,
				strm_e_dc,
				strm_addr1_dc,
				strm_addr2_dc,
				strm_addr3_dc,

				strm_sel_tab,
				strm_cur_bit,
				strm_e_in,
				strm_addr1,
				strm_addr2,
				strm_addr3,
				strm_addr4

            );



			probability_look_up(
				id_cmp!=0,

				strm_sel_tab,
				strm_cur_bit,
				strm_e_in,
				strm_addr1,
				strm_addr2,
				strm_addr3,
				strm_addr4,

				strm_bit,
				strm_prob,
				strm_e,
				strm_tab_dbg
			);
            
            hls::stream< bool >          strm_pos_o_e;
            hls::stream< unsigned char > strm_pos_o_byte;
            
            vpx_enc_syn(
                //Iteration for variable
                &range,//,unsigned char*        range,
                &count,//,int*                  cnt,
                &value,//,unsigned int*         value,
                &pre_byte,//,unsigned char*        pre_byte,
                &run,//,unsigned short*       run,
                &isFirst,//,bool*                 br_isFirst,
                &pos,//,unsigned int*         pos ,
                //input
                strm_bit,//,hls::stream<bool>&    strm_bit,
                strm_prob,//,hls::stream<uint8_t>& strm_prob,
                strm_e,//,hls::stream<bool>&    strm_e_range,
                strm_tab_dbg,//,hls::stream<uint8_t>& strm_tab_dbg,
                //output
                strm_pos_o_e,//,hls::stream<bool>&    strm_pos_o_e,
                strm_pos_o_byte//,hls::stream< unsigned char >&strm_pos_o_byte
            );
        
            //===================================================
            //To DDR
            bool     e_byte        = strm_pos_o_e.read();
            while ( !e_byte){
                    e_byte        = strm_pos_o_e.read();
                    unsigned char byte = strm_pos_o_byte.read();
                    axi_res[pos2++]   = byte;
            }
            //===================================================

            is_top_row[id_cmp] = false;
            //Updating current y points for all components
            cur_y_cmp[id_cmp]++;
            if(cur_y_cmp[0]>=axi_height[0])
                break;
        }//for(int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu ; idx_cmp++)



    }//("process_row_range_while");for( int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++)

    axi_res[pos++] = pre_byte;
	for(; run > 0; run--)
		axi_res[pos++] = 0xff;

	axi_arith.count=count;
	axi_arith.value=value;
	axi_arith.pre_byte = pre_byte;
	axi_arith.run = run;
	axi_arith.pos = pos;
	axi_arith.range = range;
	axi_arith.isFirst = isFirst;

    ///////////////////////////////////////////////////////////////////////////
    //AC  ////////////////////////////////////////////////////////////////////

}
#endif

#if 0
void tmp_Kernel_1(
	    //input
		WD_AXI          axi_coeff               [MAX_COEF_AXI],
		uint16_t        axi_width               [MAX_NUM_COLOR],//colldata->block_width(i);
		uint16_t        axi_height              [MAX_NUM_COLOR],//colldata->block_width(i);
		uint8_t         axi_map_row2cmp         [4], //     AXI                   2,1,0,0 2,1,0
		uint8_t         min_nois_thld_x         [MAX_NUM_COLOR][64],
		uint8_t         min_nois_thld_y         [MAX_NUM_COLOR][64],
        uint8_t         q_tables                [MAX_NUM_COLOR][8][8],//[64],
		int32_t         idct_q_table_x          [MAX_NUM_COLOR][8][8],
		int32_t         idct_q_table_y          [MAX_NUM_COLOR][8][8],
		int32_t         idct_q_table_l          [MAX_NUM_COLOR][8][8],

		uint16_t        axi_mcuv,
		uint8_t         axi_num_cmp_mcu,
		uint8_t         axi_num_cmp,
		//tmp output
        uint8_t         axi_res                 [MAX_NUM_PIX],
        struct_arith    &axi_arith
		)
{
#pragma HLS ARRAY_PARTITION variable = num_nonzeros_counts_7x7 block factor = 3 dim = 3
#pragma HLS ARRAY_PARTITION variable = residual_noise_counts block factor = 4 dim = 2

#pragma HLS ARRAY_PARTITION variable = exponent_counts complete dim = 2
#pragma HLS RESOURCE variable = exponent_counts core = XPM_MEMORY uram

#pragma HLS ARRAY_PARTITION variable = residual_noise_counts block factor = 4 dim = 2

#pragma HLS ARRAY_PARTITION variable = residual_threshold_counts complete dim = 4
#pragma HLS RESOURCE variable = residual_threshold_counts core = XPM_MEMORY uram

#pragma HLS ARRAY_PARTITION variable = exponent_counts_x complete dim = 4
#pragma HLS RESOURCE variable = exponent_counts_x core = XPM_MEMORY uram
	// We have to initial table here. It will take tens of us.
	//////////////////////////////////////////////////
    ///// Kernel Begin
    //////////////////////////////////////////////////
    init_hlsmodel();
    kernel_run(
        //input
        axi_coeff,
        axi_width,//colldata->block_width(i);
        axi_height,//colldata->block_width(i);
        axi_map_row2cmp, //     AXI                   2,1,0,0 2,1,0
        min_nois_thld_x,
        min_nois_thld_y,
        q_tables,//[64],
        idct_q_table_x,
        idct_q_table_y,
        idct_q_table_l,

        axi_mcuv,
        axi_num_cmp_mcu,
        axi_num_cmp,
        //tmp output
        axi_res,
		axi_arith
    );
    ///////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////
    ///// Kernel END
    //////////////////////////////////////////////////
}
#endif
//#include "XAcc_jpegdecoder.hpp"
void VP8ComponentEncoder::process_row_range2(unsigned int thread_id,
                                             const UncompressedComponents* const colldata,
                                             ResizableByteBuffer* stream,
                                             BoolEncoder* bool_encoder) {}
#if 0
void process_row_range3(
		WD_AXI*         axi_coeff,
		LeptonInput&    lepp,
        struct_arith&   axi_arith,
		uint8_t*        axi_res
       ) {

    tmp_Kernel_1(
        		axi_coeff,        //WD_AXI          axi_coeff               [MAX_NUM_COEF],
				lepp.axi_width,        //uint16_t        axi_width               [MAX_NUM_COLOR],//colldata->block_width(i);
				lepp.axi_height,       //uint16_t        axi_height              [MAX_NUM_COLOR],//colldata->block_width(i);
				lepp.axi_map_row2cmp,  //uint8_t         axi_map_row2cmp         [4], //     AXI                   2,1,0,0 2,1,0
				lepp.min_nois_thld_x,
				lepp.min_nois_thld_y,
				lepp.q_tables,         //uint16_t        q_table                 [MAX_NUM_COLOR][64],  //dqt[2][64]
				lepp.idct_q_table_x,   //int32_t         idct_q_table            [MAX_NUM_COLOR][64],
				lepp.idct_q_table_y,   //int32_t         idct_q_table_y          [MAX_NUM_COLOR][64],
				lepp.idct_q_table_l,   //int32_t         idct_q_table_l          [MAX_NUM_COLOR][64],
				lepp.axi_mcuv,         //uint16_t        axi_mcuv,
				lepp.axi_num_cmp_mcu,  //uint8_t         axi_num_cmp_mcu,
				lepp.axi_num_cmp,      //uint8_t         axi_num_cmp,
				axi_res,
                axi_arith
        		);

}
#endif
