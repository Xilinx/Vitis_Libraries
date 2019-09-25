#ifndef __KERNEL_CONFIG_HPP_
#define __KERNEL_CONFIG_HPP_

// SUB_GRP_SZ should be dividable by (512 / 32)
// Total Channel number = SUB_GRP_SZ * GRP_NM

#define SUB_GRP_SZ 16
#define GRP_NM 1
#define CH_NM (SUB_GRP_SZ * GRP_NM)
#define BURST_LEN 128

#endif
