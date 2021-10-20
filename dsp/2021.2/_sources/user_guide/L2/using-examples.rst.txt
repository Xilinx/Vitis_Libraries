..
   Copyright 2021 Xilinx, Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

.. _3_USING_EXAMPLES:

*******************
Using the Examples
*******************

=================================================
Compiling and Simulating Using the Example Design
=================================================

A Makefile is included with the example design. It is located inside the `L2/examples/fir_129t_sym/` directory. Use the following steps to compile, simulate and verify the example design using the Makefile.

#. Clean Work directory and all output files

   .. code-block::

         make clean

#. Compile the example design.

   .. code-block::

         make compile

#. Simulate the example design.

   .. code-block::

         make sim

   This generates the file `output.txt` in the `aiesimulator_output/data` directory.

#. To compare the output results with the golden reference, extract samples from output.txt, then perform a diff with respect to the reference using the following command.

   .. code-block::

         make check_op

#. Display the status summary with.

   .. code-block::

         make get_status

   This populates the status.txt file. Review this file to get the status.

   .. note:: All of the preceding steps are performed in sequence using the following command:

        .. code-block::

             make all

==================================================================
Using the Vitis Unified Software Platform
==================================================================

To create, build, and simulate a library element example using the Vitis |trade| integrated design environment (IDE), please see the `DSP Library Vitis Feature Tutorial <https://github.com/Xilinx/Vitis-Tutorials/tree/HEAD/AI_Engine_Development/Feature_Tutorials/08-dsp-library>`__
.

====================
Example Graph Coding
====================

The following example can be used as a template for graph coding:

test.h
~~~~~~

.. code-block::

        #include <adf.h>
        #include "fir_sr_sym_graph.hpp"

        #define FIR129_LENGTH 129
        #define FIR129_SHIFT 15
        #define FIR129_ROUND_MODE 0
        #define FIR129_INPUT_SAMPLES 256

        using namespace adf ;
        namespace testcase {
            class test_kernel: public graph {
               private:
               // FIR coefficients
               std::vector<int16> m_taps   = std::vector<int16>{
                        -1, -3, 3, -1, -3, 6, -1, -7, 9, -1, -12, 14, 1, -20, 19, 5,
                        -31, 26, 12, -45, 32, 23, -63, 37, 40, -86, 40, 64, -113, 39, 96, -145,
                        33, 139, -180, 17, 195, -218, -9, 266, -258, -53, 357, -299, -118, 472, -339, -215,
                        620, -376, -360, 822, -409, -585, 1118, -437, -973, 1625, -458, -1801, 2810, -470, -5012, 10783,
                        25067};
               //FIR Graph class
               xf::dsp::aie::fir::sr_sym::
                  fir_sr_sym_graph<cint16, int16, FIR129_LENGTH, FIR129_SHIFT, FIR129_ROUND_MODE, FIR129_INPUT_SAMPLES>
                     firGraph;
               public:
               port<input> in;
               port<output> out;
               // Constructor - with FIR graph class initialization
               test_kernel():firGraph(m_taps) {
                  // Make connections
                  // Size of window in Bytes.
                  // Margin gets automatically added within the FIR graph class.
                  // Margin equals to FIR length rounded up to nearest multiple of 32 Bytes.
                  connect<>(in, firGraph.in);
                  connect<>(firGraph.out, out);
               };
            };
        };

test.cpp
~~~~~~~~
.. code-block::

        #include "test.h"

        simulation::platform<1,1> platform("data/input.txt", "data/output.txt");
        testcase::test_kernel filter ;

        connect<> net0(platform.src[0], filter.in);
        connect<> net1(filter.out, platform.sink[0]);

        int main(void) {
            filter.init() ;
            filter.run() ;
            filter.end() ;
            return 0 ;
        }


.. |trade|  unicode:: U+02122 .. TRADEMARK SIGN
   :ltrim:
.. |reg|    unicode:: U+000AE .. REGISTERED TRADEMARK SIGN
   :ltrim:

