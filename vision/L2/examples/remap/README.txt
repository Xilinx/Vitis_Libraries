To use barrel/pin cushion correction, we need to first generate mapx and mapy using opencv initUndistortRectifyMap
and use that mapx and mapy in xf::cv::remap function to get undistorted output.
For xf::cv::remap function, number of rows that needs to be stored in BRAM needs to be decided. 
This is produced during first run of remap host code i.e. the value of "Number of lines to be stored". 
This value needs to be updated for XF_WIN_ROWS defined in xf_config_params.h. 
During second build and run, remap function will use optimum number of BRAM.