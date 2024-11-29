.. 

.. Copyright © 2019–2024 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _introduction_L3:

Introduction to Ultrasound Library Level 3  
==========================================

.. toctree::
   :hidden:
   :maxdepth: 1

Ultrasound Library - Level 3 (L3)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The last level of abstraction is a complete beamformer composed of the units provided in L2. There are three types of beamformer provided: SA, PW, and ScanLine.

**Scanline_AllinAIE Beamformer**
---------------------------------

This beamformer is a variant of the one used in PW where the difference consists in the receiver delay computation. As the graph is obtained by the composition of L2 libraries, it has the summation of the components in I/O of the L2 libraries:

- **Graph Inputs**:
	- `rf_data`: rf data point to be interpolated;
- **Graph Outputs**:
	- `mult0`: 1/4 of result data to form the png;
	- `mult1`: 1/4 of result data to form the png;
	- `mult2`: 1/4 of result data to form the png;
	- `mult3`: 1/4 of result data to form the png;


The details design of the graph_scanline graph is shown blow:

.. image:: /images/graph_scanline_L3.jpg
   :alt: Block Design of graph_scanline_L3
   :scale: 90%
   :align: center

- A set of C-models of scanline is provided, which can be seen as a step-by-step flow from algorithm-end to AIE-end. The C-model functions can also be used to generate input and verify output simultaneously and conveniently.
- Also, the output of mult can be converted to a png format. Refer to the tutorial for more details.

**ScanLine Beamformer**
---------------------------------

This beamformer is a variant of the one used in PW where the difference consists in the receiver delay computation. As the graph is obtained by the composition of L2 libraries, it has the summation of the components in I/O of the L2 libraries:

- **Graph Inputs**:
	- `start_positions`: the position that you start the investigation in cartesian coordinate;
	- `directions`: X-Y-Z values of propagation of the spherical or planar wave emitted;
	- `samples_arange`: array with the index of the rf-data;
	- `image_points_from_PL`: the result of Image Points;
	- `image_points_from_PL2`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `tx_def_delay_distance`: X-Y-Z array that represents the absolute distance with respect to the focus;
	- `tx_def_delay_distance2`: X-Y-Z array that represents the absolute distance with respect to the focus;
	- `tx_def_focal_point`: X-Y-Z array that represent the focus;
	- `t_start`: The starting time of emission;
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `xdc_def_0`: X component of the vector that represents the transducer positions;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `xdc_def_1`: Y component of the vector that represents the transducer positions;
	- `image_points_from_PL_2`: Result of Image Points;
	- `delay_from_PL`: Result of Delay;
	- `xdc_def_positions`: X-Y-Z vector that represents the positions of the transducers in the probe;
	- `sampling_frequency`: The sampling frequency of the probe;
	- `image_points`: Result of Image Points;
	- `apodization_reference`: X-Y-Z vector that represents the positions of the  apodization reference;
	- `apo_distance_k`: Result of Focusing;
	- `F_number`: The selected F number for the application;
	- `P1`: First point to be interpolated;
	- `P2`: Second point to be interpolated;
	- `P3`: Second point to be interpolated;
	- `P4`: Third point to be interpolated;
	- `P5`: Third point to be interpolated;
	- `P6`: Fourth point to be interpolated;
- **Graph Outputs**:
	- `Image Points`: A Nx4 matrix that represents the points to analyze;
	- `delay_to_PL`: A vector that represents time delay per point to analyze;
	- `focusing_output`: A vector that represents apodization distance per transducer;
	- `samples_to_PL`: A vector that represents the valid entries in the rf-data vector;
	- `apodization`: A vector that represents the Hanning Window for the reference point chosen;
	- `C`: A vector with the result of the interpolation.

**PW Beamformer**
---------------------------------

This beamformer is used to compute PW beamformation. As the graph is obtained by the composition of L2 libraries, it has the summation of the components in I/O of the L2 libraries:

- **Graph Inputs**:
	- `start_positions`: the position that you start the investigation in cartesian coordinate;
	- `directions`: X-Y-Z values of propagation of the spherical or planar wave emitted;
	- `samples_arange`: array with the index of the rf-data;
	- `image_points_from_PL`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `t_start`: The starting time of emission;
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `xdc_def_0`: X component of the vector that represents the transducer positions;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `xdc_def_1`: Y component of the vector that represents the transducer positions;
	- `image_points_from_PL_2`: Result of Image Points;
	- `delay_from_PL`: Result of Delay;
	- `xdc_def_positions`: X-Y-Z vector that represents the positions of the transducers in the probe;
	- `sampling_frequency`: The sampling frequency of the probe;
	- `image_points`: Result of Image Points;
	- `apodization_reference`: X-Y-Z vector that represents the positions of the apodization reference;
	- `apo_distance_k`: Result of Focusing;
	- `F_number`: The selected F number for the application;
	- `P1`: First point to be interpolated;
	- `P2`: Second point to be interpolated;
	- `P3`: Second point to be interpolated;
	- `P4`: Third point to be interpolated;
	- `P5`: Third point to be interpolated;
	- `P6`: Fourth point to be interpolated;
- **Graph Outputs**:
	- `Image Points`: A Nx4 matrix that represents the points to analyze;
	- `delay_to_PL`: A vector that represents time delay per point to analyze;
	- `focusing_output`: A vector that represents apodization distance per transducer;
	- `samples_to_PL`: A vector that represents the valid entries in the rf-data vector;
	- `apodization`: A vector that represents the Hanning Window for the reference point chosen;
	- `C`: A vector with the result of the interpolation

**SA Beamformer**
---------------------------------

This beamformer is used to compute SA beamformation. As the graph is obtained by the composition of L2 libraries, it has the summation of the components in I/O of the L2 libraries:
	
- **Graph Inputs**:
	- `start_positions`: the position that you start the investigation in cartesian coordinate;
	- `directions`: X-Y-Z values of propagation of the spherical or planar wave emitted;
	- `samples_arange`: array with the index of the rf-data;
	- `image_points_from_PL`: the result of Image Points;
	- `image_points_from_PL2`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `tx_def_delay_distance`: X-Y-Z array that represents an absolute distance with respect to the focus;
	- `tx_def_delay_distance2`: X-Y-Z array that represents an absolute distance with respect to the focus;
	- `tx_def_focal_point`: X-Y-Z array that represents the focus;
	- `t_start`: The starting time of emission;
	- `apo_ref_0_tx`: X component of the vector of apodization reference;
	- `img_points_0`: X component of the Image Points result;
	- `apo_ref_1_tx`: Y component of the vector of apodization reference;
	- `img_points_1`: Y component of the Image Points result;
	- `image_points`: Result of Image Points;
	- `apodization_reference_tx`: X-Y-Z vector that represents the positions of the apodization reference (transmission);
	- `apo_distance_k_tx`: Result of Focusing_SA;
	- `F_number`: The selected F number for the application;
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `xdc_def_0`: X component of the vector that represents the transducer positions;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `xdc_def_1`: Y component of the vector that represents the transducer positions;
	- `image_points_from_PL_2`: Result of Image Points;
	- `delay_from_PL`: Result of Delay;
	- `xdc_def_positions`: X-Y-Z vector that represents the positions of the transducers in the probe;
	- `sampling_frequency`: The sampling frequency of the probe;
	- `image_points`: Result of Image Points;
	- `apodization_reference`: X-Y-Z vector that represents the positions of the apodization reference;
	- `apo_distance_k`: Result of Focusing;
	- `F_number`: The selected F number for the application;
	- `P1`: First point to be interpolated;
	- `P2`: Second point to be interpolated;
	- `P3`: Second point to be interpolated;
	- `P4`: Third point to be interpolated;
	- `P5`: Third point to be interpolated;
	- `P6`: Fourth point to be interpolated;
- **Graph Outputs**:
	- `Image Points`: A Nx4 matrix that represents the points to analyze;
	- `delay_to_PL`: A vector that represents time delay per point to analyze;
	- `focusing_output_tx`: A vector that represents apodization distance in transmission;
	- `apodization_tx`: A vector that represents the Hanning Window for the reference point chosen;
	- `focusing_output`: A vector that represents apodization distance per transducer;
	- `samples_to_PL`: A vector that represents the valid entries in the rf-data vector;
	- `apodization`: A vector that represents the Hanning Window for the reference point chosen;
	- `C`: A vector with the result of the interpolation