.. 
   Copyright 2019 Xilinx, Inc.
  
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
  
       http://www.apache.org/licenses/LICENSE-2.0
  
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. meta::
   :keywords: python, examples, pybind, PyBind11
   :description: As an alternative to C++ it is possible to run the L3 models from the Python language. The python sub-directory within L3 contains examples for the various financial models.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials


********************************
Python
********************************

As an alternative to C++ it is possible to run the L3 models from the Python language. 

The python sub-directory within L3 contains examples for the various financial models.

This utilises the pybind tools to provide the abstraction from C++ api's to Python.

Note that they too require the shared library as described within the getting started section.

Python Module Description
#########################
The python modules are created using PyBind11. 
The file **module.cpp** contains the code to create all the python financial modules, pybind is written in C++. 
These reference the C++ software apis i.e. the **.hpp** files.

Building the module
####################
To generate the module just run the Makefile - *make* from within your copy of the ../L3/python directory .
This will create a **module.o** file in the output sub-directory. The python examples then import the required functions from this file, similarly to the standard python modules.

Running the examples
####################
The python sub-directory within L3 contains python examples. 
Once the module has been compiled the simplest way to run is to copy the desired example python script and matching generated xclbin file into the output subdirectory and run from that directory
 - for example to run the Dow Jones Monte Carlo European financial python script:
 
.. code-block:: sh

	cd output
	python36 ./dje_test.py
 
Note within each example the card type is defined - so please check that matches the card you have and alter is necessary.

Additionally note there is a comment describing the expected result. These generally mirror the C++ examples. 
