# Vitis Motor Control Library

Motor Control Library is an open-sourced library written in C/C++ for accelerating developments of motor control applications. It covers four algorithm-level L1 APIs including FOC, SVPWM_DUTY, PWM_GEN and QEI. These APIs provide AXI configuration interfaces and can be integrated into systems by using the IPI flow.

For the 2026.1 release, the non-FP32 Motor Control HLS source implementation is restored to the 2024.1 source-based implementation for KD240 application integration readiness. The recent FP32 model and precision-test infrastructure remains available under `L1/include/models_fp` and `L1/tests/tests_fp32`.

## Overview

The 4 algorithm-level algorithms APIs implemented by Vitis Motor Control Library include:

- FOC: the API is for sensor based field-orientated control (FOC), including speed and torque control modes and field-weakening control.
- SVPWM_DUTY: the API is the front-end for Space Vector Pulse Width Modulation (SVPWM) to calculate ratios.
- PWM_GEN: the API is the back-end for Space Vector Pulse Width Modulation (SVPWM) to generate output signals based on ratios.
- QEI: the API is for quadrature encoder interface(QEI).


## Documentations

For more details of the Motor Control library, please refer to Motor Control Library chapter in
[Viis Libraries doc](https://docs.xilinx.com/r/en-US/Vitis_Libraries).

## License

The source code in this library is licensed under the MIT X11 license,
which you can find in the LICENSE.txt file.

    Copyright (C) 2022-2022, Xilinx, Inc.
    Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
