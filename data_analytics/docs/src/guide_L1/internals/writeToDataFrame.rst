.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

******************************
WriteToDataFrame
******************************

Apache Arrow is a cross-language, cross-platform, columnar data format that is used in the field of data analytics, database, etc. An Apache Arrow like format "data-frame" is defined and employed in the AMD Vitis™ library.

This document first introduces the data-frame format and the difference between data frame and Apache Arrow. Thereafter, the structure of the WriteToDataFrame() implementation is explained. WriteToDataFrame() reads in parsed data from a JSON parser etc., re-formats the data according to different data type, and saves the data in the data-frame format.

Data Frame Format (on DDR)
===============================

An Apache Arrow format data can be represented in the illustrated figure on the left side. The whole data is seperated into multiple record batches; each batch consists of multiple columns with the same length. 

.. image:: /images/data_frame_layout.png
   :alt: data frame layout 
   :width: 80%
   :align: center

It is worth mentioning that the length of each record batch is a statistic info, unknown while reading/writing each record batch data. Besides, the data width of different data types are different, especially for string, because the length of each string data is variable.

Thus, the Apache Arrow columnar data format cannot be implemented directly on hardware. A straight-forward implementation of the arrow data would be, for each field id, one fixed size DDR buffer is predefined. However, because the number and data type of each field is unknown, DDR space is wasted heavily. To fully utilize the DDR memory on the FPGA, the "data-frame" format is defined and employed, which can be seen in the right side of the preceding figure.

The DDR is split into multiple mem blocks. Each block is 4 MB in size with a 64-bit width. The mem block address and linking info is recored on the meta section of DDR header. In other words, for each column/field, the data is stored in 4M -> 4M -> 4M linkable mem blocks. The length, size, count, etc. info are also saved in the DDR header. 

Three types of data are columnar stored differently comparing to the Apache Arrow format, namely, Null, Boolean, and String. For Null and Boolean, because to only 1-bit is required for each data, bitmap[4096][16] and boolbuff[4096][16] (each data 64-bit) is used to save the data, respectively. The following figure illustrates the bitmap layout; each 64-bit data indicates 64 x input data, and the maximum supported number of input data number of 64 x 4096. And the supported maximum field num is 16. The same data storage buffer is employed for Boolbuff.

.. image:: /images/data_layout1.png
   :alt: data layout1 
   :width: 50%
   :align: center

As for the String data, four lines of input example is provided. The input data are given at the left side, and the compact arrow format data storage is in the middle. It is clear that no bubbles exist in the data buffer, and in data-frame, the string data layout is shown on the right side. Each input string data is consisiting of one or multi-lines of 64 bit data; each char is 8 bits. If the string is not 64-bit aligned, bubbles are inserted to the ending 64-bit string. The reason that you introduced bubbles to data-frame storage is to ensure each string data is started in a new DDR address. This greatly guaranteed the string data access is faster without a timing issue. Simliar to the arrow format, the offset buffer always points to the starting address of each string input.  

.. image:: /images/string_layout.png
   :alt: string layout
   :width: 80%
   :align: center

For the normal 4 MB mem blocks, the f_buff saves the starting and ending Node address of each mem block. The tail mem block size is also counted. The detailed info of each node is provided in the LinkTable buffer. 

Besides the data, the input data length, size, etc. info are also counted and added to the according buffer when the input stream ends. 

.. image:: /images/data_layout2.png
   :alt: data layout2
   :width: 80%
   :align: center

Input Data Stream 
===============================

After introducing the data-frame layout, now switch to another general used struct: Object struct. The Object struct defines the input data and all related info of each parsed data, which is represented as follows: 

.. image:: /images/obj_interface.png
   :alt: object interface
   :width: 80%
   :align: center

As can be seen from the struct, the valid data bits, field id, data type and flags are all provided for each data. Data-frame APIs achieve read and write the data-frame format data to/from the data streams that packed as an Object struct. For instance, the CSV/JSON parser results are structed as an Object struct stream.

Overall Structure
===============================

The writeToDataframe() process includes two stages: 1) parse input data and lively store Null/Boolean data to LUTRAM/BRAM/URAM, Int64/Double/Date/String data to DDR; 2) save the on-chip memory data to DDR. 

The structure of stage One is as follows:

.. image:: /images/write_to_mem.png
   :alt: stage one
   :width: 80%
   :align: center

ProcessNull module adds a 1-bit flag to each data, to indicate whether each input data is null or not. This flag info is saved in an URAM bit_map. Meanwhile, the row number of input data and the number of null data are recorded on l_buff and n_buff.

If the input data is not null, based on the data type, different actions are taken. For boolean data, similar to null, a 1-bit value is used to save the real value and saved on bool_buf. For other non-string data typels, a module collectData is employed to convert the data from 64-bit to 32-bit. 

For string data type, the offset/length of each string data is recorded. Because of this length, the info for each data is 32-bit. Another collectData module is employed here.

While outputing 32-bit data from two collectData modules, each data generates 1x 32-bit data write request to a memManage module. This module accumulates the request number to 32 and generates a burst write 32x 32-bit data request. This request includes a writing address and data number. The actual 32x32-bit data is bufferred in the combine module.

The reason that these two combine modules are added here is because the DDR is 64-bit and your data is 32-bit. These two modules are converting the data again from 32-bit to 64-bit. A detailed explaination graph is provided.


.. image:: /images/mem_manage.png
   :alt: mem manage
   :width: 80%
   :align: center


