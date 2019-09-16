#define FILTER_SIZE_3 0
#define FILTER_SIZE_5 1
#define FILTER_SIZE_7 0

#define RO 1
#define NO 0

#define T_8U 1  // Input type of 8U
#define T_16U 0 // Input type of 16U
#define T_16S 0 // Input type of 16S

#if FILTER_SIZE_3
#define FILTER_WIDTH 3
#elif FILTER_SIZE_5
#define FILTER_WIDTH 5
#elif FILTER_SIZE_7
#define FILTER_WIDTH 7
#endif

#define XF_USE_URAM false

#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 256
