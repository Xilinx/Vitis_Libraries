.. _global-namespace:

Global Namespace
================

.. index:: pair: namespace; global

.. toctree::
	:hidden:

	namespace_adf.rst
	namespace_xf.rst
	class_create_casc_kernel.rst
	class_create_casc_kernel-2.rst
	class_create_casc_kernel_recur.rst
	class_create_casc_kernel_recur-2.rst





.. _doxid-fft__bufs_8h_1a94fa2c69972b4d73ba48ec4807df13f6:
.. _cid-chess_storage:
.. _doxid-fft__bufs_8h_1a30dabd3906bd62b1867f98d99d8e20e1:
.. _cid-chess_storage-2:
.. _doxid-fft__ifft__dit__1ch__graph_8hpp_1a20336fc81ebceab8ae0f0650eadbaeef:
.. _cid-ceil:
.. ref-code-block:: cpp
	:class: overview-code-block

	// namespaces

	namespace :ref:`adf<doxid-namespaceadf>`
	namespace :ref:`xf<doxid-namespacexf>`
	    namespace :ref:`xf::dsp<doxid-namespacexf_1_1dsp>`
	        namespace :ref:`xf::dsp::aie<doxid-namespacexf_1_1dsp_1_1aie>`
	            namespace :ref:`xf::dsp::aie::blas<doxid-namespacexf_1_1dsp_1_1aie_1_1blas>`
	                namespace :ref:`xf::dsp::aie::blas::matrix_mult<doxid-namespacexf_1_1dsp_1_1aie_1_1blas_1_1matrix__mult>`
	            namespace :ref:`xf::dsp::aie::fft<doxid-namespacexf_1_1dsp_1_1aie_1_1fft>`
	                namespace :ref:`xf::dsp::aie::fft::dit_1ch<doxid-namespacexf_1_1dsp_1_1aie_1_1fft_1_1dit__1ch>`
	            namespace :ref:`xf::dsp::aie::fir<doxid-namespacexf_1_1dsp_1_1aie_1_1fir>`
	                namespace :ref:`xf::dsp::aie::fir::decimate_asym<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1decimate__asym>`
	                namespace :ref:`xf::dsp::aie::fir::decimate_hb<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1decimate__hb>`
	                namespace :ref:`xf::dsp::aie::fir::decimate_sym<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1decimate__sym>`
	                namespace :ref:`xf::dsp::aie::fir::interpolate_asym<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1interpolate__asym>`
	                namespace :ref:`xf::dsp::aie::fir::interpolate_fract_asym<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1interpolate__fract__asym>`
	                namespace :ref:`xf::dsp::aie::fir::interpolate_hb<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1interpolate__hb>`
	                namespace :ref:`xf::dsp::aie::fir::sr_asym<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1sr__asym>`
	                namespace :ref:`xf::dsp::aie::fir::sr_sym<doxid-namespacexf_1_1dsp_1_1aie_1_1fir_1_1sr__sym>`
	            namespace :ref:`xf::dsp::aie::widget<doxid-namespacexf_1_1dsp_1_1aie_1_1widget>`
	                namespace :ref:`xf::dsp::aie::widget::api_cast<doxid-namespacexf_1_1dsp_1_1aie_1_1widget_1_1api__cast>`
	                namespace :ref:`xf::dsp::aie::widget::real2complex<doxid-namespacexf_1_1dsp_1_1aie_1_1widget_1_1real2complex>`

	// classes

	template <
	    int dim,
	    typename TT_DATA,
	    typename TT_TWIDDLE,
	    unsigned int TP_POINT_SIZE,
	    unsigned int TP_FFT_NIFFT,
	    unsigned int TP_SHIFT,
	    unsigned int TP_CASC_LEN
	    >
	class :ref:`create_casc_kernel<doxid-classcreate__casc__kernel>` 

	template <
	    int dim,
	    unsigned int TP_POINT_SIZE,
	    unsigned int TP_FFT_NIFFT,
	    unsigned int TP_SHIFT,
	    unsigned int TP_CASC_LEN
	    >
	class :ref:`create_casc_kernel <dim, cfloat, cfloat, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN><doxid-classcreate__casc__kernel_3_01dim_00_01cfloat_00_01cfloat_00_01_t_p___p_o_i_n_t___s_i_z_e_00_01_5e33aad387a48d90f9881bde9de1be36>` 

	template <
	    int dim,
	    typename TT_DATA,
	    typename TT_INT_DATA,
	    typename TT_TWIDDLE,
	    unsigned int TP_POINT_SIZE,
	    unsigned int TP_FFT_NIFFT,
	    unsigned int TP_SHIFT,
	    unsigned int TP_CASC_LEN,
	    unsigned int TP_END_RANK,
	    unsigned int TP_RANKS_PER_KERNEL
	    >
	class :ref:`create_casc_kernel_recur<doxid-classcreate__casc__kernel__recur>` 

	template <
	    typename TT_DATA,
	    typename TT_INT_DATA,
	    typename TT_TWIDDLE,
	    unsigned int TP_POINT_SIZE,
	    unsigned int TP_FFT_NIFFT,
	    unsigned int TP_SHIFT,
	    unsigned int TP_CASC_LEN,
	    unsigned int TP_END_RANK,
	    unsigned int TP_RANKS_PER_KERNEL
	    >
	class :ref:`create_casc_kernel_recur <1, TT_DATA, TT_INT_DATA, TT_TWIDDLE, TP_POINT_SIZE, TP_FFT_NIFFT, TP_SHIFT, TP_CASC_LEN, TP_END_RANK, TP_RANKS_PER_KERNEL><doxid-classcreate__casc__kernel__recur_3_011_00_01_t_t___d_a_t_a_00_01_t_t___i_n_t___d_a_t_a_00_01_t_t03876e192f4c23116830e13c34e8e459>` 

	// macros

	#define CEIL( \
	    x, \
	    y \
	    )

