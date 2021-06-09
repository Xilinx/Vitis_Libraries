.. index:: pair: class; create_casc_kernel_recur
.. _doxid-classcreate__casc__kernel__recur:
.. _cid-create_casc_kernel_recur:

template class create_casc_kernel_recur
=======================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "fft_ifft_dit_1ch_cascade.hpp"




.. _doxid-classcreate__casc__kernel__recur_1a086d0c2d60cc4d6f9973c1bdb2e0ab0a:
.. _cid-create_casc_kernel_recur::create:
.. ref-code-block:: cpp
	:class: overview-code-block

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
	class create_casc_kernel_recur

