#ifndef _IES_FPGA_IO_H_
#define _IES_FPGA_IO_H_
/* #include <cstdint> */
#include "main_types.h"

/* #define alt_u8 uchar */
/* typedef uint8_t alt_u8 ; */
#define SYSTEM_BUS_WIDTH (32)


u32 IORD(u32 base, u32 offset);
void IOWR(u32 base, u32 offset, u32 val);



u32 IORD_32DIRECT(u32 base, u32 offset);
void IOWR_32DIRECT(u32 base, u32 offset, u32 data);

/* Native bus access functions */

#define __IO_CALC_ADDRESS_NATIVE(BASE, REGNUM) \
  ((void *)(((alt_u8*)BASE) + ((REGNUM) * (SYSTEM_BUS_WIDTH/8))))

// // #define IORD(BASE, REGNUM) \
// //   __builtin_ldwio (__IO_CALC_ADDRESS_NATIVE ((BASE), (REGNUM)))
// // #define IOWR(BASE, REGNUM, DATA) \
// //   __builtin_stwio (__IO_CALC_ADDRESS_NATIVE ((BASE), (REGNUM)), (DATA))
// // 
// // 
#endif /* _IES_FPGA_IO_H_ */
