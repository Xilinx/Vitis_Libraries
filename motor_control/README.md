# Vitis Motor Control Library

Motor Control Library is an open-sourced library written in C/C++ for accelerating developments of motor control applications. It has covered 4 algorithm-level L1 APIs including FOC, SVPWM__DUTY, PWM_GEN and QEI. These four APIs have AXI configuration interfaces, that can be directly integrated into the system by using IPI flow.

From 24.1 release 12 new fine-grained function-level APIs are provided for supporting traditional IP integration flow. These APIs are based on integer types and can simplify computational logic in suitable scenarios.

## Overview

The 4 algorithm-level algorithms APIs implemented by Vitis Motor Control Library include:

- FOC: the API is for sensor based field-orientated control (FOC).From 2024.2 release, the L1/test/IP_FOC functionally becomes a register container module which can be composed by the 12 new-added fine-grained function-level APIs.
- SVPWM_DUTY: the API is the front-end for Space Vector Pulse Width Modulation (SVPWM) to calculate ratios.
- PWM_GEN: the API is the back-end for Space Vector Pulse Width Modulation (SVPWM) to generate output signals based on ratios.
- QEI: the API is for quadrature encoder interface(QEI).


## Documentations

For more details of the Motor Control library, please refer to Motor Control Library chapter in
[Viis Libraries doc](https://docs.xilinx.com/r/en-US/Vitis_Libraries).

## License

The source code in this library is licensed under the MIT X11 license,
which you can find in the LICENSE.txt file.
