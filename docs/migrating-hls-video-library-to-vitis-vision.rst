


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

   +--------+------------------------------+------------------------------------+
   | Functi | HLS Video Library -API       | Vitis vision Library-API           |
   | ons    |                              |                                    |
   +========+==============================+====================================+
   | addS   | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int POLICY_TYPE,       |
   |        | LS, int SRC_T, typename _T,  |  int SRC_T, int ROWS, int CO       |
   |        | int DST_T>                   | LS, int NPC =1>                    |
   |        |    void AddS(Mat<ROWS, COLS, |    void addS(xf::cv::Mat<SRC_T,    |
   |        |  SRC_T>&src,Scalar<HLS_MAT_C | ROWS, COLS, NPC> & _src1, un       |
   |        | N(SRC_T), _T> scl, Mat<ROWS, | signed char _scl[XF_CHANNELS       |
   |        |  COLS, DST_T>& dst)          | (SRC_T,NPC)],xf::cv::Mat<SRC_T,    |
   |        |                              | ROWS, COLS, NPC> & _dst)           |
   +--------+------------------------------+------------------------------------+
   | AddWei | .. code:: c                  | .. code:: c                        |
   | ghted  |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T,int D       |
   |        | LS, int SRC1_T, int SRC2_T,  | ST_T, int ROWS, int COLS, in       |
   |        | int DST_T, typename P_T>     | t NPC = 1>                         |
   |        |    void AddWeighted(Mat<ROWS |    void addWeighted(xf::cv::Mat<   |
   |        | , COLS, SRC1_T>& src1,P_T al | SRC_T, ROWS, COLS, NPC> & sr       |
   |        | pha,Mat<ROWS, COLS, SRC2_T>& | c1,float alpha, xf::cv::Mat<SRC_   |
   |        |  src2,P_T beta, P_T gamma,Ma | T, ROWS, COLS, NPC> & src2,f       |
   |        | t<ROWS, COLS, DST_T>& dst)   | loat beta, float gama, xf::cv::M   |
   |        |                              | at<DST_T, ROWS, COLS, NPC> &       |
   |        |                              |  dst)                              |
   +--------+------------------------------+------------------------------------+
   | Cmp    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int CMP_OP, int        |
   |        | LS, int SRC1_T, int SRC2_T,  | SRC_T, int ROWS, int COLS, i       |
   |        | int DST_T>                   | nt NPC =1>                         |
   |        |    void Cmp(Mat<ROWS, COLS,  |    void compare(xf::cv::Mat<SRC_   |
   |        | SRC1_T>& src1,Mat<ROWS, COLS | T, ROWS, COLS, NPC> & _src1,       |
   |        | , SRC2_T>& src2,             |  xf::cv::Mat<SRC_T, ROWS, COLS,    |
   |        |    Mat<ROWS, COLS, DST_T>& d | NPC> & _src2,xf::cv::Mat<SRC_T,    |
   |        | st,int cmp_op)               | ROWS, COLS, NPC> & _dst)           |
   +--------+------------------------------+------------------------------------+
   | CmpS   | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int CMP_OP, int        |
   |        | LS, int SRC_T, typename P_T, | SRC_T, int ROWS, int COLS, i       |
   |        |  int DST_T>                  | nt NPC =1>                         |
   |        |    void CmpS(Mat<ROWS, COLS, |    void compare(xf::cv::Mat<SRC_   |
   |        |  SRC_T>& src,  P_T value, Ma | T, ROWS, COLS, NPC> & _src1,       |
   |        | t<ROWS, COLS, DST_T>& dst, i |  unsigned char _scl[XF_CHANN       |
   |        | nt cmp_op)                   | ELS(SRC_T,NPC)],xf::cv::Mat<SRC_   |
   |        |                              | T, ROWS, COLS, NPC> & _dst)        |
   +--------+------------------------------+------------------------------------+
   | Max    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int R       |
   |        | LS, int SRC1_T, int SRC2_T,  | OWS, int COLS, int NPC =1>         |
   |        | int DST_T>                   |    void Max(xf::cv::Mat<SRC_T, R   |
   |        |    void Max(Mat<ROWS, COLS,  | OWS, COLS, NPC> & _src1, xf:       |
   |        | SRC1_T>& src1,               | :Mat<SRC_T, ROWS, COLS, NPC>       |
   |        |            Mat<ROWS, COLS, S |  & _src2,xf::cv::Mat<SRC_T, ROWS   |
   |        | RC2_T>& src2,                | , COLS, NPC> & _dst)               |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | MaxS   | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T, int        |
   |        | LS, int SRC_T, typename _T,  | ROWS, int COLS, int NPC =1>        |
   |        | int DST_T>                   |    void max(xf::cv::Mat<SRC_T, R   |
   |        |    void MaxS(Mat<ROWS, COLS, | OWS, COLS, NPC> & _src1,  un       |
   |        |  SRC_T>& src,                | signed char _scl[XF_CHANNELS       |
   |        |    _T value, Mat<ROWS, COLS, | (SRC_T,NPC)],xf::cv::Mat<SRC_T,    |
   |        |  DST_T>& dst)                | ROWS, COLS, NPC> & _dst)           |
   +--------+------------------------------+------------------------------------+
   | Min    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T, int        |
   |        | LS, int SRC1_T, int SRC2_T,  | ROWS, int COLS, int NPC =1>        |
   |        | int DST_T>                   |    void Min(xf::cv::Mat<SRC_T, R   |
   |        |    void Min(Mat<ROWS, COLS,  | OWS, COLS, NPC> & _src1, xf:       |
   |        | SRC1_T>& src1,               | :Mat<SRC_T, ROWS, COLS, NPC>       |
   |        |            Mat<ROWS, COLS, S |  & _src2,xf::cv::Mat<SRC_T, ROWS   |
   |        | RC2_T>& src2,                | , COLS, NPC> & _dst)               |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | MinS   | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T, int        |
   |        | LS, int SRC_T, typename _T,  | ROWS, int COLS, int NPC =1>        |
   |        | int DST_T>                   |    void min(xf::cv::Mat<SRC_T, R   |
   |        |    void MinS(Mat<ROWS, COLS, | OWS, COLS, NPC> & _src1,  un       |
   |        |  SRC_T>& src,                | signed char _scl[XF_CHANNELS       |
   |        |            _T value,Mat<ROWS | (SRC_T,NPC)],xf::cv::Mat<SRC_T,    |
   |        | , COLS, DST_T>& dst)         | ROWS, COLS, NPC> & _dst)           |
   +--------+------------------------------+------------------------------------+
   | PaintM | .. code:: c                  | .. code:: c                        |
   | ask    |                              |                                    |
   |        |    template<int SRC_T,int MA |    template< int SRC_T,int M       |
   |        | SK_T,int ROWS,int COLS>      | ASK_T, int ROWS, int COLS,in       |
   |        |    void PaintMask(           | t NPC=1>                           |
   |        |    Mat<ROWS,COLS,SRC_T>   &_ |    void paintmask(xf::cv::Mat<SR   |
   |        | src,                         | C_T, ROWS, COLS, NPC> & _src       |
   |        |    Mat<ROWS,COLS,MASK_T>&_ma | _mat, xf::cv::Mat<MASK_T, ROWS,    |
   |        | sk,                          | COLS, NPC> & in_mask, xf::cv::Ma   |
   |        |    Mat<ROWS,COLS,SRC_T>&_dst | t<SRC_T, ROWS, COLS, NPC> &        |
   |        | ,Scalar<HLS_MAT_CN(SRC_T),HL | _dst_mat, unsigned char _col       |
   |        | S_TNAME(SRC_T)> _color)      | or[XF_CHANNELS(SRC_T,NPC)])        |
   +--------+------------------------------+------------------------------------+
   | Reduce | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<typename INTER_S |    template< int REDUCE_OP,        |
   |        | UM_T, int ROWS, int COLS, in | int SRC_T,int DST_T, int ROW       |
   |        | t SRC_T, int DST_ROWS, int D | S, int COLS,int ONE_D_HEIGHT       |
   |        | ST_COLS, int DST_T>          | , int ONE_D_WIDTH, int NPC=1       |
   |        |    void Reduce(              | >                                  |
   |        |             Mat<ROWS, COLS,  |    void reduce(xf::cv::Mat<SRC_T   |
   |        | SRC_T> &src,                 | , ROWS, COLS, NPC> & _src_ma       |
   |        |             Mat<DST_ROWS, DS | t,  xf::cv::Mat<DST_T, ONE_D_HEI   |
   |        | T_COLS, DST_T> &dst,         | GHT, ONE_D_WIDTH, 1> & _dst_       |
   |        |             int dim,         | mat, unsigned char dim)            |
   |        |             int op=HLS_REDUC |                                    |
   |        | E_SUM)                       |                                    |
   +--------+------------------------------+------------------------------------+
   | Zero   | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T, int        |
   |        | LS, int SRC_T, int DST_T>    | ROWS, int COLS, int NPC =1>        |
   |        |    void Zero(Mat<ROWS, COLS, |    void zero(xf::cv::Mat<SRC_T,    |
   |        |  SRC_T>& src,                | ROWS, COLS, NPC> & _src1,xf:       |
   |        |              Mat<ROWS, COLS, | :Mat<SRC_T, ROWS, COLS, NPC>       |
   |        |  DST_T>& dst)                |  & _dst)                           |
   +--------+------------------------------+------------------------------------+
   | Sum    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<typename DST_T,  |    template< int SRC_T, int        |
   |        | int ROWS, int COLS, int SRC_ | ROWS, int COLS, int NPC = 1>       |
   |        | T>                           |    void sum(xf::cv::Mat<SRC_T, R   |
   |        |    Scalar<HLS_MAT_CN(SRC_T), | OWS, COLS, NPC> & src1, doub       |
   |        |  DST_T> Sum(                 | le sum[XF_CHANNELS(SRC_T,NPC       |
   |        |            Mat<ROWS, COLS, S | )] )                               |
   |        | RC_T>& src)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | SubS   | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int POLICY_TYPE,       |
   |        | LS, int SRC_T, typename _T,  |  int SRC_T, int ROWS, int CO       |
   |        | int DST_T>                   | LS, int NPC =1>                    |
   |        |    void SubS(Mat<ROWS, COLS, |    void SubS(xf::cv::Mat<SRC_T,    |
   |        |  SRC_T>& src,                | ROWS, COLS, NPC> & _src1,  u       |
   |        |            Scalar<HLS_MAT_CN | nsigned char _scl[XF_CHANNEL       |
   |        | (SRC_T), _T> scl,            | S(SRC_T,NPC)],xf::cv::Mat<SRC_T,   |
   |        |            Mat<ROWS, COLS, D |  ROWS, COLS, NPC> & _dst)          |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | SubRS  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int POLICY_TYPE,       |
   |        | LS, int SRC_T, typename _T,  |  int SRC_T, int ROWS, int CO       |
   |        | int DST_T>                   | LS, int NPC =1>                    |
   |        |    void SubRS(Mat<ROWS, COLS |    void SubRS(xf::cv::Mat<SRC_T,   |
   |        | , SRC_T>& src,               |  ROWS, COLS, NPC> & _src1, u       |
   |        |            Scalar<HLS_MAT_CN | nsigned char _scl[XF_CHANNEL       |
   |        | (SRC_T), _T> scl,            | S(SRC_T,NPC)],xf::cv::Mat<SRC_T,   |
   |        |            Mat<ROWS, COLS, D |  ROWS, COLS, NPC> & _dst)          |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | Set    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T, int        |
   |        | LS, int SRC_T, typename _T,  | ROWS, int COLS, int NPC =1>        |
   |        | int DST_T>                   |    void set(xf::cv::Mat<SRC_T, R   |
   |        |    void Set(Mat<ROWS, COLS,  | OWS, COLS, NPC> & _src1,  un       |
   |        | SRC_T>& src,                 | signed char _scl[XF_CHANNELS       |
   |        |            Scalar<HLS_MAT_CN | (SRC_T,NPC)],xf::cv::Mat<SRC_T,    |
   |        | (SRC_T), _T> scl,            | ROWS, COLS, NPC> & _dst)           |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | Absdif | .. code:: c                  | .. code:: c                        |
   | f      |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int R       |
   |        | LS, int SRC1_T, int SRC2_T,  | OWS, int COLS, int NPC =1>         |
   |        | int DST_T>                   |    void absdiff(xf::cv::Mat<SRC_   |
   |        |    void AbsDiff(             | T, ROWS, COLS, NPC> & _src1,       |
   |        |            Mat<ROWS, COLS, S | xf::cv::Mat<SRC_T, ROWS, COLS, N   |
   |        | RC1_T>& src1,                | PC> & _src2,xf::cv::Mat<SRC_T, R   |
   |        |            Mat<ROWS, COLS, S | OWS, COLS, NPC> & _dst)            |
   |        | RC2_T>& src2,                |                                    |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | And    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int R       |
   |        | LS, int SRC1_T, int SRC2_T,  | OWS, int COLS, int NPC = 1>        |
   |        | int DST_T>                   |    void bitwise_and(xf::cv::Mat<   |
   |        |    void And(                 | SRC_T, ROWS, COLS, NPC> & _s       |
   |        |            Mat<ROWS, COLS, S | rc1, xf::cv::Mat<SRC_T, ROWS, CO   |
   |        | RC1_T>& src1,                | LS, NPC> & _src2, xf::cv::Mat<SR   |
   |        |            Mat<ROWS, COLS, S | C_T, ROWS, COLS, NPC> &_dst)       |
   |        | RC2_T>& src2,                |                                    |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>&  dst)                 |                                    |
   +--------+------------------------------+------------------------------------+
   | Dilate | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int Shape_type,i |    template<int BORDER_TYPE,       |
   |        | nt ITERATIONS,int SRC_T, int |  int TYPE, int ROWS, int COL       |
   |        |  DST_T, typename KN_T,int IM | S,int K_SHAPE,int K_ROWS,int       |
   |        | G_HEIGHT,int IMG_WIDTH,int K |  K_COLS, int ITERATIONS, int       |
   |        | _HEIGHT,int K_WIDTH>         |  NPC=1>                            |
   |        |    void Dilate(Mat<IMG_HEIGH |    void dilate (xf::cv::Mat<TYPE   |
   |        | T, IMG_WIDTH, SRC_T>&_src,Ma | , ROWS, COLS, NPC> & _src, x       |
   |        | t<IMG_HEIGHT, IMG_WIDTH, DST | f::Mat<TYPE, ROWS, COLS, NPC       |
   |        | _T&_dst,Window<K_HEIGHT,K_WI | > & _dst,unsigned char _kern       |
   |        | DTH,KN_T>&_kernel)           | el[K_ROWS*K_COLS])                 |
   +--------+------------------------------+------------------------------------+
   | Duplic | .. code:: c                  | .. code:: c                        |
   | ate    |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int R       |
   |        | LS, int SRC_T, int DST_T>    | OWS, int COLS,int NPC>             |
   |        |    void Duplicate(Mat<ROWS,  |    void duplicateMat(xf::cv::Mat   |
   |        | COLS, SRC_T>& src,Mat<ROWS,  | <SRC_T, ROWS, COLS, NPC> & _       |
   |        | COLS, DST_T>& dst1,Mat<ROWS, | src, xf::cv::Mat<SRC_T, ROWS, CO   |
   |        |  COLS, DST_T>& dst2)         | LS, NPC> & _dst1,xf::cv::Mat<SRC   |
   |        |                              | _T, ROWS, COLS, NPC> & _dst2       |
   |        |                              | )                                  |
   +--------+------------------------------+------------------------------------+
   | Equali | .. code:: c                  | .. code:: c                        |
   | zeHist |                              |                                    |
   |        |    template<int SRC_T, int D |    template<int SRC_T, int R       |
   |        | ST_T,int ROW, int COL>       | OWS, int COLS, int NPC = 1>        |
   |        |    void EqualizeHist(Mat<ROW |    void equalizeHist(xf::cv::Mat   |
   |        | , COL, SRC_T>&_src,Mat<ROW,  | <SRC_T, ROWS, COLS, NPC> & _       |
   |        | COL, DST_T>&_dst)            | src,xf::cv::Mat<SRC_T, ROWS, COL   |
   |        |                              | S, NPC> & _src1,xf::cv::Mat<SRC_   |
   |        |                              | T, ROWS, COLS, NPC> & _dst)        |
   +--------+------------------------------+------------------------------------+
   | erode  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int Shape_type,i |    template<int BORDER_TYPE,       |
   |        | nt ITERATIONS,int SRC_T, int |  int TYPE, int ROWS, int COL       |
   |        |  DST_T, typename KN_T,int IM | S,int K_SHAPE,int K_ROWS,int       |
   |        | G_HEIGHT,int IMG_WIDTH,int K |  K_COLS, int ITERATIONS, int       |
   |        | _HEIGHT,int K_WIDTH>         |  NPC=1>                            |
   |        |    void Erode(Mat<IMG_HEIGHT |    void erode (xf::cv::Mat<TYPE,   |
   |        | , IMG_WIDTH, SRC_T>&_src,Mat |  ROWS, COLS, NPC> & _src, xf::cv   |
   |        | <IMG_HEIGHT,IMG_WIDTH,DST_T> | ::Mat<TYPE, ROWS, COLS, NPC>       |
   |        | &_dst,Window<K_HEIGHT,K_WIDT |  & _dst,unsigned char _kerne       |
   |        | H,KN_T>&_kernel)             | l[K_ROWS*K_COLS])                  |
   +--------+------------------------------+------------------------------------+
   | FASTX  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int SRC_T,int RO |    template<int NMS,int SRC_       |
   |        | WS,int COLS>                 | T,int ROWS, int COLS,int NPC       |
   |        |    void FASTX(Mat<ROWS,COLS, | =1>                                |
   |        | SRC_T> &_src,                |    void fast(xf::cv::Mat<SRC_T,    |
   |        |    Mat<ROWS,COLS,HLS_8UC1>&_ | ROWS, COLS, NPC> & _src_mat,       |
   |        | mask,HLS_TNAME(SRC_T)_thresh | xf::cv::Mat<SRC_T, ROWS, COLS, N   |
   |        | old,bool _nomax_supression)  | PC> & _dst_mat,unsigned char       |
   |        |                              |  _threshold)                       |
   +--------+------------------------------+------------------------------------+
   | Filter | .. code:: c                  | .. code:: c                        |
   | 2D     |                              |                                    |
   |        |    template<int SRC_T, int D |    template<int BORDER_TYPE,       |
   |        | ST_T, typename KN_T, typenam | int FILTER_WIDTH,int FILTER_       |
   |        | e POINT_T,                   | HEIGHT, int SRC_T,int DST_T,       |
   |        |    int IMG_HEIGHT,int IMG_WI |  int ROWS, int COLS,int NPC>       |
   |        | DTH,int K_HEIGHT,int K_WIDTH |    void filter2D(xf::cv::Mat<SRC   |
   |        | >                            | _T, ROWS, COLS, NPC> & _src_       |
   |        |    void Filter2D(Mat<IMG_HEI | mat,xf::cv::Mat<DST_T, ROWS, COL   |
   |        | GHT, IMG_WIDTH, SRC_T> &_src | S, NPC> & _dst_mat,short int       |
   |        | ,Mat<IMG_HEIGHT, IMG_WIDTH,  |  filter[FILTER_HEIGHT*FILTER       |
   |        | DST_T>  &_dst,Window<K_HEIGH | _WIDTH],unsigned char _shift       |
   |        | T,K_WIDTH,KN_T>&_kernel,Poin | )                                  |
   |        | t_<POINT_T>anchor)           |                                    |
   +--------+------------------------------+------------------------------------+
   | Gaussi | .. code:: c                  | .. code:: c                        |
   | anBlur |                              |                                    |
   |        |    template<int KH,int KW,ty |    template<int FILTER_SIZE,       |
   |        | pename BORDERMODE,int SRC_T, |  int BORDER_TYPE, int SRC_T,       |
   |        | int DST_T,int ROWS,int COLS> |  int ROWS, int COLS,int NPC        |
   |        |    void GaussianBlur(Mat<ROW | = 1>                               |
   |        | S, COLS, SRC_T>              |    void GaussianBlur(xf::cv::Mat   |
   |        |    &_src, Mat<ROWS, COLS, DS | <SRC_T, ROWS, COLS, NPC> & _       |
   |        | T_T>                         | src, xf::cv::Mat<SRC_T, ROWS, CO   |
   |        |    &_dst,double sigmaX=0,dou | LS, NPC> & _dst, float sigma       |
   |        | ble sigmaY=0)                | )                                  |
   +--------+------------------------------+------------------------------------+
   | Harris | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int blockSize,in |    template<int FILTERSIZE,i       |
   |        | t Ksize,typename KT,int SRC_ | nt BLOCKWIDTH, int NMSRADIUS       |
   |        | T,int DST_T,int ROWS,int COL | ,int SRC_T,int ROWS, int COL       |
   |        | S>                           | S,int NPC=1,bool USE_URAM=fa       |
   |        |    void Harris(Mat<ROWS, COL | lse>                               |
   |        | S, SRC_T>                    |    void cornerHarris(xf::cv::Mat   |
   |        |    &_src,Mat<ROWS, COLS, DST | <SRC_T, ROWS, COLS, NPC> & s       |
   |        | _T>&_dst,KT k,int threshold  | rc,xf::cv::Mat<SRC_T, ROWS, COLS   |
   |        |                              | , NPC> & dst,uint16_t thresh       |
   |        |                              | old, uint16_t k)                   |
   +--------+------------------------------+------------------------------------+
   | Corner | .. code:: c                  | .. code:: c                        |
   | Harris |                              |                                    |
   |        |    template<int blockSize,in |    template<int FILTERSIZE,i       |
   |        | t Ksize,typename KT,int SRC_ | nt BLOCKWIDTH, int NMSRADIUS       |
   |        | T,int DST_T,int ROWS,int COL | ,int SRC_T,int ROWS, int COL       |
   |        | S>                           | S,int NPC=1,bool USE_URAM=fa       |
   |        |    void CornerHarris(        | lse>                               |
   |        |    Mat<ROWS, COLS, SRC_T>&_s |    void cornerHarris(xf::cv::Mat   |
   |        | rc,Mat<ROWS, COLS, DST_T>&_d | <SRC_T, ROWS, COLS, NPC> & s       |
   |        | st,KT k)                     | rc,xf::cv::Mat<SRC_T, ROWS, COLS   |
   |        |                              | , NPC> & dst,uint16_t thresh       |
   |        |                              | old, uint16_t k                    |
   +--------+------------------------------+------------------------------------+
   | HoughL | .. code:: c                  | .. code:: c                        |
   | ines2  |                              |                                    |
   |        |    template<unsigned int the |    template<unsigned int RHO       |
   |        | ta,unsigned int rho,typename | ,unsigned int THETA,int MAXL       |
   |        |  AT,typename RT,int SRC_T,in | INES,int DIAG,int MINTHETA,i       |
   |        | t ROW,int COL,unsigned int l | nt MAXTHETA,int SRC_T, int R       |
   |        | inesMax>                     | OWS, int COLS,int NPC>             |
   |        |    void HoughLines2(Mat<ROW, |    void HoughLines(xf::cv::Mat<S   |
   |        | COL,SRC_T> &_src,            | RC_T, ROWS, COLS, NPC> & _sr       |
   |        |    Polar_<AT,RT> (&_lines)[l | c_mat,float outputrho[MAXLIN       |
   |        | inesMax],unsigned int thresh | ES],float outputtheta[MAXLIN       |
   |        | old)                         | ES],short threshold,short li       |
   |        |                              | nesmax)                            |
   +--------+------------------------------+------------------------------------+
   | Integr | .. code:: c                  | .. code:: c                        |
   | al     |                              |                                    |
   |        |    template<int SRC_T, int D |    template<int SRC_TYPE,int       |
   |        | ST_T,                        |  DST_TYPE, int ROWS, int COL       |
   |        |    int ROWS,int COLS>        | S, int NPC>                        |
   |        |    void Integral(Mat<ROWS, C |    void integral(xf::cv::Mat<SRC   |
   |        | OLS, SRC_T>&_src,            | _TYPE, ROWS, COLS, NPC> & _s       |
   |        |            Mat<ROWS+1, COLS+ | rc_mat, xf::cv::Mat<DST_TYPE, RO   |
   |        | 1, DST_T>&_sum )             | WS, COLS, NPC> & _dst_mat)         |
   +--------+------------------------------+------------------------------------+
   | Merge  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int D       |
   |        | LS, int SRC_T, int DST_T>    | ST_T, int ROWS, int COLS, in       |
   |        |    void Merge(               | t NPC=1>                           |
   |        |            Mat<ROWS, COLS, S |    void merge(xf::cv::Mat<SRC_T,   |
   |        | RC_T>& src0,                 |  ROWS, COLS, NPC> &_src1, xf       |
   |        |            Mat<ROWS, COLS, S | ::Mat<SRC_T, ROWS, COLS, NPC       |
   |        | RC_T>& src1,                 | > &_src2, xf::cv::Mat<SRC_T, ROW   |
   |        |            Mat<ROWS, COLS, S | S, COLS, NPC> &_src3, xf::cv::Ma   |
   |        | RC_T>& src2,                 | t<SRC_T, ROWS, COLS, NPC> &_       |
   |        |            Mat<ROWS, COLS, S | src4, xf::cv::Mat<DST_T, ROWS, C   |
   |        | RC_T>& src3,                 | OLS, NPC> &_dst)                   |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst)                  |                                    |
   +--------+------------------------------+------------------------------------+
   | MinMax | .. code:: c                  | .. code:: c                        |
   | Loc    |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T,int RO       |
   |        | LS, int SRC_T, typename P_T> | WS,int COLS,int NPC=0>             |
   |        |    void MinMaxLoc(Mat<ROWS,  |    void minMaxLoc(xf::cv::Mat<SR   |
   |        | COLS, SRC_T>& src,           | C_T, ROWS, COLS, NPC> & _src       |
   |        |    P_T* min_val,P_T* max_val | ,int32_t *min_value, int32_t       |
   |        | ,Point& min_loc,             |  *max_value,uint16_t *_minlo       |
   |        |    Point& max_loc)           | cx, uint16_t *_minlocy, uint       |
   |        |                              | 16_t *_maxlocx, uint16_t *_m       |
   |        |                              | axlocy )                           |
   +--------+------------------------------+------------------------------------+
   | Mul    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |     template<int POLICY_TYPE       |
   |        | LS, int SRC1_T, int SRC2_T,  | , int SRC_T, int ROWS, int C       |
   |        | int DST_T>                   | OLS, int NPC = 1>                  |
   |        |    void Mul(Mat<ROWS, COLS,  |    void multiply(xf::cv::Mat<SRC   |
   |        | SRC1_T>& src1,               | _T, ROWS, COLS, NPC> & src1,       |
   |        |            Mat<ROWS, COLS, S |  xf::cv::Mat<SRC_T, ROWS, COLS,    |
   |        | RC2_T>& src2,                | NPC> & src2, xf::cv::Mat<SRC_T,    |
   |        |            Mat<ROWS, COLS, D | ROWS, COLS, NPC> & dst,float       |
   |        | ST_T>& dst)                  |  scale)                            |
   +--------+------------------------------+------------------------------------+
   | Not    | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int R       |
   |        | LS, int SRC_T, int DST_T>    | OWS, int COLS, int NPC = 1>        |
   |        |    void Not(Mat<ROWS, COLS,  |    void bitwise_not(xf::cv::Mat<   |
   |        | SRC_T>& src,                 | SRC_T, ROWS, COLS, NPC> & sr       |
   |        |            Mat<ROWS, COLS, D | c, xf::cv::Mat<SRC_T, ROWS, COLS   |
   |        | ST_T>& dst)                  | , NPC> & dst)                      |
   +--------+------------------------------+------------------------------------+
   | Range  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int R       |
   |        | LS, int SRC_T, int DST_T, ty | OWS, int COLS,int NPC=1>           |
   |        | pename P_T>                  |    void inRange(xf::cv::Mat<SRC_   |
   |        |    void Range(Mat<ROWS, COLS | T, ROWS, COLS, NPC> & src,un       |
   |        | , SRC_T>& src,               | signed char lower_thresh,uns       |
   |        |            Mat<ROWS, COLS, D | igned char upper_thresh,xf::cv::   |
   |        | ST_T>& dst,                  | Mat<SRC_T, ROWS, COLS, NPC>        |
   |        |            P_T start,P_T end | & dst)                             |
   |        | )                            |                                    |
   +--------+------------------------------+------------------------------------+
   | Resize | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int SRC_T, int R |    template<int INTERPOLATIO       |
   |        | OWS,int COLS,int DROWS,int D | N_TYPE, int TYPE, int SRC_RO       |
   |        | COLS>                        | WS, int SRC_COLS, int DST_RO       |
   |        |    void Resize (             | WS, int DST_COLS, int NPC, i       |
   |        |            Mat<ROWS, COLS, S | nt MAX_DOWN_SCALE>                 |
   |        | RC_T> &_src,                 |    void resize (xf::cv::Mat<TYPE   |
   |        |            Mat<DROWS, DCOLS, | , SRC_ROWS, SRC_COLS, NPC> &       |
   |        |  SRC_T> &_dst,               |  _src, xf::cv::Mat<TYPE, DST_ROW   |
   |        |            int interpolation | S, DST_COLS, NPC> & _dst)          |
   |        | =HLS_INTER_LINEAR )          |                                    |
   +--------+------------------------------+------------------------------------+
   | sobel  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int XORDER, int  |    template<int BORDER_TYPE,       |
   |        | YORDER, int SIZE, int SRC_T, | int FILTER_TYPE, int SRC_T,i       |
   |        |  int DST_T, int ROWS,int COL | nt DST_T, int ROWS, int COLS       |
   |        | S,int DROWS,int DCOLS>       | ,int NPC=1,bool USE_URAM = f       |
   |        |    void Sobel (Mat<ROWS, COL | alse>                              |
   |        | S, SRC_T>                    |    void Sobel(xf::cv::Mat<SRC_T,   |
   |        |    &_src,Mat<DROWS, DCOLS, D |  ROWS, COLS, NPC> & _src_mat       |
   |        | ST_T> &_dst)                 | ,xf::cv::Mat<DST_T, ROWS, COLS,    |
   |        |                              | NPC> & _dst_matx,xf::cv::Mat<DST   |
   |        |                              | _T, ROWS, COLS, NPC> & _dst_       |
   |        |                              | maty)                              |
   +--------+------------------------------+------------------------------------+
   | split  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int SRC_T, int D       |
   |        | LS, int SRC_T, int DST_T>    | ST_T, int ROWS, int COLS, in       |
   |        |    void Split(               | t NPC=1>                           |
   |        |            Mat<ROWS, COLS, S |    void extractChannel(xf::cv::M   |
   |        | RC_T>& src,                  | at<SRC_T, ROWS, COLS, NPC> &       |
   |        |            Mat<ROWS, COLS, D |  _src_mat, xf::cv::Mat<DST_T, RO   |
   |        | ST_T>& dst0,                 | WS, COLS, NPC> & _dst_mat, u       |
   |        |            Mat<ROWS, COLS, D | int16_t _channel)                  |
   |        | ST_T>& dst1,                 |                                    |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst2,                 |                                    |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>& dst3)                 |                                    |
   +--------+------------------------------+------------------------------------+
   | Thresh | .. code:: c                  | .. code:: c                        |
   | old    |                              |                                    |
   |        |    template<int ROWS, int CO |    template<int THRESHOLD_TY       |
   |        | LS, int SRC_T, int DST_T>    | PE, int SRC_T, int ROWS, int       |
   |        |    void Threshold(           |  COLS,int NPC=1>                   |
   |        |            Mat<ROWS, COLS, S |    void Threshold(xf::cv::Mat<SR   |
   |        | RC_T>& src,                  | C_T, ROWS, COLS, NPC> & _src       |
   |        |            Mat<ROWS, COLS, D | _mat,xf::cv::Mat<SRC_T, ROWS, CO   |
   |        | ST_T>& dst,                  | LS, NPC> & _dst_mat,short in       |
   |        |            HLS_TNAME(SRC_T)  | t thresh,short int maxval )        |
   |        | thresh,                      |                                    |
   |        |            HLS_TNAME(DST_T)  |                                    |
   |        | maxval,                      |                                    |
   |        |            int thresh_type)  |                                    |
   +--------+------------------------------+------------------------------------+
   | Scale  | .. code:: c                  | .. code:: c                        |
   |        |                              |                                    |
   |        |    template<int ROWS, int CO |    template< int SRC_T,int D       |
   |        | LS, int SRC_T, int DST_T, ty | ST_T, int ROWS, int COLS, in       |
   |        | pename P_T>                  | t NPC = 1>                         |
   |        |    void Scale(Mat<ROWS, COLS |    void scale(xf::cv::Mat<SRC_T,   |
   |        | , SRC_T>& src,Mat<ROWS, COLS |  ROWS, COLS, NPC> & src1, xf       |
   |        | , DST_T>& dst, P_T scale=1.0 | ::Mat<DST_T, ROWS, COLS, NPC       |
   |        | ,P_T shift=0.0)              | > & dst,float scale, float s       |
   |        |                              | hift)                              |
   +--------+------------------------------+------------------------------------+
   | InitUn | .. code:: c                  | .. code:: c                        |
   | distor |                              |                                    |
   | tRecti |    template<typename CMT, ty |    template< int CM_SIZE, in       |
   | fyMapI | pename DT, typename ICMT, in | t DC_SIZE, int MAP_T, int RO       |
   | nverse | t ROWS, int COLS, int MAP1_T | WS, int COLS, int NPC >            |
   |        | , int MAP2_T, int N>         |    void InitUndistortRectify       |
   |        |    void InitUndistortRectify | MapInverse (                       |
   |        | MapInverse (                 |            ap_fixed<32,12> *       |
   |        |     Window<3,3, CMT> cameraM | cameraMatrix,                      |
   |        | atrix,DT(&distCoeffs)[N],Win |            ap_fixed<32,12> *       |
   |        | dow<3,3, ICMT> ir, Mat<ROWS, | distCoeffs,                        |
   |        |  COLS, MAP1_T>  &map1,Mat<RO |            ap_fixed<32,12> *       |
   |        | WS, COLS, MAP2_T>  &map2,int | ir,                                |
   |        |  noRotation=false)           |            xf::cv::Mat<MAP_T, RO   |
   |        |                              | WS, COLS, NPC> &_mapx_mat,xf       |
   |        |                              | ::Mat<MAP_T, ROWS, COLS, NPC       |
   |        |                              | > &_mapy_mat,int _cm_size, i       |
   |        |                              | nt _dc_size)                       |
   +--------+------------------------------+------------------------------------+
   | Avg,   | .. code:: c                  | .. code:: c                        |
   | mean,  |                              |                                    |
   | AvgStd |    template<typename DST_T,  |    template<int SRC_T,int RO       |
   | dev    | int ROWS, int COLS, int SRC_ | WS, int COLS,int NPC=1>void        |
   |        | T>                           | meanStdDev(xf::cv::Mat<SRC_T, RO   |
   |        |    DST_T Mean(Mat<ROWS, COLS | WS, COLS, NPC> & _src,unsign       |
   |        | , SRC_T>& src)               | ed short* _mean,unsigned sho       |
   |        |                              | rt* _stddev)                       |
   +--------+------------------------------+------------------------------------+
   | CvtCol | .. code:: c                  | `Color                             |
   | or     |                              | Conversion <api-reference          |
   |        |    template<typename CONVERS | .html#toi1504034269249>`__         |
   |        | ION,int SRC_T, int DST_T,int |                                    |
   |        |  ROWS,int COLS>              |                                    |
   |        |    void CvtColor(Mat<ROWS, C |                                    |
   |        | OLS, SRC_T>  &_src,          |                                    |
   |        |            Mat<ROWS, COLS, D |                                    |
   |        | ST_T>    &_dst)              |                                    |
   +--------+------------------------------+------------------------------------+


Note: All the functions except Reduce can process N-pixels per clock
where N is power of 2.


Using the Vitis vision Library
--------------------------

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




Using the Vitis vision Library Functions on Hardware
------------------------------------------------

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
  
