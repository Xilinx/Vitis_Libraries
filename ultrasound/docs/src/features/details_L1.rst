.. 
   
.. Copyright © 2019–2024 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _introduction_L1:

Introduction for Ultrasound Library Level 1  
===========================================

.. toctree::
   :hidden:
   :maxdepth: 1

Ultrasound Library - Level 1 (L1)
---------------------------------

As explained in the introduction, the lowest level of the libraries is a set of APIs, which are created to create a closest mapping possible between *NumPy* and the *C++* SIMD APIs of the AI Engine. The APIs resulted are a more comprehensive and easy interface for you to create your application using numpy-like interfaces. 
The APIs can be divided in two main groups:

1. Element-wise operations between vectors and matrices. Those operations vary between basic operations (that is, sum, multiplication, and so on) to more complex ones (that is, reciprocal, sign, and so on);
2. Vector managing and creation. Those operations are intended to be used by you to create or modify the dimension of the vectorial operands. Two example of those operations are *Tile* and *Ones*.

The APIs are written in a templatic way, such that you might parametrize the length of the operands or of the SIMD operations to suit the necessities of the customers.
Before starting with the single kernels explanation is the nomenclature used in the kernels. The kernels are thought to be used as you would do in NumPy, but on the contrary, with respect to what is possible in python, in C++, the dimensionality of the operands must be known a priori to write appropriate and correct operations. For common DSP and BLAS applications, such as Ultrasound Imaging, it is important to control the dimensionality of the data to build appropriate applications. This is why the kernels have been written in the following way:

The first part of the name indicates the type of operation chosen. The second and the third parts of the names of the interface are optional, depending on the number of operands of the operation. Provided that they are two, the letter S/V/M indicates their dimensionality. S stands for **Scalar**, V for **Vector**, and M for **Matrix**.
For example, `mulMV` is the operation, which multiplies (element-wise) the rows of a Matrix (as first operand, M) with a Vector of the same dimension (as second operand, V). Another example is `diffSV`, an operation that subtracts a scalar value (as first operand, S) to every element of a Vector (as second operand, V).

The details of the L1 kernel available in the library is as follows:

kernel name: `kernel_imagepoints`
###################################

.. code-block::cpp
	template <class T,
			int NUM_LINE_t,
			int NUM_ELEMENT_t,
			int NUM_SAMPLE_t,
			int NUM_SEG_t,
			int VECDIM_img_t,
			int LEN_OUT_img_t,
			int LEN32b_PARA_img_t>
	void kfun_img_1d(const T (&para_const)[LEN32b_PARA_img_t],
					const T (&para_start)[NUM_LINE_t],
					output_buffer<T, adf::extents<LEN_OUT_img_t> >& out);


The kernel_imagepoints kernel is used to create an array(specific coordinate direction), which represents the part of investigation made by the specific emission of the Ultrasound Probe. 
It is dependent on the investigation depth and the incremental investigation that you want to perform (based on our sampling frquency).

- **Template params**:
	- `T`: type of the operation;
	- `NUM_LINE_t`: number of scanlines;
	- `NUM_ELEMENT_t`: number of transducer's elements;
	- `NUM_SAMPLE_t`: number of time samples;
	- `NUM_SEG_t`: number of segments in one scanline;
	- `VECDIM_img_t`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `LEN_IN_mult_t`: number of inputdata per invoking;
	- `LEN_OUT_img_t`: number of outputdata per invoking;
	- `LEN32b_PARA_img_t`: number of inputdata per invoking;
- **kernel Inputs**:
	- `para_const`: kernel_imagepoints self-used structural parameters include step(the distance between adjacent points), iter_line(record number of lines that have been processed), and so on.
	- `para_start`: The position that you start the investigation in cartesian coordinate;
- **kernel Outputs**:
	- `out`: An array that represents the points(specific coordinate direction) to analyze;



kernel name: `kernel_focusing`
###################################

.. code-block::cpp
	template <class T,
			int NUM_LINE_t,
			int NUM_ELEMENT_t,
			int NUM_SAMPLE_t,
			int NUM_SEG_t,
			int VECDIM_foc_t,
			int LEN32b_PARA_foc_t>
	void kfun_foc(const T (&para_const)[LEN32b_PARA_foc_t],
				const T (&para_xdc_def_pos_4d)[NUM_ELEMENT_t * 4],
				output_buffer<T, adf::extents<NUM_ELEMENT_t> >& out);

This kernel is used to compute the distance of the reference apodization point for the dynamic apodization with respect to the transducers position. It returns an array of values, which represent the magnitude per transducer.

- **Template params**:
	- `T`: type of the operation;
	- `NUM_LINE_t`: number of scanlines;
	- `NUM_ELEMENT_t`: number of transducer's elements;
	- `NUM_SAMPLE_t`: number of time samples;
	- `NUM_SEG_t`: number of segments in one scanline;
	- `VECDIM_foc_t`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `LEN32b_PARA_foc_t`: number of inputdata per invoking;
- **kernel Inputs**:
	- `para_const`: kernel_imagepoints self-used structural parameters include step(the distance between adjacent points), iter_line(record number of lines that have been processed), and so on.
	- `para_xdc_def_pos_4d`: X and Y components of the vector that represent the transducer positions;
- **kernel Outputs**:
	- `out`: An array that represents apodization distance per transducer;



Kernel name: `kfun_apodization_preprocess`
###################################


.. code-block::cpp
		template <typename T, int LEN_OUT, int LEN_IN, int VECDIM, int APODI_PRE_LEN32b_PARA>
		void kfun_apodization_pre(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_invD_out,
								adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_x_in,
								adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_points_z_in,
								const int (&para_const)[APODI_PRE_LEN32b_PARA]);

This kernel is the preprocess that is used to compute a dynamic Hanning Window for every transducer.

- **Template params**:
	- `T`: type of the operation;
	- `LEN_OUT`: number of outputdata per invoking;
	- `LEN_IN`: number of inputdata per invoking;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `APODI_PRE_LEN32b_PARA`: number of RTP data per invoking;
- **Kernel Inputs**:
	- `param`: kernel_apodization_preprocess self-used structural parameters include iter_line, iter_element, and so on. Its definition can be seen in L1/include/kernel_apodization_pre.hpp;
	- `p_points_x_in`: The result of kernel_imagepoints in x-dimension;
	- `p_points_z_in`: The result of kernel_imagepoints in z-dimension;
- **Kernel Outputs**:
	- `p_invD_out`: A vector that represents the distance of x&z dimension;


Kernel name: `kfun_apodization_main`
###################################

.. code-block::cpp
		template <typename T, int LEN_OUT, int LEN_IN_F, int LEN_IN_D, int VECDIM, int APODI_PRE_LEN32b_PARA>
		void __attribute__((noinline))
		kfun_apodization_main(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_apodization_out,
							adf::input_buffer<T, adf::extents<LEN_IN_F> >& __restrict p_focal_in,
							adf::input_buffer<T, adf::extents<LEN_IN_D> >& __restrict p_invD_in,
							const int (&para_const)[APODI_PRE_LEN32b_PARA]);

This kernel is used to compute a dynamic Hanning Window for every transducer. So, this kernel is used to calculate the apodization for the reception.

- **Template params**:
	- `T`: type of the operation;
	- `LEN_OUT`: number of outputdata per invoking;
	- `LEN_IN_F`: number of inputdata per invoking;
	- `LEN_IN_D`: number of inputdata per invoking;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `APODI_PRE_LEN32b_PARA`: number of RTP data per invoking;
- **Kernel Inputs**:
	- `para_const`: kernel_apodization_main self-used structural parameters include iter_line, iter_element, and so on. Its definition can be seen in L1/include/kernel_apodization_main.hpp;
	- `p_focal_in`: Result of Focusing;
	- `p_invD_in`: The result of kernel_apodization_preprocess output;
- **Kernel Outputs**:
	- `p_apodization_out`: A vector that represents our Hanning Window for the reference point chosen;

Kernel name: `kernel_delay`
###################################

.. code-block::cpp
		template <class T, int NUM_LINE_t, int VECDIM_delay_t, int LEN_IN_delay_t, int LEN_OUT_delay_t, int LEN32b_PARA_delay_t>
		void __attribute__((noinline))
		kfun_UpdatingDelay_line_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT_delay_t> >& __restrict out_delay,
										adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_x,
										adf::input_buffer<T, adf::extents<LEN_IN_delay_t> >& __restrict in_img_z,
										const T (&para_const)[LEN32b_PARA_delay_t],
										const T (&para_t_start)[NUM_LINE_t]);

This kernel is used to compute the transmit delay. It returns an array of values, which represent the transmission time for every incremental point of the investigation.

- **Template params**:
	- `T`: type of the operation;
	- `NUM_LINE_t`: number of scanlines;
	- `VECDIM_delay_t`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `LEN_IN_delay_t`: number of inputdata per invoking;
	- `LEN_OUT_delay_t`: number of outputdata per invoking;
	- `LEN32b_PARA_delay_t`: number of inputdata per invoking;
- **Kernel Inputs**:
	- `para_const`: kernel_delay self-used structural parameters include focal coordinates 'focal_point_x' and 'focal_point_z', inverse speed of sound 'inverse_speed_of_sound,' and so on. Its definition can be seen in L1/include/kernel_delay.hpp;
	- `para_t_start`: Input array, where each element corresponds to the emission starting time of each scanline;
	- `in_img_x`: The result of kernel_imagepoints in x-dimension;
	- `in_img_z`: The result of kernel_imagepoints in z-dimension;
- **Kernel Outputs**:
	- `out_delay`: An array that represents our time delay per point to analyze;
  

Kernel name: `kernel_samples`
###################################

.. code-block::cpp
		template <class T,
				int NUM_LINE_t,
				int NUM_ELEMENT_t,
				int VECDIM_sample_t,
				int LEN_IN_sample_t,
				int LEN_OUT_sample_t,
				int LEN32b_PARA_sample_t>
		void __attribute__((noinline))
		kfun_genLineSample_wrapper(adf::output_buffer<int, adf::extents<LEN_OUT_sample_t> >& __restrict out_sample, // output
								adf::output_buffer<int, adf::extents<LEN_OUT_sample_t> >& __restrict out_inside, // output
								adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_img_x,       // input
								adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_img_z,       // input
								adf::input_buffer<T, adf::extents<LEN_IN_sample_t> >& __restrict in_delay,       // input
								const T (&para_const)[LEN32b_PARA_sample_t],
								const T (&para_rfdim)[NUM_LINE_t],
								const T (&para_elem)[NUM_ELEMENT_t * 4]);

This kernel is used to compute the delay in reception for every transducer. It also sums the delay in transmission to obtain the valid samples for the interpolation.

- **Template params**:
	- `T`: type of the operation;
	- `NUM_LINE_t`: number of scanlines;
	- `NUM_ELEMENT_t`: number of transducer's elements;
	- `VECDIM_sample_t`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `LEN_IN_sample_t`: number of inputdata per invoking;
	- `LEN_OUT_sample_t`: number of outputdata per invoking;
	- `LEN32b_PARA_sample_t`: number of inputdata per invoking;
- **Kernel Inputs**:
	- `para_const`: kernel_samples self-used structural parameter include sampling frequency, inverse speed of sound, and so on. Its definition can be seen in L1/include/kernel_sample.hpp.
	- `para_rfdim`: Input array, used to filter whether the input rf-data of each scanline is in the region of interest;
	- `para_elem`: X-Y-Z vector which represents the positions of our transducers in the probe;
	- `in_img_x`: The result of kernel_imagepoints in x-dimension;
	- `in_img_z`: The result of kernel_imagepoints in z-dimension;
	- `in_delay`: Result of kernel_delay;
- **Kernel Outputs**:
	- `out_sample`: A vector that represents our valid entries in the rf-data vector;
	- `out_inside`: A vector with only 0 and 1 values, which represents whether each element of our rf-data vector is in the region of interest;


Kernel name: `kfun_rfbuf_wrapper`
###################################

.. code-block::cpp
	    template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN , int VECDIM, int INTERP_LEN32b_PARA>
	    kfun_rfbuf_wrapper(adf::output_buffer<T, 
                            adf::extents<LEN_OUT> >& __restrict p_rfbuf_out,
                            input_stream<T>* strm_rfdata,
                            const T (&local_data)[LEN_OUT],
                            const int (&para_const)[INTERP_LEN32b_PARA]);


Read one betch rf data and buffer them to local for resamp.

- **Template params**:
	- `T`: type of the operation;
	- `LEN_OUT`: number of outputdata per invoking;
	- `LEN_IN`: number of inputdata per invoking;
    - `LEN_RF_IN`: no use for now;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `INTERP_LEN32b_PARA`: number of RTP data per invoking;
- **Function params**:
    - `para_const`: kernel_rfbuf self-used structural parameter include sampling frequency, inverse speed of sound, and so on. Its definition can be seen in L1/include/kernel_interpolation.hpp.
	- `strm_rfdata`: elements of input stream vector.
    - `local_data`: elements of local data to temp save the input vector.
	- `p_rfbuf_out`: elements of output vector.


Kernel name: `kfun_resamp_wrapper`
###################################

.. code-block::cpp
	    template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
	    void kfun_resamp_wrapper2(adf::output_buffer<T, adf::extents<LEN_IN> >& __restrict p_resamp_out,
                     adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,

                     adf::input_buffer<T, adf::extents<LEN_OUT> >& __restrict p_rfbuf_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_sample_in,
                     adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                     const int (&para_const)[INTERP_LEN32b_PARA]);


Read one betch rf data and resamp, output for gen-window.

- **Template params**:
	- `T`: type of the operation;
	- `LEN_OUT`: number of outputdata per invoking;
	- `LEN_IN`: number of inputdata per invoking;
    - `LEN_RF_IN`: no use for now;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `INTERP_LEN32b_PARA`: number of RTP data per invoking;
- **Function params**:
	- `para_const`: kernel_rfbuf self-used structural parameter include sampling frequency, inverse speed of sound and so on. Its definition can be seen in L1/include/kernel_interpolation.hpp.
	- `p_rfbuf_in`: elements of input rf data vector.
    - `p_sample_in`: elements of input sample address vector.
	- `p_inside_in`: elements of input inside(bool) vector.
	- `p_resamp_out`: elements of output result after resample.
    - `p_inside_out`: elements of output bypassed inside(bool) vector.


Kernel name: `kfun_genwin_wrapper`
###################################

.. code-block::cpp
	    template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
	    kfun_genwin_wrapper2(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_vec_out,
                            adf::output_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_out,
                            adf::input_buffer<T, adf::extents<LEN_IN> >& __restrict p_resamp_in,
                            adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                            const int (&para_const)[INTERP_LEN32b_PARA]);


Read one batch resamp data and generate window data output for interpolation.

- **Template params**:
	- `T`: type of the operation;
	- `LEN_OUT`: number of outputdata per invoking;
	- `LEN_IN`: number of inputdata per invoking;
    - `LEN_RF_IN`: no use for now;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `INTERP_LEN32b_PARA`: number of RTP data per invoking;
- **Function params**:
	- `para_const`: kernel_rfbuf self-used structural parameter include sampling frequency, inverse speed of sound, and so on. Its definition can be seen in L1/include/kernel_interpolation.hpp.
    - `p_resamp_in`: elements of input resample vector.
	- `p_inside_in`: elements of input inside(bool) vector.
	- `p_vec_out`: elements of output result after resample.
    - `p_inside_out`: elements of output bypassed inside(bool) vector.


Kernel name: `kfun_interpolation_wrapper`
###################################

.. code-block::cpp
	    template <typename T, int LEN_OUT, int LEN_IN, int LEN_RF_IN, int VECDIM, int INTERP_LEN32b_PARA>
	    void kfun_interpolation_wrapper(adf::output_buffer<T, adf::extents<LEN_OUT> >& __restrict p_interpolation,
                                        adf::input_buffer<T, adf::extents<LEN_OUT> >& __restrict p_vec_in,
                                        adf::input_buffer<int, adf::extents<LEN_IN> >& __restrict p_inside_in,
                                        const int (&para_const)[INTERP_LEN32b_PARA]);


Spline interpolation.

- **Template params**:
	- `T`: type of the operation;
	- `LEN_OUT`: number of outputdata per invoking;
	- `LEN_IN`: number of inputdata per invoking;
    - `LEN_RF_IN`: no use for now;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `INTERP_LEN32b_PARA`: number of RTP data per invoking;
- **Function params**:
	- `para_const`: kfun_interpolation_wrapper self-used structural parameter include sampling frequency, inverse speed of sound, and so on. Its definition can be seen in L1/include/kernel_interpolation.hpp.
    - `p_vec_in`: elements of resample and window vector.
	- `p_inside_in`: elements of input inside(bool) vector.
    - `p_interpolation`: elements of output bypassed inside(bool) vector.


kernel name: `kfun_mult_pre`
###########################################

.. code-block::cpp
		template <class T,
				int NUM_LINE_t,
				int NUM_ELEMENT_t,
				int NUM_SAMPLE_t,
				int NUM_SEG_t,
				int NUM_DEP_SEG_t,
				int VECDIM_mult_t,
				int LEN_IN_mult_t,
				int LEN_OUT_mult_t,
				int LEN32b_PARA_mult_t,
				int MULT_ID_t>
		void __attribute__((noinline)) kfun_mult_pre(const T (&para_const)[LEN32b_PARA_mult_t],
													adf::input_buffer<T, adf::extents<LEN_IN_mult_t> >& __restrict in_interp,
													adf::input_buffer<T, adf::extents<LEN_IN_mult_t / 4> >& __restrict in_apod,
													output_stream<accfloat>* p_out_cascade);


This kernel is used to compute the final result. It multiplies the results of apodization and the interpolation and pass the result to follow-up kfun_mult_cascade.

- **Template params**:
	- `T`: type of the operation;
	- `NUM_LINE_t`: number of scanlines;
	- `NUM_ELEMENT_t`: number of transducer's elements;
	- `NUM_SAMPLE_t`: number of time samples;
	- `NUM_SEG_t`: number of segments in one scanline;
	- `NUM_DEP_SEG_t`: number of data in one segment;
	- `VECDIM_mult_t`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `LEN_IN_mult_t`: number of inputdata per invoking;
	- `LEN_OUT_mult_t`: number of outputdata per invoking;
	- `LEN32b_PARA_mult_t`: number of inputdata per invoking;
	- `MULT_ID_t`: default ID of mult kernel;
- **kernel Inputs**:
	- `para_const`: kernel_mult's kernel kfun_mult_0 self-used structural parameter include iter_line, iter_element to record the number of lines processed;
	- `in_interp`: Results of interpolation.
	- `in_apod`: Results of apodization.
- **kernel Outputs**:
	- `p_out_cascade`: Output to next mult kernel;

kernel name: `kfun_mult_cascade`
###########################################

.. code-block::cpp
	template <class T,
			int NUM_LINE_t,
			int NUM_ELEMENT_t,
			int NUM_SAMPLE_t,
			int NUM_SEG_t,
			int NUM_DEP_SEG_t,
			int VECDIM_mult_t,
			int LEN_IN_mult_t,
			int LEN_OUT_mult_t,
			int LEN32b_PARA_mult_t,
			int MULT_ID_t>
	void __attribute__((noinline)) kfun_mult_cascade(const T (&para_const)[LEN32b_PARA_mult_t],
													const T (&local_data_0)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
													const T (&local_data_1)[NUM_DEP_SEG_t], // NUM_DEP_SEG_t
													input_stream<accfloat>* p_in_cascade,
													output_stream<accfloat>* p_out_cascade,
													output_stream<T>* p_out_mult);


This kernel is used to compute the final result. It accumulates the results of each transducer.

- **Template params**:
	- `T`: type of the operation;
	- `NUM_LINE_t`: number of scanlines;
	- `NUM_ELEMENT_t`: number of transducer's elements;
	- `NUM_SAMPLE_t`: number of time samples;
	- `NUM_SEG_t`: number of segments in one scanline;
	- `NUM_DEP_SEG_t`: number of data in one segment;
	- `VECDIM_mult_t`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `LEN_IN_mult_t`: number of inputdata per invoking;
	- `LEN_OUT_mult_t`: number of outputdata per invoking;
	- `LEN32b_PARA_mult_t`: number of inputdata per invoking;
	- `MULT_ID_t`: default ID of mult kernel;
- **kernel Inputs**:
	- `para_const`: kernel_mult's kernel kfun_mult_0 self-used structural parameter include iter_line, iter_element to record the number of lines processed;
	- `local_data_0`: kernel_mult's kernel kfun_mult_0 self-used local array, used to store accumulated temporary results;
	- `local_data_1`: kernel_mult's kernel kfun_mult_0 self-used local array, used to store accumulated temporary results. To avoid bank conflict, this data memory is applied;
	- `p_in_cascade`: Results of kfun_mult_pre.
- **kernel Outputs**:
	- `p_out_cascade`: Output to next mult kernel;
	- `p_out_mult`: Output one segment of final result;


Kernel name: `absV`
###################################


.. code-block::cpp
	    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	    void absV(adf::input_buffer<T>& __restrict in, adf::output_buffer<T>& __restrict out);


Element-Wise absolute values of a vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
Kernel name: `cosV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void cosV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);


Element-Wise cosine values of a vector. Elements must be expressed in radians with the range [0...2k$\pi$] 

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `diffMV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void diffMV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise difference between the of the row of a matrix and the values of a vector. The number of the column of the matrix and the entry of the vector must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first matrix to be passed to the kernel.
	- `in2`: elements of the array to be passed to the kernel.
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

Kernel name: `diffSV`
###################################


.. code-block::cpp
    template<typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
    void diffSV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise difference between a scalar and the values of a vector. For every iteration (expressed by `LEN`), pass four times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: scalar element to be passed to the kernel.
	- `in2`: elements of the array to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `diffVS`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void diffVS(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise difference between the values of a vector and a scalar. For every iteration (expressed by `LEN`), pass four times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
Kernel name: `divVS`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void divVSSpeedOfSound(adf::input_buffer<T>& in1, adf::output_buffer<T>& out);


Element-Wise division between the values of a vector and a scalar (SpeedOfSound). For every iteration (expressed by `LEN`), pass four times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
Kernel name: `equalS`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
    void equalS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);


Check whether the element of an array are equal to a specific number. An array of 0s or 1s is returned. 1 means that the element at that specific position is equal to the scalar; otherwise, 0 is returned.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `SCALAR`: scalar to which the elements of the array are compared to;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.
	
Kernel name: `lessOrEqualThanS`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
    void lessOrEqualThanS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);


Check whether the element of an array are less or equal to a specific number. An array of 0s or 1s is returned. 1 means that the element at that specific position is less or equal to the scalar. Otherwise, 0 is returned.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
	- `SCALAR`: scalar to which the elements of the array are compared to;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `mulMM`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void mulMM(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise multiplication of two matrixes. The first matrix and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first matrix to be passed to the kernel.
	- `in2`: elements of the second matrix to be passed to the kernel.
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

Kernel name: `mulVS`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void mulVS(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise multiplication between the values of a vector and a scalar. For every iteration (expressed by `LEN`), pass four times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter which indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `mulVV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void mulVV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise multiplication of two vectors. The first vector and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first vector to be passed to the kernel.
	- `in2`: elements of the second vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `norm_axis_1`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
    void norm_axis_1(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);


Perform row wise the euclidean norm of a matrix of the columns. Because for every row returns a number, the result is a vector of values that represent for every row the magnitude of the euclidean norm.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `ones`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void ones(adf::output_buffer<T>& __restrict out);


Return a vector with all entry set to 1. 

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `outer`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void outer(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Perform the outer product (also named cross-product or vector-product) between the two vectors. The result of this operation is a matrix, which rows are the number of the entry of the first vector and the column the number of the entry of the second one.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first vector to be passed to the kernel.
	- `in2`: elements of the second vector to be passed to the kernel.
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.

Kernel name: `reciprocalV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void reciprocalV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);


Element-wise inverse operation of the entry of the vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `sqrtV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void sqrtV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);



Element-wise square root operation of the entry of the vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `squareV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void squareV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);



Element-wise square operation of the entry of the vector.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `sum_axis_1`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void sum_axis_1(adf::input_buffer<T>& in1, adf::output_buffer<T>& out);



Perform row wise the reduce add of a matrix of the columns. Because for every row returns a number, the result is a vector of values that represent the magnitude of the reduce add operation for every row.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `sumMM`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void sumMM(adf::input_buffer<T>& in1, adf::input_buffer<T>& in2, adf::output_buffer<T>& out);



Element-Wise sum of two matrixes. The first matrix and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first matrix to be passed to the kernel.
	- `in2`: elements of the second matrix to be passed to the kernel.
	- `out`: elements of the result of the operation to be passed from the kernel.

Kernel name: `sumVS`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void sumVSStream(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise addition between the values of a vector and a scalar. For every iteration (expressed by `LEN`) we need to pass four times the scalar value to the stream of the scalar value.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the array to be passed to the kernel.
	- `in2`: scalar element to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `sumVV`
###################################


.. code-block::cpp
    template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void sumVV(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);


Element-Wise addition of two vectors. The first vector and the second one must have the same size.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `in1`: elements of the first vector to be passed to the kernel.
	- `in2`: elements of the second vector to be passed to the kernel.
	- `out`: elements of the result of the operation (vector) to be passed from the kernel.

Kernel name: `tileV`
###################################


.. code-block::cpp
	template<typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
	void tileVApo(adf::output_buffer<T>& __restrict out);

This kernel create a vector and returns it `LEN` times.

- **Template params**:
	- `T`: type of the operation;
	- `LEN`: number of elements to be processed in the kernel per iteration;
	- `INCREMENT`: parameter that indicates how much iterations have been performed by the SIMD with respect to the intended total length;
	- `VECDIM`: dimension of the SIMD to be performed. Addressed in the Xilinx UG1076, it depends on the type chosen;
- **Function params**:
	- `out`: elements of the result of the operation (matrix) to be passed from the kernel.