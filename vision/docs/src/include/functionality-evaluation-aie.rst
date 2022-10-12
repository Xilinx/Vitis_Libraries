Evaluating the Functionality
=============================

You can build the kernels and test the functionality through x86 simulation, cycle
accurate aie simulation, hw emulation or hw run on the board. Use the following
commands to setup the basic environment:

.. _x86_simulation:

x86 Simulation
--------------

Please refer to `x86 Functional Simulation`_ section in Vitis Unified Software Development Platform 2022.2 Documentation. For host code development, please refer to `Programming the PS Host Application`_ section

.. _x86 Functional Simulation: https://docs.xilinx.com/r/en-US/ug1076-ai-engine-environment/Simulating-an-AI-Engine-Graph-Application
.. _Programming the PS Host Application: https://docs.xilinx.com/r/en-US/ug1076-ai-engine-environment/Programming-the-PS-Host-Application

.. _aie_simulation:

AIE Simulation
--------------

Please refer to `AIE Simulation`_ section in Vitis Unified Software Development Platform 2022.2 Documentation. For host code development, please refer to `Programming the PS Host Application`_ section.

.. _AIE Simulation: https://docs.xilinx.com/r/en-US/ug1076-ai-engine-environment/Simulating-an-AI-Engine-Graph-Application

.. _hw_emulation:

HW emulation
------------

Please refer to `Programming the PS Host Application`_ section in Vitis Unified Software Development Platform 2022.2 Documentation.

.. _hw_testing:

Testing on HW
-------------

After the build for hardware target completes, sd_card.img file will be generated in the build directory. 

1. Use a software like Etcher to flash the sd_card.img file on to a SD Card. 
2. After flashing is complete, insert the SD card in the SD card slot on board and power on the board.
3. Use Teraterm to connect to COM port and wait for the system to boot up.
4. After the boot up is done, goto /media/sd-mmcblk0p1 directory and run the executable file.

Please refer to `hw_run`_ section in Vitis Unified Software Development Platform 2022.2 Documentation.

.. _hw_run: https://docs.xilinx.com/r/en-US/ug1393-vitis-application-acceleration/Running-the-Application-Hardware-Build