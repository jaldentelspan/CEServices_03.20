/*
 *
 *  tsd_pcm.c
 *
 * Created on: 2020-12-08 13:16
 *     Author: jalden
 *
 */

#include "tsd_register_common.h"
#include "tsd_pcm.h"

///////////////////////////////////////////////////////////////////////////
//// Function Definitions
///////////////////////////////////////////////////////////////////////////

int get_tsd_pcm_ctrl_encode_ch7_frame_size( uint32_t base, uint32_t *encode_ch7_frame_size )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_ENCODE_CH7_FRAME_SIZE_MASK,
        TSD_PCM_CTRL_ENCODE_CH7_FRAME_SIZE_SHFT,
        encode_ch7_frame_size );
}

int set_tsd_pcm_ctrl_encode_ch7_frame_size( uint32_t base, uint32_t encode_ch7_frame_size )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_ENCODE_CH7_FRAME_SIZE_MASK,
        TSD_PCM_CTRL_ENCODE_CH7_FRAME_SIZE_SHFT,
        encode_ch7_frame_size );
}


int get_tsd_pcm_ctrl_encode_hdlc_enable( uint32_t base, uint32_t *encode_hdlc_enable )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_ENCODE_HDLC_ENABLE_MASK,
        TSD_PCM_CTRL_ENCODE_HDLC_ENABLE_SHFT,
        encode_hdlc_enable );
}

int set_tsd_pcm_ctrl_encode_hdlc_enable( uint32_t base, uint32_t encode_hdlc_enable )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_ENCODE_HDLC_ENABLE_MASK,
        TSD_PCM_CTRL_ENCODE_HDLC_ENABLE_SHFT,
        encode_hdlc_enable );
}


int get_tsd_pcm_ctrl_hdlc_send_addr( uint32_t base, uint32_t *hdlc_send_addr )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_SEND_ADDR_MASK,
        TSD_PCM_CTRL_HDLC_SEND_ADDR_SHFT,
        hdlc_send_addr );
}

int set_tsd_pcm_ctrl_hdlc_send_addr( uint32_t base, uint32_t hdlc_send_addr )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_SEND_ADDR_MASK,
        TSD_PCM_CTRL_HDLC_SEND_ADDR_SHFT,
        hdlc_send_addr );
}


int get_tsd_pcm_ctrl_hdlc_ch7_mux( uint32_t base, uint32_t *hdlc_ch7_mux )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_CH7_MUX_MASK,
        TSD_PCM_CTRL_HDLC_CH7_MUX_SHFT,
        hdlc_ch7_mux );
}

int set_tsd_pcm_ctrl_hdlc_ch7_mux( uint32_t base, uint32_t hdlc_ch7_mux )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_CH7_MUX_MASK,
        TSD_PCM_CTRL_HDLC_CH7_MUX_SHFT,
        hdlc_ch7_mux );
}


int get_tsd_pcm_ctrl_encode_ch7_enable( uint32_t base, uint32_t *encode_ch7_enable )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_ENCODE_CH7_ENABLE_MASK,
        TSD_PCM_CTRL_ENCODE_CH7_ENABLE_SHFT,
        encode_ch7_enable );
}

int set_tsd_pcm_ctrl_encode_ch7_enable( uint32_t base, uint32_t encode_ch7_enable )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_ENCODE_CH7_ENABLE_MASK,
        TSD_PCM_CTRL_ENCODE_CH7_ENABLE_SHFT,
        encode_ch7_enable );
}


int get_tsd_pcm_ctrl_hdlc_addr( uint32_t base, uint32_t *hdlc_addr )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_ADDR_MASK,
        TSD_PCM_CTRL_HDLC_ADDR_SHFT,
        hdlc_addr );
}

int set_tsd_pcm_ctrl_hdlc_addr( uint32_t base, uint32_t hdlc_addr )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_ADDR_MASK,
        TSD_PCM_CTRL_HDLC_ADDR_SHFT,
        hdlc_addr );
}


int get_tsd_pcm_ctrl_hdlc_ctrl( uint32_t base, uint32_t *hdlc_ctrl )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_CTRL_MASK,
        TSD_PCM_CTRL_HDLC_CTRL_SHFT,
        hdlc_ctrl );
}

int set_tsd_pcm_ctrl_hdlc_ctrl( uint32_t base, uint32_t hdlc_ctrl )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_HDLC_CTRL_MASK,
        TSD_PCM_CTRL_HDLC_CTRL_SHFT,
        hdlc_ctrl );
}


int get_tsd_pcm_ctrl_decode_ch7_frame_size( uint32_t base, uint32_t *decode_ch7_frame_size )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_DECODE_CH7_FRAME_SIZE_MASK,
        TSD_PCM_CTRL_DECODE_CH7_FRAME_SIZE_SHFT,
        decode_ch7_frame_size );
}

int set_tsd_pcm_ctrl_decode_ch7_frame_size( uint32_t base, uint32_t decode_ch7_frame_size )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_DECODE_CH7_FRAME_SIZE_MASK,
        TSD_PCM_CTRL_DECODE_CH7_FRAME_SIZE_SHFT,
        decode_ch7_frame_size );
}


int get_tsd_pcm_ctrl_decode_hdlc_enable( uint32_t base, uint32_t *decode_hdlc_enable )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_DECODE_HDLC_ENABLE_MASK,
        TSD_PCM_CTRL_DECODE_HDLC_ENABLE_SHFT,
        decode_hdlc_enable );
}

int set_tsd_pcm_ctrl_decode_hdlc_enable( uint32_t base, uint32_t decode_hdlc_enable )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_DECODE_HDLC_ENABLE_MASK,
        TSD_PCM_CTRL_DECODE_HDLC_ENABLE_SHFT,
        decode_hdlc_enable );
}


int get_tsd_pcm_ctrl_decode_ch7_enable( uint32_t base, uint32_t *decode_ch7_enable )
{
    return get_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_DECODE_CH7_ENABLE_MASK,
        TSD_PCM_CTRL_DECODE_CH7_ENABLE_SHFT,
        decode_ch7_enable );
}

int set_tsd_pcm_ctrl_decode_ch7_enable( uint32_t base, uint32_t decode_ch7_enable )
{
    return set_register_region(base,
        TSD_PCM_CTRL_REG,
        TSD_PCM_CTRL_DECODE_CH7_ENABLE_MASK,
        TSD_PCM_CTRL_DECODE_CH7_ENABLE_SHFT,
        decode_ch7_enable );
}


int get_tsd_pcm_pll_ctrl_addr( uint32_t base, uint32_t *addr )
{
    return get_register_region(base,
        TSD_PCM_PLL_CTRL_REG,
        TSD_PCM_PLL_CTRL_ADDR_MASK,
        TSD_PCM_PLL_CTRL_ADDR_SHFT,
        addr );
}

int set_tsd_pcm_pll_ctrl_addr( uint32_t base, uint32_t addr )
{
    return set_register_region(base,
        TSD_PCM_PLL_CTRL_REG,
        TSD_PCM_PLL_CTRL_ADDR_MASK,
        TSD_PCM_PLL_CTRL_ADDR_SHFT,
        addr );
}


int get_tsd_pcm_pll_ctrl_read_bit( uint32_t base, uint32_t *read_bit )
{
    return get_register_region(base,
        TSD_PCM_PLL_CTRL_REG,
        TSD_PCM_PLL_CTRL_READ_BIT_MASK,
        TSD_PCM_PLL_CTRL_READ_BIT_SHFT,
        read_bit );
}

int set_tsd_pcm_pll_ctrl_read_bit( uint32_t base, uint32_t read_bit )
{
    return set_register_region(base,
        TSD_PCM_PLL_CTRL_REG,
        TSD_PCM_PLL_CTRL_READ_BIT_MASK,
        TSD_PCM_PLL_CTRL_READ_BIT_SHFT,
        read_bit );
}


int get_tsd_pcm_pll_ctrl_write_bit( uint32_t base, uint32_t *write_bit )
{
    return get_register_region(base,
        TSD_PCM_PLL_CTRL_REG,
        TSD_PCM_PLL_CTRL_WRITE_BIT_MASK,
        TSD_PCM_PLL_CTRL_WRITE_BIT_SHFT,
        write_bit );
}

int set_tsd_pcm_pll_ctrl_write_bit( uint32_t base, uint32_t write_bit )
{
    return set_register_region(base,
        TSD_PCM_PLL_CTRL_REG,
        TSD_PCM_PLL_CTRL_WRITE_BIT_MASK,
        TSD_PCM_PLL_CTRL_WRITE_BIT_SHFT,
        write_bit );
}


int get_tsd_pcm_pll_write_data_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_PCM_PLL_WRITE_DATA_REG,
        TSD_PCM_PLL_WRITE_DATA_REG_MASK,
        TSD_PCM_PLL_WRITE_DATA_REG_SHFT,
        reg );
}

int set_tsd_pcm_pll_write_data_reg( uint32_t base, uint32_t reg )
{
    return set_register_region(base,
        TSD_PCM_PLL_WRITE_DATA_REG,
        TSD_PCM_PLL_WRITE_DATA_REG_MASK,
        TSD_PCM_PLL_WRITE_DATA_REG_SHFT,
        reg );
}


int get_tsd_pcm_pll_read_data_reg( uint32_t base, uint32_t *reg )
{
    return get_register_region(base,
        TSD_PCM_PLL_READ_DATA_REG,
        TSD_PCM_PLL_READ_DATA_REG_MASK,
        TSD_PCM_PLL_READ_DATA_REG_SHFT,
        reg );
}


