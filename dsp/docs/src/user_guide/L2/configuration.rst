..
   Copyright © 2019–2024 Advanced Micro Devices, Inc
   
   `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _CONFIGURATION:

Configuring the Library Elements
--------------------------------

**Prerequisites**:
    Python bin to be used for config_helper.py:

.. code-block::

	setenv PATH "<your-Vitis-install-path>/lin64/Vitis/2024.2/aietools/tps/lnx64/python-3.8.3/bin:$PATH"

DSPLIB AIE-IPs can be configured by utilizing a Python script called "config_helper.py". The "config_helper.py" is designed to assist users in establishing a valid configuration for any DSPLIB AIE-IP. This script communicates with the user through the console interface.

The config helper will print either a legal set or a legal range for a parameter asking the user input a value. If the parameter is set to a legal value, the config helper will move to the next parameter on the parameter list of the chosen IP. If the given value is not legal, config helper will return error and ask the user to choose a legal value from the legal set/range. User can go back to the previous parameter by entering z/Z and return at any time.

Once all the parameters are set, the config helper will output a graph_*ip_name_instance_name*.txt file to the output directory by default. This file involves graph level class of the IP that can be used to instantiate it. The user can use PRINT_GRAPH option to print the graph on the console. If graph is not needed and config_helper.py is only used to verify a set of parameters, call config_helper.py with NO_GRAPH argument.

Running Config Helper
^^^^^^^^^^^^^^^^^^^^^

config_helper.py is located within the `xf_dsp/L2/meta` directory. To run config_helper.py, cd into xf_dsp repository and run the following with requested options:

.. code-block::

python3 `xf_dsp/L2/meta/config_helper.py` [Options]
	--h [helper prints]
	--ip ip_name [providing the config helper the IP to configure]
	--mdir metadata_directory [by default config helper will guide you to `xf_dsp/L2/meta`]
	--outdir output_directory [by default config helper will guide you to `xf_dsp/L2/meta`]
	LIST_PARAMS [Lists the parameters of the chosen IP to configure]
	PRINT_GRAPH [Prints the resulting graph at the end of the configuration]
	NO_INSTANCE [no graph instance is to be generated at the end of the configuration]
	LIST_IPS [prints the IP list in DSPLIB]

Config Helper Example
^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

    python3 xf_dsp/L2/meta/config_helper.py --ip hadamard LIST_PARAMS