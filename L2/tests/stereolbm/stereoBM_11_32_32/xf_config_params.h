/* SAD window size must be an odd number and it must be less than minimum of image height and width and less than the
 * tested size '21' */
#define SAD_WINDOW_SIZE 11

/* NO_OF_DISPARITIES must be greater than '0' and less than the image width */
#define NO_OF_DISPARITIES 32

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 32

#define XF_USE_URAM false
