#include "../util/options.hh"
#include "boolwriter.hh"
#include "../../io/MuxReader.hh"
#include "../../other/loop_stt.h"
#include "XModified.hpp"
#include "XAcc_dc.hpp"
extern LoopNodeFactory g_loops;
class VPXBoolWriter {
   public:
    hls::stream<tmp_struct> str_77;
    hls::stream<tmp_struct> str_edges;
    hls::stream<tmp_struct> str_dc;
    hls::stream<int> str_77_cnt;
    hls::stream<int> str_edges_cnt;
    hls::stream<int> str_dc_cnt;
    int cnt_77, cnt_edges, cnt_dc;
    vpx_writer boolwriter;
    Sirikata::MuxReader::ResizableByteBuffer output_;

   private:
//    vpx_writer boolwriter;
//    Sirikata::MuxReader::ResizableByteBuffer output_;
#ifdef DEBUG_ARICODER
    bool any_written;
#endif
    enum { MIN_SIZE = 1024 * 1024 };
    enum { SIZE_CHECK = 0xfff00000 };

   public:
    VPXBoolWriter() {
#ifdef DEBUG_ARICODER
        any_written = false;
#endif
        static_assert(MIN_SIZE & SIZE_CHECK, "min size must be caught by the size check, so allocations happen after");
        static_assert(((MIN_SIZE - 1) & SIZE_CHECK) == 0, "min size -1 must not be caught by the size check");
    }
    void init() {
#ifdef DEBUG_ARICODER
        always_assert(!any_written);
#endif
        /*	output_.resize((std::max((unsigned int)MIN_SIZE,
                                         std::min((unsigned int)4096 * 1024,
                                                  (unsigned int)(5120 * 1024 / NUM_THREADS))))
                               + 1024);*/
        output_.resize(805306368);
        //        vpx_start_encode(&boolwriter, output_.data());
    }
    void put(const bool value, Branch& branch, Billing bill) {
        g_loops.START("PROC:put of AC", PROC);
        g_loops.CNT();
        g_loops.END();
#ifdef DEBUG_ARICODER
        if (!any_written) {
            any_written = true;
            static int count = 0;
            w_bitcount = count * 500000000;
            ++count;
        }
#endif
        vpx_write(&boolwriter, value, branch.prob(), bill);
        if (__builtin_expect(boolwriter.pos & SIZE_CHECK, false)) {
            // check if we're out of buffer space
            if (boolwriter.pos + 128 > output_.size()) {
                output_.resize(output_.size() * 2);
                boolwriter.buffer = &output_[0]; // reset buffer
            }
        }
        branch.record_obs_and_update(value);
    }
    //    stt_dis tmp_dis;//to statistic the distance between two branch
    void put(const bool value, hls_Branch* branch, Billing bill) {
        //    	tmp_dis.get_dis(branch);
        g_loops.START("PROC:put of AC", PROC);
        g_loops.CNT();
        g_loops.END();
        vpx_write(&boolwriter, value, branch->prob2(), bill);
        /*       if (__builtin_expect(boolwriter.pos & SIZE_CHECK, false)) {
                   // check if we're out of buffer space
                   if (boolwriter.pos + 128 > output_.size()) {
                       output_.resize(output_.size() * 2);
                       boolwriter.buffer = &output_[0]; //reset buffer
                   }
               }*/
        // uint8_t p_old = branch->prob();

        branch->record_obs_and_update(value);
        // uint8_t p_new = branch->prob();
        // Shift_table[p_old][value]=p_new;
        // if(p_old==128 && value==false){
        // 	if(p_new!=170)
        // 		p_new=p_new;
        //}
    }
    void put_77(const bool value, hls_Branch* branch, Billing bill) {
        tmp_struct tt = {value, branch, (int)bill};
        this->cnt_77++;
        this->str_77.write(tt);
        /*
        vpx_write(&boolwriter, value, branch->prob2(), bill);
        if (__builtin_expect(boolwriter.pos & SIZE_CHECK, false)) {
            // check if we're out of buffer space
            if (boolwriter.pos + 128 > output_.size()) {
                output_.resize(output_.size() * 2);
                boolwriter.buffer = &output_[0]; //reset buffer
            }
        }

        branch->record_obs_and_update(value);*/
    }
    void put_edges(const bool value, hls_Branch* branch, Billing bill) {
        tmp_struct tt = {value, branch, (int)bill};
        this->cnt_edges++;
        this->str_edges.write(tt);
    }
    void put_dc(const bool value, hls_Branch* branch, Billing bill) {
        tmp_struct tt = {value, branch, (int)bill};
        this->cnt_dc++;
        this->str_dc.write(tt);
    }
    int end_77() {
        this->str_77_cnt.write(this->cnt_77);
        return this->cnt_77;
    }
    int end_edges() {
        this->str_edges_cnt.write(this->cnt_edges);
        return this->cnt_edges;
    }
    int end_dc() {
        this->str_dc_cnt.write(this->cnt_dc);
        return this->cnt_dc;
    }
    void ColllectPut() {
        int cnt = this->str_77_cnt.read();
        for (int i = 0; i < cnt; i++) {
            tmp_struct tt = this->str_77.read();
            this->put(tt.value, tt.branch, (Billing)tt.bill);
        }

        cnt = this->str_edges_cnt.read();
        for (int i = 0; i < cnt; i++) {
            tmp_struct tt = this->str_edges.read();
            this->put(tt.value, tt.branch, (Billing)tt.bill);
        }

        cnt = this->str_dc_cnt.read();
        for (int i = 0; i < cnt; i++) {
            tmp_struct tt = this->str_dc.read();
            this->put(tt.value, tt.branch, (Billing)tt.bill);
        }
    }

    void put(const bool value, Branch& branch, hls_Branch* branch2, Billing bill) {
        vpx_write(&boolwriter, value, branch.prob(), bill);
        if (__builtin_expect(boolwriter.pos & SIZE_CHECK, false)) {
            // check if we're out of buffer space
            if (boolwriter.pos + 128 > output_.size()) {
                output_.resize(output_.size() * 2);
                boolwriter.buffer = &output_[0]; // reset buffer
            }
        }
        uint8_t p1 = branch.prob();
        uint8_t p2 = branch2->prob2();
        if (p1 != p2) p2 = p1;
        branch.record_obs_and_update(value);
        branch2->record_obs_and_update(value);
    }
    void finish(Sirikata::MuxReader::ResizableByteBuffer& finish) {
        //        vpx_stop_encode(&boolwriter);
        //        output_.resize(boolwriter.pos);
        finish.swap(output_);
    }
};
