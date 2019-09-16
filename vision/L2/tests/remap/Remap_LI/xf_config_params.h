// Configure this based on the number of rows needed for the remap purpose
// e.g., If its a right to left flip two rows are enough
#define XF_WIN_ROWS 8

#define GRAY 1
#define RGB 0

// The type of interpolation, define "XF_REMAP_INTERPOLATION" as either "XF_INTERPOLATION_NN" or
// "XF_INTERPOLATION_BILINEAR"
#define XF_REMAP_INTERPOLATION XF_INTERPOLATION_BILINEAR

#define XF_USE_URAM false
