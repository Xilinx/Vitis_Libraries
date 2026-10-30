/*
Copyright (C) 2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#include "../../../IP_FOC/src/foc_demo.hpp"
#include "../../../IP_FOC/src/test_foc_modes.hpp"
#include "../../../IP_FOC_Multiple/src/ip_foc_multiple.hpp"
#include "../../../../include/models_fp/foc_fp32_sync.hpp"

using namespace xf::motorcontrol;

#ifndef __SYNTHESIS__
RangeTracer ranger;
#endif

struct Fp32Registers {
    int ppr;
    int mode;
    int fixperiod;
    float flux_sp;
    float flux_kp;
    float flux_ki;
    float flux_kd;
    float torque_sp;
    float torque_kp;
    float torque_ki;
    float torque_kd;
    float speed_sp;
    float speed_kp;
    float speed_ki;
    float speed_kd;
    int angle_shift;
    float vd;
    float vq;
    float fw_kp;
    float fw_ki;
    float id;
    float flux_acc;
    float flux_err;
    float flux_out;
    float iq;
    float torque_acc;
    float torque_err;
    float torque_out;
    float speed;
    float speed_acc;
    float speed_err;
    float speed_out;
    int angle;
    float va_stts;
    float vb_stts;
    float vc_stts;
    float ialpha;
    float ibeta;
    float ihomopolar;
    int fixed_angle;
};

#define FP32_ARGS(r)                                                                                             \
    (r).ppr, (r).mode, (r).fixperiod, (r).flux_sp, (r).flux_kp, (r).flux_ki, (r).flux_kd, (r).torque_sp,         \
        (r).torque_kp, (r).torque_ki, (r).torque_kd, (r).speed_sp, (r).speed_kp, (r).speed_ki, (r).speed_kd,     \
        (r).angle_shift, (r).vd, (r).vq, (r).fw_kp, (r).fw_ki, (r).id, (r).flux_acc, (r).flux_err, (r).flux_out, \
        (r).iq, (r).torque_acc, (r).torque_err, (r).torque_out, (r).speed, (r).speed_acc, (r).speed_err,         \
        (r).speed_out, (r).angle, (r).va_stts, (r).vb_stts, (r).vc_stts, (r).ialpha, (r).ibeta, (r).ihomopolar,  \
        (r).fixed_angle

#define AP_FIXED_ARGS(a)                                                                                          \
    (volatile int&)(a).ppr_args, (volatile int&)(a).control_mode_args, (volatile int&)(a).control_fixperiod_args, \
        (volatile int&)(a).flux_sp_args, (volatile int&)(a).flux_kp_args, (volatile int&)(a).flux_ki_args,        \
        (volatile int&)(a).flux_kd_args, (volatile int&)(a).torque_sp_args, (volatile int&)(a).torque_kp_args,    \
        (volatile int&)(a).torque_ki_args, (volatile int&)(a).torque_kd_args, (volatile int&)(a).speed_sp_args,   \
        (volatile int&)(a).speed_kp_args, (volatile int&)(a).speed_ki_args, (volatile int&)(a).speed_kd_args,     \
        (volatile int&)(a).angle_sh_args, (volatile int&)(a).vd_args, (volatile int&)(a).vq_args,                 \
        (volatile int&)(a).fw_kp_args, (volatile int&)(a).fw_ki_args, (volatile int&)(a).id_stts,                 \
        (volatile int&)(a).flux_acc_stts, (volatile int&)(a).flux_err_stts, (volatile int&)(a).flux_out_stts,     \
        (volatile int&)(a).iq_stts, (volatile int&)(a).torque_acc_stts, (volatile int&)(a).torque_err_stts,       \
        (volatile int&)(a).torque_out_stts, (volatile int&)(a).speed_stts, (volatile int&)(a).speed_acc_stts,     \
        (volatile int&)(a).speed_err_stts, (volatile int&)(a).speed_out_stts, (volatile int&)(a).angle_stts,      \
        (volatile int&)(a).Va_cmd_stts, (volatile int&)(a).Vb_cmd_stts, (volatile int&)(a).Vc_cmd_stts,           \
        (volatile int&)(a).Ialpha_stts, (volatile int&)(a).Ibeta_stts, (volatile int&)(a).Ihomopolar_stts,        \
        (volatile int&)(a).fixed_angle_args

void hls_foc_periodic_fp32(::hls::stream<float>& Ia,
                           ::hls::stream<float>& Ib,
                           ::hls::stream<float>& Ic,
                           ::hls::stream<int>& rpm_theta,
                           ::hls::stream<int>& motor_id,
                           ::hls::stream<float>& Va,
                           ::hls::stream<float>& Vb,
                           ::hls::stream<float>& Vc,
                           ::hls::stream<int>& motor_id_out,
                           volatile int& ppr,
                           volatile int& mode,
                           volatile int& fixperiod,
                           volatile float& flux_sp,
                           volatile float& flux_kp,
                           volatile float& flux_ki,
                           volatile float& flux_kd,
                           volatile float& torque_sp,
                           volatile float& torque_kp,
                           volatile float& torque_ki,
                           volatile float& torque_kd,
                           volatile float& speed_sp,
                           volatile float& speed_kp,
                           volatile float& speed_ki,
                           volatile float& speed_kd,
                           volatile int& angle_shift,
                           volatile float& vd,
                           volatile float& vq,
                           volatile float& fw_kp,
                           volatile float& fw_ki,
                           volatile float& id,
                           volatile float& flux_acc,
                           volatile float& flux_err,
                           volatile float& flux_out,
                           volatile float& iq,
                           volatile float& torque_acc,
                           volatile float& torque_err,
                           volatile float& torque_out,
                           volatile float& speed,
                           volatile float& speed_acc,
                           volatile float& speed_err,
                           volatile float& speed_out,
                           volatile int& angle,
                           volatile float& va_stts,
                           volatile float& vb_stts,
                           volatile float& vc_stts,
                           volatile float& ialpha,
                           volatile float& ibeta,
                           volatile float& ihomopolar,
                           volatile int& fixed_angle);

struct Sample {
    float ia;
    float ib;
    float ic;
    int speed_theta;
    float va;
    float vb;
    float vc;
};

struct ErrorStats {
    double sum_sq = 0;
    double sum_abs = 0;
    float max_abs = 0;
    int max_sample = 0;
    int count = 0;

    void add(float error, int sample) {
        const float abs_error = std::fabs(error);
        sum_abs += abs_error;
        sum_sq += static_cast<double>(error) * error;
        if (abs_error > max_abs) {
            max_abs = abs_error;
            max_sample = sample;
        }
        ++count;
    }

    void print(const char* label) const {
        std::printf("%s max=%g(sample %d) mae=%g rms=%g\n", label, max_abs, max_sample, sum_abs / count,
                    std::sqrt(sum_sq / count));
    }
};

static void copy_config(Fp32Registers& fp, const FocAxiParameters<t_glb_q15q16>& ap) {
    fp.ppr = ap.ppr_args;
    fp.mode = ap.control_mode_args;
    fp.fixperiod = ap.control_fixperiod_args;
    fp.flux_sp = ap.flux_sp_args.to_float();
    fp.flux_kp = ap.flux_kp_args.to_float();
    fp.flux_ki = ap.flux_ki_args.to_float();
    fp.flux_kd = ap.flux_kd_args.to_float();
    fp.torque_sp = ap.torque_sp_args.to_float();
    fp.torque_kp = ap.torque_kp_args.to_float();
    fp.torque_ki = ap.torque_ki_args.to_float();
    fp.torque_kd = ap.torque_kd_args.to_float();
    fp.speed_sp = ap.speed_sp_args.to_float();
    fp.speed_kp = ap.speed_kp_args.to_float();
    fp.speed_ki = ap.speed_ki_args.to_float();
    fp.speed_kd = ap.speed_kd_args.to_float();
    fp.angle_shift = ap.angle_sh_args;
    fp.vd = ap.vd_args.to_float();
    fp.vq = ap.vq_args.to_float();
    fp.fw_kp = ap.fw_kp_args.to_float();
    fp.fw_ki = ap.fw_ki_args.to_float();
    fp.fixed_angle = ap.fixed_angle_args.to_int();
}

static void copy_fp32_status_for_log(FocAxiParameters<t_glb_q15q16>& log_regs,
                                     const Fp32Registers& fp,
                                     const Sample& sample) {
    log_regs.id_stts = fp.id;
    log_regs.flux_acc_stts = fp.flux_acc;
    log_regs.flux_err_stts = fp.flux_err;
    log_regs.flux_out_stts = fp.flux_out;
    log_regs.iq_stts = fp.iq;
    log_regs.torque_acc_stts = fp.torque_acc;
    log_regs.torque_err_stts = fp.torque_err;
    log_regs.torque_out_stts = fp.torque_out;
    log_regs.speed_stts = fp.speed;
    log_regs.speed_acc_stts = fp.speed_acc;
    log_regs.speed_err_stts = fp.speed_err;
    log_regs.speed_out_stts = fp.speed_out;
    log_regs.angle_stts = fp.angle;
    // Stream outputs are the exact voltages applied to the virtual motor.
    log_regs.Va_cmd_stts = sample.va;
    log_regs.Vb_cmd_stts = sample.vb;
    log_regs.Vc_cmd_stts = sample.vc;
    log_regs.Ialpha_stts = fp.ialpha;
    log_regs.Ibeta_stts = fp.ibeta;
    log_regs.Ihomopolar_stts = fp.ihomopolar;
    log_regs.fixed_angle_args = fp.fixed_angle;
}

static void fp32_stream_call(::hls::stream<float>& ia,
                             ::hls::stream<float>& ib,
                             ::hls::stream<float>& ic,
                             ::hls::stream<int>& speed_theta,
                             ::hls::stream<int>& motor_id,
                             ::hls::stream<float>& va,
                             ::hls::stream<float>& vb,
                             ::hls::stream<float>& vc,
                             ::hls::stream<int>& motor_id_out,
                             Fp32Registers& regs,
                             long& trip_count,
                             golden::FOCContext<float> ctx[1]) {
    xf::motorcontrol::hls::hls_foc_strm_fp32<COMM_MACRO_CPR, 1, float, int>(
        ia, ib, ic, speed_theta, motor_id, va, vb, vc, motor_id_out, FP32_ARGS(regs), trip_count, ctx);
}

static int pack_speed_theta(short rpm, short theta) {
    return ((static_cast<int>(theta) & 0xffff) << 16) | (static_cast<int>(rpm) & 0xffff);
}

static Sample fp32_scalar(Model_motor<double, int>& motor, Fp32Registers& regs, golden::FOCContext<float> ctx[1]) {
    const t_glb_foc2pwm ia_fixed = motor.out_va;
    const t_glb_foc2pwm ib_fixed = motor.out_vb;
    const t_glb_foc2pwm ic_fixed = motor.out_vc;
    const int speed_theta =
        pack_speed_theta(OmigaToRpm(motor.w), Theta_mTo_Theta_CPR(motor.theta_r, COMM_MACRO_CPR, regs.ppr));
    ::hls::stream<float> ia;
    ::hls::stream<float> ib;
    ::hls::stream<float> ic;
    ::hls::stream<int> speed;
    ::hls::stream<int> motor_id;
    ::hls::stream<float> va;
    ::hls::stream<float> vb;
    ::hls::stream<float> vc;
    ::hls::stream<int> motor_id_out;
    ia << ia_fixed.to_float();
    ib << ib_fixed.to_float();
    ic << ic_fixed.to_float();
    speed << speed_theta;
    motor_id << 0;
    long trip_count = 1;
    fp32_stream_call(ia, ib, ic, speed, motor_id, va, vb, vc, motor_id_out, regs, trip_count, ctx);
    assert(motor_id_out.read() == 0);
    return {
        ia_fixed.to_float(), ib_fixed.to_float(), ic_fixed.to_float(), speed_theta, va.read(), vb.read(), vc.read()};
}

static void write_sample(FILE* file, const Sample& sample, bool output) {
    if (output) {
        std::fprintf(file, "%.9g\t%.9g\t%.9g\n", sample.va, sample.vb, sample.vc);
    } else {
        std::fprintf(file, "%.9g\t%.9g\t%.9g\t%d\t0\t0\t0\n", sample.ia, sample.ib, sample.ic, sample.speed_theta);
    }
}

static std::vector<Sample> read_samples(const char* input_name, const char* output_name) {
    FILE* input = std::fopen(input_name, "r");
    FILE* output = std::fopen(output_name, "r");
    assert(input && output);
    std::vector<Sample> samples;
    Sample sample;
    int ignored0;
    int ignored1;
    int ignored2;
    while (std::fscanf(input, "%f%f%f%d%d%d%d", &sample.ia, &sample.ib, &sample.ic, &sample.speed_theta, &ignored0,
                       &ignored1, &ignored2) == 7) {
        assert(std::fscanf(output, "%f%f%f", &sample.va, &sample.vb, &sample.vc) == 3);
        samples.push_back(sample);
    }
    std::fclose(input);
    std::fclose(output);
    return samples;
}

static void model_phase(Model_motor<double, int>& motor,
                        FocAxiParameters<t_glb_q15q16>& ap_regs,
                        Fp32Registers& fp_regs,
                        golden::FOCContext<float> fp_ctx[1],
                        double& time,
                        FILE* model_log,
                        const char* prefix) {
    char para_name[128];
    char input_name[128];
    char output_name[128];
    std::snprintf(para_name, sizeof(para_name), "%s.para.foc", prefix);
    std::snprintf(input_name, sizeof(input_name), "%s.in.foc", prefix);
    std::snprintf(output_name, sizeof(output_name), "%s.out.foc", prefix);
    FILE* para = std::fopen(para_name, "w");
    FILE* input = std::fopen(input_name, "w");
    FILE* output = std::fopen(output_name, "w");
    assert(para && input && output);
    ap_regs.printParameters(para);
    std::fprintf(para, "\n");
    copy_config(fp_regs, ap_regs);

    for (int sample_index = 0; sample_index < 3000; ++sample_index) {
        motor.updating(0.00001);
        const Sample sample = fp32_scalar(motor, fp_regs, fp_ctx);
        motor.va = sample.va;
        motor.vb = sample.vb;
        motor.vc = sample.vc;
        write_sample(input, sample, false);
        write_sample(output, sample, true);
        if (sample_index % 3 == 0) {
            std::fprintf(model_log, "MM2_B_Time is \t%04.6f(s)\t", time);
            motor.printParameters(model_log, 0);
            // Preserve ap_regs for ap_fixed replay; only the log needs FP32 status.
            FocAxiParameters<t_glb_q15q16> log_regs = ap_regs;
            copy_fp32_status_for_log(log_regs, fp_regs, sample);
            log_regs.printParameters(model_log);
            std::fprintf(model_log, "\tMM2_E\n");
        }
        time += 0.00001;
    }
    std::fclose(para);
    std::fclose(input);
    std::fclose(output);
    std::printf("SIM_FOC_FP32_M: motor_id=0 mode=%s files=%s %s %s\n", string_FOC_Mode[ap_regs.control_mode_args],
                para_name, input_name, output_name);
}

static void replay_and_compare(const char* prefix, FocAxiParameters<t_glb_q15q16> ap_regs, Fp32Registers fp_regs) {
    char input_name[128];
    char output_name[128];
    std::snprintf(input_name, sizeof(input_name), "%s.in.foc", prefix);
    std::snprintf(output_name, sizeof(output_name), "%s.out.foc", prefix);
    const std::vector<Sample> samples = read_samples(input_name, output_name);
    assert(samples.size() == 3000);

    ::hls::stream<float> fp_ia;
    ::hls::stream<float> fp_ib;
    ::hls::stream<float> fp_ic;
    ::hls::stream<int> fp_speed;
    ::hls::stream<int> fp_id;
    ::hls::stream<float> fp_va;
    ::hls::stream<float> fp_vb;
    ::hls::stream<float> fp_vc;
    ::hls::stream<int> fp_id_out;
    ::hls::stream<t_glb_foc2pwm> ap_ia;
    ::hls::stream<t_glb_foc2pwm> ap_ib;
    ::hls::stream<t_glb_foc2pwm> ap_ic;
    ::hls::stream<t_glb_speed_theta> ap_speed;
    ::hls::stream<int> ap_id;
    ::hls::stream<t_glb_foc2pwm> ap_va;
    ::hls::stream<t_glb_foc2pwm> ap_vb;
    ::hls::stream<t_glb_foc2pwm> ap_vc;
    ::hls::stream<int> ap_id_out;

    for (const Sample& sample : samples) {
        fp_ia << sample.ia;
        fp_ib << sample.ib;
        fp_ic << sample.ic;
        fp_speed << sample.speed_theta;
        fp_id << 0;
        ap_ia << t_glb_foc2pwm(sample.ia);
        ap_ib << t_glb_foc2pwm(sample.ib);
        ap_ic << t_glb_foc2pwm(sample.ic);
        ap_speed << t_glb_speed_theta(sample.speed_theta);
        ap_id << 0;
    }
    hls_foc_periodic_fp32(fp_ia, fp_ib, fp_ic, fp_speed, fp_id, fp_va, fp_vb, fp_vc, fp_id_out, FP32_ARGS(fp_regs));
    hls_foc_periodic_ap_fixed_multiple(ap_ia, ap_ib, ap_ic, ap_speed, ap_id, ap_va, ap_vb, ap_vc, ap_id_out,
                                       AP_FIXED_ARGS(ap_regs));

    ErrorStats fp32_replay[3];
    ErrorStats cross_precision[3];
    for (size_t sample_index = 0; sample_index < samples.size(); ++sample_index) {
        assert(fp_id_out.read() == 0);
        assert(ap_id_out.read() == 0);
        const float fp_output[] = {fp_va.read(), fp_vb.read(), fp_vc.read()};
        const float ap_output[] = {ap_va.read().to_float(), ap_vb.read().to_float(), ap_vc.read().to_float()};
        const float expected[] = {samples[sample_index].va, samples[sample_index].vb, samples[sample_index].vc};
        for (int channel = 0; channel < 3; ++channel) {
            fp32_replay[channel].add(fp_output[channel] - expected[channel], sample_index);
            cross_precision[channel].add(fp_output[channel] - ap_output[channel], sample_index);
        }
    }
    std::printf("SIM_FOC_FP32_F: motor_id=0 phase=%s\n", prefix);
    fp32_replay[0].print("  fp32 replay Va");
    fp32_replay[1].print("  fp32 replay Vb");
    fp32_replay[2].print("  fp32 replay Vc");
    cross_precision[0].print("  fp32-ap_fixed Va");
    cross_precision[1].print("  fp32-ap_fixed Vb");
    cross_precision[2].print("  fp32-ap_fixed Vc");
}

int main() {
    static_assert(FOC_FP32_NUM_MOTORS == 1, "The FP32 model testbench is only for one motor.");
    const char* prefixes[] = {"motor0_fp32_sim_torqueWithoutSpeed",
                              "motor0_fp32_sim_rpm10k",
                              "motor0_fp32_sim_rpm16k",
                              "motor0_fp32_sim_rpm16k_weak",
                              "motor0_fp32_sim_manualTorqueFlux",
                              "motor0_fp32_sim_manualTorque",
                              "motor0_fp32_sim_manualFlux",
                              "motor0_fp32_sim_stop",
                              "motor0_fp32_sim_manualTorqueFluxFixedSpeed1",
                              "motor0_fp32_sim_manualTorqueFluxFixedAngle",
                              "motor0_fp32_sim_manualTorqueFluxFixedAngle1"};

    Model_motor<double, int> motor;
    motor.setObjName("motor0");
    motor.dt_sim = 0.00001;
    FocAxiParameters<t_glb_q15q16> ap_regs;
    ap_regs.Init_24V();
    ap_regs.sample_interval_minus1_args = 10;
    Fp32Registers fp_regs = {};
    golden::FOCContext<float> fp_ctx[1];
    FocAxiParameters<t_glb_q15q16> phase_ap[11];
    Fp32Registers phase_fp[11];
    double time = 0;
    FILE* model_log = std::fopen("motor0_fp32_sim_foc_ModelFoc.log", "w");
    assert(model_log);
    std::printf("SIM_FOC_FP32_M: model-driven simulation for motor_id=0, NUM_MOTORS=1\n");

    ap_regs.control_mode_args = FOC_Mode::MOD_TORQUE_WITHOUT_SPEED;
    ap_regs.torque_sp_args = COMM_MOTOR_PARA_UMAX * 0.2f;
    ap_regs.torque_kp_args = 5.0f;
    ap_regs.torque_ki_args = 1.0 / 300.0;
    motor.TL_th = 0.03f;
    for (int phase = 0; phase < 11; ++phase) {
        if (phase == 1) {
            ap_regs.control_mode_args = FOC_Mode::MOD_SPEED_WITH_TORQUE;
            ap_regs.torque_kp_args = 0.04;
            motor.TL_th = 0;
        } else if (phase == 2) {
            ap_regs.speed_sp_args = -16000;
        } else if (phase == 3) {
            ap_regs.control_mode_args = FOC_Mode::MOD_FLUX;
        } else if (phase == 4) {
            ap_regs.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX;
            ap_regs.vq_args = COMM_MOTOR_PARA_UMAX * 0.6f;
            ap_regs.vd_args = 0;
        } else if (phase == 5) {
            ap_regs.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE;
            ap_regs.vq_args = COMM_MOTOR_PARA_UMAX * 0.5f;
        } else if (phase == 6) {
            ap_regs.control_mode_args = FOC_Mode::MOD_MANUAL_FLUX;
            ap_regs.vd_args = COMM_MOTOR_PARA_UMAX * 0.25f * 0.25f;
        } else if (phase == 7) {
            ap_regs.control_mode_args = FOC_Mode::MOD_STOPPED;
        } else if (phase == 8) {
            ap_regs.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED;
            ap_regs.control_fixperiod_args = 1;
        } else if (phase == 9) {
            ap_regs.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE;
            ap_regs.fixed_angle_args = 50;
        } else if (phase == 10) {
            ap_regs.fixed_angle_args = 100;
        }
        model_phase(motor, ap_regs, fp_regs, fp_ctx, time, model_log, prefixes[phase]);
        phase_ap[phase] = ap_regs;
        phase_fp[phase] = fp_regs;
    }
    std::fclose(model_log);

    for (int phase = 0; phase < 9; ++phase) {
        replay_and_compare(prefixes[phase], phase_ap[phase], phase_fp[phase]);
    }
    std::printf("SIM_FOC_FP32: motor_id=0 generated 11 phase file sets and model log.\n");
    return 0;
}
