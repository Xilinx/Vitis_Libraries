.. meta::
   :keywords: BLAS, Library, Vitis BLAS Library, python, setup
   :description: Python environment setup guide.
   :xlnxdocumentclass: Document
   :xlnxdocumenttype: Tutorials



Python Environment Setup Guide
===============================

**1. Installing Anaconda3**

1) Download Anaconda3

.. code-block:: bash

  $ wget https://repo.anaconda.com/archive/Anaconda3-2019.03-Linux-x86_64.sh

1) Run the installer (Installer requires bzip. Install it if you do not have it.)

.. code-block:: bash

  $ bash ./Anaconda3-2019.03-Linux-x86_64.sh

Choose Yes for the question- Do you wish the installer to initialize Anaconda3 by running conda init?. More information about Anaconda can be found from `Anaconda Documentation`_.

.. _Anaconda Documentation: https://docs.anaconda.com/anaconda/

3) Add Anaconda3 to PATH. For example:

.. code-block:: bash

  $ export PATH=/home/<user>/anaconda3/bin:$PATH
  $ . /home/<user>/anaconda3/etc/profile.d/conda.sh


**2. Setting up xf_hpc environment to include all conda packages used by xf_hpc L1 primitive testing infrastructure.**

Run the following command under directory xf_hpc/. 

.. code-block:: bash

  $ conda create -n xf_hpc python=3.7 numpy
  $ conda activate xf_hpc


**3. Deactivate xf_hpc environment after testing**

.. Note:: Do not take this step if you intend to run L1 primitives' testing process. 
Take it only after you have finished all testing.

.. code-block:: bash

  $ conda deactivate
