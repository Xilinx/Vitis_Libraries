.. index:: pair: struct; xf::motorcontrol::details::pwmStrmIO
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1pwm_strm_i_o:
.. _cid-xf::motorcontrol::details::pwmstrmio:

template struct xf::motorcontrol::details::pwmStrmIO
====================================================

.. toctree::
	:hidden:

.. code-block:: cpp
	:class: overview-code-block

	#include "svpwm.hpp"




.. _doxid-structxf_1_1motorcontrol_1_1details_1_1pwm_strm_i_o_1a9ab6c2f6a03394b535da9c9a10f47eef:
.. _cid-xf::motorcontrol::details::pwmstrmio::va_cmd:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1pwm_strm_i_o_1a5d25a83b6b93464a7cebb9e06eb9ffad:
.. _cid-xf::motorcontrol::details::pwmstrmio::vb_cmd:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1pwm_strm_i_o_1a5c30267cc46aebb7d987bb2d9d91cb62:
.. _cid-xf::motorcontrol::details::pwmstrmio::vc_cmd:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1pwm_strm_i_o_1a407946b0018c1c2b5224118a6b7e0a2f:
.. _cid-xf::motorcontrol::details::pwmstrmio::dc_link:
.. _doxid-structxf_1_1motorcontrol_1_1details_1_1pwm_strm_i_o_1af35a740772ebd19659b97064f7df2f0a:
.. _cid-xf::motorcontrol::details::pwmstrmio::isend_cond:
.. ref-code-block:: cpp
	:class: overview-code-block

	template <class T_PWM_IO>
	struct pwmStrmIO

	// fields

	T_PWM_IO Va_cmd
	T_PWM_IO Vb_cmd
	T_PWM_IO Vc_cmd
	T_PWM_IO dc_link
	bool isEnd_cond

