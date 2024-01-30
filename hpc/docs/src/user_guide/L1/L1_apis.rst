.. 
   .. Copyright © 2019–2023 Advanced Micro Devices, Inc

`Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _user_guide_overview_l1_rtm_api:

******************
L1 APIs
******************


MLP
===========

The basic components of the FCN are defined in the template class FCN. Frequent
used activation functions are also implemented.

.. toctree::
   :maxdepth: 2

.. include:: namespace_xf_hpc_mlp.rst

CG Solver
===========

Some basic CG components are defined under the namespace **cg** 

.. toctree::
   :maxdepth: 2

.. include:: namespace_xf_hpc_cg.rst

Reverse Time Migration
==================================
Here describes the basic components for solving wave equations by explicit FDTD method, and components for RTM. 
These components are further classified according to the
problems' dimensions. For example, 2D or 3D. 

Data Movers
---------------------------------
Data movers are implemented for kernel memory communication, dataType conversion, and stream datawidth
conversion. 

.. toctree::
   :maxdepth: 2

.. include:: namespace_xf_hpc_rtm.rst

