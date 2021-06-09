.. index:: pair: class; create_casc_kernel
.. _doxid-classcreate__casc__kernel:
.. _cid-create_casc_kernel:

template class create_casc_kernel
=================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "fft_ifft_dit_1ch_cascade.hpp"




.. _doxid-classcreate__casc__kernel_1a230d0e09b0ffe101ee9c5674969f7942:
.. _cid-create_casc_kernel::t_internaldatatype:
.. _doxid-classcreate__casc__kernel_1a99570f6622458896e53aa6dbfdb48d0d:
.. _cid-create_casc_kernel::create:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <
	    int dim,
	    typename TT_DATA,
	    typename TT_TWIDDLE,
	    unsigned int TP_POINT_SIZE,
	    unsigned int TP_FFT_NIFFT,
	    unsigned int TP_SHIFT,
	    unsigned int TP_CASC_LEN
	    >
	class create_casc_kernel

	// typedefs

	typedef std::conditional <std::is_same <TT_DATA, cint16>::value, cint32_t, TT_DATA>::type T_internalDataType

