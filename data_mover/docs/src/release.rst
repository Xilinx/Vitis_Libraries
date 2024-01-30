.. Copyright © 2019–2023 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

.. _release_note:

=============
Release Notes
=============

.. toctree::
   :hidden:
   :maxdepth: 1

2023.1
======

In this release, all data mover designs were migrated from the utils library. Also a 4D datamover with an internal URAM buffer support and tile based descriptor was added.

Known Issues
============

A Jinja2 template is used to generate the source code of the data mover. When running on RHEL 9.x, it will fail. This will get this fixed in the next release.