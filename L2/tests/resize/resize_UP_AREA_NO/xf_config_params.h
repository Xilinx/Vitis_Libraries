/* Optimization type */
#define RO 0 // Resource Optimized (8-pixel implementation)
#define NO 1 // Normal Operation (1-pixel implementation)

// port widths
#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

// For Nearest Neighbor & Bilinear Interpolation, max down scale factor 2 for all 1-pixel modes, and for upscale in x
// direction
#define MAXDOWNSCALE 2

#define RGB 0
#define GRAY 1
/* Interpolation type*/
#define INTERPOLATION 2
// 0 - Nearest Neighbor Interpolation
// 1 - Bilinear Interpolation
// 2 - AREA Interpolation

/* Input image Dimensions */
#define WIDTH 1920  // Maximum Input image width
#define HEIGHT 1080 // Maximum Input image height

/* Output image Dimensions */
#define NEWWIDTH 3840  // Maximum output image width
#define NEWHEIGHT 2160 // Maximum output image height
