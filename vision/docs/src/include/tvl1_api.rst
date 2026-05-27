
.. _tvl1:

.. 
   Copyright 2023 Advanced Micro Devices, Inc
  
.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

TVL1 Optical Flow
==================

The ``tvl1`` function is used for Optical flow application and its formulation is based on Total Variation (TV) 
regularization and the robust L1 normalization in the data fidelity term. TVL1 algorithm preserves discontinuities 
in the flow field and increases robustness against occlusions, illumination changes, and noise. 

.. rubric:: Public member functions
.. code:: c

	// Compute optical flow for previous two frames & 
	// read current frame to create image pyramid.
	// First two calc function call will create image 
	// pyramid only and flow vector won't be computed.
	// From third calc function call, it will give flow 
	// vector by processing previous two frame.
	calc( cv::Mat current_frame, Mat_<Point2f> flow) 

	//get tau paramter
	getTau()
	
	//get lambda paramter
	getLambda()
	
	//get theta paramter
	getTheta()
	
	//get nscales parameter
	getScalesNumber()
	
	//get warps parameter
   	getWarpingsNumber()
	
	//get epsilon parameter
	getEpsilon()
	
	//get innnerIterations parameter
	getInnerIterations()
	
	//get outerIterations parameter
	getOuterIterations()
	
	//get scaleStep parameter
	getScaleStep()
	
	//get gamma parameter	
	getGamma()
	
	//get medianFiltering parameter
    getMedianFiltering()
	
	//get UseInitialFlow parameter
    getUseInitialFlow()

	//set tau paramter
	setTau(double val)
	
	//set lambda paramter
    setLambda(double val)
	
	//set theta paramter
    setTheta(double val)
	
	//set nscales parameter
    setScalesNumber(int val)
	
	//set warps parameter
    setWarpingsNumber(int val)
	
	//set epsilon parameter
    setEpsilon(double val)
	
	//set innnerIterations parameter
    setInnerIterations(int val)
	
	//set outerIterations parameter
    setOuterIterations(int val)
	
	//set scaleStep parameter
    setScaleStep(double val)
	
	//set gamma parameter
    setGamma(double val)
	
	//set medianFiltering parameter
    setMedianFiltering(int val)
	
	//set UseInitialFlow parameter
    setUseInitialFlow(bool val)


.. rubric:: Static public member functions
.. code:: c

	static Ptr xf::cv::DualTVL1OpticalFlow::create( double tau = 0.25, 
							double lambda = 0.15, 
							double theta = 0.3, 
							int nscales = 5, 
							int warps = 5, 
							double epsilon = 0.01, 
							int innnerIterations = 30, 
							int outerIterations = 10, 
							double scaleStep = 0.8, 
							double gamma = 0.0, 
							int medianFiltering = 5,
							bool UseInitialFlow = false)

	

.. rubric:: Parameter Descriptions


The following table describes the template and the function parameters.

.. table:: Table: TVL1 Parameters Description

   +-------------------+--------------------------------------------------+
   | Parameter         | Description                                      |
   +===================+==================================================+
   | current_frame     | Grey scale input image                           |
   +-------------------+--------------------------------------------------+
   | flow              | Computed flow image that has the same size as    |
   |                   | input image and type CV_32FC2.                   |
   +-------------------+--------------------------------------------------+
   | tau               | Time step of the numerical scheme. Default value |
   |                   | is 0.25                                          |
   +-------------------+--------------------------------------------------+
   | lambda            | Weight parameter for the data term, attachment   |
   |                   | parameter. Default value is 0.15                 |
   +-------------------+--------------------------------------------------+
   | theta             | Weight parameter for (u - v)^2, tightness        |
   |                   | parameter. Default value is 0.3                  |
   +-------------------+--------------------------------------------------+
   | nscales           | Number of scales used to create the pyramid of   |
   |                   | images. Default value is 5                       |
   +-------------------+--------------------------------------------------+
   | warps             | Number of warpings per scale. Default value is 5 |
   +-------------------+--------------------------------------------------+
   | epsilon           | Stopping criterion threshold used in the         |
   |                   | numerical scheme, which is a trade-off between   |
   |                   | precision and running time.Default values is 0.01|
   +-------------------+--------------------------------------------------+
   | innnerIterations  | Inner iterations (between outlier filtering) used|
   |                   | in the numerical scheme. Default value is 30     |
   +-------------------+--------------------------------------------------+
   | outerIterations   | Outer iterations (number of inner loops) used in |
   |                   | the numerical scheme. Default value is 10        |
   +-------------------+--------------------------------------------------+
   | scaleStep         | Step between scales (<1). Default value is 0.8   |
   +-------------------+--------------------------------------------------+
   | gamma             | coefficient for additional illumination variation|
   |                   | term. Only gamma = 0.0 is supported              |
   +-------------------+--------------------------------------------------+
   | medianFiltering   | MedianBlur Filter size. Only filter size = 5 is  |
   |                   | supported.                                       |
   +-------------------+--------------------------------------------------+
   | UseInitialFlow    | Use initial flow. Supported value is false       |
   +-------------------+--------------------------------------------------+
   
.. rubric:: Resource Utilization


The following table summarizes the resource utilization in different configurations, generated using the Vitis HLS 2022.1 tool for the
xcu50-fsvh2104-2-e FPGA.

.. table:: Table: TVL1 Function Resource Utilization Summary

    +----------------+---------------------------+----------------------+-----------+--------+--------+
    | Operating Mode | Operating Frequency (MHz) |               Utilization Estimate                 |
    +                +                           +----------------------+-----------+--------+--------+
    |                |                           | BRAM_18K             | DSP_48Es  |  FF    |  LUT   |
    +================+===========================+======================+===========+========+========+
    | 2 Pixel        | 300                       |     408              |    931    | 246513 | 220169 |
    +----------------+---------------------------+----------------------+-----------+--------+--------+


.. rubric:: Performance Estimate


The following table summarizes the performance of the kernel in 2-pixel
mode as generated using the Vitis HLS 2022.1 tool for the
xcu50-fsvh2104-2-e FPGA to process a grayscale 384x288 image
image.

.. table:: Table: TVL1 Function Performance Estimate Summary

    +-----------------------------+------------------+
    | Operating Mode              | Latency Estimate |
    +                             +------------------+
    |                             | Max Latency (ms) |
    +=============================+==================+
    | 2 pixel operation (300 MHz) |       95.6       |
    +-----------------------------+------------------+
	
