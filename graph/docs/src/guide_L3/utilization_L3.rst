.. 
   .. Copyright © 2020–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. meta::
   :keywords: Asynchronous, XRM, XRT, graph L3, Graph Library, OpenCL
   :description: The graph L3 layer provides an asynchronous and easy-to-integrate framework. By using the Xilinx FPGA Resource Manager (XRM) and this completely original and innovative L3 asynchronous framework, you can easily call the L3 asynchronous APIs in the pure software (heterochronous or synchronous) codes without any hardware related things. The innovative L3 asynchronous framework can automatically acquire current platform's available resources (available FPGA boards, boards' serie numbers, available compute units, kernel names, and so on). In addition, this framework separates FPGA deployments with L3 API calling, so the graph L3 layer can be easily deployed in the Cloud and can be easily used by pure software developers.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



***********
User Guide
***********

The graph L3 layer provides an asynchronous and easy-to-integrate framework. By using the Xilinx FPGA Resource Manager (XRM) and this completely original and innovative L3 asynchronous framework, you can easily call the L3 asynchronous APIs in the pure software (heterochronous or synchronous) codes without any hardware related things. The innovative L3 asynchronous framework can automatically acquire current platform's available resources (available FPGA boards, boards' serie numbers, available compute units, kernel names, and so on). In addition, this framework separates FPGA deployments with L3 API calling, so the graph L3 layer can be easily deployed in the Cloud and can be easily used by pure software developers.


.. toctree::
	:maxdepth: 4

	L3_internal/architecture.rst
    L3_internal/getting_started.rst
	L3_internal/user_model.rst
	L3_internal/louvainPartition.rst
	L3_internal/louvainRun.rst




   
