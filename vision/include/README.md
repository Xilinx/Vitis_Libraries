# xfOpenCV/include
The include folder in the xfOpenCV repository contains the headers necessary to work with the kernels in the xfOpenCV library.

The headers are organized into four different folders - 

| Folder Name | Description |
| :------------- | :------------- |
| common | Contains common library infrastructure headers such as types specific to the library |
| core | Contains core library functionality headers such as math functions |
| features | Contains feature extraction kernel function definitions. For example, Harris |
| imgproc | Contains all the kernel function definitions except the ones available in the features, video folders |
| video | Contains the kernel function definitions of KalmanFilter and pyramidal, non-pyramidal optical flow functions |