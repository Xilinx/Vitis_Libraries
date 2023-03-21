.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _introduction_L2:

Introducton for Ultrasound Library Level 2  
==========================================

.. toctree::
   :hidden:
   :maxdepth: 1

Ultrasound Library - Level 2 (L2)
---------------------------------

As stated in the introduction, the L2 level of the Ultrasound Library is composed of the mathematical components needed for the beamformation of the rf-data. Those components, on the other hand of the L1 APIs, are AIE graphs, and no longer just single kernels.
As introduced in the dedicated section, the AIE graphs are a compositions of AIE kernels presents in L1 library. For these reasons the level of abstraction is increased with respect to the previous level.
It is going now to be detailed every components of the L2 library with a connection to the already explained theory to bind the concepts with their actual implementation.

### Graph name: `Image Points`

![imagePoints Demo](images/imagePointsDemo.png)

The Image Points graph is used to create a matrix which represents the part of investigation made by the specific emission of the Ultrasound Probe. The rows of this matrix have incremental values which represents our values used to compute delay for the **virtual sources**. It is dependent on the investigation depth and the incremental investigation which we want to perform (based on our sampling frquency).

- **Graph Inputs**:
	- `start_positions`: the position which we start the investigation in cartesian coordinate;
	- `directions`: X-Y-Z values of propagation of the spherical or planar wave emitted;
	- `samples_arange`: array with the index of the rf-data;
- **Graph Outputs**:
	- `Image Points`: A Nx4 matrix which represents our points to analyze;

### Graph name: `Delay`

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
	
### Graph name: `Delay_PW`

This graph is used to compute the transmit delay. It returns an array of values which represent the transmission time for every incremental point of the investigation. *This graph is used in PW  application*.

- **Graph Inputs**:
	- `image_points_from_PL`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `t_start`: The starting time of emission;
- **Graph Outputs**:
	- `delay_to_PL`: A vector which represents our time delay per point to analyze;

### Graph name: `Focusing`

This graph is used to compute the distance of our reference apodization point for the dynamic apodization with respect to the transducers position. It returns an array of values which represent the magnitude per transducer.

- **Graph Inputs**:
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `xdc_def_0`: X component of the vector which represent the transducer positions;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `xdc_def_1`: Y component of the vector which represent the transducer positions;
- **Graph Outputs**:
	- `focusing_output`: A vector which represents apodization distance per transducer;
	
### Graph name: `Focusing_SA`

This graph is used to compute the distance of our reference apodization point for the dynamic apodization with respect to the image points. This version is used in SA to create a dynamic apodization also in transmission and not only in reception.

- **Graph Inputs**:
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `img_points_0`: X component of the Image Points result;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `img_points_1`: Y component of the Image Points result;
- **Graph Outputs**:
	- `focusing_output`: A vector which represents apodization distance in transmission;
	
### Graph name: `Samples`

This graph is used to compute the delay in reception for every transducer. It sums also the delay in transmission to obtain the valid samples for the interpolation.

- **Graph Inputs**:
	- `image_points_from_PL_2`: Result of Image Points;
	- `delay_from_PL`: Result of Delay;
	- `xdc_def_positions`: X-Y-Z vector which represents the positions of our transducers in the probe;
	- `sampling_frequency`: The sampling frequency of the probe;
- **Graph Outputs**:
	- `samples_to_PL`: A vector which represents our valid entries in the rf-data vector;

### Graph name: `Apodization`

This graph is used to compute a dynamic Hanning Window for every transducer. So, this graph is used to calculate the apodization for the reception.

- **Graph Inputs**:
	- `image_points`: Result of Image Points;
	- `apodization_reference`: X-Y-Z vector which represents the positions of our apodization reference;
	- `apo_distance_k`: Result of Focusing;
	- `F_number`: The selected F number for the application;
- **Graph Outputs**:
	- `apodization`: A vector which represents our Hanning Window for the reference point chosen;

### Graph name: `Apodization_SA`

This graph is used to compute a dynamic Hanning Window for the transmission. *For this reason this graph is computed every line only in SA*

- **Graph Inputs**:
	- `image_points`: Result of Image Points;
	- `apodization_reference_tx`: X-Y-Z vector which represents the positions of our apodization reference (transmission);
	- `apo_distance_k_tx`: Result of Focusing_SA;
	- `F_number`: The selected F number for the application;
- **Graph Outputs**:
	- `apodization`: A vector which represents our Hanning Window for the reference point chosen;

### Graph name: `bSpline`

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