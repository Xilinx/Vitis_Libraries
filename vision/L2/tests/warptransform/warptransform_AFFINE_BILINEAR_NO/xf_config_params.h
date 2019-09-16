#define RO 0 // 8 Pixel Processing
#define NO 1 // 1 Pixel Processing

// Number of rows in the input image
#define HEIGHT 2160
// Number of columns in  in the input image
#define WIDTH 3840

// Number of rows of input image to be stored
#define NUM_STORE_ROWS 100

// Number of rows of input image after which output image processing must start
#define START_PROC 50

#define RGBA 0
#define GRAY 1

// transform type 0-NN 1-BILINEAR
#define INTERPOLATION 1

// transform type 0-AFFINE 1-PERSPECTIVE
#define TRANSFORM_TYPE 0
#define XF_USE_URAM false
