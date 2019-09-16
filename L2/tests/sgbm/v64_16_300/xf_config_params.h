/* set the height and width */
#define HEIGHT 1080
#define WIDTH 1920

/* set penalties for SGM */
#define SMALL_PENALTY 20
#define LARGE_PENALTY 40

/* Census transform window size */
#define WINDOW_SIZE 5

/* NO_OF_DISPARITIES must be greater than '0' and less than the image width */
#define TOTAL_DISPARITY 64

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 32

/* Number of directions */
#define NUM_DIR 4
