/*  set the input types  */
#define T_8U 1  // Input type of 8U
#define T_16S 0 // Input type of 16S

/*  set the optimisation type  */
#define NO 1 // Normal Operation
#define RO 0 // Resource Optimized

#define GRAY 1

#define ARRAY 0
#define SCALAR 1
// macros for accel
#define FUNCT_NUM 15
//#define EXTRA_ARG  0.05
#define EXTRA_PARM XF_CMP_LE

// OpenCV reference macros
#define CV_FUNCT_NAME compare
#define CV_EXTRA_ARG CV_CMP_LE
#define FUNCT_COMPARE
