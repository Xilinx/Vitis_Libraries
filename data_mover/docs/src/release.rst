.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _release_note:

=============
Release Notes
=============

.. toctree::
   :hidden:
   :maxdepth: 1

.. note:: Known Issues

   * A Jinja2 template is used to generate the source code of the data mover. When running on RHEL 9.x, it fails. A fix is planned for the next release.
   * ``L2/demo/4d_mover_with_handshake``, ``L2/demo/bi_4d_mover_with_handshake``, and ``L2/tests/bi_dm_s2mm_mm2s_s2s`` encountered a deadlock issue. A fix is planned for the next release.

2023.1
======

All Data Mover designs were migrated from the utils library. A 4D Data Mover with internal URAM buffer support and a tile-based descriptor was also added.
