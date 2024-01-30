.. Copyright © 2019–2024 Advanced Micro Devices, Inc

.. `Terms and Conditions <https://www.amd.com/en/corporate/copyright>`_.

******************************
ReadFromDataframe
******************************

Apache Arrow is a cross-language, cross-platform, columnar data format that used in the field of data analytics, database, etc. An Apache Arrow like format "Dataframe" is defined and employed in the AMD Vitis™ library.

This document describes the structure of the readFromDataframe() API. This API reads the "Dataframe" format data on double-data rate (DDR), and convert to a line based storage format output. Each output line contains one data from each field. 

Input Data 
===============================

The Dataframe meta data on DDR is introduced in detail in the writeToDataFrame() API.

To start data processing, all meta info should be loaded to on-chip buffers first, which is implemented in function memToLocalRam(). Thereafter, the DDR data is burst-read in field by field.

Output Data Stream 
===============================

The output data is packed into an object stream, which includes valid data, field id, data type, and end flags info. For detailed info of the Object stream, refer to the writeToDataFrame API.

Overall Structure
===============================

After all meta info is loaded to LUTRAM/BRAM or URAM. The data reading process is drawn:

.. image:: /images/read_to_obj_strm.png
   :alt: data read 
   :width: 80%
   :align: center

Null and Boolean flag data is read from URAM bit_map and bool_buff. The Int64/Double/Date/String offset data reading address is generated and round-robin output. Thereafter, in the breadWrapper module, each field data is read in burst mode. 

It is worth mentioning that, to read the valid string data, two times of read are required: first, offset/strlen, then, string data. Deep first in first outs (FIFOs) are used to buffer each field burst out data.

Finally, row based data is output by the writeObjOut module. Each data is packed into an Object struct.
