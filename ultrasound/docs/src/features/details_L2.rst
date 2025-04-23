.. 
   
.. Copyright © 2019–2024 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _introduction_L2:

Introduction for Ultrasound Library Level 2  
==========================================

.. toctree::
   :hidden:
   :maxdepth: 1

Ultrasound Library - Level 2 (L2)
---------------------------------

As stated in the introduction, the L2 level of the Ultrasound Library is composed of the mathematical components needed for the beamformation of the rf-data. Those components, on the other hand of the L1 APIs, are AIE graphs and no longer just single kernels.
As introduced in the dedicated section, the AIE graphs are a compositions of AIE kernels presents in L1 library. For these reasons, the level of abstraction is increased with respect to the previous level.
Following is an explanation of every component of the L2 library with a connection to the already explained theory to bind the concepts with their actual implementation.

Graph name: `graph_imagepoints`
###################################

The graph_imagepoints graph is used to create an array(specific coordinate direction), which represents the part of investigation made by the specific emission of the Ultrasound Probe. 
It is dependent on the investigation depth and the incremental investigation that you want to perform (based on our sampling frquency).

- **Graph Inputs**:
	- `para_const`: Graph_imagepoints self-used structural parameters include step(the distance between adjacent points), iter_line(record number of lines that have been processed), and so on.
	- `para_start`: The position that you start the investigation in cartesian coordinate;
- **Graph Outputs**:
	- `dataout1`: An array that represents the points(specific coordinate direction) to analyze;

Graph name: `graph_focusing`
###################################

This graph is used to compute the distance of the reference apodization point for the dynamic apodization with respect to the transducers position. It returns an array of values that represent the magnitude per transducer.

- **Graph Inputs**:
	- `p_para_const`: Graph_focusing self-used structural parameters include iter_line, iter_element, and so on. Its definition can be seen in L1/include/kernel_focusing.hpp;
	- `p_para_pos`: X and Y component of the vector that represents the transducer positions;
- **Graph Outputs**:
	- `dataout1`: An array that represents apodization distance per transducer;

Graph name: `graph_apodization_preprocess`
###########################################

This graph is the preprocess that is used to compute a dynamic Hanning Window for every transducer.

- **Graph Inputs**:
	- `para_amain_const`: Graph_apodization_preprocess self-used structural parameters include iter_line, iter_element, and so on. Its definition can be seen in L1/include/kernel_apodization_pre.hpp;
	- `img_x`: The result of graph_imagepoints in x-dimension;
	- `img_z`: The result of graph_imagepoints in z-dimension;
- **Graph Outputs**:
	- `out`: A vector that represents the distance of x&z dimension;

Graph name: `graph_apodization`
###########################################

This graph is used to compute a dynamic Hanning Window for every transducer. So, this graph is used to calculate the apodization for the reception.

- **Graph Inputs**:
	- `para_amain_const`: Graph_apodization_main self-used structural parameters include iter_line, iter_element, and so on. Its definition can be seen in L1/include/kernel_apodization_main.hpp;
	- `p_focal`: Result of Focusing;
	- `p_invD`: The result of graph_apodization_preprocess output;
- **Graph Outputs**:
	- `out`: A vector that represents the Hanning Window for the reference point chosen;

Graph name: `graph_delay`
###########################################

This graph is used to compute the transmit delay. It returns an array of values that represent the transmission time for every incremental point of the investigation.

- **Graph Inputs**:
	- `para_const`: Graph_delay self-used structural parameters include focal coordinates 'focal_point_x' and 'focal_point_z', inverse speed of sound 'inverse_speed_of_sound' and so on. Its definition can be seen in L1/include/kernel_delay.hpp;
	- `para_t_start`: Input array, where each element corresponds to the emission starting time of each scanline;
	- `img_x`: The result of graph_imagepoints in x-dimension;
	- `img_z`: The result of graph_imagepoints in z-dimension;
- **Graph Outputs**:
	- `delay`: An array that represents time delay per point to analyze;

Graph name: `graph_samples`
###################################

This graph is used to compute the delay in reception for every transducer. It also sums the delay in transmission to obtain the valid samples for the interpolation.

- **Graph Inputs**:
	- `para_const`: Graph_samples self-used structural parameter include sampling frequency, inverse speed of sound and so on. Its definition can be seen in L1/include/kernel_sample.hpp.
	- `para_rfdim`: Input array, used to filter whether the input rf-data of each scanline is in the region of interest;
	- `para_elem`: X-Y-Z vector that represents the positions of our transducers in the probe;
	- `img_x`: The result of graph_imagepoints in x-dimension;
	- `img_z`: The result of graph_imagepoints in z-dimension;
	- `delay`: Result of graph_delay;
- **Graph Outputs**:
	- `sample`: A vector that represents the valid entries in the rf-data vector;
	- `inside`: A vector with only 0 and 1 values, which represents whether each element of the rf-data vector is in the region of interest;

Graph name: `graph_interpolation`
###########################################

This graph is used to compute the chordal version of the Catmull-Rom. It is also called *bSpline*. Generally, you refer to the Catmull-Rom as the centripetal one.

- **Graph Inputs**:
	- `para_interp_const`: Graph_interpolation self-used structural parameters include iter_line, iter_element, and so on. Its definition can be seen in L1/include/kernel_interpolation.hpp;
	- `p_sample_in`: The result of graph_sample output;
	- `p_inside_in`: The result of graph_sample output;
	- `p_rfdata_in`: Input rf points to be interpolated;
- **Graph Outputs**:
	- `out`: A vector with the result of the interpolation;

Graph name: `graph_mult`
###########################################

This graph is used to compute the final result. It multiplies the results of apodization and the interpolation and accumulates the results of each transducer.

- **Graph Inputs**:
	- `para_const_pre`: Graph_mult's kernel kfun_mult_pre self-used structural parameter include iter_line, iter_element to record the number of lines processed. Its definition can be seen in L1/include/kernel_mult.hpp.
	- `para_const_0`: Graph_mult's kernel kfun_mult_0 self-used structural parameter include iter_line, iter_element to record the number of lines processed;
	- `para_const_1`: Graph_mult's kernel kfun_mult_1 self-used structural parameter include iter_line, iter_element to record the number of lines processed;
	- `para_const_2`: Graph_mult's kernel kfun_mult_2 self-used structural parameter include iter_line, iter_element to record the number of lines processed;
	- `para_const_3`: Graph_mult's kernel kfun_mult_3 self-used structural parameter include iter_line, iter_element to record the number of lines processed;
	- `para_const_0_0`: Graph_mult's kernel kfun_mult_0 self-used local array, used to store accumulated temporary results;
	- `para_const_0_1`: Graph_mult's kernel kfun_mult_1 self-used local array, used to store accumulated temporary results;
	- `para_const_0_2`: Graph_mult's kernel kfun_mult_2 self-used local array, used to store accumulated temporary results;
	- `para_const_0_3`: Graph_mult's kernel kfun_mult_3 self-used local array, used to store accumulated temporary results;
	- `para_const_1_0`: Graph_mult's kernel kfun_mult_0 self-used local array, used to store accumulated temporary results. To avoid bank conflict, this data memory is applied;
	- `para_const_1_1`: Graph_mult's kernel kfun_mult_1 self-used local array, used to store accumulated temporary results. To avoid bank conflict, this data memory is applied;
	- `para_const_1_2`: Graph_mult's kernel kfun_mult_2 self-used local array, used to store accumulated temporary results. To avoid bank conflict, this data memory is applied;
	- `para_const_1_3`: Graph_mult's kernel kfun_mult_3 self-used local array, used to store accumulated temporary results. To avoid bank conflict, this data memory is applied;
	- `interp`: Results of interpolation.
	- `apod`: Results of apodization.
- **Graph Outputs**:
	- `mult_0`: Output first segment of final result;
	- `mult_1`: Output second segment of final result;
	- `mult_2`: Output third segment of final result;
	- `mult_3`: Output last segment of final result;

Graph name: `graph_scanline`
###################################

This graph is a combination of graph_imagepoints, graph_delay, graph_focsing, graph_samples, graph_interpolation, graph_apodization, graph_mult.

- **Graph Inputs**:
	- `input_rfd_file_name`: Graph_scanline's input rf-data path, used in x86sim and aiesim.
- **Graph Outputs**:
	- `mult_0`: Output first segment of final result;
	- `mult_1`: Output second segment of final result;
	- `mult_2`: Output third segment of final result;
	- `mult_3`: Output last segment of final result;

The details design of the graph_scanline graph is shown below:

.. image:: /images/graph_scanline_L2.jpg
   :alt: Block Design of graph_scanline_L2
   :scale: 90%
   :align: center

- For details, refer to L2 or L3 tutorial.

Graph name: `Image Points`
###################################

The Image Points graph is used to create a matrix, which represents the part of investigation made by the specific emission of the Ultrasound Probe. The rows of this matrix have incremental values, which represents our values used to compute delay for the **virtual sources**. It is dependent on the investigation depth and the incremental investigation that you want to perform (based on our sampling frquency).

- **Graph Inputs**:
	- `start_positions`: the position that you start the investigation in cartesian coordinate;
	- `directions`: X-Y-Z values of propagation of the spherical or planar wave emitted;
	- `samples_arange`: array with the index of the rf-data;
- **Graph Outputs**:
	- `Image Points`: A Nx4 matrix that represents the points to analyze;

Graph name: `Delay`
###################################

This graph is used to compute the transmit delay. It returns an array of values, which represent the transmission time for every incremental point of the investigation. *This graph is used in ScanLine and SA applications*.

- **Graph Inputs**:
	- `image_points_from_PL`: the result of Image Points;
	- `image_points_from_PL2`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `tx_def_delay_distance`: X-Y-Z array that represents the absolute distance with respect to the focus;
	- `tx_def_delay_distance2`: X-Y-Z array that represents the absolute distance with respect to the focus;
	- `tx_def_focal_point`: X-Y-Z array that represent our focus;
	- `t_start`: The starting time of emission;
- **Graph Outputs**:
	- `delay_to_PL`: A vector that represents the time delay per point to analyze;
	
Graph name: `Delay_PW`
###################################

This graph is used to compute the transmit delay. It returns an array of values that represent the transmission time for every incremental point of the investigation. *This graph is used in PW  application*.

- **Graph Inputs**:
	- `image_points_from_PL`: the result of Image Points;
	- `tx_def_ref_point`: X-Y-Z array with the reference point of the investigation for that iteration;
	- `t_start`: The starting time of emission;
- **Graph Outputs**:
	- `delay_to_PL`: A vector that represents time delay per point to analyze;

Graph name: `Focusing`
###################################

This graph is used to compute the distance of our reference apodization point for the dynamic apodization with respect to the transducers position. It returns an array of values that represent the magnitude per transducer.

- **Graph Inputs**:
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `xdc_def_0`: X component of the vector that represents the transducer positions;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `xdc_def_1`: Y component of the vector that represents the transducer positions;
- **Graph Outputs**:
	- `focusing_output`: A vector that represents apodization distance per transducer;
	
Graph name: `Focusing_SA`
###################################

This graph is used to compute the distance of our reference apodization point for the dynamic apodization with respect to the image points. This version is used in SA to create a dynamic apodization also in transmission and not only in reception.

- **Graph Inputs**:
	- `apo_ref_0`: X component of the vector of apodization reference;
	- `img_points_0`: X component of the Image Points result;
	- `apo_ref_1`: Y component of the vector of apodization reference;
	- `img_points_1`: Y component of the Image Points result;
- **Graph Outputs**:
	- `focusing_output`: A vector that represents apodization distance in transmission;
	
Graph name: `Samples`
###################################

This graph is used to compute the delay in reception for every transducer. It also sums the delay in transmission to obtain the valid samples for the interpolation.

- **Graph Inputs**:
	- `image_points_from_PL_2`: Result of Image Points;
	- `delay_from_PL`: Result of Delay;
	- `xdc_def_positions`: X-Y-Z vector that represents the positions of our transducers in the probe;
	- `sampling_frequency`: The sampling frequency of the probe;
- **Graph Outputs**:
	- `samples_to_PL`: A vector that represents our valid entries in the rf-data vector;

Graph name: `Apodization`
###################################

This graph is used to compute a dynamic Hanning Window for every transducer. So, this graph is used to calculate the apodization for the reception.

- **Graph Inputs**:
	- `image_points`: Result of Image Points;
	- `apodization_reference`: X-Y-Z vector that represents the positions of our apodization reference;
	- `apo_distance_k`: Result of Focusing;
	- `F_number`: The selected F number for the application;
- **Graph Outputs**:
	- `apodization`: A vector that represents the Hanning Window for the reference point chosen;

Graph name: `Apodization_SA`
###################################

This graph is used to compute a dynamic Hanning Window for the transmission. *For this reason this graph is computed every line only in SA*

- **Graph Inputs**:
	- `image_points`: Result of Image Points;
	- `apodization_reference_tx`: X-Y-Z vector that represents the positions of our apodization reference (transmission);
	- `apo_distance_k_tx`: Result of Focusing_SA;
	- `F_number`: The selected F number for the application;
- **Graph Outputs**:
	- `apodization`: A vector that represents our Hanning Window for the reference point chosen;

Graph name: `bSpline`
###################################

This graph is used to compute the chordal version of the Catmull-Rom. It is called *bSpline*. Generally, it is referred to as Catmull-Rom as the centripetal one.

- **Graph Inputs**:
	- `P1`: First point to be interpolated;
	- `P2`: Second point to be interpolated;
	- `P3`: Second point to be interpolated;
	- `P4`: Third point to be interpolated;
	- `P5`: Third point to be interpolated;
	- `P6`: Fourth point to be interpolated;
- **Graph Outputs**:
	- `C`: A vector with the result of the interpolation