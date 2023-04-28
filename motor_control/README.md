# Vitis Motor Control Library

Motor Control Library is an open-sourced library written in C/C++ for accelerating developments of motor control applications. It now covers 4 algorithm-level L1 APIs including FOC, SVPWM__DUTY, PWM_GEN and QEI. Operator-level APIs, such as Clarke transform and its inverse transform, Park transform and its inverse transform and PID are also implemented. The use of ap_fixed data types makes the code easy to understand and further develop. A virtual motor model is provided for doing the verifications of FOC solely in the Vitis environment.

## Overview

The algorithms implemented by Vitis Motor Control Library include:

- FOC: the API is for sensor based field-orientated control (FOC).The eight control modes it supports cover basic speed and torque control modes, as well as field-weakning control.
- SVPWM_DUTY: the API is the front-end for Space Vector Pulse Width Modulation (SVPWM) to calculate ratios.
- PWM_GEN: the API is the back-end for Space Vector Pulse Width Modulation (SVPWM) to generate output signals based on ratios.
- QEI: the API is for quadrature encoder interface(QEI).


## Documentations

For more details of the Motor Control library, please refer to Motor Control Library chapter in
[Viis Libraries doc](https://docs.xilinx.com/r/en-US/Vitis_Libraries).

## License

The source code in this library is licensed under the MIT X11 license,
which you can find in the LICENSE.txt file.

