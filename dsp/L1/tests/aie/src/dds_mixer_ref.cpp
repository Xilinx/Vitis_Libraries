/*
 * Copyright 2021 Xilinx, Inc.
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
/*
DDS reference model
*/

#include "dds_mixer_ref.hpp"
#include "fir_ref_utils.hpp"

//#include "debug_utils.h"
#include <math.h>

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

// REF DDS function
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE>
void dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE>::ddsMix(input_window<TT_DATA>* inWindowA,
                                                                          input_window<TT_DATA>* inWindowB,
                                                                          output_window<TT_DATA>* outWindow) {
    //   const unsigned int shift = TP_SHIFT;
    //   T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    TT_DATA d_in2;
    TT_DATA ddsOut;
    TT_DATA mixerMult;
    TT_DATA ddsMixerOut;

    unsigned int phaseUpdate = m_samplePhaseInc;
    double cos_out;
    double sin_out;
    double angle_rads;
    double angle_deg;
    double max;
    int angle_inc;
    double pi = 3.14159265;
    int scaleDds = 32768;        // multiplier to scale dds output
    int scaleMix = scaleDds * 2; // divider to scale mixer output after multiply operation
    int scaleMac = scaleDds * 2; // divider to scale MAC output after multiply operation

    double dds_real, dds_imag;
    double dds_cc_real, dds_cc_imag; // complex congugate
    double mixer_real, mixer_imag;
    double mac_real, mac_imag;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        // Accumulation
        angle_rads = ((phase_update_accum * pi) / pow(2, 31));
        angle_deg = angle_rads * 180 / pi;
        cos_out = cos(angle_rads); // angle in radians
        sin_out = sin(angle_rads); // angle in radians
        // update phase_accum for next sample
        phase_update_accum = phase_update_accum + (phaseUpdate); // accumulate phase over multiple input windows of data

        // update ddsOut reg with calculated dds output  ( ddsOut = acc0a )
        ddsOut.real = round(scaleDds * cos_out);
        ddsOut.imag = round(scaleDds * sin_out);

        // cater to avoid +32768 mapping to -32768
        if (cos_out == 1) {
            dds_real = floor((scaleDds - 1) * cos_out);
        } else {
            dds_real = floor(scaleDds * cos_out);
        }
        if (sin_out == 1) {
            dds_imag = floor((scaleDds - 1) * sin_out);
        } else {
            dds_imag = floor(scaleDds * sin_out);
        }

        // complex congugate of a+jb  is a-jb  , i.e. reverse sign of imag part
        dds_cc_real = dds_real;
        dds_cc_imag = (-1) * dds_imag;

        d_in = window_readincr(inWindowA);
        d_in2 = window_readincr(inWindowB);

        //  complex multiply
        //    (a+jb)(c+jd)
        //  = (ac-bd) +j(ad+bc)

        mixer_real = round(((d_in.real * dds_real) - (d_in.imag * dds_imag)) / scaleMix);
        mixer_imag = round(((d_in.real * dds_imag) + (d_in.imag * dds_real)) / scaleMix);

        // mixerMult = acc1, 1st time  -- line 225
        mixerMult.real = mixer_real;
        mixerMult.imag = mixer_imag;

        //  complex multiply _cc
        //    (a-jb)(c-jd)
        //  = (ac+bd) -j(ad+bc)
        // MAC function
        // Add input0 mixer value,ie mixerMult_real/imag, to be added to input2 mixer value
        mac_real = mixer_real + round(((d_in2.real * dds_cc_real) - (d_in2.imag * dds_cc_imag)) / scaleMac);
        mac_imag = mixer_imag + round(((d_in2.real * dds_cc_imag) + (d_in2.imag * dds_cc_real)) / scaleMac);

        //   ddsMixerOut.real = dds_cc_real ;  // equivalent to acc0b  = dds_cc output
        //   ddsMixerOut.imag = dds_cc_imag ;  // equivalent to acc0b  = dds_cc output
        //   ddsMixerOut.real = dds_real ;  // equivalent to acc0a  = dds output
        //   ddsMixerOut.imag = dds_imag ;  // equivalent to acc0a  = dds output
        ddsMixerOut.real = mac_real; // equivalent to mac8 operation for acc1
        ddsMixerOut.imag = mac_imag; // equivalent to mac8 operation for acc1

        // write single dds raf sample to output window
        // window_writeincr((output_window<TT_DATA> *)outWindow, mixerMult);
        window_writeincr((output_window<TT_DATA>*)outWindow, ddsMixerOut);
    }
    printf(" TP_MIXER_MODE = %d \n", TP_MIXER_MODE);
    printf(" DDS_MIXER_REF_GEN FINISHED \n");
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1
//===============

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
void dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1>::ddsMix(input_window<TT_DATA>* inWindowA,
                                                                         output_window<TT_DATA>* outWindow) {
    //   const unsigned int shift = TP_SHIFT;
    //   T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    TT_DATA d_in2;
    TT_DATA ddsOut;
    TT_DATA mixerMult;
    TT_DATA ddsMixerOut;

    unsigned int phaseUpdate = m_samplePhaseInc;
    double cos_out;
    double sin_out;
    double angle_rads;
    double angle_deg;
    double max;
    int angle_inc;
    double pi = 3.14159265;
    int scaleDds = 32768;        // multiplier to scale dds output
    int scaleMix = scaleDds * 2; // divider to scale mixer output after multiply operation
    int scaleMac = scaleDds * 2; // divider to scale MAC output after multiply operation

    double dds_real, dds_imag;
    double dds_cc_real, dds_cc_imag; // complex congugate
    double mixer_real, mixer_imag;
    double mac_real, mac_imag;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        // Accumulation
        angle_rads = ((phase_update_accum * pi) / pow(2, 31));
        angle_deg = angle_rads * 180 / pi;
        cos_out = cos(angle_rads); // angle in radians
        sin_out = sin(angle_rads); // angle in radians
        // update phase_accum for next sample
        phase_update_accum = phase_update_accum + (phaseUpdate); // accumulate phase over multiple input windows of data

        // update ddsOut reg with calculated dds output  ( ddsOut = acc0a )
        ddsOut.real = round(scaleDds * cos_out);
        ddsOut.imag = round(scaleDds * sin_out);

        // cater to avoid +32768 mapping to -32768
        if (cos_out == 1) {
            dds_real = floor((scaleDds - 1) * cos_out);
        } else {
            dds_real = floor(scaleDds * cos_out);
        }
        if (sin_out == 1) {
            dds_imag = floor((scaleDds - 1) * sin_out);
        } else {
            dds_imag = floor(scaleDds * sin_out);
        }

        // complex congugate of a+jb  is a-jb  , i.e. reverse sign of imag part
        dds_cc_real = dds_real;
        dds_cc_imag = (-1) * dds_imag;

        // use input windows as needed for each mixer mode
        d_in = window_readincr(inWindowA);

        //  complex multiply
        //    (a+jb)(c+jd)
        //  = (ac-bd) +j(ad+bc)

        mixer_real = round(((d_in.real * dds_real) - (d_in.imag * dds_imag)) / scaleMix);
        mixer_imag = round(((d_in.real * dds_imag) + (d_in.imag * dds_real)) / scaleMix);

        // mixerMult = acc1, 1st time  -- line 225
        mixerMult.real = mixer_real;
        mixerMult.imag = mixer_imag;

        //  complex multiply _cc
        //    (a-jb)(c-jd)
        //  = (ac+bd) -j(ad+bc)
        // MAC function
        // Add input0 mixer value,ie mixerMult_real/imag, to be added to input2 mixer value
        mac_real = mixer_real + round(((d_in2.real * dds_cc_real) - (d_in2.imag * dds_cc_imag)) / scaleMac);
        mac_imag = mixer_imag + round(((d_in2.real * dds_cc_imag) + (d_in2.imag * dds_cc_real)) / scaleMac);

        //   ddsMixerOut.real = dds_cc_real ;  // equivalent to acc0b  = dds_cc output
        //   ddsMixerOut.imag = dds_cc_imag ;  // equivalent to acc0b  = dds_cc output
        //   ddsMixerOut.real = dds_real ;  // equivalent to acc0a  = dds output
        //   ddsMixerOut.imag = dds_imag ;  // equivalent to acc0a  = dds output
        ddsMixerOut.real = mixer_real; // equivalent to mixer 1 data port operation for acc1
        ddsMixerOut.imag = mixer_imag; // equivalent to mixer 1 data port operation for acc1
        //   ddsMixerOut.real = mac_real ;  // equivalent to mixer 2 data ports (mac8) operation for acc1
        //   ddsMixerOut.imag = mac_imag ;  // equivalent to mixer 2 data ports (mac8) operation for acc1

        // write single dds raf sample to output window
        // window_writeincr((output_window<TT_DATA> *)outWindow, mixerMult);
        window_writeincr((output_window<TT_DATA>*)outWindow, ddsMixerOut);
    }
    //    printf(" TP_MIXER_MODE = %d \n", TP_MIXER_MODE) ;
    printf(" DDS_MIXER_REF_GEN FINISHED FOR MIXER MODE 1 \n");
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0
//===============

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
void dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0>::ddsMix(output_window<TT_DATA>* outWindow) {
    //   const unsigned int shift = TP_SHIFT;
    //   T_accRef<TT_DATA> accum;
    TT_DATA d_in;
    TT_DATA d_in2;
    TT_DATA ddsOut;
    TT_DATA mixerMult;
    TT_DATA ddsMixerOut;

    unsigned int phaseUpdate = m_samplePhaseInc;
    double cos_out;
    double sin_out;
    double angle_rads;
    double angle_deg;
    double max;
    int angle_inc;
    double pi = 3.14159265;
    int scaleDds = 32768;        // multiplier to scale dds output
    int scaleMix = scaleDds * 2; // divider to scale mixer output after multiply operation
    int scaleMac = scaleDds * 2; // divider to scale MAC output after multiply operation

    double dds_real, dds_imag;
    double dds_cc_real, dds_cc_imag; // complex congugate
    double mixer_real, mixer_imag;
    double mac_real, mac_imag;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE; i++) {
        // Accumulation
        angle_rads = ((phase_update_accum * pi) / pow(2, 31));
        angle_deg = angle_rads * 180 / pi;
        cos_out = cos(angle_rads); // angle in radians
        sin_out = sin(angle_rads); // angle in radians
        // update phase_accum for next sample
        phase_update_accum = phase_update_accum + (phaseUpdate); // accumulate phase over multiple input windows of data

        // update ddsOut reg with calculated dds output  ( ddsOut = acc0a )
        ddsOut.real = round(scaleDds * cos_out);
        ddsOut.imag = round(scaleDds * sin_out);

        // cater to avoid +32768 mapping to -32768
        if (cos_out == 1) {
            dds_real = floor((scaleDds - 1) * cos_out);
        } else {
            dds_real = floor(scaleDds * cos_out);
        }
        if (sin_out == 1) {
            dds_imag = floor((scaleDds - 1) * sin_out);
        } else {
            dds_imag = floor(scaleDds * sin_out);
        }

        // complex congugate of a+jb  is a-jb  , i.e. reverse sign of imag part
        dds_cc_real = dds_real;
        dds_cc_imag = (-1) * dds_imag;

        //  complex multiply
        //    (a+jb)(c+jd)
        //  = (ac-bd) +j(ad+bc)

        mixer_real = round(((d_in.real * dds_real) - (d_in.imag * dds_imag)) / scaleMix);
        mixer_imag = round(((d_in.real * dds_imag) + (d_in.imag * dds_real)) / scaleMix);

        // mixerMult = acc1, 1st time  -- line 225
        mixerMult.real = mixer_real;
        mixerMult.imag = mixer_imag;

        //  complex multiply _cc
        //    (a-jb)(c-jd)
        //  = (ac+bd) -j(ad+bc)
        // MAC function
        // Add input0 mixer value,ie mixerMult_real/imag, to be added to input2 mixer value
        mac_real = mixer_real + round(((d_in2.real * dds_cc_real) - (d_in2.imag * dds_cc_imag)) / scaleMac);
        mac_imag = mixer_imag + round(((d_in2.real * dds_cc_imag) + (d_in2.imag * dds_cc_real)) / scaleMac);

        //   ddsMixerOut.real = dds_cc_real ;  // equivalent to acc0b  = dds_cc output
        //   ddsMixerOut.imag = dds_cc_imag ;  // equivalent to acc0b  = dds_cc output
        ddsMixerOut.real = dds_real; // equivalent to acc0a  = dds output
        ddsMixerOut.imag = dds_imag; // equivalent to acc0a  = dds output
        //   ddsMixerOut.real = mixer_real ;  // equivalent to mixer 1 data port operation for acc1
        //   ddsMixerOut.imag = mixer_imag ;  // equivalent to mixer 1 data port operation for acc1
        //   ddsMixerOut.real = mac_real ;  // equivalent to mixer 2 data ports (mac8) operation for acc1
        //   ddsMixerOut.imag = mac_imag ;  // equivalent to mixer 2 data ports (mac8) operation for acc1

        // write single dds raf sample to output window
        // window_writeincr((output_window<TT_DATA> *)outWindow, mixerMult);
        window_writeincr((output_window<TT_DATA>*)outWindow, ddsMixerOut);
    }
    //     printf(" TP_MIXER_MODE = %d \n", TP_MIXER_MODE) ;
    printf(" DDS_MIXER_REF_GEN FINISHED FOR MIXER MODE 0 \n");
};
}
}
}
}
}
