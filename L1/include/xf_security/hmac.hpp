
/*
 * Copyright 2019 Xilinx, Inc.
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
 * @file hmac.hpp
 * @brief header file for HMAC.
 * This file part of Vitis Security Library.
 * TODO
 * @detail .
 */

#ifndef _XF_SECURITY_HMAC_HPP_
#define _XF_SECURITY_HMAC_HPP_

#include <ap_int.h>
#include <hls_stream.h>
#include <xf_security/types.hpp>

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
#include <iostream>
#endif
namespace xf {
namespace security {

namespace internal {
// typedef ap_uint<64> u64;

/**
 * @brief compute hash value according to specific hash function and input data.
 *
 * @tparam W the width of input stream msgStrm.
 * @tparam hshW the width of output stream hshStrm.
 * @tparam F hash function, iW is its input stream's width and oW is output stream's width.
 *
 * @param msgStrm input stream.
 * @param len the length of input stream.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
template <int W, int lW, int hshW, template <int iW, int ilW, int oW> class F>
void FF(hls::stream<ap_uint<W> >& msgStrm,
        ap_uint<lW> len,
        hls::stream<ap_uint<hshW> >& hshStrm,
        hls::stream<bool>& eHshStrm) {
    //#pragma HLS inline
    hls::stream<ap_uint<lW> > lenStrm;
#pragma HLS stream variable = lenStrm depth = 32
    hls::stream<bool> eLenStrm;
#pragma HLS stream variable = eLenStrm depth = 32
    // hash function asks len in bytes instead of W bits.
    lenStrm.write(len);
    eLenStrm.write(false);
    eLenStrm.write(true);

    F<W, lW, hshW>::hash(msgStrm, lenStrm, eLenStrm, hshStrm, eHshStrm);
}

/**
 * @brief compute hash value according to specific hash function and input data.
 *
 * @tparam W the width of input message.
 * @tparam hshW the width of output stream hshStrm.
 * @tparam F hash function, iW is its input stream's width and oW is output stream's width.
 *
 * @param msg  input meassge.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
// template <int W, int hshW, template <int iW, int oW> class F>
template <int W, int lW, int hshW, template <int iW, int ilW, int oW> class F>
ap_uint<hshW> FF(ap_uint<W> msg, hls::stream<ap_uint<hshW> >& hshStrm, hls::stream<bool>& eHshStrm) {
#pragma HLS inline
    hls::stream<ap_uint<W> > msgStrm;
#pragma HLS stream variable = msgStrm depth = 1
    msgStrm.write(msg);
    ap_uint<lW> c = 1;
    FF<W, lW, hshW, F>(msgStrm, c, hshStrm, eHshStrm);
}

/**
 * @brief compute hmac value according to specific hash function and input data in sequence.
 *
 * @tparam keyW the width of input stream keyStrm.
 * @tparam msgW the width of input stream msgStrm.
 * @tparam hshW the width of output stream hshStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param keyStrm  input key stream.
 * @param keyLen  the length of  input key stream.
 * @param msgStrm  input meassge stream.
 * @param msgLen  the length of  input message stream.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
template <int keyW, int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hmacSqeunce(hls::stream<ap_uint<keyW> >& keyStrm,
                 ap_uint<lW> keyLen,
                 hls::stream<ap_uint<msgW> >& msgStrm,
                 ap_uint<lW> msgLen,
                 hls::stream<ap_uint<hshW> >& hshStrm,
                 hls::stream<bool>& eHshStrm) {
    //#pragma HLS dataflow
    const int tl = blockSize * 8 + hshW;
    hls::stream<ap_uint<keyW> > oneKeyStrm;
#pragma HLS stream variable = oneKeyStrm depth = 128
    hls::stream<bool> eOneKeyStrm;
#pragma HLS stream variable = eOneKeyStrm depth = 128
    hls::stream<ap_uint<msgW> > oneMsgStrm;
#pragma HLS stream variable = oneMsgStrm depth = 128
    hls::stream<bool> eOneMsgStrm;
#pragma HLS stream variable = eOneMsgStrm depth = 128
    hls::stream<ap_uint<msgW> > m2Strm;
#pragma HLS stream variable = m2Strm depth = 128

    hls::stream<ap_uint<hshW> > hshKey;
#pragma HLS stream variable = hshKey depth = 2
    hls::stream<bool> eHshKey;
#pragma HLS stream variable = eHshKey depth = 2
    hls::stream<ap_uint<hshW> > hshMsg;
#pragma HLS stream variable = hshMsg depth = 2
    hls::stream<bool> eHshMsg;
#pragma HLS stream variable = eHshMsg depth = 2
    hls::stream<ap_uint<hshW> > hsh;
#pragma HLS stream variable = hsh depth = 2
    hls::stream<bool> eHsh;
#pragma HLS stream variable = eHsh depth = 2
    ap_uint<lW> kl = keyLen;
    ap_uint<lW> ml = msgLen;
    ap_uint<lW> ckl = (kl * 8 + keyW - 1) / keyW;
    ap_uint<lW> cml = (ml * 8 + msgW - 1) / msgW;
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << "input key:";
#endif
    ap_uint<blockSize* 8> k1 = 0;
    ap_uint<blockSize* 8> kopad = 0;
    ap_uint<blockSize* 8> kipad = 0;
    if (kl <= blockSize) {
        // len(Key) < blockSize, padding 0 on right.
        for (int i = 0; i < ckl; ++i) {
#pragma HLS pipeline II = 1
            ap_uint<keyW> data = keyStrm.read();
            int high = blockSize * 8 - i * keyW - 1;
            int low = blockSize * 8 - (i + 1) * keyW;
            k1.range(high, low) = data;
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
            std::cout << std::hex << data;
#endif
        }
    } else {
        FF<keyW, lW, hshW, F>(keyStrm, kl, hshKey, eHshKey);
        ap_uint<hshW> outHash = hshKey.read();
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << std::hex << "first hash=" << outHash << std::endl;
#endif
        eHshKey.read();
        eHshKey.read();
        for (int i = 0; i < hshW / keyW; ++i) {
#pragma HLS unroll
            int high = blockSize * 8 - i * keyW - 1;
            int low = blockSize * 8 - (i + 1) * keyW;
            int bs = i * keyW;
            ap_uint<keyW> part = outHash.range(bs + keyW - 1, bs);
            k1.range(high, low) = part;
        }
    }

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << std::endl;
#endif
    ap_uint<blockSize* 8> ipad = 0;
    ap_uint<blockSize* 8> opad = 0;
    for (int i = 0; i < blockSize; ++i) {
#pragma HLS unroll
        kopad((i + 1) * 8 - 1, i * 8) = 0x5c ^ k1.range((i + 1) * 8 - 1, i * 8);
        kipad((i + 1) * 8 - 1, i * 8) = 0x36 ^ k1.range((i + 1) * 8 - 1, i * 8);
    }

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << "k1=" << k1 << std::endl;
    std::cout << "kipad=" << kipad << std::endl;
    std::cout << "kopad=" << kopad << std::endl;
#endif

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << "oneMsg" << std::endl;
#endif
    const int len = (blockSize * 8 + msgW - 1) / msgW;
    for (int i = 0; i < len + cml; ++i) {
#pragma HLS pipeline II = 1
        ap_uint<msgW> data = (i < len) ? kipad.range((len - i) * msgW - 1, (len - (1 + i)) * msgW) : msgStrm.read();
        oneMsgStrm.write(data);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << data << std::endl;
#endif
    }

    const int tlen = (tl + msgW - 1) / msgW;
    FF<msgW, lW, hshW, F>(oneMsgStrm, (ml + blockSize), hshMsg, eHshMsg);
    ap_uint<hshW> h2 = hshMsg.read();
    eHshMsg.read();
    eHshMsg.read();
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << "h2=" << h2 << std::endl;
#endif
    ap_uint<tl> m2 = 0;
    for (int i = 0; i < hshW / msgW; ++i) {
#pragma HLS unroll
        m2.range((i + 1) * msgW - 1, i * msgW) = h2.range(hshW - (i * msgW) - 1, hshW - (i + 1) * msgW);
    }

    m2.range(tl - 1, hshW) = kopad;

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << "input m2=";
#endif
    for (int i = 0; i < tlen; ++i) {
#pragma HLS pipeline II = 1
        ap_uint<msgW> data = 0; // m2.range( (1+i)*msgW-1, i*msgW);
        if (tl - (1 + i) * msgW < 0) {
            data = m2.range(tl - (i)*msgW - 1, 0);
        } else
            data = m2.range(tl - (i)*msgW - 1, tl - (1 + i) * msgW);
        m2Strm.write(data);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << data;
#endif
    }
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << std::endl;
#endif
    FF<msgW, lW, hshW, F>(m2Strm, tl / 8, hsh, eHsh);
    ap_uint<hshW> h = hsh.read();
    eHsh.read();
    eHsh.read();
    // ap_uint<hshW> h =FF<msgW,hshW,F>(m2Strm,tl/8);
    hshStrm.write(h);
    eHshStrm.write(false);
}

/**
 * @brief compute kipad and kopad after padding 0 on right and xor operation.
 *
 * @tparam keyW the width of input stream keyStrm.
 * @tparam hshW the width of output stream hshStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param keyStrm  input key stream.
 * @param keyLenStrm  the length stream of  input key stream.
 * @param eLenStrm  the end flag of length stream.
 * @param kipadStrm kipad stream.
 * @param kopadStrm kopad stream.
 * @param eLStrm end flag stream, which is a duplate of eLenStrm.
 *
 */
template <int keyW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void kpad(hls::stream<ap_uint<keyW> >& keyStrm,
          hls::stream<ap_uint<lW> >& keyLenStrm,
          hls::stream<bool>& eLenStrm,
          hls::stream<ap_uint<blockSize * 8> >& kipadStrm,
          hls::stream<ap_uint<blockSize * 8> >& kopadStrm,
          hls::stream<bool>& eLStrm) {
    hls::stream<ap_uint<hshW> > hshKey;
#pragma HLS stream variable = hshKey depth = 2
    hls::stream<bool> eHshKey;
#pragma HLS stream variable = eHshKey depth = 2
    while (!eLenStrm.read()) {
        //#pragma HLS dataflow
        ap_uint<lW> kl = keyLenStrm.read();
        ap_uint<lW> ckl = (kl * 8 + keyW - 1) / keyW;
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << "input key:";
#endif
        ap_uint<blockSize* 8> k1 = 0;
        ap_uint<blockSize* 8> kopad = 0;
        ap_uint<blockSize* 8> kipad = 0;
        if (kl <= blockSize) {
            // len(Key) < blockSize, padding 0 on right.
            for (int i = 0; i < ckl; ++i) {
                //    #pragma HLS pipeline II=1
                ap_uint<keyW> data = keyStrm.read();
                int high = blockSize * 8 - i * keyW - 1;
                int low = blockSize * 8 - (i + 1) * keyW;
                k1.range(high, low) = data;
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
                std::cout << std::hex << data;
#endif
            }
        } else {
            FF<keyW, lW, hshW, F>(keyStrm, kl, hshKey, eHshKey);
            ap_uint<hshW> outHash = hshKey.read();
            eHshKey.read();
            eHshKey.read();
            // ap_uint<hshW> outHash = 0;//FF<keyW, hshW,F>(keyStrm,kl);
            for (int i = 0; i < hshW / keyW; ++i) {
#pragma HLS unroll
                int high = blockSize * 8 - i * keyW - 1;
                int low = blockSize * 8 - (i + 1) * keyW;
                int bs = i * keyW;
                ap_uint<keyW> part = outHash.range(bs + keyW - 1, bs);
                k1.range(high, low) = part;
            }
        }

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << std::endl;
#endif
        ap_uint<blockSize* 8> ipad = 0;
        ap_uint<blockSize* 8> opad = 0;
        for (int i = 0; i < blockSize; ++i) {
#pragma HLS unroll
            kopad((i + 1) * 8 - 1, i * 8) = 0x5c ^ k1.range((i + 1) * 8 - 1, i * 8);
            kipad((i + 1) * 8 - 1, i * 8) = 0x36 ^ k1.range((i + 1) * 8 - 1, i * 8);
        }
        kipadStrm.write(kipad);
        kopadStrm.write(kopad);
        eLStrm.write(false);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << "k1=" << k1 << std::endl;
        std::cout << "kipad=" << kipad << std::endl;
        std::cout << "kopad=" << kopad << std::endl;
#endif
    }
    eLStrm.write(true);
}

/**
 * @brief compute hash(kipad and msg).
 *
 * @tparam msgW the width of input stream msgStrm.
 * @tparam hshW the width of output stream hshStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param kipadStrm  input kipad stream.
 * @param msgStrm  input message stream.
 * @param msgLenStrm  the length stream of  input msg stream.
 * @param eLenStrm  the end flag of length stream.
 * @param mHshStrm the hash value stream.
 * @param eMHshStrm the end flag of hash value stream.
 *
 */
template <int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hashKeyMsg(hls::stream<ap_uint<blockSize * 8> >& kipadStrm,
                hls::stream<ap_uint<msgW> >& msgStrm,
                hls::stream<ap_uint<lW> >& msgLenStrm,
                hls::stream<bool>& eLenStrm,
                hls::stream<ap_uint<hshW> >& mHshStrm,
                hls::stream<bool>& eMHshStrm) {
    hls::stream<ap_uint<hshW> > hshMsg;
#pragma HLS stream variable = hshMsg depth = 2
    hls::stream<bool> eHshMsg;
#pragma HLS stream variable = eHshMsg depth = 2
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << "oneMsg" << std::endl;
#endif

    hls::stream<ap_uint<msgW> > oneMsgStrm;
#pragma HLS stream variable = oneMsgStrm depth = 128
    const int len = (blockSize * 8 + msgW - 1) / msgW;
    while (!eLenStrm.read()) {
        ap_uint<lW> ml = msgLenStrm.read();
        ap_uint<lW> cml = (ml * 8 + msgW - 1) / msgW;
//#pragma HLS dataflow
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << "input key:";
#endif
        ap_uint<blockSize* 8> k1 = 0;
        ap_uint<blockSize* 8> kipad = kipadStrm.read();
        for (int i = 0; i < len + cml; ++i) {
            //  #pragma HLS pipeline II=1
            ap_uint<msgW> data = (i < len) ? kipad.range((len - i) * msgW - 1, (len - (1 + i)) * msgW) : msgStrm.read();
            oneMsgStrm.write(data);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
            std::cout << data << std::endl;
#endif
        }

        FF<msgW, lW, hshW, F>(oneMsgStrm, (ml + blockSize), hshMsg, eHshMsg);
        ap_uint<hshW> h2 = hshMsg.read();
        eHshMsg.read();
        eHshMsg.read();
// the following state will lead to cosim  hanging.
//   ap_uint<hshW> h2 = FF<msgW,hshW,F>(oneMsgStrm,(ml+blockSize));
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << "h2=" << h2 << std::endl;
#endif
        mHshStrm.write(h2);
        eMHshStrm.write(false);
    }
    eMHshStrm.write(true);
}

/**
 * @brief compute hash(kopad and lastHash).
 *
 * @tparam msgW the width of input stream msgStrm.
 * @tparam hshW the width of output stream hshStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param kopadStrm  input kopad stream.
 * @param mHshStrm  input last hash value stream.
 * @param eMHshStrm the end flag of mHshStrm.
 * @param hshStrm the hash value stream.
 * @param eHshStrm the end flag of hash value stream.
 *
 */
template <int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hashKopadHsh(hls::stream<ap_uint<blockSize * 8> >& kopadStrm,
                  hls::stream<ap_uint<hshW> >& mHshStrm,
                  hls::stream<bool>& eMHshStrm,
                  hls::stream<ap_uint<hshW> >& hshStrm,
                  hls::stream<bool>& eHshStrm) {
    hls::stream<ap_uint<hshW> > hsh;
#pragma HLS stream variable = hsh depth = 2
    hls::stream<bool> eHsh;
#pragma HLS stream variable = eHsh depth = 2
    const int tl = blockSize * 8 + hshW;
    const int tlen = (tl + msgW - 1) / msgW;
    hls::stream<ap_uint<msgW> > m2Strm;
#pragma HLS stream variable = m2Strm depth = 32
    while (!eMHshStrm.read()) {
        ap_uint<blockSize* 8> kopad = kopadStrm.read();
        ap_uint<hshW> h2 = mHshStrm.read();
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << "h2=" << h2 << std::endl;
#endif
        ap_uint<tl> m2 = 0;
        for (int i = 0; i < hshW / msgW; ++i) {
#pragma HLS unroll
            m2.range((i + 1) * msgW - 1, i * msgW) = h2.range(hshW - (i * msgW) - 1, hshW - (i + 1) * msgW);
        }

        m2.range(tl - 1, hshW) = kopad;

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << "input m2=";
#endif
        for (int i = 0; i < tlen; ++i) {
            //#pragma HLS pipeline II=1
            ap_uint<msgW> data = 0; // m2.range( (1+i)*msgW-1, i*msgW);
            if (tl - (1 + i) * msgW < 0) {
                data = m2.range(tl - (i)*msgW - 1, 0);
            } else
                data = m2.range(tl - (i)*msgW - 1, tl - (1 + i) * msgW);
            m2Strm.write(data);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
            std::cout << data;
#endif
        }
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << std::endl;
#endif
        FF<msgW, lW, hshW, F>(m2Strm, tl / 8, hsh, eHsh);
        ap_uint<hshW> h = hsh.read();
        eHsh.read();
        eHsh.read();
        hshStrm.write(h);
        eHshStrm.write(false);
    } // while
    eHshStrm.write(true);
}

/**
 * @brief compute hmac value according to specific hash function and input data in sequence.
 *
 * @tparam keyW the width of input stream keyStrm.
 * @tparam msgW the width of input stream msgStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam hshW the width of output stream hshStrm.
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param keyStrm  input key stream.
 * @param keyLenStrm  the length stream of  input key stream.
 * @param msgStrm  input meassge stream.
 * @param msgLenStrm  the length stream of  input message stream.
 * @param eLenStrm  the end flag of length stream.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
template <int keyW, int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hmacSqeunce(hls::stream<ap_uint<keyW> >& keyStrm,
                 hls::stream<ap_uint<lW> >& keyLenStrm,
                 hls::stream<ap_uint<msgW> >& msgStrm,
                 hls::stream<ap_uint<lW> >& msgLenStrm,
                 hls::stream<bool>& eLenStrm,
                 hls::stream<ap_uint<hshW> >& hshStrm,
                 hls::stream<bool>& eHshStrm) {
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    ap_uint<lW> cntml = 0;
    ap_uint<lW> cntkl = 0;
#endif
    while (!eLenStrm.read()) {
        //#pragma HLS dataflow
        ap_uint<lW> kl = keyLenStrm.read();
        ap_uint<lW> ml = msgLenStrm.read();
        ap_uint<lW> ckl = (kl * 8 + keyW - 1) / keyW;
        ap_uint<lW> cml = (ml * 8 + msgW - 1) / msgW;
        hmacSqeunce<keyW, msgW, lW, hshW, blockSize, F>(keyStrm, kl, msgStrm, ml, hshStrm, eHshStrm);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        cntml += ml;
        cntkl += kl;
#endif
    }
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << std::dec << "cntml=" << cntml << std::endl;
    std::cout << std::dec << "cntkl=" << cntkl << std::endl;
#endif
    eHshStrm.write(true);
}

/**
 * @brief compute hmac value according to specific hash function and input data in parallel.
 *
 * @tparam keyW the width of input stream keyStrm.
 * @tparam msgW the width of input stream msgStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam hshW the width of output stream hshStrm.
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param keyStrm  input key stream.
 * @param keyLenStrm  the length stream of  input key stream, in bytes not keyW-bit.
 * @param msgStrm  input meassge stream.
 * @param msgLenStrm  the length stream of  input message stream, in bytes not msgW-bit.
 * @param eLenStrm  the end flag of length stream.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
template <int keyW, int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hmacDataflow(hls::stream<ap_uint<keyW> >& keyStrm,
                  hls::stream<ap_uint<lW> >& keyLenStrm,
                  hls::stream<ap_uint<msgW> >& msgStrm,
                  hls::stream<ap_uint<lW> >& msgLenStrm,
                  hls::stream<bool>& eLenStrm,
                  hls::stream<ap_uint<hshW> >& hshStrm,
                  hls::stream<bool>& eHshStrm) {
/*
 *                      +-----------------+
 * keyStrm    --------->|    key to       |
 * keyLenStrm --------->| kipad and kopad |---------+ kopad
 *               +----->|                 |         |        +---------------+
 *               |      +-----------------+         |        |               |
 *               |              |                   +------> | hash(         |
 * eLenStrm   ----              | kipad                      |  kopad + mhsh |----> hmac
 *               |             \|/                  +------> |    )          |
 *               |      +-----------------+         |        +---------------+
 *               +----->|  hash(          |         |
 * msgStrm    --------->|   kipad+msg     |-------->  mhsh
 * msgLenStrm --------->|      )          |
 *                      +-----------------+
 *
 * eLenStrm is shared to keyLenStrm and msgLenStrm
 *
 *
 */

#pragma HLS dataflow
    hls::stream<ap_uint<blockSize * 8> > kipad;
#pragma HLS stream variable = kipad depth = 32
    hls::stream<ap_uint<blockSize * 8> > kopad;
#pragma HLS stream variable = kopad depth = 32
    hls::stream<bool> eLen;
#pragma HLS stream variable = eLen depth = 32
    hls::stream<ap_uint<hshW> > mhsh;
#pragma HLS stream variable = mhsh depth = 32
    hls::stream<bool> eMHsh;
#pragma HLS stream variable = eMHsh depth = 32

    kpad<keyW, lW, hshW, blockSize, F>(keyStrm, keyLenStrm, eLenStrm, kipad, kopad, eLen);

    hashKeyMsg<msgW, lW, hshW, blockSize, F>(kipad, msgStrm, msgLenStrm, eLen, mhsh, eMHsh);

    hashKopadHsh<msgW, lW, hshW, blockSize, F>(kopad, mhsh, eMHsh, hshStrm, eHshStrm);
}

/**
 * @brief compute hmac value according to specific hash function and input data.
 *
 * @tparam keyW the width of input stream keyStrm.
 * @tparam msgW the width of input stream msgStrm.
 * @tparam blockSize  the block size(bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam hshW the width of output stream hshStrm.
 * @tparam F hash function, iW is its input stream's  width and oW is output stream's width.
 *
 * @param keyStrm  input key stream.
 * @param keyLenStrm  the length stream of  input key stream.
 * @param msgStrm  input meassge stream.
 * @param msgLenStrm  the length stream of  input message stream.
 * @param eLenStrm  the end flag of length stream.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
template <int keyW, int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hmacImp(hls::stream<ap_uint<keyW> >& keyStrm,
             hls::stream<ap_uint<lW> >& keyLenStrm,
             hls::stream<ap_uint<msgW> >& msgStrm,
             hls::stream<ap_uint<lW> >& msgLenStrm,
             hls::stream<bool>& eLenStrm,
             hls::stream<ap_uint<hshW> >& hshStrm,
             hls::stream<bool>& eHshStrm) {
#ifndef XF_SECURITY_DECRYPT_HMAC_DATAFLOW
    hmacSqeunce<keyW, msgW, lW, hshW, blockSize, F>(keyStrm, keyLenStrm, msgStrm, msgLenStrm, eLenStrm, hshStrm,
                                                    eHshStrm);
#else
    hmacDataflow<keyW, msgW, lW, hshW, blockSize, F>(keyStrm, keyLenStrm, msgStrm, msgLenStrm, eLenStrm, hshStrm,
                                                     eHshStrm);
#endif
}

} // end of namespace internal

/**
 * @brief Compute HMAC value according to specified hash function and input data.
 *
 *  keyW, keyStrm, keyLenStrm, msgW, msgStrm, and msgLenStrm would be used as
 *  parameters or input for the hash function, so they need to align with the API
 *  of the hash function.
 *
 *  Hash function is wrapped to a template struct which must have a static function named `hash`.
 *
 *  Take md5 for example::
 *
 *  template <int msgW, int lW, int hshW>
 *  struct md5_wrapper {
 *      static void hash(hls::stream<ap_uint<msgW> >& msgStrm,
 *                       hls::stream<lW>& lenStrm,
 *                       hls::stream<bool>& eLenStrm,
 *                       hls::stream<ap_uint<hshW> >& hshStrm,
 *                       hls::stream<bool>& eHshStrm) {
 *          xf::security::md5(msgStrm, lenStrm, eLenStrm, hshStrm, eHshStrm);
 *      }
 *  };
 *
 *  then use hmac like this,
 *
 *   xf::security::hmac<32, 32, 64, 128, 64, md5_wrapper>(...);
 *
 * @tparam keyW the width of input stream keyStrm.
 * @tparam msgW the width of input stream msgStrm.
 * @tparam blockSize  the block size (in bytes) of the underlying hash function (e.g. 64 bytes for md5 and SHA-1).
 * @tparam hshW the width of output stream hshStrm.
 * @tparam F a wrapper of hash function which must have a static fucntion named `hash`.
 *
 * @param keyStrm  input key stream.
 * @param keyLenStrm  the length stream of input key stream. The length is counted in bytes, not in keyW bits.
 *  That is to say, hmac reads keyLen*8 bits(not keyLen*keyW bits) from keyStrm as a key.
 *  For example, if a key contains 3 bytes and keyW=32, then keyLen=3.
 *  The lowest 3 bytes in the data which is read from keyStrm are key and the highest one byte is discarded.
 * @param msgStrm  input meassge stream.
 * @param msgLenStrm  the length stream of input message stream.
 * @param eLenStrm  the end flag of length stream.
 * @param hshStrm output stream.
 * @param eHshStrm end flag of output stream hshStrm.
 *
 */
template <int keyW, int msgW, int lW, int hshW, int blockSize, template <int iW, int ilW, int oW> class F>
void hmac(hls::stream<ap_uint<keyW> >& keyStrm,
          hls::stream<ap_uint<lW> >& keyLenStrm,
          hls::stream<ap_uint<msgW> >& msgStrm,
          hls::stream<ap_uint<lW> >& msgLenStrm,
          hls::stream<bool>& eLenStrm,
          hls::stream<ap_uint<hshW> >& hshStrm,
          hls::stream<bool>& eHshStrm) {
    internal::hmacImp<keyW, msgW, lW, hshW, blockSize, F>(keyStrm, keyLenStrm, msgStrm, msgLenStrm, eLenStrm, hshStrm,
                                                          eHshStrm);
}
} // end of namespace security
} // end of namespace xf

#endif // _XF_SECURITY_HMAC_HPP_
