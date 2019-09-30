


Migrating HLS Video Library to Vitis vision
-------------------------------------------

The HLS video library will soon be deprecated. All the functions and
most of the infrastructure available in HLS video library are now
available in Vitis vision with their names changed and some modifications.
These HLS video library functions ported to Vitis vision supports build
flow also.

This section provides the details on using the C++ video processing
functions and the infrastructure present in HLS video library.

Infrastructure Functions and Classes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All the functions imported from HLS video library now take xf::cv::Mat (in
sync with Vitis vision library) to represent image data instead of hls::Mat.
The main difference between these two is that the hls::Mat uses
hls::stream to store the data whereas xf::cv::Mat uses a pointer. Therefore,
hls:: Mat cannot be exactly replaced with xf::cv::Mat for migrating.

Below table summarizes the differences between member functions of
hls::Mat to xf::cv::Mat.

.. table:: Table : Infrastructure Functions and Classes

   +----------------------+----------------------+--------------------------+
   | Member Function      | hls::Mat (HLS Video  | xf::cv::Mat (Vitis vision|
   |                      | lib)                 | lib)                     |
   +======================+======================+==========================+
   | channels()           | Returns the number   | Returns the number       |
   |                      | of channels          | of channels              |
   +----------------------+----------------------+--------------------------+
   | type()               | Returns the enum     | Returns the enum         |
   |                      | value of pixel type  | value of pixel type      |
   +----------------------+----------------------+--------------------------+
   | depth()              | Returns the enum     | Returns the depth of     |
   |                      | value of pixel type  | pixel including          |
   |                      |                      | channels                 |
   +----------------------+----------------------+--------------------------+
   | read()               | Readout a value and  | Readout a value from     |
   |                      | return it as a       | a given location and     |
   |                      | scalar from stream   | return it as a           |
   |                      |                      | packed (for              |
   |                      |                      | multi-pixel/clock)       |
   |                      |                      | value.                   |
   +----------------------+----------------------+--------------------------+
   | operator >>          | Similar to read()    | Not available in         |
   |                      |                      | Vitis vision             |
   +----------------------+----------------------+--------------------------+
   | operator <<          | Similar to write()   | Not available in         |
   |                      |                      | Vitis vision             |
   +----------------------+----------------------+--------------------------+
   | Write()              | Write a scalar value | Writes a packed (for     |
   |                      | into the stream      | multi-pixel/clock)       |
   |                      |                      | value into the given     |
   |                      |                      | location.                |
   +----------------------+----------------------+--------------------------+

Infrastructure files available in HLS Video Library hls_video_core.h,
hls_video_mem.h, hls_video_types.h are moved to xf_video_core.h,
xf_video_mem.h, xf_video_types.h in Vitis vision Library and
hls_video_imgbase.h is deprecated. Code inside these files unchanged
except that these are now under xf::cv::namespace.

Classes
~~~~~~~

Memory Window Buffer
   hls::window is now xf::cv::window. No change in the implementation,
   except the namespace change. This is located in “xf_video_mem.h”
   file.
Memory Line Buffer
   hls::LineBuffer is now xf::cv::LineBuffer. No difference between the two,
   except xf::cv::LineBuffer has extra template arguments for inferring
   different types of RAM structures, for the storage structure used.
   Default storage type is “RAM_S2P_BRAM” with RESHAPE_FACTOR=1.
   Complete description can be found here
   `xf::cv::LineBuffer <Migrating HLS Video Library to Vitis vision.html#ndi1542884914646>`__. This is
   located in xf_video_mem.h file.

Funtions
~~~~~~~~

OpenCV interface functions
   These functions covert image data of OpenCV Mat format to/from HLS
   AXI types. HLS Video Library had 14 interface functions, out of
   which, two functions are available in Vitis vision Library:
   cvMat2AXIvideo and AXIvideo2cvMat located in “xf_axi.h” file. The
   rest are all deprecated.

AXI4-Stream I/O Functions
   The I/O functions which convert hls::Mat to/from AXI4-Stream
   compatible data type (hls::stream) are hls::AXIvideo2Mat,
   hls::Mat2AXIvideo. These functions are now deprecated and added 2 new
   functions xf::cv::AXIvideo2xfMat and xf::cv:: xfMat2AXIvideo to facilitate
   the xf::cv::Mat to/from conversion. To use these functions, the header
   file "xf_infra.h" must be included.



xf::cv::window
~~~~~~~~~~~~~~

A template class to represent the 2D window buffer. It has three
parameters to specify the number of rows, columns in window buffer and
the pixel data type.

Class definition
^^^^^^^^^^^^^^^^

.. code:: c

   template<int ROWS, int COLS, typename T>
   class Window {
   public:
       Window() 
      /* Window main APIs */
       void shift_pixels_left();
       void shift_pixels_right();
       void shift_pixels_up();
       void shift_pixels_down();
       void insert_pixel(T value, int row, int col);
       void insert_row(T value[COLS], int row);
       void insert_top_row(T value[COLS]);
       void insert_bottom_row(T value[COLS]);
       void insert_col(T value[ROWS], int col);
       void insert_left_col(T value[ROWS]);
       void insert_right_col(T value[ROWS]);
       T& getval(int row, int col);
       T& operator ()(int row, int col);
       T val[ROWS][COLS];
   #ifdef __DEBUG__
       void restore_val();
       void window_print();
       T val_t[ROWS][COLS];
   #endif
   };

Parameter Descriptions
^^^^^^^^^^^^^^^^^^^^^^

The following table lists the xf::cv::Window class members and their
descriptions.

.. table:: Table : Window Function Parameter Descriptions

   +-----------------+----------------------------------------------------+
   | Parameter       | Description                                        |
   +=================+====================================================+
   | Val             | 2-D array to hold the contents of buffer.          |
   +-----------------+----------------------------------------------------+

Member Function Description
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. table:: Table : Member Function Description

   +-----------------------------------+-----------------------------------+
   | Function                          | Description                       |
   +===================================+===================================+
   | shift_pixels_left()               | Shift the window left, that moves |
   |                                   | all stored data within the window |
   |                                   | right, leave the leftmost column  |
   |                                   | (col = COLS-1) for inserting new  |
   |                                   | data.                             |
   +-----------------------------------+-----------------------------------+
   | shift_pixels_right()              | Shift the window right, that      |
   |                                   | moves all stored data within the  |
   |                                   | window left, leave the rightmost  |
   |                                   | column (col = 0) for inserting    |
   |                                   | new data.                         |
   +-----------------------------------+-----------------------------------+
   | shift_pixels_up()                 | Shift the window up, that moves   |
   |                                   | all stored data within the window |
   |                                   | down, leave the top row (row =    |
   |                                   | ROWS-1) for inserting new data.   |
   +-----------------------------------+-----------------------------------+
   | shift_pixels_down()               | Shift the window down, that moves |
   |                                   | all stored data within the window |
   |                                   | up, leave the bottom row (row =   |
   |                                   | 0) for inserting new data.        |
   +-----------------------------------+-----------------------------------+
   | insert_pixel(T value, int row,    | Insert a new element value at     |
   | int col)                          | location (row, column) of the     |
   |                                   | window.                           |
   +-----------------------------------+-----------------------------------+
   | insert_row(T value[COLS], int     | Inserts a set of values in any    |
   | row)                              | row of the window.                |
   +-----------------------------------+-----------------------------------+
   | insert_top_row(T value[COLS])     | Inserts a set of values in the    |
   |                                   | top row = 0 of the window.        |
   +-----------------------------------+-----------------------------------+
   | insert_bottom_row(T value[COLS])  | Inserts a set of values in the    |
   |                                   | bottom row = ROWS-1 of the        |
   |                                   | window.                           |
   +-----------------------------------+-----------------------------------+
   | insert_col(T value[ROWS], int     | Inserts a set of values in any    |
   | col)                              | column of the window.             |
   +-----------------------------------+-----------------------------------+
   | insert_left_col(T value[ROWS])    | Inserts a set of values in left   |
   |                                   | column = 0 of the window.         |
   +-----------------------------------+-----------------------------------+
   | insert_right_col(T value[ROWS])   | Inserts a set of values in right  |
   |                                   | column = COLS-1 of the window.    |
   +-----------------------------------+-----------------------------------+
   | T& getval(int row, int col)       | Returns the data value in the     |
   |                                   | window at position (row,column).  |
   +-----------------------------------+-----------------------------------+
   | T& operator ()(int row, int col)  | Returns the data value in the     |
   |                                   | window at position (row,column).  |
   +-----------------------------------+-----------------------------------+
   | restore_val()                     | Restore the contents of window    |
   |                                   | buffer to another array.          |
   +-----------------------------------+-----------------------------------+
   | window_print()                    | Print all the data present in     |
   |                                   | window buffer onto console.       |
   +-----------------------------------+-----------------------------------+

Template Parameter Description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. table:: Table : Template Parameter Description

   +-----------+------------------------------------------+
   | Parameter | Description                              |
   +===========+==========================================+
   | ROWS      | Number of rows in the window buffer.     |
   +-----------+------------------------------------------+
   | COLS      | Number of columns in the window buffer.  |
   +-----------+------------------------------------------+
   | T         | Data type of pixel in the window buffer. |
   +-----------+------------------------------------------+

Sample code for window buffer declaration

.. code:: c

   Window<K_ROWS, K_COLS, unsigned char> kernel;

.. _ariaid-title5:

xf::cv::LineBuffer
~~~~~~~~~~~~~~~~~~

A template class to represent 2D line buffer. It has three parameters to
specify the number of rows, columns in window buffer and the pixel data
type.

.. _class-definition-1:

Class definition
^^^^^^^^^^^^^^^^

.. code:: c

   template<int ROWS, int COLS, typename T, XF_ramtype_e MEM_TYPE=RAM_S2P_BRAM, int RESHAPE_FACTOR=1>
    class LineBuffer {
   public:
       LineBuffer()
          /* LineBuffer main APIs */
       /* LineBuffer main APIs */
       void shift_pixels_up(int col);
       void shift_pixels_down(int col);
       void insert_bottom_row(T value, int col);
       void insert_top_row(T value, int col);
       void get_col(T value[ROWS], int col);
       T& getval(int row, int col);
       T& operator ()(int row, int col);

       /* Back compatible APIs */
       void shift_up(int col);
       void shift_down(int col);
       void insert_bottom(T value, int col);
       void insert_top(T value, int col);
       T val[ROWS][COLS];
   #ifdef __DEBUG__
       void restore_val();
       void linebuffer_print(int col);
       T val_t[ROWS][COLS];
   #endif
   };   

.. _parameter-descriptions-1:

Parameter Descriptions
^^^^^^^^^^^^^^^^^^^^^^

The following table lists the xf::cv::LineBuffer class members and their
descriptions.

.. table:: Table : Line Buffer Function Parameter Descriptions

   +-----------------+----------------------------------------------------+
   | Parameter       | Description                                        |
   +=================+====================================================+
   | Val             | 2-D array to hold the contents of line buffer.     |
   +-----------------+----------------------------------------------------+

Member Functions Description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. table:: Table : Member Functions Description

   +-----------------------------------+-----------------------------------+
   | Function                          | Description                       |
   +===================================+===================================+
   | shift_pixels_up(int col)          | Line buffer contents Shift up,    |
   |                                   | new values will be placed in the  |
   |                                   | bottom row=ROWS-1.                |
   +-----------------------------------+-----------------------------------+
   | shift_pixels_down(int col)        | Line buffer contents Shift down,  |
   |                                   | new values will be placed in the  |
   |                                   | top row=0.                        |
   +-----------------------------------+-----------------------------------+
   | insert_bottom_row(T value, int    | Inserts a new value in bottom     |
   | col)                              | row= ROWS-1 of the line buffer.   |
   +-----------------------------------+-----------------------------------+
   | insert_top_row(T value, int col)  | Inserts a new value in top row=0  |
   |                                   | of the line buffer.               |
   +-----------------------------------+-----------------------------------+
   | get_col(T value[ROWS], int col)   | Get a column value of the line    |
   |                                   | buffer.                           |
   +-----------------------------------+-----------------------------------+
   | T& getval(int row, int col)       | Returns the data value in the     |
   |                                   | line buffer at position (row,     |
   |                                   | column).                          |
   +-----------------------------------+-----------------------------------+
   | T& operator ()(int row, int col); | Returns the data value in the     |
   |                                   | line buffer at position (row,     |
   |                                   | column).                          |
   +-----------------------------------+-----------------------------------+

.. _template-parameter-description-1:

Template Parameter Description
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. table:: Table : Template Parameter Description

   +-----------------------------------+-----------------------------------+
   | Parameter                         | Description                       |
   +===================================+===================================+
   | ROWS                              | Number of rows in line buffer.    |
   +-----------------------------------+-----------------------------------+
   | COLS                              | Number of columns in line buffer. |
   +-----------------------------------+-----------------------------------+
   | T                                 | Data type of pixel in line        |
   |                                   | buffer.                           |
   +-----------------------------------+-----------------------------------+
   | MEM_TYPE                          | Type of storage element. It takes |
   |                                   | one of the following enumerated   |
   |                                   | values: RAM_1P_BRAM, RAM_1P_URAM, |
   |                                   | RAM_2P_BRAM, RAM_2P_URAM,         |
   |                                   | RAM_S2P_BRAM, RAM_S2P_URAM,       |
   |                                   | RAM_T2P_BRAM, RAM_T2P_URAM.       |
   +-----------------------------------+-----------------------------------+
   | RESHAPE_FACTOR                    | Specifies the amount to divide an |
   |                                   | array.                            |
   +-----------------------------------+-----------------------------------+

Sample code for line buffer declaration:

.. code:: c

   LineBuffer<3, 1920, XF_8UC3, RAM_S2P_URAM,1>     buff; 

.. _ariaid-title6:

Video Processing Functions
~~~~~~~~~~~~~~~~~~~~~~~~~~

The following table summarizes the video processing functions ported
from HLS Video Library into Vitis vision Library along with the API
modifications.

.. table:: Table : Video Processing Functions

   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   |   Functions                    |   HLS Video Library -API                                                                                                                                   |   xfOpenCV Library-API                                                                                                                                                                                            |
   +================================+============================================================================================================================================================+===================================================================================================================================================================================================================+
   | addS                           | template<int ROWS, int COLS, int SRC_T, typename \_T, int DST_T>                                                                                           | template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void AddS(Mat<ROWS, COLS, SRC_T>&src,Scalar<HLS_MAT_CN(SRC_T), \_T> scl, Mat<ROWS, COLS, DST_T>& dst)                                                      | void addS(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                          |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | AddWeighted                    | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T, typename P_T>                                                                              | template< int SRC_T,int DST_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void AddWeighted(Mat<ROWS, COLS, SRC1_T>& src1,P_T alpha,Mat<ROWS, COLS, SRC2_T>& src2,P_T beta, P_T gamma,Mat<ROWS, COLS, DST_T>& dst)                    | void addWeighted(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1,float alpha, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2,float beta, float gama, xf::Mat<DST_T, ROWS, COLS, NPC> & dst)                                        |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Cmp                            | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T>                                                                                            | template<int CMP_OP, int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Cmp(Mat<ROWS, COLS, SRC1_T>& src1,Mat<ROWS, COLS, SRC2_T>& src2,                                                                                      | void compare(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                          |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst,int cmp_op)                                                                                                                    |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | CmpS                           | template<int ROWS, int COLS, int SRC_T, typename P_T, int DST_T>                                                                                           | template<int CMP_OP, int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void CmpS(Mat<ROWS, COLS, SRC_T>& src, P_T value, Mat<ROWS, COLS, DST_T>& dst, int cmp_op)                                                                 | void compare(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                       |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Max                            | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T>                                                                                            | template<int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                               |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Max(Mat<ROWS, COLS, SRC1_T>& src1,                                                                                                                    | void Max(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC2_T>& src2,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | MaxS                           | template<int ROWS, int COLS, int SRC_T, typename \_T, int DST_T>                                                                                           | template< int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void MaxS(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                     | void max(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                           |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | \_T value, Mat<ROWS, COLS, DST_T>& dst)                                                                                                                    |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Min                            | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T>                                                                                            | template< int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Min(Mat<ROWS, COLS, SRC1_T>& src1,                                                                                                                    | void Min(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC2_T>& src2,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | MinS                           | template<int ROWS, int COLS, int SRC_T, typename \_T, int DST_T>                                                                                           | template< int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void MinS(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                     | void min(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                           |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | \_T value,Mat<ROWS, COLS, DST_T>& dst)                                                                                                                     |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | PaintMask                      | template<int SRC_T,int MASK_T,int ROWS,int COLS>                                                                                                           | template< int SRC_T,int MASK_T, int ROWS, int COLS,int NPC=1>                                                                                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void PaintMask(                                                                                                                                            | void paintmask(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat, xf::Mat<MASK_T, ROWS, COLS, NPC> & in_mask, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst_mat, unsigned char \_color[XF_CHANNELS(SRC_T,NPC)])               |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS,COLS,SRC_T> &_src,                                                                                                                                |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS,COLS,MASK_T>&_mask,                                                                                                                               |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS,COLS,SRC_T>&_dst,Scalar<HLS_MAT_CN(SRC_T),HLS_TNAME(SRC_T)> \_color)                                                                              |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Reduce                         | template<typename INTER_SUM_T, int ROWS, int COLS, int SRC_T, int DST_ROWS, int DST_COLS, int DST_T>                                                       | template< int REDUCE_OP, int SRC_T,int DST_T, int ROWS, int COLS,int ONE_D_HEIGHT, int ONE_D_WIDTH, int NPC=1>                                                                                                    |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Reduce(                                                                                                                                               | void reduce(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat, xf::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, 1> & \_dst_mat, unsigned char dim)                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T> &src,                                                                                                                               |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<DST_ROWS, DST_COLS, DST_T> &dst,                                                                                                                       |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | int dim,                                                                                                                                                   |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | int op=HLS_REDUCE_SUM)                                                                                                                                     |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Zero                           | template<int ROWS, int COLS, int SRC_T, int DST_T>                                                                                                         | template< int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Zero(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                     | void zero(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                                                                       |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Sum                            | template<typename DST_T, int ROWS, int COLS, int SRC_T>                                                                                                    | template< int SRC_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Scalar<HLS_MAT_CN(SRC_T), DST_T> Sum(                                                                                                                      | void sum(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, double sum[XF_CHANNELS(SRC_T,NPC)] )                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | SubS                           | template<int ROWS, int COLS, int SRC_T, typename \_T, int DST_T>                                                                                           | template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void SubS(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                     | void SubS(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                          |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Scalar<HLS_MAT_CN(SRC_T), \_T> scl,                                                                                                                        |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | SubRS                          | template<int ROWS, int COLS, int SRC_T, typename \_T, int DST_T>                                                                                           | template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void SubRS(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                    | void SubRS(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                         |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Scalar<HLS_MAT_CN(SRC_T), \_T> scl,                                                                                                                        |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Set                            | template<int ROWS, int COLS, int SRC_T, typename \_T, int DST_T>                                                                                           | template< int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Set(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                      | void set(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, unsigned char \_scl[XF_CHANNELS(SRC_T,NPC)],xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                           |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Scalar<HLS_MAT_CN(SRC_T), \_T> scl,                                                                                                                        |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Absdiff                        | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T>                                                                                            | template<int SRC_T, int ROWS, int COLS, int NPC =1>                                                                                                                                                               |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void AbsDiff(                                                                                                                                              | void absdiff(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                           |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC1_T>& src1,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC2_T>& src2,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | And                            | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T>                                                                                            | template<int SRC_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void And(                                                                                                                                                  | void bitwise_and(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src2, xf::Mat<SRC_T, ROWS, COLS, NPC> &_dst)                                                                       |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC1_T>& src1,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC2_T>& src2,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Dilate                         | template<int Shape_type,int ITERATIONS,int SRC_T, int DST_T, typename KN_T,int IMG_HEIGHT,int IMG_WIDTH,int K_HEIGHT,int K_WIDTH>                          | template<int BORDER_TYPE, int TYPE, int ROWS, int COLS,int K_SHAPE,int K_ROWS,int K_COLS, int ITERATIONS, int NPC=1>                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Dilate(Mat<IMG_HEIGHT, IMG_WIDTH, SRC_T>&_src,Mat<IMG_HEIGHT, IMG_WIDTH, DST_T&_dst,Window<K_HEIGHT,K_WIDTH,KN_T>&_kernel)                            | void dilate (xf::Mat<TYPE, ROWS, COLS, NPC> & \_src, xf::Mat<TYPE, ROWS, COLS, NPC> & \_dst,unsigned char \_kernel[K_ROWS*K_COLS])                                                                                |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Duplicate                      | template<int ROWS, int COLS, int SRC_T, int DST_T>                                                                                                         | template<int SRC_T, int ROWS, int COLS,int NPC>                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Duplicate(Mat<ROWS, COLS, SRC_T>& src,Mat<ROWS, COLS, DST_T>& dst1,Mat<ROWS, COLS, DST_T>& dst2)                                                      | void duplicateMat(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst1,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst2)                                                                     |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | EqualizeHist                   | template<int SRC_T, int DST_T,int ROW, int COL>                                                                                                            | template<int SRC_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void EqualizeHist(Mat<ROW, COL, SRC_T>&_src,Mat<ROW, COL, DST_T>&_dst)                                                                                     | void equalizeHist(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst)                                                                       |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | erode                          | template<int Shape_type,int ITERATIONS,int SRC_T, int DST_T, typename KN_T,int IMG_HEIGHT,int IMG_WIDTH,int K_HEIGHT,int K_WIDTH>                          | template<int BORDER_TYPE, int TYPE, int ROWS, int COLS,int K_SHAPE,int K_ROWS,int K_COLS, int ITERATIONS, int NPC=1>                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Erode(Mat<IMG_HEIGHT, IMG_WIDTH, SRC_T>&_src,Mat<IMG_HEIGHT,IMG_WIDTH,DST_T>&_dst,Window<K_HEIGHT,K_WIDTH,KN_T>&_kernel)                              | void erode (xf::Mat<TYPE, ROWS, COLS, NPC> & \_src, xf::Mat<TYPE, ROWS, COLS, NPC> & \_dst,unsigned char \_kernel[K_ROWS*K_COLS])                                                                                 |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | FASTX                          | template<int SRC_T,int ROWS,int COLS>                                                                                                                      | template<int NMS,int SRC_T,int ROWS, int COLS,int NPC=1>                                                                                                                                                          |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void FASTX(Mat<ROWS,COLS,SRC_T> &_src,                                                                                                                     | void fast(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst_mat,unsigned char \_threshold)                                                                                      |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS,COLS,HLS_8UC1>&_mask,HLS_TNAME(SRC_T)_threshold,bool \_nomax_supression)                                                                          |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Filter2D                       | template<int SRC_T, int DST_T, typename KN_T, typename POINT_T,                                                                                            | template<int BORDER_TYPE,int FILTER_WIDTH,int FILTER_HEIGHT, int SRC_T,int DST_T, int ROWS, int COLS,int NPC>                                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | int IMG_HEIGHT,int IMG_WIDTH,int K_HEIGHT,int K_WIDTH>                                                                                                     | void filter2D(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & \_dst_mat,short int filter[FILTER_HEIGHT*FILTER_WIDTH],unsigned char \_shift)                                         |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Filter2D(Mat<IMG_HEIGHT, IMG_WIDTH, SRC_T> &_src,Mat<IMG_HEIGHT, IMG_WIDTH, DST_T> &_dst,Window<K_HEIGHT,K_WIDTH,KN_T>&_kernel,Point_<POINT_T>anchor) |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | GaussianBlur                   | template<int KH,int KW,typename BORDERMODE,int SRC_T,int DST_T,int ROWS,int COLS>                                                                          | template<int FILTER_SIZE, int BORDER_TYPE, int SRC_T, int ROWS, int COLS,int NPC = 1>                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void GaussianBlur(Mat<ROWS, COLS, SRC_T>                                                                                                                   | void GaussianBlur(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src, xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst, float sigma)                                                                                                  |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | &_src, Mat<ROWS, COLS, DST_T>                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | &_dst,double sigmaX=0,double sigmaY=0)                                                                                                                     |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Harris                         | template<int blockSize,int Ksize,typename KT,int SRC_T,int DST_T,int ROWS,int COLS>                                                                        | template<int FILTERSIZE,int BLOCKWIDTH, int NMSRADIUS,int SRC_T,int ROWS, int COLS,int NPC=1,bool USE_URAM=false>                                                                                                 |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Harris(Mat<ROWS, COLS, SRC_T>                                                                                                                         | void cornerHarris(xf::Mat<SRC_T, ROWS, COLS, NPC> & src,xf::Mat<SRC_T, ROWS, COLS, NPC> & dst,uint16_t threshold, uint16_t k)                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | &_src,Mat<ROWS, COLS, DST_T>&_dst,KT k,int threshold                                                                                                       |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | CornerHarris                   | template<int blockSize,int Ksize,typename KT,int SRC_T,int DST_T,int ROWS,int COLS>                                                                        | template<int FILTERSIZE,int BLOCKWIDTH, int NMSRADIUS,int SRC_T,int ROWS, int COLS,int NPC=1,bool USE_URAM=false>                                                                                                 |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void CornerHarris(                                                                                                                                         | void cornerHarris(xf::Mat<SRC_T, ROWS, COLS, NPC> & src,xf::Mat<SRC_T, ROWS, COLS, NPC> & dst,uint16_t threshold, uint16_t k                                                                                      |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>&_src,Mat<ROWS, COLS, DST_T>&_dst,KT k)                                                                                              |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | HoughLines2                    | template<unsigned int theta,unsigned int rho,typename AT,typename RT,int SRC_T,int ROW,int COL,unsigned int linesMax>                                      | template<unsigned int RHO,unsigned int THETA,int MAXLINES,int DIAG,int MINTHETA,int MAXTHETA,int SRC_T, int ROWS, int COLS,int NPC>                                                                               |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void HoughLines2(Mat<ROW,COL,SRC_T> &_src,                                                                                                                 | void HoughLines(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat,float outputrho[MAXLINES],float outputtheta[MAXLINES],short threshold,short linesmax)                                                                 |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Polar_<AT,RT> (&_lines)[linesMax],unsigned int threshold)                                                                                                  |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Integral                       | template<int SRC_T, int DST_T,                                                                                                                             | template<int SRC_TYPE,int DST_TYPE, int ROWS, int COLS, int NPC>                                                                                                                                                  |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | int ROWS,int COLS>                                                                                                                                         | void integral(xf::Mat<SRC_TYPE, ROWS, COLS, NPC> & \_src_mat, xf::Mat<DST_TYPE, ROWS, COLS, NPC> & \_dst_mat)                                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Integral(Mat<ROWS, COLS, SRC_T>&_src,                                                                                                                 |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS+1, COLS+1, DST_T>&_sum )                                                                                                                          |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Merge                          | template<int ROWS, int COLS, int SRC_T, int DST_T>                                                                                                         | template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>                                                                                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Merge(                                                                                                                                                | void merge(xf::Mat<SRC_T, ROWS, COLS, NPC> &_src1, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src2, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src3, xf::Mat<SRC_T, ROWS, COLS, NPC> &_src4, xf::Mat<DST_T, ROWS, COLS, NPC> &_dst) |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src0,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src1,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src2,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src3,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | MinMaxLoc                      | template<int ROWS, int COLS, int SRC_T, typename P_T>                                                                                                      | template<int SRC_T,int ROWS,int COLS,int NPC=0>                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void MinMaxLoc(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                | void minMaxLoc(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src,int32_t \*min_value, int32_t \*max_value,uint16_t \*_minlocx, uint16_t \*_minlocy, uint16_t \*_maxlocx, uint16_t \*_maxlocy )                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | P_T\* min_val,P_T\* max_val,Point& min_loc,                                                                                                                |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Point& max_loc)                                                                                                                                            |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Mul                            | template<int ROWS, int COLS, int SRC1_T, int SRC2_T, int DST_T>                                                                                            | template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Mul(Mat<ROWS, COLS, SRC1_T>& src1,                                                                                                                    | void multiply(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst,float scale)                                                                  |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC2_T>& src2,                                                                                                                             |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Not                            | template<int ROWS, int COLS, int SRC_T, int DST_T>                                                                                                         | template<int SRC_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                                              |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Not(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                      | void bitwise_not(xf::Mat<SRC_T, ROWS, COLS, NPC> & src, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)                                                                                                                    |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst)                                                                                                                               |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Range                          | template<int ROWS, int COLS, int SRC_T, int DST_T, typename P_T>                                                                                           | template<int SRC_T, int ROWS, int COLS,int NPC=1>                                                                                                                                                                 |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Range(Mat<ROWS, COLS, SRC_T>& src,                                                                                                                    | void inRange(xf::Mat<SRC_T, ROWS, COLS, NPC> & src,unsigned char lower_thresh,unsigned char upper_thresh,xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst,                                                                                                                               |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | P_T start,P_T end)                                                                                                                                         |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Resize                         | template<int SRC_T, int ROWS,int COLS,int DROWS,int DCOLS>                                                                                                 | template<int INTERPOLATION_TYPE, int TYPE, int SRC_ROWS, int SRC_COLS, int DST_ROWS, int DST_COLS, int NPC, int MAX_DOWN_SCALE>                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Resize (                                                                                                                                              | void resize (xf::Mat<TYPE, SRC_ROWS, SRC_COLS, NPC> & \_src, xf::Mat<TYPE, DST_ROWS, DST_COLS, NPC> & \_dst)                                                                                                      |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T> &_src,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<DROWS, DCOLS, SRC_T> &_dst,                                                                                                                            |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | int interpolation=HLS_INTER_LINEAR )                                                                                                                       |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | sobel                          | template<int XORDER, int YORDER, int SIZE, int SRC_T, int DST_T, int ROWS,int COLS,int DROWS,int DCOLS>                                                    | template<int BORDER_TYPE,int FILTER_TYPE, int SRC_T,int DST_T, int ROWS, int COLS,int NPC=1,bool USE_URAM = false>                                                                                                |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Sobel (Mat<ROWS, COLS, SRC_T>                                                                                                                         | void Sobel(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & \_dst_matx,xf::Mat<DST_T, ROWS, COLS, NPC> & \_dst_maty)                                                                 |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | &_src,Mat<DROWS, DCOLS, DST_T> &_dst)                                                                                                                      |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | split                          | template<int ROWS, int COLS, int SRC_T, int DST_T>                                                                                                         | template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>                                                                                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Split(                                                                                                                                                | void extractChannel(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> & \_dst_mat, uint16_t \_channel)                                                                                 |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src,                                                                                                                               |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst0,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst1,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst2,                                                                                                                              |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst3)                                                                                                                              |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Threshold                      | template<int ROWS, int COLS, int SRC_T, int DST_T>                                                                                                         | template<int THRESHOLD_TYPE, int SRC_T, int ROWS, int COLS,int NPC=1>                                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Threshold(                                                                                                                                            | void Threshold(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC> & \_dst_mat,short int thresh,short int maxval )                                                                        |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, SRC_T>& src,                                                                                                                               |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T>& dst,                                                                                                                               |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | HLS_TNAME(SRC_T) thresh,                                                                                                                                   |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | HLS_TNAME(DST_T) maxval,                                                                                                                                   |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | int thresh_type)                                                                                                                                           |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Scale                          | template<int ROWS, int COLS, int SRC_T, int DST_T, typename P_T>                                                                                           | template< int SRC_T,int DST_T, int ROWS, int COLS, int NPC = 1>                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void Scale(Mat<ROWS, COLS, SRC_T>& src,Mat<ROWS, COLS, DST_T>& dst, P_T scale=1.0,P_T shift=0.0)                                                           | void scale(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<DST_T, ROWS, COLS, NPC> & dst,float scale, float shift)                                                                                                |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | InitUndistortRectifyMapInverse | template<typename CMT, typename DT, typename ICMT, int ROWS, int COLS, int MAP1_T, int MAP2_T, int N>                                                      | template< int CM_SIZE, int DC_SIZE, int MAP_T, int ROWS, int COLS, int NPC >                                                                                                                                      |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void InitUndistortRectifyMapInverse (                                                                                                                      | void InitUndistortRectifyMapInverse (                                                                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Window<3,3, CMT> cameraMatrix,DT(&distCoeffs)[N],Window<3,3, ICMT> ir, Mat<ROWS, COLS, MAP1_T> &map1,Mat<ROWS, COLS, MAP2_T> &map2,int noRotation=false)   | ap_fixed<32,12> \*cameraMatrix,                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            | ap_fixed<32,12> \*distCoeffs,                                                                                                                                                                                     |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            | ap_fixed<32,12> \*ir,                                                                                                                                                                                             |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            | xf::Mat<MAP_T, ROWS, COLS, NPC> &_mapx_mat,xf::Mat<MAP_T, ROWS, COLS, NPC> &_mapy_mat,int \_cm_size, int \_dc_size)                                                                                               |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | Avg, mean, AvgStddev           | template<typename DST_T, int ROWS, int COLS, int SRC_T>                                                                                                    | template<int SRC_T,int ROWS, int COLS,int NPC=1>void meanStdDev(xf::Mat<SRC_T, ROWS, COLS, NPC> & \_src,unsigned short\* \_mean,unsigned short\* \_stddev)                                                        |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | DST_T Mean(Mat<ROWS, COLS, SRC_T>& src)                                                                                                                    |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | CvtColor                       | template<typename CONVERSION,int SRC_T, int DST_T,int ROWS,int COLS>                                                                                       | Color Conversion                                                                                                                                                                                                  |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | void CvtColor(Mat<ROWS, COLS, SRC_T> &_src,                                                                                                                |                                                                                                                                                                                                                   |
   |                                |                                                                                                                                                            |                                                                                                                                                                                                                   |
   |                                | Mat<ROWS, COLS, DST_T> &_dst)                                                                                                                              |                                                                                                                                                                                                                   |
   +--------------------------------+------------------------------------------------------------------------------------------------------------------------------------------------------------+-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+



Note: All the functions except Reduce can process N-pixels per clock
where N is power of 2.


Using the Vitis vision Library
-------------------------------

This section describes using the Vitis vision library in the Vitis development
environment.

Note: The instructions in this section assume that you have downloaded
and installed all the required packages. For more information, see the
`Prerequisites <getting-started-with-sdsoc.html#gyt1504034261161>`__.

include folder constitutes all the necessary components to build a
Computer Vision or Image Processing pipeline using the library. The
folders common and core contain the infrastructure that the library
functions need for basic functions, Mat class, and macros. The library
functions are categorized into 4 folders, features, video,dnn, and
imgproc based on the operation they perform. The names of the folders
are self-explanatory.

To work with the library functions, you need to include the path to the
The Vitis vision library is structured as shown in the following table. The
include folder in the Vitis project. You can include relevant header files
for the library functions you will be working with after you source the
include folder’s path to the compiler. For example, if you would like to
work with Harris Corner Detector and Bilateral Filter, you must use the
following lines in the host code:

.. code:: c

   #include “features/xf_harris.hpp” //for Harris Corner Detector
   #include “imgproc/xf_bilateral_filter.hpp” //for Bilateral Filter
   #include “video/xf_kalmanfilter.hpp”

After the headers are included, you can work with the library functions
as described in the `Vitis vision Library API
Reference <api-reference.html#ycb1504034263746>`__ using the examples
in the examples folder as reference.

The following table gives the name of the header file, including the
folder name, which contains the library function.

.. table:: Table : Vitis vision Library Contents

   +-------------------------------------------+-----------------------------------+
   | Function Name                             | File Path in the include folder   |
   +===========================================+===================================+
   | xf::cv::accumulate                        | imgproc/xf_accumulate_image.hpp   |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::accumulateSquare                  | imgproc/xf_accumulate_squared.hpp |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::accumulateWeighted                | imgproc/xf_accumulate_weighted.hp |
   |                                           | p                                 |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::absdiff, xf::cv::add,             | core/xf_arithm.hpp                |
   | xf::cv::subtract, xf::cv::bitwise_and,    |                                   |
   | xf::cv::bitwise_or, xf::cv::bitwise_not,  |                                   |
   | xf::cv::bitwise_xor,xf::cv::multiply      |                                   |
   | ,xf::cv::Max, xf::cv::Min,xf::cv::compare,|                                   |
   | xf::cv::zero, xf::cv::addS, xf::cv::SubS, |                                   |
   | xf::cv::SubRS ,xf::cv::compareS,          |                                   |
   | xf::cv::MaxS, xf::cv::MinS, xf::cv::set   |                                   |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::addWeighted                       | imgproc/xf_add_weighted.hpp       |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::autowhitebalance                  | imgproc/xf_autowhitebalance.hpp   |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::bilateralFilter                   | imgproc/xf_histogram.hpp          |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::boxFilter                         | imgproc/xf_box_filter.hpp         |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::boundingbox                       | imgproc/xf_boundingbox.hpp        |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::badpixelcorrection                | imgproc/xf_bpc.hpp                |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::Canny                             | imgproc/xf_canny.hpp              |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::Colordetect                       | imgproc/xf_colorthresholding.hpp, |
   |                                           | imgproc/xf_bgr2hsv.hpp,           |
   |                                           | imgproc/xf_erosion.hpp,           |
   |                                           | imgproc/xf_dilation.hpp           |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::merge                             | imgproc/xf_channel_combine.hpp    |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::extractChannel                    | imgproc/xf_channel_extract.hpp    |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::convertTo                         | imgproc/xf_convert_bitdepth.hpp   |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::crop                              | imgproc/xf_crop.hpp               |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::filter2D                          | imgproc/xf_custom_convolution.hpp |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::nv122iyuv, xf::cv::nv122rgba,     | imgproc/xf_cvt_color.hpp          |
   | xf::cv::nv122yuv4, xf::cv::nv212iyuv,     |                                   |
   | xf::cv::nv212rgba, xf::cv::nv212yuv4,     |                                   |
   | xf::cv::rgba2yuv4, xf::cv::rgba2iyuv,     |                                   |
   | xf::cv::rgba2nv12, xf::cv::rgba2nv21,     |                                   |
   | xf::cv::uyvy2iyuv, xf::cv::uyvy2nv12,     |                                   |
   | xf::cv::uyvy2rgba, xf::cv::yuyv2iyuv,     |                                   |
   | xf::cv::yuyv2nv12, xf::cv::yuyv2rgba,     |                                   |
   | xf::cv::rgb2iyuv,xf::cv::rgb2nv12,        |                                   |
   | xf::cv::rgb2nv21, xf::cv::rgb2yuv4,       |                                   |
   | xf::cv::rgb2uyvy, xf::cv::rgb2yuyv,       |                                   |
   | xf::cv::rgb2bgr, xf::cv::bgr2uyvy,        |                                   |
   | xf::cv::bgr2yuyv, xf::cv::bgr2rgb,        |                                   |
   | xf::cv::bgr2nv12, xf::cv::bgr2nv21,       |                                   |
   | xf::cv::iyuv2nv12, xf::cv::iyuv2rgba,     |                                   |
   | xf::cv::iyuv2rgb, xf::cv::iyuv2yuv4,      |                                   |
   | xf::cv::nv122uyvy, xf::cv::nv122yuyv,     |                                   |
   | xf::cv::nv122nv21, xf::cv::nv212rgb,      |                                   |
   | xf::cv::nv212bgr, xf::cv::nv212uyvy,      |                                   |
   | xf::cv::nv212yuyv, xf::cv::nv212nv12,     |                                   |
   | xf::cv::uyvy2rgb, xf::cv::uyvy2bgr,       |                                   |
   | xf::cv::uyvy2yuyv, xf::cv::yuyv2rgb,      |                                   |
   | xf::cv::yuyv2bgr, xf::cv::yuyv2uyvy,      |                                   |
   | xf::cv::rgb2gray, xf::cv::bgr2gray,       |                                   |
   | xf::cv::gray2rgb, xf::cv::gray2bgr,       |                                   |
   | xf::cv::rgb2xyz, xf::cv::bgr2xyz...       |                                   |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::dilate                            | imgproc/xf_dilation.hpp           |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::demosaicing                       | imgproc/xf_demosaicing.hpp        |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::erode                             | imgproc/xf_erosion.hpp            |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::fast                              | features/xf_fast.hpp              |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::GaussianBlur                      | imgproc/xf_gaussian_filter.hpp    |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::gaincontrol                       | imgproc/xf_gaincontrol.hpp        |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::gammacorrection                   | imgproc/xf_gammacorrection        |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::cornerHarris                      | features/xf_harris.hpp            |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::calcHist                          | imgproc/xf_histogram.hpp          |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::equalizeHist                      | imgproc/xf_hist_equalize.hpp      |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::HOGDescriptor                     | imgproc/xf_hog_descriptor.hpp     |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::Houghlines                        | imgproc/xf_houghlines.hpp         |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::inRange                           | imgproc/xf_inrange.hpp            |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::integralImage                     | imgproc/xf_integral_image.hpp     |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::densePyrOpticalFlow               | video/xf_pyr_dense_optical_flow.h |
   |                                           | pp                                |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::DenseNonPyrLKOpticalFlow          | video/xf_dense_npyr_optical_flow. |
   |                                           | hpp                               |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::LUT                               | imgproc/xf_lut.hpp                |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::KalmanFilter                      | video/xf_kalmanfilter.hpp         |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::magnitude                         | core/xf_magnitude.hpp             |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::MeanShift                         | imgproc/xf_mean_shift.hpp         |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::meanStdDev                        | core/xf_mean_stddev.hpp           |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::medianBlur                        | imgproc/xf_median_blur.hpp        |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::minMaxLoc                         | core/xf_min_max_loc.hpp           |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::OtsuThreshold                     | imgproc/xf_otsuthreshold.hpp      |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::phase                             | core/xf_phase.hpp                 |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::preProcess                        | dnn/xf_pre_process.hpp            |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::paintmask                         | imgproc/xf_paintmask.hpp          |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::pyrDown                           | imgproc/xf_pyr_down.hpp           |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::pyrUp                             | imgproc/xf_pyr_up.hpp             |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::reduce                            | imgrpoc/xf_reduce.hpp             |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::remap                             | imgproc/xf_remap.hpp              |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::resize                            | imgproc/xf_resize.hpp             |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::scale                             | imgproc/xf_scale.hpp              |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::Scharr                            | imgproc/xf_scharr.hpp             |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::SemiGlobalBM                      | imgproc/xf_sgbm.hpp               |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::Sobel                             | imgproc/xf_sobel.hpp              |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::StereoPipeline                    | imgproc/xf_stereo_pipeline.hpp    |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::sum                               | imgproc/xf_sum.hpp                |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::StereoBM                          | imgproc/xf_stereoBM.hpp           |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::SVM                               | imgproc/xf_svm.hpp                |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::Threshold                         | imgproc/xf_threshold.hpp          |
   +-------------------------------------------+-----------------------------------+
   | xf::cv::warpTransform                     | imgproc/xf_warp_transform.hpp     |
   +-------------------------------------------+-----------------------------------+





Changing the Hardware Kernel Configuration
------------------------------------------

   Update the <path to vision git
   folder>/visoin/L1/examples/<function>/build/xf_config_params.h file.




Using the Vitis Vision Library Functions on Hardware
-----------------------------------------------------

The following table lists the Vitis vision library functions and the command
to run the respective examples on hardware. It is assumed that your
design is completely built and the board has booted up correctly.

.. table:: Table : Using the Vitis vision Library Function on Hardware

   +--------------+---------------------------+--------------------------+
   | Example      | Function Name             | Usage on Hardware        |
   +==============+===========================+==========================+
   | accumulate   | xf::cv::accumulate        | ./<executable name>.elf  |
   |              |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | accumulatesq | xf::cv::accumulateSquare  | ./<executable name>.elf  |
   | uared        |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | accumulatewe |xf::cv::accumulateWeighted | ./<executable name>.elf  |
   | ighted       |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | addS         | xf::cv::addS              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | arithm       | xf::cv::absdiff, 	      | ./<executable name>.elf  |
   |              | xf::cv::subtract,         | <path to input image 1>  |
   |              | xf::cv::bitwise_and,      | <path to input image 2>  |
   |              | xf::cv::bitwise_or,       |                          |
   |              | xf::cv::bitwise_not,      |                          |
   |              | xf::cv::bitwise_xor       |                          |
   +--------------+---------------------------+--------------------------+
   | addweighted  | xf::cv::addWeighted       | ./<executable name>.elf  |
   |              |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | Autowhite    | xf::cv::autowhitebalance  | ./<executable name>.elf  |
   | balance      |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Bilateralfil | xf::cv::bilateralFilter   | ./<executable name>.elf  |
   | ter          |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Boxfilter    | xf::cv::boxFilter         | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Badpixelcorr | xf::cv::badpixelcorrection| ./<executable name>.elf  |
   | ection       |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Boundingbox  | xf::cv::boundingbox       | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   |              |                           | <No of ROI's>            |
   +--------------+---------------------------+--------------------------+
   | Canny        | xf::cv::Canny             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | channelcombi | xf::cv::merge             | ./<executable name>.elf  |
   | ne           |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   |              |                           | <path to input image 3>  |
   |              |                           | <path to input image 4>  |
   +--------------+---------------------------+--------------------------+
   | Channelextra | xf::cv::extractChannel    | ./<executable name>.elf  |
   | ct           |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Colordetect  | xf::cv::bgr2hsv,          | ./<executable name>.elf  |
   |              | xf::cv::colorthresholding,| <path to input image>    |
   |              | xf::cv:: erode, xf::cv::  |                          |
   |              | dilate                    |                          |
   +--------------+---------------------------+--------------------------+
   | compare      | xf::cv::compare           | ./<executable name>.elf  |
   |              |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | compareS     | xf::cv::compareS          | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Convertbitde | xf::cv::convertTo         | ./<executable name>.elf  |
   | pth          |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Cornertracke | xf::cv::cornerTracker     | ./exe <input video> <no. |
   | r            |                           | of frames> <Harris       |
   |              |                           | Threshold> <No. of       |
   |              |                           | frames after which       |
   |              |                           | Harris Corners are       |
   |              |                           | Reset>                   |
   +--------------+---------------------------+--------------------------+
   | crop         | xf::cv::crop              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Customconv   | xf::cv::filter2D          | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::iyuv2nv12         | ./<executable name>.elf  |
   | IYUV2NV12    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   |              |                           | <path to input image 3>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::iyuv2rgba         | ./<executable name>.elf  |
   | IYUV2RGBA    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   |              |                           | <path to input image 3>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::iyuv2yuv4         | ./<executable name>.elf  |
   | IYUV2YUV4    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   |              |                           | <path to input image 3>  |
   |              |                           | <path to input image 4>  |
   |              |                           | <path to input image 5>  |
   |              |                           | <path to input image 6>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::nv122iyuv         | ./<executable name>.elf  |
   | NV122IYUV    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::nv122rgba         | ./<executable name>.elf  |
   | NV122RGBA    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::nv122yuv4         | ./<executable name>.elf  |
   | NV122YUV4    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::nv212iyuv         | ./<executable name>.elf  |
   | NV212IYUV    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::nv212rgba         | ./<executable name>.elf  |
   | NV212RGBA    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::nv212yuv4         | ./<executable name>.elf  |
   | NV212YUV4    |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::rgba2yuv4         | ./<executable name>.elf  |
   | RGBA2YUV4    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::rgba2iyuv         | ./<executable name>.elf  |
   | RGBA2IYUV    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::rgba2nv12         | ./<executable name>.elf  |
   | RGBA2NV12    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::rgba2nv21         | ./<executable name>.elf  |
   | RGBA2NV21    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::uyvy2iyuv         | ./<executable name>.elf  |
   | UYVY2IYUV    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::uyvy2nv12         | ./<executable name>.elf  |
   | UYVY2NV12    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::uyvy2rgba         | ./<executable name>.elf  |
   | UYVY2RGBA    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::yuyv2iyuv         | ./<executable name>.elf  |
   | YUYV2IYUV    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::yuyv2nv12         | ./<executable name>.elf  |
   | YUYV2NV12    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | cvtcolor     | xf::cv::yuyv2rgba         | ./<executable name>.elf  |
   | YUYV2RGBA    |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Demosaicing  | xf::cv::demosaicing       | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Difference   | xf::cv::GaussianBlur,     | ./<exe-name>.elf <path   |
   | of Gaussian  | xf::cv::duplicateMat,     | to input image>          |
   |              | xf::cv::delayMat, and     |                          |
   |              | xf::cv::subtract          |                          |
   +--------------+---------------------------+--------------------------+
   | Dilation     | xf::cv::dilate            | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Erosion      | xf::cv::erode             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Fast         | xf::cv::fast              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Gaussianfilt | xf::cv::GaussianBlur      | ./<executable name>.elf  |
   | er           |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Gaincontrol  | xf::cv::gaincontrol       | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Gammacorrec  | xf::cv::gammacorrection   | ./<executable name>.elf  |
   | tion         |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Harris       | xf::cv::cornerHarris      | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Histogram    | xf::cv::calcHist          | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Histequializ | xf::cv::equalizeHist      | ./<executable name>.elf  |
   | e            |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Hog          | xf::cv::HOGDescriptor     | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Houghlines   | xf::cv::HoughLines        | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | inRange      | xf::cv::inRange           | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Integralimg  | xf::cv::integralImage     | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Lkdensepyrof | xf::cv::densePyrOpticalFlo| ./<executable name>.elf  |
   |              | w                         | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | Lknpyroflow  | xf::cv::DenseNonPyr       | ./<executable name>.elf  |
   |              | LKOpticalFlow             | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | Lut          | xf::cv::LUT               | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Kalman       | xf::cv::KalmanFilter      | ./<executable name>.elf  |
   | Filter       |                           |                          |
   +--------------+---------------------------+--------------------------+
   | Magnitude    | xf::cv::magnitude         | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Max          | xf::cv::Max               | ./<executable name>.elf  |
   |              |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | MaxS         | xf::cv::MaxS              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | meanshifttra | xf::cv::MeanShift         | ./<executable name>.elf  |
   | cking        |                           | <path to input           |
   |              |                           | video/input image files> |
   |              |                           | <Number of objects to    |
   |              |                           | track>                   |
   +--------------+---------------------------+--------------------------+
   | meanstddev   | xf::cv::meanStdDev        | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | medianblur   | xf::cv::medianBlur        | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Min          | xf::cv::Min               | ./<executable name>.elf  |
   |              |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | MinS         | xf::cv::MinS              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Minmaxloc    | xf::cv::minMaxLoc         | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | otsuthreshol | xf::cv::OtsuThreshold     | ./<executable name>.elf  |
   | d            |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | paintmask    | xf::cv::paintmask         | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Phase        | xf::cv::phase             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Pyrdown      | xf::cv::pyrDown           | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | Pyrup        | xf::cv::pyrUp             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | reduce       | xf::cv::reduce            | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | remap        | xf::cv::remap             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   |              |                           | <path to mapx data>      |
   |              |                           | <path to mapy data>      |
   +--------------+---------------------------+--------------------------+
   | Resize       | xf::cv::resize            | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | scale        | xf::cv::scale             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | scharrfilter | xf::cv::Scharr            | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | set          | xf::cv::set               | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | SemiGlobalBM | xf::cv::SemiGlobalBM      | ./<executable name>.elf  |
   |              |                           | <path to left image>     |
   |              |                           | <path to right image>    |
   +--------------+---------------------------+--------------------------+
   | sobelfilter  | xf::cv::Sobel             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | stereopipeli | xf::cv::StereoPipeline    | ./<executable name>.elf  |
   | ne           |                           | <path to left image>     |
   |              |                           | <path to right image>    |
   +--------------+---------------------------+--------------------------+
   | stereolbm    | xf::cv::StereoBM          | ./<executable name>.elf  |
   |              |                           | <path to left image>     |
   |              |                           | <path to right image>    |
   +--------------+---------------------------+--------------------------+
   | subRS        | xf::cv::SubRS             | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | subS         | xf::cv::SubS              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | sum          | xf::cv::sum               | ./<executable name>.elf  |
   |              |                           | <path to input image 1>  |
   |              |                           | <path to input image 2>  |
   +--------------+---------------------------+--------------------------+
   | svm          | xf::cv::SVM               | ./<executable name>.elf  |
   +--------------+---------------------------+--------------------------+
   | threshold    | xf::cv::Threshold         | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | warptransfor | xf::cv::warpTransform     | ./<executable name>.elf  |
   | m            |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+
   | zero         | xf::cv::zero              | ./<executable name>.elf  |
   |              |                           | <path to input image>    |
   +--------------+---------------------------+--------------------------+

.. |image0| image:: ./images/wuz1554997295362.png
   :class: image
   
.. |image1| image:: ./images/wuz1554997295362.png
   :class: image
  
.. |image2| image:: ./images/wuz1554997295362.png
   :class: image
  
