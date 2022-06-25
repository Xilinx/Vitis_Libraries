/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
#ifndef ENCODER_HH
#define ENCODER_HH
#include "model.hh"
template <bool all_neighbors_present, BlockType color>
void serialize_tokens(ConstBlockContext context,
                      BoolEncoder& encoder,
                      ProbabilityTables<all_neighbors_present, color>& probability_tables,
                      ProbabilityTablesBase&);
void serialize_tokens(BlockType color,
                      bool left_present,
                      bool above_present,
                      bool above_right_present,
                      ConstBlockContext context,
                      BoolEncoder& encoder,
                      ProbabilityTablesBase& pt);

void serialize_tokens(BlockType color,
                      bool left_present,
                      bool above_present,
                      bool above_right_present,
                      uint8_t num_nonzeros_7x7,
                      uint8_t eob_x,
                      uint8_t eob_y,
                      ConstBlockContext context,
                      BoolEncoder& encoder,
                      ProbabilityTablesBase& pt);

void serialize_tokens_77(BlockType color,
                         bool left_present,
                         bool above_present,
                         bool above_right_present,
                         uint8_t num_nonzeros_7x7,
                         uint8_t eob_x,
                         uint8_t eob_y,
                         ConstBlockContext context,
                         BoolEncoder& encoder,
                         ProbabilityTablesBase& pt);
void serialize_tokens_77(ap_uint<1> color,
                         bool left_present,
                         bool above_present,
                         bool above_right_present,
                         uint8_t num_nonzeros_7x7,
                         uint8_t num_nonzeros_above,
                         uint8_t num_nonzeros_left,
                         int16_t coef_here[64],
                         int16_t coef_left[64],
                         int16_t coef_above[64],
                         int16_t coef_above_left[64],
                         // ConstBlockContext context,
                         BoolEncoder& encoder);

void serialize_tokens_edges(BlockType color,
                            bool left_present,
                            bool above_present,
                            bool above_right_present,
                            uint8_t num_nonzeros_7x7,
                            uint8_t eob_x,
                            uint8_t eob_y,
                            int16_t coef_here[64],
                            int16_t coef_left[64],
                            int16_t coef_above[64],
                            // int16_t coef_above_left[64],
                            // ConstBlockContext context,
                            BoolEncoder& encoder);

void serialize_tokens_dc(BlockType color,
                         bool left_present,
                         bool above_present,
                         int16_t dc,
                         uint16_t q0,
                         int16_t est_v[8],
                         int16_t est_h[8],
                         // struct_ctx_edge* pctx_edge,
                         BoolEncoder& encoder);
void serialize_tokens_dc(BlockType color,
                         bool left_present,
                         bool above_present,
                         hls::stream<coef_t>& str_dc_in,
                         uint16_t q0,
                         // int16_t outp_sans_dc [64],
                         // int16_t est_v[8],
                         // int16_t est_h[8],
                         hls::stream<pix_edge_t>& str_est_v,
                         hls::stream<pix_edge_t>& str_est_h,
                         // struct_ctx_edge* p_ctx_edge,
                         // ConstBlockContext context,
                         BoolEncoder& encoder);
#endif /* ENCODER_HH */
