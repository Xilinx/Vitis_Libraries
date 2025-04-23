# Ultrasound Library - Level 2 (L2)

The level 2 of Vitis Ultrasound Library contains the host-callable kernels. For more details information, please reference to L2 User Guide in the document for usage and design information.

## Overview of APIs
The L2 APIs are composed of the mathematical components needed for the beamformation of the rf-data. Those components are `AIE graphs` based on L1 kernels.

### Image Points

The Image Points graph is used to create a matrix which represents the part of investigation made by the specific emission of the Ultrasound Probe. The rows of this matrix have incremental values which represents our values used to compute delay for the **virtual sources**. It is dependent on the investigation depth and the incremental investigation which we want to perform (based on our sampling frquency).

- **Graph Inputs**:
	- `start_positions`: the position which we start the investigation in cartesian coordinate;
	- `directions`: X-Y-Z values of propagation of the spherical or planar wave emitted;
	- `samples_arange`: array with the index of the rf-data;
- **Graph Outputs**:
	- `Image Points`: A Nx4 matrix which represents our points to analyze;

### Delay

This graph is used to compute the transmit delay. It returns an array of values which represent the transmission time for every incremental point of the investigation. *This graph is used in ScanLine and SA applications*.

- **Graph Inputs**:
	- `image_points_from_PL`: the result of Image Points;
	- `image_points_from_PL2`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `tx_def_delay_distance`: X-Y-Z array which represent our absolute distance with respect to our focus;
	- `tx_def_delay_distance2`: X-Y-Z array which represent our absolute distance with respect to our focus;
	- `tx_def_focal_point`: X-Y-Z array which represent our focus;
	- `t_start`: The starting time of emission;
- **Graph Outputs**:
	- `delay_to_PL`: A vector which represents our time delay per point to analyze;
	
### Delay_PW

This graph is used to compute the transmit delay. It returns an array of values which represent the transmission time for every incremental point of the investigation. *This graph is used in PW  application*.

- **Graph Inputs**:
	- `image_points_from_PL`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `t_start`: The starting time of emission;
- **Graph Outputs**:
	- `delay_to_PL`: A vector which represents our time delay per point to analyze;

### Focusing

This graph is used to compute the distance of our reference apodization point for the dynamic apodization with respect to the transducers position. It returns an array of values which represent the magnitude per transducer.

- **Graph Inputs**:
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `xdc_def_0`: X component of the vector which represent the transducer positions;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `xdc_def_1`: Y component of the vector which represent the transducer positions;
- **Graph Outputs**:
	- `focusing_output`: A vector which represents apodization distance per transducer;
	
### Focusing_SA

This graph is used to compute the distance of our reference apodization point for the dynamic apodization with respect to the image points. This version is used in SA to create a dynamic apodization also in transmission and not only in reception.

- **Graph Inputs**:
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `img_points_0`: X component of the Image Points result;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `img_points_1`: Y component of the Image Points result;
- **Graph Outputs**:
	- `focusing_output`: A vector which represents apodization distance in transmission;
	
### Samples

This graph is used to compute the delay in reception for every transducer. It sums also the delay in transmission to obtain the valid samples for the interpolation.

- **Graph Inputs**:
	- `image_points_from_PL_2`: Result of Image Points;
	- `delay_from_PL`: Result of Delay;
	- `xdc_def_positions`: X-Y-Z vector which represents the positions of our transducers in the probe;
	- `sampling_frequency`: The sampling frequency of the probe;
- **Graph Outputs**:
	- `samples_to_PL`: A vector which represents our valid entries in the rf-data vector;

### Apodization

This graph is used to compute a dynamic Hanning Window for every transducer. So, this graph is used to calculate the apodization for the reception.

- **Graph Inputs**:
	- `image_points`: Result of Image Points;
	- `apodization_reference`: X-Y-Z vector which represents the positions of our apodization reference;
	- `apo_distance_k`: Result of Focusing;
	- `F_number`: The selected F number for the application;
- **Graph Outputs**:
	- `apodization`: A vector which represents our Hanning Window for the reference point chosen;

### Apodization_SA

This graph is used to compute a dynamic Hanning Window for the transmission. *For this reason this graph is computed every line only in SA*

- **Graph Inputs**:
	- `image_points`: Result of Image Points;
	- `apodization_reference_tx`: X-Y-Z vector which represents the positions of our apodization reference (transmission);
	- `apo_distance_k_tx`: Result of Focusing_SA;
	- `F_number`: The selected F number for the application;
- **Graph Outputs**:
	- `apodization`: A vector which represents our Hanning Window for the reference point chosen;

### bSpline

This graph is used to compute the chordal version of the Catmull-Rom. It is called *bSpline* as generally we refer to the Catmull-Rom as the centripetal one.

- **Graph Inputs**:
	- `P1`: First point to be interpolated;
	- `P2`: Second point to be interpolated;
	- `P3`: Second point to be interpolated;
	- `P4`: Third point to be interpolated;
	- `P5`: Third point to be interpolated;
	- `P6`: Fourth point to be interpolated;
- **Graph Outputs**:
	- `C`: A vector with the result of the interpolation

## License

Licensed using the [Apache 2.0 license](https://www.apache.org/licenses/LICENSE-2.0).

    Copyright (C) 2019-2022, Xilinx, Inc.
    Copyright (C) 2022-2024, Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
