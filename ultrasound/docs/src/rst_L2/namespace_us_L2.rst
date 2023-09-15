.. index:: pair: namespace; L2
.. _doxid-namespaceus_1_1_l2:
.. _cid-us::l2:

namespace L2
============

.. toctree::
	:hidden:

	class_us_L2_apodi_main_graph.rst
	class_us_L2_apodi_pre_graph.rst
	class_us_L2_apodization_graph.rst
	class_us_L2_apodization_sa_graph.rst
	class_us_L2_bSpline_graph.rst
	class_us_L2_delay_graph.rst
	class_us_L2_delay_graph_wrapper.rst
	class_us_L2_delay_pw_graph.rst
	class_us_L2_focusing_graph.rst
	class_us_L2_focusing_sa_graph.rst
	class_us_L2_graph_foc_wrapper.rst
	class_us_L2_graph_img_wrapper.rst
	class_us_L2_imagePoints_graph.rst
	class_us_L2_interpolation_graph.rst
	class_us_L2_interpolation_graph_scaler_shell.rst
	class_us_L2_mult_graph_wrapper.rst
	class_us_L2_sample_graph_wrapper.rst
	class_us_L2_samples_graph.rst





.. ref-code-block:: cpp
	:class: overview-code-block

	// classes

	template <
	    typename T,
	    int NUM_LINE,
	    int NUM_ELEMENT,
	    int NUM_SAMPLE,
	    int NUM_SEG,
	    int LEN_OUT,
	    int LEN_IN_F,
	    int LEN_IN_D,
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class :ref:`apodi_main_graph<doxid-classus_1_1_l2_1_1apodi__main__graph>` 

	template <
	    typename T,
	    int NUM_LINE,
	    int NUM_ELEMENT,
	    int NUM_SAMPLE,
	    int NUM_SEG,
	    int LEN_OUT,
	    int LEN_IN,
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class :ref:`apodi_pre_graph<doxid-classus_1_1_l2_1_1apodi__pre__graph>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class :ref:`apodization_graph<doxid-classus_1_1_l2_1_1apodization__graph>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class :ref:`apodization_sa_graph<doxid-classus_1_1_l2_1_1apodization__sa__graph>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class :ref:`bSpline_graph<doxid-classus_1_1_l2_1_1b_spline__graph>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class :ref:`delay_graph<doxid-classus_1_1_l2_1_1delay__graph>` 

	template <
	    class T,
	    int NUM_LINE_t,
	    int VECDIM_delay_t,
	    int LEN_IN_delay_t,
	    int LEN_OUT_delay_t,
	    int LEN32b_PARA_delay_t
	    >
	class :ref:`delay_graph_wrapper<doxid-classus_1_1_l2_1_1delay__graph__wrapper>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class :ref:`delay_pw_graph<doxid-classus_1_1_l2_1_1delay__pw__graph>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH
	    >
	class :ref:`focusing_graph<doxid-classus_1_1_l2_1_1focusing__graph>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH_
	    >
	class :ref:`focusing_sa_graph<doxid-classus_1_1_l2_1_1focusing__sa__graph>` 

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int VECDIM_foc_t,
	    int LEN32b_PARA_foc_t
	    >
	class :ref:`graph_foc_wrapper<doxid-classus_1_1_l2_1_1graph__foc__wrapper>` 

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int VECDIM_img_t,
	    int LEN_OUT_img_t,
	    int LEN32b_PARA_img_t
	    >
	class :ref:`graph_img_wrapper<doxid-classus_1_1_l2_1_1graph__img__wrapper>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH
	    >
	class :ref:`imagePoints_graph<doxid-classus_1_1_l2_1_1image_points__graph>` 

	template <
	    typename T,
	    int NUM_LINE,
	    int NUM_ELEMENT,
	    int NUM_SAMPLE,
	    int NUM_SEG,
	    int LEN_OUT,
	    int LEN_IN,
	    int LEN_RF_IN,
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class :ref:`interpolation_graph<doxid-classus_1_1_l2_1_1interpolation__graph>` 

	template <
	    typename T,
	    int NUM_LINE,
	    int NUM_ELEMENT,
	    int NUM_SAMPLE,
	    int NUM_SEG,
	    int LEN_OUT,
	    int LEN_IN,
	    int LEN_RF_IN,
	    int VECDIM,
	    int APODI_PRE_LEN32b_PARA
	    >
	class :ref:`interpolation_graph_scaler_shell<doxid-classus_1_1_l2_1_1interpolation__graph__scaler__shell>` 

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int NUM_DEP_SEG_t,
	    int VECDIM_mult_t,
	    int LEN_IN_mult_t,
	    int LEN_OUT_mult_t,
	    int LEN32b_PARA_mult_t,
	    int MULT_ID_t
	    >
	class :ref:`mult_graph_wrapper<doxid-classus_1_1_l2_1_1mult__graph__wrapper>` 

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int VECDIM_sample_t,
	    int LEN_IN_sample_t,
	    int LEN_OUT_sample_t,
	    int LEN32b_PARA_sample_t
	    >
	class :ref:`sample_graph_wrapper<doxid-classus_1_1_l2_1_1sample__graph__wrapper>` 

	template <
	    typename T = float,
	    unsigned int LENGTH_ = LENGTH,
	    unsigned int SPACE_DIMENSION_ = SPACE_DIMENSION,
	    unsigned int INCREMENT_VECTOR_ = INCREMENT_VECTOR,
	    unsigned int INCREMENT_MATRIX_ = INCREMENT_MATRIX,
	    unsigned int SIMD_DEPTH_ = SIMD_DEPTH,
	    unsigned int DIM_VECTOR_ = LENGTH,
	    unsigned int DIM_MATRIX_ = (LENGTH * SPACE_DIMENSION)
	    >
	class :ref:`samples_graph<doxid-classus_1_1_l2_1_1samples__graph>` 

