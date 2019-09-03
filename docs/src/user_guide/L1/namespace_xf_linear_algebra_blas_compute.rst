.. index:: pair: namespace; xf::linear_algebra::blas
.. _doxid-namespacexf_1_1linear__algebra_1_1blas:

namespace xf::linear_algebra::blas
==================================

.. toctree::
	:hidden:

Overview
~~~~~~~~






.. index:: pair: variable; t_LogParEntries
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de:
.. index:: pair: variable; l_x
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa6d3a215d29893c5e08ed5eabc531615:
.. index:: pair: variable; l_y
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aae9defdf0f516e81aa3eb5061e592c55:




.. index:: pair: function; gbmv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ac1faa34d1380c4be82eb743b015f15d6:
.. index:: pair: function; gemv< t_DataType, t_LogParEntries, t_IndexType >
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a64bfd51bd92c789fd505acb623cf4969:
.. index:: pair: function; scal< t_DataType, 1, t_IndexType >
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa293fd8f2b06012eb1ee18ab6dbe5235:
.. index:: pair: function; axpy< t_DataType, 1, t_IndexType >
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a1c88906b679045a07a9a1bceef8175b3:
.. index:: pair: function; symv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a86a8ed1e692d38fc4ede5c9d78fc60d2:
.. index:: pair: function; trmv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ad541c7b428af8ada9a947a3d1f1b2285:




.. ref-code-block:: cpp
	:class: overview-code-block

	
	namespace blas {

	// global variables

	 t_LogParEntries;
	hls::stream<WideType<t_DataType, 1>> l_x;
	hls::stream<WideType<t_DataType, 1>> l_y;

	// global functions

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType>
	void :ref:`amax<doxid-namespacexf_1_1linear__algebra_1_1blas_1abd20b28a989c26e4bd5d528bf67ebea2>`(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_IndexType& p_result);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType>
	void :ref:`amin<doxid-namespacexf_1_1linear__algebra_1_1blas_1a401cfc76276132b7c8a58886098402c2>`(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_IndexType& p_result);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void :ref:`asum<doxid-namespacexf_1_1linear__algebra_1_1blas_1af4e5b7396fd154b38ae063f60cc9f0cb>`(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_DataType& p_sum);

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void :ref:`axpy<doxid-namespacexf_1_1linear__algebra_1_1blas_1affd2cbe50983ea31c07c03845e3a28c4>`(unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y, hls::stream<WideType<t_DataType, t_ParEntries>>& p_r);

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void :ref:`copy<doxid-namespacexf_1_1linear__algebra_1_1blas_1abc1e802e40cf7f7e0e8106fb7d6f102e>`(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y);

	template  <typename t_DataType, unsigned int t_N, unsigned int t_NumDiag, unsigned int t_EntriesInParallel>
	void :ref:`dimv<doxid-namespacexf_1_1linear__algebra_1_1blas_1abc102f6031b8a65af9b1a2cb6132a74b>`(t_DataType p_in [t_N][t_NumDiag], t_DataType p_inV [t_N], unsigned int p_n, t_DataType p_outV [t_N]);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void :ref:`dot<doxid-namespacexf_1_1linear__algebra_1_1blas_1ae529068e5f8a2780987fa6263d609e5a>`(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_y, t_DataType& p_res);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_MaxRows, typename t_IndexType = unsigned int, typename t_MacType = t_DataType>
	void gbmv(const unsigned int p_m, const unsigned int p_n, const unsigned int p_kl, const unsigned int p_ku, hls::stream<WideType<t_DataType, t_ParEntries>>& p_A, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_MacType, t_ParEntries>>& p_y);

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_MaxRows, typename t_IndexType = unsigned int, typename t_MacType = t_DataType>
	void :ref:`gbmv<doxid-namespacexf_1_1linear__algebra_1_1blas_1a31d94a92d733f4cf399f814f9106bb3b>`(const unsigned int p_m, const unsigned int p_n, const unsigned int p_kl, const unsigned int p_ku, const t_DataType p_alpha, hls::stream<WideType<t_DataType, t_ParEntries>>& p_M, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y, hls::stream<WideType<t_DataType, t_ParEntries>>& p_yr);

	gemv< t_DataType, t_LogParEntries, t_IndexType >(p_m, p_n, p_M, p_x, :ref:`l_x<doxid-namespacexf_1_1linear__algebra_1_1blas_1aa6d3a215d29893c5e08ed5eabc531615>`);
	scal< t_DataType, 1, t_IndexType >(p_m, p_beta, p_y, :ref:`l_y<doxid-namespacexf_1_1linear__algebra_1_1blas_1aae9defdf0f516e81aa3eb5061e592c55>`);
	axpy< t_DataType, 1, t_IndexType >(p_m, p_alpha, :ref:`l_x<doxid-namespacexf_1_1linear__algebra_1_1blas_1aa6d3a215d29893c5e08ed5eabc531615>`, :ref:`l_y<doxid-namespacexf_1_1linear__algebra_1_1blas_1aae9defdf0f516e81aa3eb5061e592c55>`, p_yr);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void :ref:`nrm2<doxid-namespacexf_1_1linear__algebra_1_1blas_1a048b14da86cf49591f68457007a8a1da>`(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_DataType& p_res);

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void :ref:`scal<doxid-namespacexf_1_1linear__algebra_1_1blas_1aee44ac6a6a4df8cc69d63e1ea8381f40>`(unsigned int p_n, t_DataType p_alpha, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_res);

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void :ref:`swap<doxid-namespacexf_1_1linear__algebra_1_1blas_1aa7d231959e217bc6288ee4fd60274445>`(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y, hls::stream<WideType<t_DataType, t_ParEntries>>& p_xRes, hls::stream<WideType<t_DataType, t_ParEntries>>& p_yRes);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void symv(const unsigned int p_n);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void :ref:`symv<doxid-namespacexf_1_1linear__algebra_1_1blas_1aa0f4cd1d5ddd38ec0bea4431ada7604c>`(const unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_M, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_y, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_yr);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int, typename t_MacType = t_DataType>
	void trmv(const bool uplo, const unsigned int p_n);

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void :ref:`trmv<doxid-namespacexf_1_1linear__algebra_1_1blas_1aa7a9cb6d049a99fe8786a943d3a8ad40>`(const bool uplo, const unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_M, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType, 1>>& p_y, hls::stream<WideType<t_DataType, 1>>& p_yr);

	} // namespace blas
.. _details-doxid-namespacexf_1_1linear__algebra_1_1blas:

Detailed Documentation
~~~~~~~~~~~~~~~~~~~~~~



Global Functions
----------------

amax
#####

.. index:: pair: function; amax
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1abd20b28a989c26e4bd5d528bf67ebea2:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType>
	void amax(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_IndexType& p_result)

amax function that returns the position of the vector element that has the maximum magnitude.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of stided entries entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_result

		- the resulting index, which is 0 if p_n <= 0

amin
#####

.. index:: pair: function; amin
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a401cfc76276132b7c8a58886098402c2:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType>
	void amin(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_IndexType& p_result)

amin function that returns the position of the vector element that has the minimum magnitude.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_result

		- the resulting index, which is 0 if p_n <= 0

asum
#####

.. index:: pair: function; asum
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1af4e5b7396fd154b38ae063f60cc9f0cb:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void asum(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_DataType& p_sum)

asum function that returns the sum of the magnitude of vector elements.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_sum

		- the sum, which is 0 if p_n <= 0

axpy
#####

.. index:: pair: function; axpy
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1affd2cbe50983ea31c07c03845e3a28c4:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void axpy(unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y, hls::stream<WideType<t_DataType, t_ParEntries>>& p_r)

axpy function that compute Y = alpha*X + Y.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in the input vector p_x, p_n % t_ParEntries == 0

	*
		- p_x

		- the input stream of packed entries of vector X

	*
		- p_y

		- the input stream of packed entries of vector Y

	*
		- p_r

		- the output stream of packed entries of result vector Y

copy
#####

.. index:: pair: function; copy
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1abc1e802e40cf7f7e0e8106fb7d6f102e:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void copy(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y)

copy function that compute Y = X



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the packed input vector stream

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in vector X and Y

	*
		- p_x

		- the packed input vector stream

	*
		- p_y

		- the packed output vector stream

dot
####

.. index:: pair: function; dot
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1ae529068e5f8a2780987fa6263d609e5a:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void dot(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_y, t_DataType& p_res)

dot function that returns the dot product of vector x and y.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_res

		- the dot product of x and y

gbmv
#####

.. index:: pair: function; gbmv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a31d94a92d733f4cf399f814f9106bb3b:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, unsigned int t_MaxRows, typename t_IndexType = unsigned int, typename t_MacType = t_DataType>
	void gbmv(const unsigned int p_m, const unsigned int p_n, const unsigned int p_kl, const unsigned int p_ku, const t_DataType p_alpha, hls::stream<WideType<t_DataType, t_ParEntries>>& p_M, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y, hls::stream<WideType<t_DataType, t_ParEntries>>& p_yr)

gbmv function performs general banded matrix-vector multiplication matrix and a vector y = alpha * M * x + beta * y



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_ParEntries

		- the number of parallelly processed entries in the input vector

	*
		- t_MaxRows

		- the maximum size of buffers for output vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- t_MacType

		- the datatype of the output stream

	*
		- p_m

		- the number of rows of input matrix p_M

	*
		- p_alpha

		- 

	*
		- scalar

		- alpha

	*
		- p_M

		- the input stream of packed Matrix entries

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_beta

		- 

	*
		- scalar

		- beta

	*
		- p_y

		- the output vector

gemv
#####


.. index:: pair: function; gemv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a4809493026ae2969f50665f611379657:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void gemv(const unsigned int p_m, const unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType,(1<<t_LogParEntries)>>& p_M, hls::stream<WideType<t_DataType,(1<<t_LogParEntries)>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType, 1>>& p_y, hls::stream<WideType<t_DataType, 1>>& p_yr)

gemv function that returns the result vector of the mutiplication of a matrix and a vector y = alpha * M * x + beta * y



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_m

		- the number of rows of input matrix p_M

	*
		- p_n

		- the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_alpha

		- scalar alpha

	*
		- p_M

		- the input stream of packed Matrix entries

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_beta

		- scalar beta

	*
		- p_y

		- the output vector

nrm2
#####

.. index:: pair: function; nrm2
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1a048b14da86cf49591f68457007a8a1da:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void nrm2(unsigned int p_n, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, t_DataType& p_res)

nrm2 function that returns the Euclidean norm of the vector x.



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in the input vector p_x, p_n % (1<<l_LogParEntries) == 0

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_res

		- the nrm2 of x

scal
#####

.. index:: pair: function; scal
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aee44ac6a6a4df8cc69d63e1ea8381f40:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void scal(unsigned int p_n, t_DataType p_alpha, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_res)

scal function that compute X = alpha * X



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the packed input vector stream

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in vector X, p_n % t_ParEntries == 0

	*
		- p_x

		- the packed input vector stream

	*
		- p_res

		- the packed output vector stream

swap
#####

.. index:: pair: function; swap
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa7d231959e217bc6288ee4fd60274445:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_ParEntries, typename t_IndexType = unsigned int>
	void swap(unsigned int p_n, hls::stream<WideType<t_DataType, t_ParEntries>>& p_x, hls::stream<WideType<t_DataType, t_ParEntries>>& p_y, hls::stream<WideType<t_DataType, t_ParEntries>>& p_xRes, hls::stream<WideType<t_DataType, t_ParEntries>>& p_yRes)

swap function taht swap vector x and y



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_ParEntries

		- number of parallelly processed entries in the packed input vector stream

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of entries in vector X and Y, p_n % t_ParEntries == 0

	*
		- p_x

		- the packed input vector stream

	*
		- p_y

		- the packed input vector stream

	*
		- p_xRes

		- the packed output stream

	*
		- p_yRes

		- the packed output stream

symv
#####

.. index:: pair: function; symv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa0f4cd1d5ddd38ec0bea4431ada7604c:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void symv(const unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_M, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_y, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_yr)

symv function that returns the result vector of the mutiplication of a symmetric matrix and a vector y = alpha * M * x + beta * y



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the dimention of input matrix p_M, as well as the number of entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_alpha

		- 

	*
		- scalar

		- alpha

	*
		- p_M

		- the input stream of packed Matrix entries

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_beta

		- 

	*
		- scalar

		- beta

	*
		- p_y

		- the output vector

trmv
#####

.. index:: pair: function; trmv
.. _doxid-namespacexf_1_1linear__algebra_1_1blas_1aa7a9cb6d049a99fe8786a943d3a8ad40:

.. ref-code-block:: cpp
	:class: title-code-block

	template  <typename t_DataType, unsigned int t_LogParEntries, typename t_IndexType = unsigned int>
	void trmv(const bool uplo, const unsigned int p_n, const t_DataType p_alpha, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_M, hls::stream<WideType<t_DataType,(1<<:ref:`t_LogParEntries<doxid-namespacexf_1_1linear__algebra_1_1blas_1a8a8b8c839994b5b2887df891844917de>`)>>& p_x, const t_DataType p_beta, hls::stream<WideType<t_DataType, 1>>& p_y, hls::stream<WideType<t_DataType, 1>>& p_yr)

function that returns the result vector of the mutiplication of a triangular matrix and a vector y = alpha * M * x + beta * y



.. rubric:: Parameters:

.. list-table::
	:widths: 20 80

	*
		- t_DataType

		- the data type of the vector entries

	*
		- t_LogParEntries

		- log2 of the number of parallelly processed entries in the input vector

	*
		- t_IndexType

		- the datatype of the index

	*
		- p_n

		- the number of cols of input matrix p_M, as well as the number of entries in the input vector p_x, p_n % l_ParEntries == 0

	*
		- p_alpha

		- 

	*
		- scalar

		- alpha

	*
		- p_M

		- the input stream of packed Matrix entries

	*
		- p_x

		- the input stream of packed vector entries

	*
		- p_beta

		- 

	*
		- scalar

		- beta

	*
		- p_y

		- the output vector

