.. _global-namespace:

Global Namespace
================

.. index:: pair: namespace; global

.. toctree::
	:hidden:

	namespace_xf.rst
	enum_Dirction_QEI.rst
	enum_Encoding_Mode.rst
	struct_AxiParameters_QEI.rst
	class_RangeTracer.rst
	class_VarRec.rst



Overview
~~~~~~~~



.. _doxid-foc_8hpp_1ac0d91b6b18b660ed4d49625c6a2fbace:
.. _cid-ranger:
.. _doxid-foc_8hpp_1acf70e2f51bab60f8bb35ed4b20007e60:
.. _cid-cos_table:
.. _doxid-qei_8hpp_1a6c3f9008da58afbbcdb12cd83f07feed:
.. _cid-range_qei_freq:
.. _doxid-qei_8hpp_1a07a50eae34b08ecc911d6c6af2b10b26:
.. _cid-range_qei_cpr:
.. _doxid-foc_8hpp_1aa361ed00199d545e61d798f829d7b158:
.. _cid-rangetracer:
.. ref-code-block:: cpp
	:class: overview-code-block

	// namespaces

	namespace :ref:`xf<doxid-namespacexf>`
	    namespace :ref:`xf::motorcontrol<doxid-namespacexf_1_1motorcontrol>`
	        namespace :ref:`xf::motorcontrol::details<doxid-namespacexf_1_1motorcontrol_1_1details>`

	// enums

	enum :ref:`Dirction_QEI<doxid-qei_8hpp_1afb9daed2d5861588049366eb0a39955a>`
	enum :ref:`Encoding_Mode<doxid-qei_8hpp_1a93119817305e8990d66633467a1be68e>`

	// structs

	struct :ref:`AxiParameters_QEI<doxid-struct_axi_parameters___q_e_i>` 

	// classes

	class :ref:`RangeTracer<doxid-class_range_tracer>` 
	class :ref:`VarRec<doxid-class_var_rec>` 

	// global variables

	:ref:`RangeTracer<doxid-class_range_tracer>` ranger
	static short :ref:`sin_table<doxid-foc_8hpp_1a65d225f3b30cfd94fa26501dc2fcc216>`[(1000)]
	static short cos_table[(1000)]
	static RangeDef <ap_uint <32>> RANGE_qei_freq
	static RangeDef <ap_uint <32>> RANGE_qei_cpr

	// macros

	#define :ref:`QEI_MAX_NO_EDGE_CYCLE<doxid-qei_8hpp_1a8696e15e85e4dbab2869458a235b2b19>`

	#define RANGETRACER( \
	    n, \
	    v \
	    )

Global Variables
----------------

.. _doxid-foc_8hpp_1a65d225f3b30cfd94fa26501dc2fcc216:
.. _cid-sin_table:
.. ref-code-block:: cpp
	:class: title-code-block

	static short sin_table [(1000)]

Lookup table for the cosine function in the Q16.16 format. CPR(COMM_MACRO_CPR) Number of encoder steps per one full revolution.

.. Important:: Update this table when the encoder resolution has changed, that is, when #CPR has changed.

Macros
------

.. _doxid-qei_8hpp_1a8696e15e85e4dbab2869458a235b2b19:
.. _cid-qei_max_no_edge_cycle:
.. ref-code-block:: cpp
	:class: title-code-block

	#define QEI_MAX_NO_EDGE_CYCLE

Constant values.

