/*
 *
 *  tsd_pcm.h
 *
 * Created on: 2020-12-08 13:16
 *     Author: jalden
 *
 */


#ifndef __TSD_PCM_H__
#define __TSD_PCM_H__

#include <io.h>
#include "tsd_pcm_reg.h"


#define TSD_PCM_DRIVER_VERSION (0.01)



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



///////////////////////////////////////////////////////////////////////////
//// Function Prototypes
///////////////////////////////////////////////////////////////////////////

int get_tsd_pcm_ctrl_encode_ch7_frame_size( uint32_t base, uint32_t *encode_ch7_frame_size );
int set_tsd_pcm_ctrl_encode_ch7_frame_size( uint32_t base, uint32_t encode_ch7_frame_size );

int get_tsd_pcm_ctrl_encode_hdlc_enable( uint32_t base, uint32_t *encode_hdlc_enable );
int set_tsd_pcm_ctrl_encode_hdlc_enable( uint32_t base, uint32_t encode_hdlc_enable );

int get_tsd_pcm_ctrl_hdlc_send_addr( uint32_t base, uint32_t *hdlc_send_addr );
int set_tsd_pcm_ctrl_hdlc_send_addr( uint32_t base, uint32_t hdlc_send_addr );

int get_tsd_pcm_ctrl_hdlc_ch7_mux( uint32_t base, uint32_t *hdlc_ch7_mux );
int set_tsd_pcm_ctrl_hdlc_ch7_mux( uint32_t base, uint32_t hdlc_ch7_mux );

int get_tsd_pcm_ctrl_encode_ch7_enable( uint32_t base, uint32_t *encode_ch7_enable );
int set_tsd_pcm_ctrl_encode_ch7_enable( uint32_t base, uint32_t encode_ch7_enable );

int get_tsd_pcm_ctrl_hdlc_addr( uint32_t base, uint32_t *hdlc_addr );
int set_tsd_pcm_ctrl_hdlc_addr( uint32_t base, uint32_t hdlc_addr );

int get_tsd_pcm_ctrl_hdlc_ctrl( uint32_t base, uint32_t *hdlc_ctrl );
int set_tsd_pcm_ctrl_hdlc_ctrl( uint32_t base, uint32_t hdlc_ctrl );

int get_tsd_pcm_ctrl_decode_ch7_frame_size( uint32_t base, uint32_t *decode_ch7_frame_size );
int set_tsd_pcm_ctrl_decode_ch7_frame_size( uint32_t base, uint32_t decode_ch7_frame_size );

int get_tsd_pcm_ctrl_decode_hdlc_enable( uint32_t base, uint32_t *decode_hdlc_enable );
int set_tsd_pcm_ctrl_decode_hdlc_enable( uint32_t base, uint32_t decode_hdlc_enable );

int get_tsd_pcm_ctrl_decode_ch7_enable( uint32_t base, uint32_t *decode_ch7_enable );
int set_tsd_pcm_ctrl_decode_ch7_enable( uint32_t base, uint32_t decode_ch7_enable );

int get_tsd_pcm_pll_ctrl_addr( uint32_t base, uint32_t *addr );
int set_tsd_pcm_pll_ctrl_addr( uint32_t base, uint32_t addr );

int get_tsd_pcm_pll_ctrl_read_bit( uint32_t base, uint32_t *read_bit );
int set_tsd_pcm_pll_ctrl_read_bit( uint32_t base, uint32_t read_bit );

int get_tsd_pcm_pll_ctrl_write_bit( uint32_t base, uint32_t *write_bit );
int set_tsd_pcm_pll_ctrl_write_bit( uint32_t base, uint32_t write_bit );

int get_tsd_pcm_pll_write_data_reg( uint32_t base, uint32_t *reg );
int set_tsd_pcm_pll_write_data_reg( uint32_t base, uint32_t reg );

int get_tsd_pcm_pll_read_data_reg( uint32_t base, uint32_t *reg );




#ifdef __cplusplus
}
#endif /* __cplusplus */




#endif /* __TSD_PCM_H__ */
