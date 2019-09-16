/* Optimization type */

#define RO 0 // Resource Optimized (8-pixel implementation)
#define NO 1 // Normal Operation (1-pixel implementation)

/* Conversion Type*/

#define XF_CONVERT16UTO8U 0  // set to convert bit depth from unsigned 16-bit to unsigned 8-bit
#define XF_CONVERT16STO8U 0  // set to convert bit depth from signed   16-bit to unsigned 8-bit
#define XF_CONVERT32STO8U 0  // set to convert bit depth from signed   32-bit to unsigned 8-bit
#define XF_CONVERT32STO16U 0 // set to convert bit depth from signed   32-bit to unsigned 16-bit
#define XF_CONVERT32STO16S 0 // set to convert bit depth from signed   32-bit to signed   16-bit
#define XF_CONVERT8UTO16U 0  // set to convert bit depth from unsigned 8-bit  to 16-bit unsigned
#define XF_CONVERT8UTO16S 0  // set to convert bit depth from unsigned 8-bit  to 16-bit signed
#define XF_CONVERT8UTO32S 0  // set to convert bit depth from unsigned 8-bit  to 32-bit unsigned
#define XF_CONVERT16UTO32S 0 // set to convert bit depth from unsigned 16-bit to 32-bit signed
#define XF_CONVERT16STO32S 1 // set to convert bit depth from signed   16-bit to 32-bit signed
