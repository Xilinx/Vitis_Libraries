# Vitis Motor Control Library

Motor Control Library is an open-sourced library written in C/C++ for accelerating developments of motor control applications. It has covered 4 algorithm-level L1 APIs including FOC, SVPWM__DUTY, PWM_GEN and QEI. These four APIs have AXI configuration interfaces and high integration level, that can be directly integrated into the system. The ap_fixed type has been adopted in these APIs which makes it easier to understand the physical meaning of variables' value. A virtual motor model is provided for doing the verifications of FOC in the Vitis environment.

Besides the four algorithm-level L1 APIs, from 24.1 release 12 new fine-grained function-level APIs are provided for supporting troditional IP integeration flow. These APIs are based on integer types and can simplify computational logic in suitable scenarios.

## Overview

The 4 algorithm-level algorithms APIs implemented by Vitis Motor Control Library include:

- FOC: the API is for sensor based field-orientated control (FOC).The eight control modes it supports cover basic speed and torque control modes, as well as field-weakning control.
- SVPWM_DUTY: the API is the front-end for Space Vector Pulse Width Modulation (SVPWM) to calculate ratios.
- PWM_GEN: the API is the back-end for Space Vector Pulse Width Modulation (SVPWM) to generate output signals based on ratios.
- QEI: the API is for quadrature encoder interface(QEI).

The 12 new fine-grained function-level APIs include 1) angle_generation, 2) Clarke_Direct, 3) Clarke_Inverse, 4) demuxer_pi, 5) ps_iir_filter, 6) muxer_pi, 7) Park_Direct, 8) Park_Inverse, 9) PI_Control, 10) PI_Control_stream, 11) SVPWM and 12) voltage_modulation. 

## Documentations

For more details of the Motor Control library, please refer to Motor Control Library chapter in
[Viis Libraries doc](https://docs.xilinx.com/r/en-US/Vitis_Libraries).

## License

The source code in this library is licensed under the MIT X11 license,
which you can find in the LICENSE.txt file.
