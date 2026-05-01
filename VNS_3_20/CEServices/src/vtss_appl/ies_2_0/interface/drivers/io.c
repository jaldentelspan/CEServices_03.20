#include "io.h"
#include "vns_fpga_api.h"

u32 IORD(u32 base, u32 offset)
{
    u32 reg_addr = base + offset*4;
    int retval = 0;
    u32 reg_val;
    vns_register_info reg_info;
    reg_info.regnum = reg_addr;
    reg_info.mask = 0xFFFFFFFF;
    reg_info.shift = 0;

    retval = _GetRegister(reg_info, &reg_val);
    if(retval != 0)
    {
        return 0xDEADBEEF;
    }
    return reg_val;
}
void IOWR(u32 base, u32 offset, u32 val)
{
    u32 reg_addr = base + offset*4;
    int retval = 0;
    vns_register_info reg_info;
    reg_info.regnum = reg_addr;
    reg_info.mask = 0xFFFFFFFF;
    reg_info.shift = 0;

    retval = _SetRegister(reg_info, val);
}

u32 IORD_32DIRECT(u32 base, u32 offset)
{
    return IORD( base,  offset);
}
void IOWR_32DIRECT(u32 base, u32 offset, u32 data)
{
    IOWR( base,  offset, data);
}

