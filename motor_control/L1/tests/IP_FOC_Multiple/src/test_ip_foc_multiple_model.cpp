/*
Copyright (C) 2026, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11
*/

#include <cassert>
#include <cstdio>

#include "../../IP_FOC/src/foc_demo.hpp"
#include "../../IP_FOC/src/test_foc_modes.hpp"
#include "ip_foc_multiple.hpp"

using namespace xf::motorcontrol;

#ifndef __SYNTHESIS__
RangeTracer ranger;
#endif

#define FOC_AXI_ARGUMENTS(AxiPara)                                                                                     \
    (volatile int&)(AxiPara).ppr_args, (volatile int&)(AxiPara).control_mode_args,                                     \
        (volatile int&)(AxiPara).control_fixperiod_args, (volatile int&)(AxiPara).flux_sp_args,                        \
        (volatile int&)(AxiPara).flux_kp_args, (volatile int&)(AxiPara).flux_ki_args,                                  \
        (volatile int&)(AxiPara).flux_kd_args, (volatile int&)(AxiPara).torque_sp_args,                                \
        (volatile int&)(AxiPara).torque_kp_args, (volatile int&)(AxiPara).torque_ki_args,                              \
        (volatile int&)(AxiPara).torque_kd_args, (volatile int&)(AxiPara).speed_sp_args,                               \
        (volatile int&)(AxiPara).speed_kp_args, (volatile int&)(AxiPara).speed_ki_args,                                \
        (volatile int&)(AxiPara).speed_kd_args, (volatile int&)(AxiPara).angle_sh_args,                                \
        (volatile int&)(AxiPara).vd_args, (volatile int&)(AxiPara).vq_args, (volatile int&)(AxiPara).fw_kp_args,       \
        (volatile int&)(AxiPara).fw_ki_args, (volatile int&)(AxiPara).id_stts, (volatile int&)(AxiPara).flux_acc_stts, \
        (volatile int&)(AxiPara).flux_err_stts, (volatile int&)(AxiPara).flux_out_stts,                                \
        (volatile int&)(AxiPara).iq_stts, (volatile int&)(AxiPara).torque_acc_stts,                                    \
        (volatile int&)(AxiPara).torque_err_stts, (volatile int&)(AxiPara).torque_out_stts,                            \
        (volatile int&)(AxiPara).speed_stts, (volatile int&)(AxiPara).speed_acc_stts,                                  \
        (volatile int&)(AxiPara).speed_err_stts, (volatile int&)(AxiPara).speed_out_stts,                              \
        (volatile int&)(AxiPara).angle_stts, (volatile int&)(AxiPara).Va_cmd_stts,                                     \
        (volatile int&)(AxiPara).Vb_cmd_stts, (volatile int&)(AxiPara).Vc_cmd_stts,                                    \
        (volatile int&)(AxiPara).Ialpha_stts, (volatile int&)(AxiPara).Ibeta_stts,                                     \
        (volatile int&)(AxiPara).Ihomopolar_stts, (volatile int&)(AxiPara).fixed_angle_args

static void call_foc_multiple(bool one_sample,
                              hls::stream<t_glb_foc2pwm>& ia,
                              hls::stream<t_glb_foc2pwm>& ib,
                              hls::stream<t_glb_foc2pwm>& ic,
                              hls::stream<t_glb_speed_theta>& speed_theta,
                              hls::stream<int>& motor_id,
                              hls::stream<t_glb_foc2pwm>& va,
                              hls::stream<t_glb_foc2pwm>& vb,
                              hls::stream<t_glb_foc2pwm>& vc,
                              hls::stream<int>& motor_id_out,
                              FocAxiParameters<t_glb_q15q16>& axi) {
    if (one_sample) {
        hls_foc_oneSample_ap_fixed_multiple(ia, ib, ic, speed_theta, motor_id, va, vb, vc, motor_id_out,
                                            FOC_AXI_ARGUMENTS(axi));
    } else {
        hls_foc_periodic_ap_fixed_multiple(ia, ib, ic, speed_theta, motor_id, va, vb, vc, motor_id_out,
                                           FOC_AXI_ARGUMENTS(axi));
    }
}

static int call_foc_multiple_scalar(t_glb_foc2pwm ia,
                                    t_glb_foc2pwm ib,
                                    t_glb_foc2pwm ic,
                                    float motor_w,
                                    float motor_theta,
                                    t_glb_foc2pwm& va,
                                    t_glb_foc2pwm& vb,
                                    t_glb_foc2pwm& vc,
                                    FocAxiParameters<t_glb_q15q16>& axi) {
    const short rpm = OmigaToRpm(motor_w);
    const short theta = Theta_mTo_Theta_CPR(motor_theta, axi.cpr_args, axi.ppr_args);
    t_glb_speed_theta speed_theta;
    speed_theta(31, 16) = theta;
    speed_theta(15, 0) = rpm;

    hls::stream<t_glb_foc2pwm> ia_stream;
    hls::stream<t_glb_foc2pwm> ib_stream;
    hls::stream<t_glb_foc2pwm> ic_stream;
    hls::stream<t_glb_speed_theta> speed_theta_stream;
    hls::stream<int> motor_id_stream;
    hls::stream<t_glb_foc2pwm> va_stream;
    hls::stream<t_glb_foc2pwm> vb_stream;
    hls::stream<t_glb_foc2pwm> vc_stream;
    hls::stream<int> motor_id_out_stream;

    ia_stream << ia;
    ib_stream << ib;
    ic_stream << ic;
    speed_theta_stream << speed_theta;
    motor_id_stream << 0;
    call_foc_multiple(true, ia_stream, ib_stream, ic_stream, speed_theta_stream, motor_id_stream, va_stream, vb_stream,
                      vc_stream, motor_id_out_stream, axi);

    va = va_stream.read();
    vb = vb_stream.read();
    vc = vc_stream.read();
    assert(motor_id_out_stream.read() == 0);
    return speed_theta;
}

static void reset_foc_multiple() {
    FocAxiParameters<t_glb_q15q16> axi;
    axi.trip_cnt = 1;
    axi.control_mode_args = FOC_Mode::MOD_STOPPED;
    t_glb_foc2pwm va;
    t_glb_foc2pwm vb;
    t_glb_foc2pwm vc;
    call_foc_multiple_scalar(0, 0, 0, 0, 0, va, vb, vc, axi);
}

template <typename T_Mdouble, typename T_Mint>
static void model_based_sim(Model_motor<T_Mdouble, T_Mint>& motor,
                            FocAxiParameters<t_glb_q15q16>& axi,
                            double& time_start,
                            double dt,
                            int num_step,
                            int print_interval,
                            FILE* model_log,
                            const char* file_prefix) {
    char file_para[128];
    char file_input[128];
    char file_output[128];
    std::snprintf(file_para, sizeof(file_para), "%s.para.foc", file_prefix);
    std::snprintf(file_input, sizeof(file_input), "%s.in.foc", file_prefix);
    std::snprintf(file_output, sizeof(file_output), "%s.out.foc", file_prefix);

    FILE* para_log = std::fopen(file_para, "w");
    FILE* input_log = std::fopen(file_input, "w");
    FILE* output_log = std::fopen(file_output, "w");
    assert(para_log && input_log && output_log);
    axi.printParameters(para_log);
    std::fprintf(para_log, "\n");

    for (int sample = 0; sample < num_step; ++sample) {
        motor.updating(dt);
        const t_glb_foc2pwm ia = motor.out_va;
        const t_glb_foc2pwm ib = motor.out_vb;
        const t_glb_foc2pwm ic = motor.out_vc;
        t_glb_foc2pwm va;
        t_glb_foc2pwm vb;
        t_glb_foc2pwm vc;
        const int speed_theta = call_foc_multiple_scalar(ia, ib, ic, motor.w, motor.theta_r, va, vb, vc, axi);
        motor.va = va;
        motor.vb = vb;
        motor.vc = vc;

        printFocInput(input_log, ia, ib, ic, speed_theta);
        printFocOutput(output_log, va, vb, vc);
        if (sample % print_interval == 0) {
            std::fprintf(model_log, "MM2_B_Time is \t%04.6f(s)\t", time_start);
            motor.printParameters(model_log, 0);
            axi.printParameters(model_log);
            std::fprintf(model_log, "\tMM2_E\n");
        }
        time_start += dt;
    }
    std::fclose(para_log);
    std::fclose(input_log);
    std::fclose(output_log);

    std::printf("SIM_FOC_MULTIPLE_M: motor_id=0 mode=%s samples=%d files=%s %s %s\n",
                string_FOC_Mode[axi.control_mode_args], num_step, file_para, file_input, file_output);
}

static int file_based_sim(const char* file_prefix,
                          FocAxiParameters<t_glb_q15q16>& axi,
                          float max_voltage,
                          float threshold) {
    char file_para[128];
    char file_input[128];
    char file_output[128];
    std::snprintf(file_para, sizeof(file_para), "%s.para.foc", file_prefix);
    std::snprintf(file_input, sizeof(file_input), "%s.in.foc", file_prefix);
    std::snprintf(file_output, sizeof(file_output), "%s.out.foc", file_prefix);

    hls::stream<t_glb_foc2pwm> ia_stream;
    hls::stream<t_glb_foc2pwm> ib_stream;
    hls::stream<t_glb_foc2pwm> ic_stream;
    hls::stream<t_glb_speed_theta> speed_theta_stream;
    hls::stream<int> motor_id_stream;
    hls::stream<t_glb_foc2pwm> va_stream;
    hls::stream<t_glb_foc2pwm> vb_stream;
    hls::stream<t_glb_foc2pwm> vc_stream;
    hls::stream<int> motor_id_out_stream;

    const int num_samples =
        getInputFromFile(file_para, axi, file_input, ia_stream, ib_stream, ic_stream, speed_theta_stream);
    if (num_samples != TESTNUMBER) {
        std::printf("SIM_FOC_MULTIPLE_F: motor_id=0 expected %d samples but read %d from %s\n", TESTNUMBER, num_samples,
                    file_input);
        return 1;
    }
    for (int sample = 0; sample < num_samples; ++sample) {
        motor_id_stream << 0;
    }
    axi.trip_cnt = num_samples;
    call_foc_multiple(false, ia_stream, ib_stream, ic_stream, speed_theta_stream, motor_id_stream, va_stream, vb_stream,
                      vc_stream, motor_id_out_stream, axi);
    for (int sample = 0; sample < num_samples; ++sample) {
        if (motor_id_out_stream.read() != 0) {
            std::printf("SIM_FOC_MULTIPLE_F: unexpected output motor_id at sample %d\n", sample);
            return 1;
        }
    }

    const int errors =
        compareGoldenFromFile(file_output, va_stream, vb_stream, vc_stream, num_samples, max_voltage, threshold);
    std::printf("SIM_FOC_MULTIPLE_F: motor_id=0 golden=%s over-threshold=%d\n", file_output, errors);
    return errors;
}

int main() {
    static_assert(IP_FOC_NUM_MOTORS == 1, "The model testbench is only selected for one motor.");

    const int samples_per_phase = TESTNUMBER;
    const int print_interval = 3;
    const double dt = 0.00001;
    const char* prefixes[] = {
        "motor0_sim_torqueWithoutSpeed",
        "motor0_sim_rpm10k",
        "motor0_sim_rpm16k",
        "motor0_sim_rpm16k_weak",
        "motor0_sim_manualTorqueFlux",
        "motor0_sim_manualTorque",
        "motor0_sim_manualFlux",
        "motor0_sim_stop",
        "motor0_sim_manualTorqueFluxFixedSpeed1",
        "motor0_sim_manualTorqueFluxFixedAngle",
        "motor0_sim_manualTorqueFluxFixedAngle1",
    };

    Model_motor<double, int> motor;
    motor.setObjName("motor0");
    motor.dt_sim = dt;
    motor.va = 0;
    motor.vb = 0;
    motor.vc = 0;
    double time = 0;

    FocAxiParameters<t_glb_q15q16> axi;
    axi.Init_24V();
    axi.sample_interval_minus1_args = 10;

    FILE* model_log = std::fopen("motor0_sim_foc_ModelFoc.log", "w");
    assert(model_log);
    std::fprintf(model_log, "MM2_B_Time is \t%04.6f(s)\t", time);
    motor.printParameters(model_log, -1);
    axi.printTitle(model_log);
    std::fprintf(model_log, "\tMM2_E\n");
    std::printf("SIM_FOC_MULTIPLE_M: model-driven simulation for motor_id=0, NUM_MOTORS=1\n");

    axi.control_mode_args = FOC_Mode::MOD_TORQUE_WITHOUT_SPEED;
    axi.torque_sp_args = COMM_MOTOR_PARA_UMAX * 0.2f;
    axi.torque_kp_args = 5.0f;
    axi.torque_ki_args = 1.0 / 300.0;
    motor.TL_th = 0.03f;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[0]);

    axi.control_mode_args = FOC_Mode::MOD_SPEED_WITH_TORQUE;
    axi.torque_kp_args = 0.04;
    motor.TL_th = 0;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[1]);

    axi.speed_sp_args = -16000;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[2]);

    axi.control_mode_args = FOC_Mode::MOD_FLUX;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[3]);

    axi.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX;
    axi.vq_args = COMM_MOTOR_PARA_UMAX * 0.6f;
    axi.vd_args = 0;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[4]);

    axi.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE;
    axi.vq_args = COMM_MOTOR_PARA_UMAX * 0.5f;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[5]);

    axi.control_mode_args = FOC_Mode::MOD_MANUAL_FLUX;
    axi.vd_args = COMM_MOTOR_PARA_UMAX * 0.25f * 0.25f;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[6]);

    axi.control_mode_args = FOC_Mode::MOD_STOPPED;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[7]);

    axi.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_SPEED;
    axi.control_fixperiod_args = 1;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[8]);

    axi.control_mode_args = FOC_Mode::MOD_MANUAL_TORQUE_FLUX_FIXED_ANGLE;
    axi.fixed_angle_args = 50;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[9]);
    axi.fixed_angle_args = 100;
    model_based_sim(motor, axi, time, dt, samples_per_phase, print_interval, model_log, prefixes[10]);
    std::fclose(model_log);

    reset_foc_multiple();
    FocAxiParameters<t_glb_q15q16> replay_axi[9];
    const float threshold = motor.Umax * 0.05f;
    int errors = 0;
    for (int phase = 0; phase < 9; ++phase) {
        errors += file_based_sim(prefixes[phase], replay_axi[phase], motor.Umax, threshold);
    }

    std::printf("SIM_FOC_MULTIPLE: motor_id=0 generated 11 phase file sets and motor0_sim_foc_ModelFoc.log\n");
    std::printf("IP_FOC_Multiple model csim %s for NUM_MOTORS=1, motor_id=0.\n", errors == 0 ? "passed" : "failed");
    return errors;
}
