.. index:: pair: struct; xf::motorcontrol::details::gen_sampler_pkg
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg:

template struct xf::motorcontrol::details::gen_sampler_pkg
==========================================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "svpwm.hpp"




.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1a77ea9a17fdf5f59ac9c28bf29fde081e:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::duty_ratio_a:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1a97ec5158fa89fb93cf4d636f39769082:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::duty_ratio_b:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1ab235bbc99d498410542d47b0baa02dd1:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::duty_ratio_c:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1af29f0a3705a90bc26356c41c23dab569:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::pwm_freq:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1a0c1476a62b235e5480ebace5bb98a6c2:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::dead_cycles:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1aeb92d69147324852ebbe688f8639f907:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::phase_shift:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1gen__sampler__pkg_1ab28f84109e2d4a5ed61097be15630c23:
.. _cid-xf::motorcontrol::details::gen_sampler_pkg::trip_cnt:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <class T_IN>
	struct gen_sampler_pkg

	// fields

	T_IN duty_ratio_a
	T_IN duty_ratio_b
	T_IN duty_ratio_c
	int pwm_freq
	int dead_cycles
	int phase_shift
	long trip_cnt

