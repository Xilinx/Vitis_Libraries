.. index:: pair: namespace; motorcontrol
.. _doxid-namespacexf_1_1motorcontrol:
.. _cid-xf::motorcontrol:

namespace motorcontrol
======================

.. toctree::
	:hidden:

	namespace_xf_motorcontrol_details.rst
	enum_xf_motorcontrol_FOC_Mode.rst
	enum_xf_motorcontrol_MODE_PWM_DC_SRC.rst
	enum_xf_motorcontrol_MODE_PWM_PHASE_SHIFT.rst



.. _doxid-namespacexf_1_1motorcontrol_1a4b6ac0ebc3946f803b3ae03f5f90702e:
.. _cid-xf::motorcontrol::hls_foc_strm_int:
.. ref-code-block:: cpp
	:class: overview-code-block

	// namespaces

	namespace :ref:`xf::motorcontrol::details<doxid-namespacexf_1_1motorcontrol_1_1details>`

	// enums

	enum :ref:`FOC_Mode<doxid-namespacexf_1_1motorcontrol_1a24e0b7d90832dc4756987e8ba2332102>`
	enum :ref:`MODE_PWM_DC_SRC<doxid-namespacexf_1_1motorcontrol_1ab18276cfb91c602071f7d3273032dd63>`
	enum :ref:`MODE_PWM_PHASE_SHIFT<doxid-namespacexf_1_1motorcontrol_1a0e1e88eed3776b6774a01b49e7aa6d49>`





.. FunctionSection

.. _doxid-namespacexf_1_1motorcontrol_1a91c46ef744926a55d92113b9d8d27405:
.. _cid-xf::motorcontrol::hls_foc_strm_ap_fixed:

hls_foc_strm_ap_fixed
---------------------


.. code-block:: cpp
	
	#include "foc.hpp"



.. ref-code-block:: cpp
	:class: title-code-block

	template <
	    int VALUE_CPR,
	    typename T_IO,
	    int MAX_IO,
	    int W,
	    int I,
	    typename T_RPM_THETA_FOC
	    >
	void hls_foc_strm_ap_fixed (
	    hls::stream <T_IO>& Ia,
	    hls::stream <T_IO>& Ib,
	    hls::stream <T_IO>& Ic,
	    hls::stream <T_RPM_THETA_FOC>& FOC_RPM_THETA_m,
	    hls::stream <T_IO>& Va_cmd,
	    hls::stream <T_IO>& Vb_cmd,
	    hls::stream <T_IO>& Vc_cmd,
	    volatile int& ppr_args,
	    volatile int& control_mode_args,
	    volatile int& control_fixperiod_args,
	    volatile int& flux_sp_args,
	    volatile int& flux_kp_args,
	    volatile int& flux_ki_args,
	    volatile int& flux_kd_args,
	    volatile int& torque_sp_args,
	    volatile int& torque_kp_args,
	    volatile int& torque_ki_args,
	    volatile int& torque_kd_args,
	    volatile int& speed_sp_args,
	    volatile int& speed_kp_args,
	    volatile int& speed_ki_args,
	    volatile int& speed_kd_args,
	    volatile int& angle_sh_args,
	    volatile int& vd_args,
	    volatile int& vq_args,
	    volatile int& fw_kp_args,
	    volatile int& fw_ki_args,
	    volatile int& id_stts,
	    volatile int& flux_acc_stts,
	    volatile int& flux_err_stts,
	    volatile int& flux_out_stts,
	    volatile int& iq_stts,
	    volatile int& torque_acc_stts,
	    volatile int& torque_err_stts,
	    volatile int& torque_out_stts,
	    volatile int& speed_stts,
	    volatile int& speed_acc_stts,
	    volatile int& speed_err_stts,
	    volatile int& speed_out_stts,
	    volatile int& angle_stts,
	    volatile int& Va_cmd_stts,
	    volatile int& Vb_cmd_stts,
	    volatile int& Vc_cmd_stts,
	    volatile int& Ialpha_stts,
	    volatile int& Ibeta_stts,
	    volatile int& Ihomopolar_stts,
	    volatile int& fixed_angle_args,
	    volatile long& trip_cnt
	    )

sensor based field-orientated control (FOC) in the form of a demo



.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - VALUE_CPR

        - Number of encoder steps per one full revolution. ex. 1000

    *
        - T_IO

        - Data type for input currents and output commands. ex. ap_fixed<24, 8>

    *
        - MAX_IO

        - Maximum absolute value for input currents and output commands. ex. 24(V)

    *
        - W

        - Width of T_IO. ex. 24 for ap_fixed<24, 8>

    *
        - I

        - Integer part width of T_IO(inluding sign bit). ex. 8 for ap_fixed<24, 8>

    *
        - T_RPM_THETA_FOC

        - Data type for packaged RPM and Theta scale value mode by VALUE_CPR, 32-bit aligned

    *
        - Ia

        - Input Phase A current

    *
        - Ib

        - Input Phase B current

    *
        - Ic

        - Input Phase C current

    *
        - FOC_RPM_THETA_m

        - Input THETA_m in [31:16] and RPM in [15:0]

    *
        - Va_cmd

        - Output Va

    *
        - Vb_cmd

        - Output Vb

    *
        - Vc_cmd

        - Output Vc

    *
        - ppr_args

        - input number of pole pairs per phase of the motor; full sinus periods per revolution.

    *
        - control_mode_args

        - Input control mode of foc, enum FOC_Mode. Read every latency cycles of LOOP_FOC

    *
        - control_fixperiod_args

        - input control_fixperiod. Read every latency cycles of LOOP_FOC

    *
        - flux_sp_args

        - Input Args setting point for PID control of Flux

    *
        - flux_kp_args

        - Input Args Proportional coefficient for PID control of Flux

    *
        - flux_ki_args

        - Input Args Integral coefficient for PID control of Flux

    *
        - flux_kd_args

        - Input Args Differential coefficient for PID control of Flux

    *
        - torque_sp_args

        - Input Args setting point for PID control of Torque

    *
        - torque_kp_args

        - Input Args Proportional coefficient for PID control of Torque

    *
        - torque_ki_args

        - Input Args Integral coefficient for PID control of Torque

    *
        - torque_kd_args

        - Input Args Differential coefficient for PID control of Torque

    *
        - speed_sp_args

        - Input Args setting point for PID control of RPM

    *
        - speed_kp_args

        - Input Args Proportional coefficient for PID control of RPM

    *
        - speed_ki_args

        - Input Args Integral coefficient for PID control of RPM

    *
        - speed_kd_args

        - Input Args Differential coefficient for PID control of RPM

    *
        - angle_sh_args

        - Input Args for angle shift

    *
        - vd_args

        - Input Args for setting fixed vd

    *
        - vq_args

        - Input Args for setting fixed vq

    *
        - fw_kp_args

        - Input Args setting point for PID control of field weakening

    *
        - fw_ki_args

        - Input Args Integral coefficient for PID control of field weakening

    *
        - id_stts

        - Output status to monitor stator d-axis current

    *
        - flux_acc_stts

        - Output status to monitor flux accumulate value

    *
        - flux_err_stts

        - Output status to monitor flux latest error value

    *
        - flux_out_stts

        - Output status to monitor flux PID's output

    *
        - iq_stts

        - Output status to monitor stator q-axis current

    *
        - torque_acc_stts

        - Output status to monitor torque accumulate value

    *
        - torque_err_stts

        - Output status to monitor torque latest error value

    *
        - torque_out_stts

        - Output status to monitor torque PID's output

    *
        - speed_stts

        - Output status to monitor speed(RPM) of motor

    *
        - speed_acc_stts

        - Output status to monitor speed(RPM) accumulate value

    *
        - speed_err_stts

        - Output status to monitor speed(RPM) latest error value

    *
        - speed_out_stts

        - Output status to monitor speed(RPM) PID's output

    *
        - angle_stts

        - Output status to monitor Theta_m of motor (scale value to [0, VALUE_CPR])

    *
        - Va_cmd_stts

        - Output status to monitor Output Va

    *
        - Vb_cmd_stts

        - Output status to monitor Output Vb

    *
        - Vc_cmd_stts

        - Output status to monitor Output Vc

    *
        - Ialpha_stts

        - Output status to monitor Ialpha (output of Clarke_Direct)

    *
        - Ibeta_stts

        - Output status to monitor Ibeta (output of Clarke_Direct)

    *
        - Ihomopolar_stts

        - Output status to monitor Ihomopolar (output of Clarke_Direct)

    *
        - fixed_angle_args

        - Input Args for fixed angle value in CPR range by Q15Q16 format

    *
        - trip_cnt

        - Input Args to set the trip count of foc loop

.. _doxid-namespacexf_1_1motorcontrol_1a505f65c0bf245e3c41c56eb1117118e4:
.. _cid-xf::motorcontrol::hls_svpwm_duty_axi:

hls_svpwm_duty_axi
------------------


.. code-block:: cpp
	
	#include "svpwm.hpp"



.. ref-code-block:: cpp
	:class: title-code-block

	template <
	    class T_FOC_COM,
	    class T_RATIO_16b
	    >
	void hls_svpwm_duty_axi (
	    hls::stream <T_FOC_COM>& strm_Va_cmd,
	    hls::stream <T_FOC_COM>& strm_Vb_cmd,
	    hls::stream <T_FOC_COM>& strm_Vc_cmd,
	    hls::stream <T_FOC_COM>& strm_dc_link,
	    hls::stream <T_RATIO_16b>& strm_duty_ratio_a,
	    hls::stream <T_RATIO_16b>& strm_duty_ratio_b,
	    hls::stream <T_RATIO_16b>& strm_duty_ratio_c,
	    volatile int& pwm_args_dc_link_ref,
	    volatile int& pwm_stt_cnt_iter,
	    volatile int& pwm_args_dc_src_mode,
	    volatile int& pwm_args_sample_ii,
	    volatile long& pwm_args_cnt_trip,
	    volatile int& pwm_stt_Va_cmd,
	    volatile int& pwm_stt_Vb_cmd,
	    volatile int& pwm_stt_Vc_cmd
	    )

hls_svpwm_duty: calculate the duty cycles from the input three-phase voltages.



.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - T_FOC_COM

        - The data type for input voltages

    *
        - T_RATIO_16b

        - The data type for output duty cycles

    *
        - strm_Va_cmd

        - in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.

    *
        - strm_Vb_cmd

        - in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.

    *
        - strm_Vc_cmd

        - in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.

    *
        - strm_dc_link

        - in<strm>: Every pwm_args_sample_ii cycles, one output of FOC can be consumed.

    *
        - strm_duty_ratio_a

        - out<strm>: the duty ratio of a, within every pwm cycle.

    *
        - strm_duty_ratio_b

        - out<strm>: the duty ratio of b, within every pwm cycle.

    *
        - strm_duty_ratio_c

        - out<strm>: the duty ratio of c, within every pwm cycle.

    *
        - pwm_args_dc_link_ref

        - in<reg>: Q15Q16 representation for dc_link_ref format, Eg. 0x180000: 24.00000(q15q16)

    *
        - pwm_stt_cnt_iter

        - out<reg>: constantly monitoring how many pwm command sent.

    *
        - pwm_args_dc_src_mode

        - in<reg>: 0 - PWM voltage reference based on ADC measured DC link; 1 - PWM voltage reference uses static register value.

    *
        - pwm_args_sample_ii

        - in<reg>: Sampling interval for more real co-sim.

    *
        - pwm_args_cnt_trip

        - in<reg>: Inner trip counter.

    *
        - pwm_stt_Va_cmd

        - out<reg>: contantly monitoring the Va_cmd inside the kernel calculate_ratios.

    *
        - pwm_stt_Vb_cmd

        - out<reg>: contantly monitoring the Vb_cmd inside the kernel calculate_ratios.

    *
        - pwm_stt_Vc_cmd

        - out<reg>: contantly monitoring the Vc_cmd inside the kernel calculate_ratios.

.. _doxid-namespacexf_1_1motorcontrol_1ae02d2bed8c0902b5dddaa201f147afbd:
.. _cid-xf::motorcontrol::hls_pwm_gen_axi:

hls_pwm_gen_axi
---------------


.. code-block:: cpp
	
	#include "svpwm.hpp"



.. ref-code-block:: cpp
	:class: title-code-block

	template <class T_RATIO_16b>
	void hls_pwm_gen_axi (
	    hls::stream <T_RATIO_16b>& strm_duty_ratio_a,
	    hls::stream <T_RATIO_16b>& strm_duty_ratio_b,
	    hls::stream <T_RATIO_16b>& strm_duty_ratio_c,
	    hls::stream <ap_uint <1>>& strm_h_a,
	    hls::stream <ap_uint <1>>& strm_h_b,
	    hls::stream <ap_uint <1>>& strm_h_c,
	    hls::stream <ap_uint <1>>& strm_l_a,
	    hls::stream <ap_uint <1>>& strm_l_b,
	    hls::stream <ap_uint <1>>& strm_l_c,
	    hls::stream <ap_uint <1>>& strm_sync_a,
	    hls::stream <ap_uint <1>>& strm_sync_b,
	    hls::stream <ap_uint <1>>& strm_sync_c,
	    volatile int& pwm_args_pwm_freq,
	    volatile int& pwm_args_dead_cycles,
	    volatile int& pwm_args_phase_shift,
	    volatile int& pwm_stt_pwm_cycle,
	    volatile long& pwm_args_cnt_trip,
	    volatile int& pwm_args_sample_ii,
	    volatile int& pwm_stt_duty_ratio_a,
	    volatile int& pwm_stt_duty_ratio_b,
	    volatile int& pwm_stt_duty_ratio_c
	    )

hls_pwm_gen: generate the gating bitstream of each switch according to the duty cycles.



.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - T_RATIO_16b

        - The data type of input duty cycles.

    *
        - strm_duty_ratio_a

        - in<strm>: the duty ratio of switch bridge pair a, within every pwm cycle.

    *
        - strm_duty_ratio_b

        - in<strm>: the duty ratio of switch bridge pair b, within every pwm cycle.

    *
        - strm_duty_ratio_c

        - in<strm>: the duty ratio of switch bridge pair c, within every pwm cycle.

    *
        - strm_h_a

        - out<strm>: controls the gating of upper switch at bridge pair a.

    *
        - strm_h_b

        - out<strm>: controls the gating of upper switch at bridge pair b.

    *
        - strm_h_c

        - out<strm>: controls the gating of upper switch at bridge pair c.

    *
        - strm_l_a

        - out<strm>: controls the gating of lower switch at bridge pair a.

    *
        - strm_l_b

        - out<strm>: controls the gating of lower switch at bridge pair b.

    *
        - strm_l_c

        - out<strm>: controls the gating of lower switch at bridge pair c.

    *
        - strm_sync_a

        - out<strm>: send sync sampling signal to the ADC a.

    *
        - strm_sync_b

        - out<strm>: send sync sampling signal to the ADC b.

    *
        - strm_sync_c

        - out<strm>: send sync sampling signal to the ADC c.

    *
        - pwm_args_pwm_freq

        - in<reg>: pwm cycle, the value in test is 100,000 Hz.

    *
        - pwm_args_dead_cycles

        - in<reg>: dead cycle, the value in test is pwm_args_dead_cycles<10> cycles, with global clk freq 100MHz.

    *
        - pwm_args_phase_shift

        - in<reg>: 0 - No phase shift for output; 1 - 120 degree phase shift for output.

    *
        - pwm_stt_pwm_cycle

        - out<reg>: constantly monitoring the integer value of pwm_factor=COMM_CLOCK_FREQ/pwm_freq.

    *
        - pwm_args_cnt_trip

        - in<reg>: inner trip count.

    *
        - pwm_args_sample_ii

        - in<reg>: sampling the AXIS input at [-ii] rate.

    *
        - pwm_stt_duty_ratio_a

        - out<reg>: constantly monitoring the duty_ratio_a value.

    *
        - pwm_stt_duty_ratio_b

        - out<reg>: constantly monitoring the duty_ratio_b value.

    *
        - pwm_stt_duty_ratio_c

        - out<reg>: constantly monitoring the duty_ratio_c value.

.. _doxid-namespacexf_1_1motorcontrol_1a1ce70a3930ca4770cee2f1cd5210b250:
.. _cid-xf::motorcontrol::hls_qei_axi:

hls_qei_axi
-----------


.. code-block:: cpp
	
	#include "qei.hpp"



.. ref-code-block:: cpp
	:class: title-code-block

	template <
	    class T_bin,
	    class T_err
	    >
	void hls_qei_axi (
	    hls::stream <T_bin>& strm_qei_A,
	    hls::stream <T_bin>& strm_qei_B,
	    hls::stream <T_bin>& strm_qei_I,
	    hls::stream <ap_uint <32>>& strm_qei_RPM_THETA_m,
	    hls::stream <T_bin>& strm_qei_dir,
	    hls::stream <T_err>& strm_qei_err,
	    volatile int& qei_args_cpr,
	    volatile int& qei_args_ctrl,
	    volatile int& qei_stts_RPM_THETA_m,
	    volatile int& qei_stts_dir,
	    volatile int& qei_stts_err,
	    volatile long& qei_args_cnt_trip
	    )

Quadrature Encoder Interface(QEI) control demo top interface and paramters list.



.. rubric:: Parameters:

.. list-table::
    :widths: 20 80

    *
        - T_bin

        - The data type for ABI's signals

    *
        - T_err

        - The data type for qei's error status

    *
        - strm_qei_A

        - The input stream for A signals

    *
        - strm_qei_B

        - The input stream for B signals

    *
        - strm_qei_I

        - The input stream for I signals

    *
        - strm_qei_RPM_THETA_m

        - The output stream for rpm and theta_m

    *
        - strm_qei_dir

        - The output stream for direction value

    *
        - strm_qei_err

        - The output stream for error status value

    *
        - qei_args_cpr

        - Read for user setting or written back by kernel

    *
        - qei_args_ctrl

        - The lowest bit of this value indicates the encoding mode

    *
        - qei_stts_RPM_THETA_m

        - Rpm and theta_m written back by kernel on axi_lite port

    *
        - qei_stts_dir

        - Dir written back by kernel on axi_lite port

    *
        - qei_stts_err

        - Err written back by kernel on axi_lite port

    *
        - qei_args_cnt_trip

        - input of trip count

