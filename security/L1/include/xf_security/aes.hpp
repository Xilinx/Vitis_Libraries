#ifndef _XF_SECURITY_AES_HPP_
#define _XF_SECURITY_AES_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#ifndef __SYNTHESIS__
#include <iostream>
#endif
namespace xf {
namespace security {

class aesTable {
   public:
    aesTable() {
#pragma HLS inline
#pragma HLS resource variable = ssbox core = ROM_nP_LUTRAM
#pragma HLS resource variable = iibox core = ROM_nP_LUTRAM
    }

    const ap_uint<8> ssbox[256] = {
        0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x1,  0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82,
        0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26,
        0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x4,  0xc7, 0x23, 0xc3, 0x18, 0x96,
        0x5,  0x9a, 0x7,  0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
        0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x0,  0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb,
        0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x2,  0x7f,
        0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff,
        0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
        0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32,
        0x3a, 0x0a, 0x49, 0x6,  0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d,
        0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6,
        0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70, 0x3e, 0xb5, 0x66, 0x48, 0x3,  0xf6, 0x0e,
        0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e,
        0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f,
        0xb0, 0x54, 0xbb, 0x16};
    const ap_uint<8> iibox[256] = {
        0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB, 0x7C, 0xE3,
        0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB, 0x54, 0x7B, 0x94, 0x32,
        0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E, 0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9,
        0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25, 0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16,
        0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92, 0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15,
        0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84, 0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05,
        0xB8, 0xB3, 0x45, 0x06, 0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13,
        0x8A, 0x6B, 0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
        0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E, 0x47, 0xF1,
        0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B, 0xFC, 0x56, 0x3E, 0x4B,
        0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4, 0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07,
        0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F, 0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D,
        0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF, 0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB,
        0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61, 0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63,
        0x55, 0x21, 0x0C, 0x7D};
};

template <int W>
class aesEnc {
   public:
    void updateKey(ap_uint<W> cipherkey) {}
    void process(ap_uint<128> plaintext, ap_uint<W> cipherkey, ap_uint<128>& ciphertext) {}
};

template <>
class aesEnc<256> : public aesTable {
   public:
    ap_uint<128> key_list[16];

    aesEnc() {
#pragma HLS inline
#pragma HLS ARRAY_PARTITION variable = key_list complete dim = 1
    }

    void updateKey(ap_uint<256> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<256> lastRound = cipherkey;
        key_list[0] = lastRound.range(127, 0);
        key_list[1] = lastRound.range(255, 128);
        for (ap_uint<5> iter = 2; iter < 15; iter += 2) {
#pragma HLS pipeline II = 1
            //#pragma HLS pipeline II = 1
            ap_uint<128> currRound_0, currRound_1;

            ap_uint<32> round_tmp = lastRound.range(255, 224);
            round_tmp = (round_tmp >> 8) | (round_tmp << 24);

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)] ^ Rcon[(iter >> 1) - 1];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            currRound_0.range(31, 0) = lastRound.range(31, 0) ^ round_tmp;
            currRound_0.range(63, 32) = lastRound.range(63, 32) ^ currRound_0.range(31, 0);
            currRound_0.range(95, 64) = lastRound.range(95, 64) ^ currRound_0.range(63, 32);
            currRound_0.range(127, 96) = lastRound.range(127, 96) ^ currRound_0.range(95, 64);

            ap_uint<32> round_tmp2 = currRound_0.range(127, 96);

            round_tmp2.range(7, 0) = ssbox[round_tmp2.range(7, 0)];
            round_tmp2.range(15, 8) = ssbox[round_tmp2.range(15, 8)];
            round_tmp2.range(23, 16) = ssbox[round_tmp2.range(23, 16)];
            round_tmp2.range(31, 24) = ssbox[round_tmp2.range(31, 24)];

            currRound_1.range(31, 0) = lastRound.range(159, 128) ^ round_tmp2;
            currRound_1.range(63, 32) = lastRound.range(191, 160) ^ currRound_1.range(31, 0);
            currRound_1.range(95, 64) = lastRound.range(223, 192) ^ currRound_1.range(63, 32);
            currRound_1.range(127, 96) = lastRound.range(255, 224) ^ currRound_1.range(95, 64);
            lastRound.range(127, 0) = currRound_0;
            lastRound.range(255, 128) = currRound_1;
            key_list[iter] = currRound_0;
            key_list[iter + 1] = currRound_1;
        }
    }

    // the function of Finite field multiplication *2
    static ap_uint<8> GFMul2(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        if (a[7] == 1) {
            a = (a << 1) ^ kModulus;
        } else {
            a <<= 1;
        }

        return a;
    }

    void process(ap_uint<128> plaintext, ap_uint<256> cipherkey, ap_uint<128>& ciphertext) {
        ap_uint<128> state, state_1;
        ap_uint<8> tmp_1, tmp_2_1, tmp_2_2, tmp_3;
        ap_uint<4> round_counter;

        // state init and add roundkey[0]
        state = plaintext ^ key_list[0];

        // Start 14 rounds of process
        for (round_counter = 1; round_counter <= 14; round_counter++) {
            // SubByte
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                state(i * 8 + 7, i * 8) = ssbox[state(i * 8 + 7, i * 8)];
            }
            // ShiftRow
            tmp_1 = state(15, 8);
            state(15, 8) = state(47, 40);
            state(47, 40) = state(79, 72);
            state(79, 72) = state(111, 104);
            state(111, 104) = tmp_1;

            tmp_2_1 = state(23, 16);
            state(23, 16) = state(87, 80);
            state(87, 80) = tmp_2_1;

            tmp_2_2 = state(55, 48);
            state(55, 48) = state(119, 112);
            state(119, 112) = tmp_2_2;

            tmp_3 = state(127, 120);
            state(127, 120) = state(95, 88);
            state(95, 88) = state(63, 56);
            state(63, 56) = state(31, 24);
            state(31, 24) = tmp_3;

            // MixColumn
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                ap_uint<8> tmp = state(i * 8 + 7, i * 8);
                state_1(i * 8 + 7, i * 8) = state(i * 8 + 7, i * 8);
            }

            if (round_counter < 14) {
                // clang-format off
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    state(i * 32 + 7, i * 32) =GFMul2(state_1(i * 32 + 7, i * 32)) ^ 
                                               GFMul2(state_1(i * 32 + 15, i * 32 + 8))^ 
                                               state_1(i * 32 + 15, i * 32 + 8)^
                                               state_1(i * 32 + 23, i * 32 + 16) ^ 
                                               state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 15, i * 32 + 8) = state_1(i * 32 + 7, i * 32) ^ 
                                                     GFMul2(state_1(i * 32 + 15, i * 32 + 8)) ^
                                                     GFMul2(state_1(i * 32 + 23, i * 32 + 16)) ^
                                                     state_1(i * 32 + 23, i * 32 + 16)^
                                                     state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 23, i * 32 + 16) = state_1(i * 32 + 7, i * 32) ^ 
                                                      state_1(i * 32 + 15, i * 32 + 8) ^
                                                      GFMul2(state_1(i * 32 + 23, i * 32 + 16)) ^
                                                      GFMul2(state_1(i * 32 + 31, i * 32 + 24)) ^
                                                      state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 31, i * 32 + 24) = GFMul2(state_1(i * 32 + 7, i * 32))^
                                                      state_1(i * 32 + 7, i * 32) ^ 
                                                      state_1(i * 32 + 15, i * 32 + 8) ^
                                                      state_1(i * 32 + 23, i * 32 + 16) ^
                                                      GFMul2(state_1(i * 32 + 31, i * 32 + 24));
                }
                // clang-format on
            } else {
                state = state_1;
            }

            state ^= key_list[round_counter];
        }
        ciphertext = state;
    }

    void updateKey_2(ap_uint<256> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<128> lastRound[2];
        key_list[0] = cipherkey.range(127, 0);
        key_list[1] = cipherkey.range(255, 128);
        lastRound[0] = cipherkey.range(127, 0);
        lastRound[1] = cipherkey.range(255, 128);
        for (ap_uint<5> iter = 2; iter < 15; iter++) {
#pragma HLS pipeline II = 1
            ap_uint<32> round_tmp = lastRound[ap_uint<1>(1) - iter[0]].range(127, 96);
            if (iter[0] == ap_uint<1>(0)) {
                round_tmp = (round_tmp >> 8) | (round_tmp << 24);
            }

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            if (iter[0] == ap_uint<1>(0)) {
                round_tmp.range(7, 0) ^= Rcon[(iter >> 1) - 1];
            }

            ap_uint<128> tmp_key;
            tmp_key.range(31, 0) = lastRound[iter[0]].range(31, 0) ^ round_tmp;
            tmp_key.range(63, 32) = lastRound[iter[0]].range(63, 32) ^ tmp_key.range(31, 0);
            tmp_key.range(95, 64) = lastRound[iter[0]].range(95, 64) ^ tmp_key.range(63, 32);
            tmp_key.range(127, 96) = lastRound[iter[0]].range(127, 96) ^ tmp_key.range(95, 64);

            key_list[iter] = tmp_key;
            lastRound[iter[0]] = tmp_key;
        }
    }
};

template <>
class aesEnc<192> : public aesTable {
   public:
    ap_uint<128> key_list[14];

    aesEnc() {
#pragma HLS inline
#pragma HLS ARRAY_PARTITION variable = key_list complete dim = 1
    }

    void updateKey(ap_uint<192> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<192> lastRound = cipherkey;
        key_list[0] = lastRound.range(127, 0);
        key_list[1].range(63, 0) = lastRound.range(191, 128);
        ap_uint<4> rIter = 0;

        for (ap_uint<5> iter = 3; iter < 26; iter += 3) {
#pragma HLS pipeline II = 1

            ap_uint<192> thisRound = 0;

            ap_uint<32> round_tmp = lastRound.range(191, 160);
            round_tmp = (round_tmp >> 8) | (round_tmp << 24);

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)] ^ Rcon[rIter++];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            thisRound.range(31, 0) = lastRound.range(31, 0) ^ round_tmp;
            thisRound.range(63, 32) = lastRound.range(63, 32) ^ thisRound.range(31, 0);
            thisRound.range(95, 64) = lastRound.range(95, 64) ^ thisRound.range(63, 32);
            thisRound.range(127, 96) = lastRound.range(127, 96) ^ thisRound.range(95, 64);
            thisRound.range(159, 128) = lastRound.range(159, 128) ^ thisRound.range(127, 96);
            thisRound.range(191, 160) = lastRound.range(191, 160) ^ thisRound.range(159, 128);

            if (iter[0] == 1) {
                key_list[iter.range(4, 1)].range(127, 64) = thisRound.range(63, 0);
                key_list[iter.range(4, 1) + 1].range(127, 0) = thisRound.range(191, 64);
            } else {
                key_list[iter.range(4, 1)].range(127, 0) = thisRound.range(127, 0);
                key_list[iter.range(4, 1) + 1].range(63, 0) = thisRound.range(191, 128);
            }
            lastRound = thisRound;
        }
    }

    // the function of Finite field multiplication *2

    static ap_uint<8> GFMul2(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        if (a[7] == 1) {
            a = (a << 1) ^ kModulus;
        } else {
            a <<= 1;
        }

        return a;
    }

    void process(ap_uint<128> plaintext, ap_uint<192> cipherkey, ap_uint<128>& ciphertext) {
        ap_uint<128> state, state_1;
        ap_uint<8> tmp_1, tmp_2_1, tmp_2_2, tmp_3;
        ap_uint<4> round_counter;

        // state init and add roundkey[0]
        state = plaintext ^ key_list[0];

        // Start 14 rounds of process
        for (round_counter = 1; round_counter <= 12; round_counter++) {
            // SubByte
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                state(i * 8 + 7, i * 8) = ssbox[state(i * 8 + 7, i * 8)];
            }
            // ShiftRow
            tmp_1 = state(15, 8);
            state(15, 8) = state(47, 40);
            state(47, 40) = state(79, 72);
            state(79, 72) = state(111, 104);
            state(111, 104) = tmp_1;

            tmp_2_1 = state(23, 16);
            state(23, 16) = state(87, 80);
            state(87, 80) = tmp_2_1;

            tmp_2_2 = state(55, 48);
            state(55, 48) = state(119, 112);
            state(119, 112) = tmp_2_2;

            tmp_3 = state(127, 120);
            state(127, 120) = state(95, 88);
            state(95, 88) = state(63, 56);
            state(63, 56) = state(31, 24);
            state(31, 24) = tmp_3;

            // MixColumn
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                ap_uint<8> tmp = state(i * 8 + 7, i * 8);
                state_1(i * 8 + 7, i * 8) = state(i * 8 + 7, i * 8);
            }

            if (round_counter < 12) {
                // clang-format off
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    state(i * 32 + 7, i * 32) =GFMul2(state_1(i * 32 + 7, i * 32)) ^ 
                                               GFMul2(state_1(i * 32 + 15, i * 32 + 8))^ 
                                               state_1(i * 32 + 15, i * 32 + 8)^
                                               state_1(i * 32 + 23, i * 32 + 16) ^ 
                                               state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 15, i * 32 + 8) = state_1(i * 32 + 7, i * 32) ^ 
                                                     GFMul2(state_1(i * 32 + 15, i * 32 + 8)) ^
                                                     GFMul2(state_1(i * 32 + 23, i * 32 + 16)) ^
                                                     state_1(i * 32 + 23, i * 32 + 16)^
                                                     state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 23, i * 32 + 16) = state_1(i * 32 + 7, i * 32) ^ 
                                                      state_1(i * 32 + 15, i * 32 + 8) ^
                                                      GFMul2(state_1(i * 32 + 23, i * 32 + 16)) ^
                                                      GFMul2(state_1(i * 32 + 31, i * 32 + 24)) ^
                                                      state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 31, i * 32 + 24) = GFMul2(state_1(i * 32 + 7, i * 32))^
                                                      state_1(i * 32 + 7, i * 32) ^ 
                                                      state_1(i * 32 + 15, i * 32 + 8) ^
                                                      state_1(i * 32 + 23, i * 32 + 16) ^
                                                      GFMul2(state_1(i * 32 + 31, i * 32 + 24));
                }
                // clang-format on
            } else {
                state = state_1;
            }

            state ^= key_list[round_counter];
        }
        ciphertext = state;
    }
};

template <>
class aesEnc<128> : public aesTable {
   public:
    ap_uint<128> key_list[11];
    aesEnc() {
#pragma HLS inline
#pragma HLS ARRAY_PARTITION variable = key_list complete dim = 1
    }

    void updateKey(ap_uint<128> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<128> lastRound = cipherkey;
        key_list[0] = lastRound;
        for (ap_uint<5> iter = 1; iter < 11; iter++) {
#pragma HLS pipeline II = 1

            ap_uint<32> round_tmp = lastRound.range(127, 96);
            round_tmp = (round_tmp >> 8) | (round_tmp << 24);

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)] ^ Rcon[iter - 1];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            key_list[iter].range(31, 0) = lastRound.range(31, 0) ^ round_tmp;
            key_list[iter].range(63, 32) = lastRound.range(63, 32) ^ key_list[iter].range(31, 0);
            key_list[iter].range(95, 64) = lastRound.range(95, 64) ^ key_list[iter].range(63, 32);
            key_list[iter].range(127, 96) = lastRound.range(127, 96) ^ key_list[iter].range(95, 64);

            lastRound.range(127, 0) = key_list[iter];
        }
    }

    // the function of Finite field multiplication *2

    static ap_uint<8> GFMul2(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;
        if (a[7] == 1) {
            a = (a << 1) ^ kModulus;
        } else {
            a <<= 1;
        }

        return a;
    }

    void process(ap_uint<128> plaintext, ap_uint<128> cipherkey, ap_uint<128>& ciphertext) {
        ap_uint<128> state, state_1;
        ap_uint<8> tmp_1, tmp_2_1, tmp_2_2, tmp_3;
        ap_uint<4> round_counter;

        // state init and add roundkey[0]
        state = plaintext ^ key_list[0];
        // std::cout << "the state is " <<state<< std::endl;

        // Start 10 rounds of process
        for (round_counter = 1; round_counter <= 10; round_counter++) {
            // SubByte
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                state(i * 8 + 7, i * 8) = ssbox[state(i * 8 + 7, i * 8)];
            }

            // ShiftRow
            tmp_1 = state(15, 8);
            state(15, 8) = state(47, 40);
            state(47, 40) = state(79, 72);
            state(79, 72) = state(111, 104);
            state(111, 104) = tmp_1;

            tmp_2_1 = state(23, 16);
            state(23, 16) = state(87, 80);
            state(87, 80) = tmp_2_1;

            tmp_2_2 = state(55, 48);
            state(55, 48) = state(119, 112);
            state(119, 112) = tmp_2_2;

            tmp_3 = state(127, 120);
            state(127, 120) = state(95, 88);
            state(95, 88) = state(63, 56);
            state(63, 56) = state(31, 24);
            state(31, 24) = tmp_3;

            // MixColumn

            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                ap_uint<8> tmp = state(i * 8 + 7, i * 8);
                state_1(i * 8 + 7, i * 8) = state(i * 8 + 7, i * 8);
            }
            if (round_counter < 10) {
                // clang-format off
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll

                    state(i * 32 + 7, i * 32) =GFMul2(state_1(i * 32 + 7, i * 32)) ^ 
                                               GFMul2(state_1(i * 32 + 15, i * 32 + 8))^ 
                                               state_1(i * 32 + 15, i * 32 + 8)^
                                               state_1(i * 32 + 23, i * 32 + 16) ^ 
                                               state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 15, i * 32 + 8) = state_1(i * 32 + 7, i * 32) ^ 
                                                     GFMul2(state_1(i * 32 + 15, i * 32 + 8)) ^
                                                     GFMul2(state_1(i * 32 + 23, i * 32 + 16)) ^
                                                     state_1(i * 32 + 23, i * 32 + 16)^
                                                     state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 23, i * 32 + 16) = state_1(i * 32 + 7, i * 32) ^ 
                                                      state_1(i * 32 + 15, i * 32 + 8) ^
                                                      GFMul2(state_1(i * 32 + 23, i * 32 + 16)) ^
                                                      GFMul2(state_1(i * 32 + 31, i * 32 + 24)) ^
                                                      state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 31, i * 32 + 24) = GFMul2(state_1(i * 32 + 7, i * 32))^
                                                      state_1(i * 32 + 7, i * 32) ^ 
                                                      state_1(i * 32 + 15, i * 32 + 8) ^
                                                      state_1(i * 32 + 23, i * 32 + 16) ^
                                                      GFMul2(state_1(i * 32 + 31, i * 32 + 24));
                }
                // clang-format on
            } else {
                state = state_1;
            }
            state ^= key_list[round_counter];
        }
        ciphertext = state;
    }
};

/**
 * @brief AES decryption
 *
 * @tparam W Bit width of AES key, which is 128, 192 or 256
 */
template <int W>
class aesDec {
   public:
    /**
     * @brief Update key before using it to decrypt.
     *
     * @param cipherkey Key to be used in decryption.
     */
    void updateKey(ap_uint<W> cipherkey) {}
    /**
     * @brief Decrypt message using AES algorithm
     *
     * @param ciphertext Cipher text to be decrypted.
     * @param cipherkey Key to be used in decryption.
     * @param plaintext Decryption result.
     */
    void process(ap_uint<128> ciphertext, ap_uint<W> cipherkey, ap_uint<128>& plaintext) {}
};

template <>
class aesDec<256> : public aesTable {
   public:
    ap_uint<128> key_list[16];

    aesDec() {
#pragma HLS inline
#pragma HLS ARRAY_PARTITION variable = key_list complete dim = 1
    }

    void updateKey(ap_uint<256> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<256> lastRound = cipherkey;
        key_list[0] = lastRound.range(127, 0);
        key_list[1] = lastRound.range(255, 128);
        for (ap_uint<5> iter = 2; iter < 15; iter += 2) {
#pragma HLS pipeline II = 1

            ap_uint<32> round_tmp = lastRound.range(255, 224);
            round_tmp = (round_tmp >> 8) | (round_tmp << 24);

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)] ^ Rcon[(iter >> 1) - 1];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            key_list[iter].range(31, 0) = lastRound.range(31, 0) ^ round_tmp;
            key_list[iter].range(63, 32) = lastRound.range(63, 32) ^ key_list[iter].range(31, 0);
            key_list[iter].range(95, 64) = lastRound.range(95, 64) ^ key_list[iter].range(63, 32);
            key_list[iter].range(127, 96) = lastRound.range(127, 96) ^ key_list[iter].range(95, 64);

            ap_uint<32> round_tmp2 = key_list[iter].range(127, 96);

            round_tmp2.range(7, 0) = ssbox[round_tmp2.range(7, 0)];
            round_tmp2.range(15, 8) = ssbox[round_tmp2.range(15, 8)];
            round_tmp2.range(23, 16) = ssbox[round_tmp2.range(23, 16)];
            round_tmp2.range(31, 24) = ssbox[round_tmp2.range(31, 24)];

            key_list[iter + 1].range(31, 0) = lastRound.range(159, 128) ^ round_tmp2;
            key_list[iter + 1].range(63, 32) = lastRound.range(191, 160) ^ key_list[iter + 1].range(31, 0);
            key_list[iter + 1].range(95, 64) = lastRound.range(223, 192) ^ key_list[iter + 1].range(63, 32);
            key_list[iter + 1].range(127, 96) = lastRound.range(255, 224) ^ key_list[iter + 1].range(95, 64);
            lastRound.range(127, 0) = key_list[iter];
            lastRound.range(255, 128) = key_list[iter + 1];
        }
    }

    // the function of Finite field multiplication *2 *4 *8

    static ap_uint<8> GFMul2(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        if (a[7] == 1) {
            a = (a << 1) ^ kModulus;
        } else {
            a <<= 1;
        }

        return a;
    }

    static ap_uint<8> GFMul4(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        for (int i = 0; i < 2; ++i) {
            if (a[7] == 1) {
                a = (a << 1) ^ kModulus;
            } else {
                a <<= 1;
            }
        }

        return a;
    }

    static ap_uint<8> GFMul8(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        for (int i = 0; i < 3; ++i) {
            if (a[7] == 1) {
                a = (a << 1) ^ kModulus;
            } else {
                a <<= 1;
            }
        }

        return a;
    }

    void process(ap_uint<128> ciphertext, ap_uint<256> cipherkey, ap_uint<128>& plaintext) {
        ap_uint<128> state;
        ap_uint<4> round_counter;

        // state init and add roundkey[0]
        state = ciphertext ^ key_list[14];

        // Start 14 rounds of process
        for (round_counter = 1; round_counter <= 14; round_counter++) {
            ap_uint<8> tmp_1, tmp_2_1, tmp_2_2, tmp_3;
            ap_uint<128> state_1;
            ap_uint<8> tmp16_s[4] = {0x0, 0x0, 0x0, 0x0};
            // Inv ShiftRow
            tmp_1 = state(111, 104);
            state(111, 104) = state(79, 72);
            state(79, 72) = state(47, 40);
            state(47, 40) = state(15, 8);
            state(15, 8) = tmp_1;

            tmp_2_1 = state(87, 80);
            state(87, 80) = state(23, 16);
            state(23, 16) = tmp_2_1;

            tmp_2_2 = state(119, 112);
            state(119, 112) = state(55, 48);
            state(55, 48) = tmp_2_2;

            tmp_3 = state(31, 24);
            state(31, 24) = state(63, 56);
            state(63, 56) = state(95, 88);
            state(95, 88) = state(127, 120);
            state(127, 120) = tmp_3;
            // Inv SubByte
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                state(i * 8 + 7, i * 8) = iibox[state(i * 8 + 7, i * 8)];
            }

            // Add Round Key
            state ^= key_list[14 - round_counter];

            // Inverse Mix Column
            for (int i = 0; i < 16; i++) {
                state_1(i * 8 + 7, i * 8) = state(i * 8 + 7, i * 8);
            }
            if (round_counter < 14) {
                // clang-format off
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    state(i * 32 + 7, i * 32) = GFMul8(state_1(i * 32 + 7, i * 32))^GFMul4(state_1(i * 32 + 7, i * 32))^GFMul2(state_1(i * 32 + 7, i * 32))^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8)) ^GFMul2(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8)^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16))^GFMul4(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16)^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 15, i * 32 + 8) = GFMul8(state_1(i * 32 + 15, i * 32 + 8))^GFMul4(state_1(i * 32 + 15, i * 32 + 8))^GFMul2(state_1(i * 32 + 15, i * 32 + 8))^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16)) ^GFMul2(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16)^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24))^GFMul4(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24)^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32);
                    state(i * 32 + 23, i * 32 + 16) = GFMul8(state_1(i * 32 + 23, i * 32 + 16))^GFMul4(state_1(i * 32 + 23, i * 32 + 16))^GFMul2(state_1(i * 32 + 23, i * 32 + 16))^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24)) ^GFMul2(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24)^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^GFMul4(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32)^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8);
                    state(i * 32 + 31, i * 32 + 24) = GFMul8(state_1(i * 32 + 31, i * 32 + 24))^GFMul4(state_1(i * 32 + 31, i * 32 + 24))^GFMul2(state_1(i * 32 + 31, i * 32 + 24))^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^GFMul2(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32)^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8))^GFMul4(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8)^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16);
                }
                // clang-format on
            }
        }
        plaintext = state;
    }
};

template <>
class aesDec<192> : public aesTable {
   public:
    ap_uint<128> key_list[14];

    aesDec() {
#pragma HLS inline
#pragma HLS ARRAY_PARTITION variable = key_list complete dim = 1
    }

    void updateKey(ap_uint<192> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<192> lastRound = cipherkey;
        key_list[0] = lastRound.range(127, 0);
        key_list[1].range(63, 0) = lastRound.range(191, 128);
        ap_uint<4> rIter = 0;

        for (ap_uint<5> iter = 3; iter < 26; iter += 3) {
#pragma HLS pipeline II = 1

            ap_uint<192> thisRound = 0;

            ap_uint<32> round_tmp = lastRound.range(191, 160);
            round_tmp = (round_tmp >> 8) | (round_tmp << 24);

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)] ^ Rcon[rIter++];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            thisRound.range(31, 0) = lastRound.range(31, 0) ^ round_tmp;
            thisRound.range(63, 32) = lastRound.range(63, 32) ^ thisRound.range(31, 0);
            thisRound.range(95, 64) = lastRound.range(95, 64) ^ thisRound.range(63, 32);
            thisRound.range(127, 96) = lastRound.range(127, 96) ^ thisRound.range(95, 64);
            thisRound.range(159, 128) = lastRound.range(159, 128) ^ thisRound.range(127, 96);
            thisRound.range(191, 160) = lastRound.range(191, 160) ^ thisRound.range(159, 128);

            if (iter[0] == 1) {
                key_list[iter.range(4, 1)].range(127, 64) = thisRound.range(63, 0);
                key_list[iter.range(4, 1) + 1].range(127, 0) = thisRound.range(191, 64);
            } else {
                key_list[iter.range(4, 1)].range(127, 0) = thisRound.range(127, 0);
                key_list[iter.range(4, 1) + 1].range(63, 0) = thisRound.range(191, 128);
            }
            lastRound = thisRound;
        }
    }
    // the function of Finite field multiplication *2 *4 *8
    static ap_uint<8> GFMul2(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        if (a[7] == 1) {
            a = (a << 1) ^ kModulus;
        } else {
            a <<= 1;
        }

        return a;
    }

    static ap_uint<8> GFMul4(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        for (int i = 0; i < 2; ++i) {
            if (a[7] == 1) {
                a = (a << 1) ^ kModulus;
            } else {
                a <<= 1;
            }
        }

        return a;
    }

    static ap_uint<8> GFMul8(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        for (int i = 0; i < 3; ++i) {
            if (a[7] == 1) {
                a = (a << 1) ^ kModulus;
            } else {
                a <<= 1;
            }
        }

        return a;
    }

    void process(ap_uint<128> ciphertext, ap_uint<192> cipherkey, ap_uint<128>& plaintext) {
        ap_uint<128> state;
        ap_uint<4> round_counter;

        // state init and add roundkey[0]
        state = ciphertext ^ key_list[12];

        // Start 14 rounds of process
        for (round_counter = 1; round_counter <= 12; round_counter++) {
            ap_uint<8> tmp_1, tmp_2_1, tmp_2_2, tmp_3;
            ap_uint<128> state_1;
            // Inv ShiftRow
            tmp_1 = state(111, 104);
            state(111, 104) = state(79, 72);
            state(79, 72) = state(47, 40);
            state(47, 40) = state(15, 8);
            state(15, 8) = tmp_1;

            tmp_2_1 = state(87, 80);
            state(87, 80) = state(23, 16);
            state(23, 16) = tmp_2_1;

            tmp_2_2 = state(119, 112);
            state(119, 112) = state(55, 48);
            state(55, 48) = tmp_2_2;

            tmp_3 = state(31, 24);
            state(31, 24) = state(63, 56);
            state(63, 56) = state(95, 88);
            state(95, 88) = state(127, 120);
            state(127, 120) = tmp_3;
            // Inv SubByte
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                state(i * 8 + 7, i * 8) = iibox[state(i * 8 + 7, i * 8)];
            }

            // Add Round Key
            state ^= key_list[12 - round_counter];

            // Inverse Mix Column
            for (int i = 0; i < 16; i++) {
                state_1(i * 8 + 7, i * 8) = state(i * 8 + 7, i * 8);
            }
            if (round_counter < 12) {
                // clang-format off
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    state(i * 32 + 7, i * 32) = GFMul8(state_1(i * 32 + 7, i * 32))^GFMul4(state_1(i * 32 + 7, i * 32))^GFMul2(state_1(i * 32 + 7, i * 32))^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8)) ^GFMul2(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8)^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16))^GFMul4(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16)^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 15, i * 32 + 8) = GFMul8(state_1(i * 32 + 15, i * 32 + 8))^GFMul4(state_1(i * 32 + 15, i * 32 + 8))^GFMul2(state_1(i * 32 + 15, i * 32 + 8))^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16)) ^GFMul2(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16)^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24))^GFMul4(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24)^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32);
                    state(i * 32 + 23, i * 32 + 16) = GFMul8(state_1(i * 32 + 23, i * 32 + 16))^GFMul4(state_1(i * 32 + 23, i * 32 + 16))^GFMul2(state_1(i * 32 + 23, i * 32 + 16))^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24)) ^GFMul2(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24)^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^GFMul4(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32)^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8);
                    state(i * 32 + 31, i * 32 + 24) = GFMul8(state_1(i * 32 + 31, i * 32 + 24))^GFMul4(state_1(i * 32 + 31, i * 32 + 24))^GFMul2(state_1(i * 32 + 31, i * 32 + 24))^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^GFMul2(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32)^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8))^GFMul4(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8)^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16);                            
                }
                // clang-format on
            }
        }
        plaintext = state;
    }
};

template <>
class aesDec<128> : public aesTable {
   public:
    ap_uint<128> key_list[11];

    aesDec() {
#pragma HLS inline
#pragma HLS ARRAY_PARTITION variable = key_list complete dim = 1
    }

    void updateKey(ap_uint<128> cipherkey) {
#pragma HLS inline off
        const ap_uint<8> Rcon[10] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};
        ap_uint<128> lastRound = cipherkey;
        key_list[0] = lastRound;
        for (ap_uint<5> iter = 1; iter < 11; iter++) {
#pragma HLS pipeline II = 1

            ap_uint<32> round_tmp = lastRound.range(127, 96);
            round_tmp = (round_tmp >> 8) | (round_tmp << 24);

            round_tmp.range(7, 0) = ssbox[round_tmp.range(7, 0)] ^ Rcon[iter - 1];
            round_tmp.range(15, 8) = ssbox[round_tmp.range(15, 8)];
            round_tmp.range(23, 16) = ssbox[round_tmp.range(23, 16)];
            round_tmp.range(31, 24) = ssbox[round_tmp.range(31, 24)];

            key_list[iter].range(31, 0) = lastRound.range(31, 0) ^ round_tmp;
            key_list[iter].range(63, 32) = lastRound.range(63, 32) ^ key_list[iter].range(31, 0);
            key_list[iter].range(95, 64) = lastRound.range(95, 64) ^ key_list[iter].range(63, 32);
            key_list[iter].range(127, 96) = lastRound.range(127, 96) ^ key_list[iter].range(95, 64);

            lastRound.range(127, 0) = key_list[iter];
        }
    }

    // the function of Finite field multiplication *2 *4 *8

    static ap_uint<8> GFMul2(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        if (a[7] == 1) {
            a = (a << 1) ^ kModulus;
        } else {
            a <<= 1;
        }

        return a;
    }

    static ap_uint<8> GFMul4(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        for (int i = 0; i < 2; ++i) {
            if (a[7] == 1) {
                a = (a << 1) ^ kModulus;
            } else {
                a <<= 1;
            }
        }

        return a;
    }

    static ap_uint<8> GFMul8(ap_uint<8> a) {
        ap_uint<8> kModulus = 0b00011011;

        for (int i = 0; i < 3; ++i) {
            if (a[7] == 1) {
                a = (a << 1) ^ kModulus;
            } else {
                a <<= 1;
            }
        }

        return a;
    }

    void process(ap_uint<128> ciphertext, ap_uint<128> cipherkey, ap_uint<128>& plaintext) {
        ap_uint<128> state;
        ap_uint<4> round_counter;

        // state init and add roundkey[0]
        state = ciphertext ^ key_list[10];

        // Start 10 rounds of process
        for (round_counter = 1; round_counter <= 10; round_counter++) {
            ap_uint<8> tmp_1, tmp_2_1, tmp_2_2, tmp_3;
            ap_uint<128> state_1;
            // Inv ShiftRow
            tmp_1 = state(111, 104);
            state(111, 104) = state(79, 72);
            state(79, 72) = state(47, 40);
            state(47, 40) = state(15, 8);
            state(15, 8) = tmp_1;

            tmp_2_1 = state(87, 80);
            state(87, 80) = state(23, 16);
            state(23, 16) = tmp_2_1;

            tmp_2_2 = state(119, 112);
            state(119, 112) = state(55, 48);
            state(55, 48) = tmp_2_2;

            tmp_3 = state(31, 24);
            state(31, 24) = state(63, 56);
            state(63, 56) = state(95, 88);
            state(95, 88) = state(127, 120);
            state(127, 120) = tmp_3;
            // Inv SubByte
            for (int i = 0; i < 16; i++) {
#pragma HLS unroll
                state(i * 8 + 7, i * 8) = iibox[state(i * 8 + 7, i * 8)];
            }

            // Add Round Key
            state ^= key_list[10 - round_counter];

            // Inverse Mix Column
            for (int i = 0; i < 16; i++) {
                ap_uint<8> tmp = state(i * 8 + 7, i * 8);
                state_1(i * 8 + 7, i * 8) = state(i * 8 + 7, i * 8);
            }
            if (round_counter < 10) {
                // clang-format off
                for (int i = 0; i < 4; i++) {
#pragma HLS unroll
                    state(i * 32 + 7, i * 32) = GFMul8(state_1(i * 32 + 7, i * 32))^GFMul4(state_1(i * 32 + 7, i * 32))^GFMul2(state_1(i * 32 + 7, i * 32))^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8)) ^GFMul2(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8)^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16))^GFMul4(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16)^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24);
                    state(i * 32 + 15, i * 32 + 8) = GFMul8(state_1(i * 32 + 15, i * 32 + 8))^GFMul4(state_1(i * 32 + 15, i * 32 + 8))^GFMul2(state_1(i * 32 + 15, i * 32 + 8))^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16)) ^GFMul2(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16)^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24))^GFMul4(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24)^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32);
                    state(i * 32 + 23, i * 32 + 16) = GFMul8(state_1(i * 32 + 23, i * 32 + 16))^GFMul4(state_1(i * 32 + 23, i * 32 + 16))^GFMul2(state_1(i * 32 + 23, i * 32 + 16))^
                                                GFMul8(state_1(i * 32 + 31, i * 32 + 24)) ^GFMul2(state_1(i * 32 + 31, i * 32 + 24))^state_1(i * 32 + 31, i * 32 + 24)^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^GFMul4(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32)^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8);
                    state(i * 32 + 31, i * 32 + 24) = GFMul8(state_1(i * 32 + 31, i * 32 + 24))^GFMul4(state_1(i * 32 + 31, i * 32 + 24))^GFMul2(state_1(i * 32 + 31, i * 32 + 24))^
                                                GFMul8(state_1(i * 32 + 7, i * 32))^GFMul2(state_1(i * 32 + 7, i * 32))^state_1(i * 32 + 7, i * 32)^
                                                GFMul8(state_1(i * 32 + 15, i * 32 + 8))^GFMul4(state_1(i * 32 + 15, i * 32 + 8))^state_1(i * 32 + 15, i * 32 + 8)^
                                                GFMul8(state_1(i * 32 + 23, i * 32 + 16))^state_1(i * 32 + 23, i * 32 + 16);                            
                }
                // clang-format on
            }
        }
        plaintext = state;
    }
};

} // namespace security
} // namespace xf
#endif
