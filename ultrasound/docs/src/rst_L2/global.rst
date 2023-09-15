.. _global-namespace:

Global Namespace
================

.. index:: pair: namespace; global

.. toctree::
	:hidden:

	namespace_adf.rst
	namespace_aie.rst
	namespace_us.rst
	class_graph_scanline.rst





.. _doxid-graph__scanline_8hpp_1abec4a056107c79d9df31773784e53e39:
.. _cid-enable_apodi_main:
.. _doxid-graph__scanline_8hpp_1ae891975952b87c971c26f4d707c86386:
.. _cid-enable_apodi_pre:
.. _doxid-graph__scanline_8hpp_1acf3394c2dca022fcd6a354c4f2e65d5a:
.. _cid-enable_delay:
.. _doxid-graph__scanline_8hpp_1adac8d665a98da964dcd5956829cba521:
.. _cid-enable_foc:
.. _doxid-graph__scanline_8hpp_1a16db9decb39aaaa60ccdd582c2044ac6:
.. _cid-enable_interpolation:
.. _doxid-graph__scanline_8hpp_1af342e576a0537f8d274adb6e7c6fe0b9:
.. _cid-enable_mult:
.. _doxid-graph__scanline_8hpp_1a17f222d9256a28425507a8839e750d1b:
.. _cid-enable_sample:
.. ref-code-block:: cpp
	:class: overview-code-block

	// namespaces

	namespace :ref:`adf<doxid-namespaceadf>`
	namespace :ref:`aie<doxid-namespaceaie>`
	namespace :ref:`us<doxid-namespaceus>`
	    namespace :ref:`us::L2<doxid-namespaceus_1_1_l2>`

	// classes

	template <
	    class T,
	    int NUM_LINE_t,
	    int NUM_ELEMENT_t,
	    int NUM_SAMPLE_t,
	    int NUM_SEG_t,
	    int NUM_DEP_SEG_t,
	    int VECDIM_img_t,
	    int LEN_OUT_img_t,
	    int LEN32b_PARA_img_t,
	    int VECDIM_foc_t,
	    int LEN_OUT_foc_t,
	    int LEN32b_PARA_foc_t,
	    int VECDIM_delay_t,
	    int LEN_IN_delay_t,
	    int LEN_OUT_delay_t,
	    int LEN32b_PARA_delay_t,
	    int VECDIM_apodi_t,
	    int LEN_IN_apodi_t,
	    int LEN_OUT_apodi_t,
	    int LEN32b_PARA_apodi_t,
	    int LEN_IN_apodi_f_t,
	    int LEN_IN_apodi_d_t,
	    int VECDIM_interp_t,
	    int LEN_IN_interp_t,
	    int LEN_IN_interp_rf_t,
	    int LEN_OUT_interp_t,
	    int NUM_UPSample_t,
	    int LEN32b_PARA_interp_t,
	    int VECDIM_sample_t,
	    int LEN_IN_sample_t,
	    int LEN_OUT_sample_t,
	    int LEN32b_PARA_sample_t,
	    int VECDIM_mult_t,
	    int LEN_IN_mult_t,
	    int LEN_OUT_mult_t,
	    int NUM_DEP_SEG_mult_t,
	    int MULT_ID_t,
	    int LEN32b_PARA_mult_t
	    >
	class :ref:`graph_scanline<doxid-classgraph__scanline>` 

	// macros

	#define ENABLE_APODI_MAIN
	#define ENABLE_APODI_PRE
	#define ENABLE_DELAY
	#define ENABLE_FOC
	#define ENABLE_INTERPOLATION
	#define ENABLE_MULT
	#define ENABLE_SAMPLE

