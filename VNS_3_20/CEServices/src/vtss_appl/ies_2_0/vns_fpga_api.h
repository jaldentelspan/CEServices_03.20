/*
 * vns_fpga_api.h
 *
 *  Created on: Mar 21, 2013
 *      Author: Eric Lamphear
 *      Copyright: Telspan Data 2012-2013
 */

#ifndef VNS_FPGA_API_H_
#define VNS_FPGA_API_H_

#include "vns_types.h"
#include "main_types.h"     /* For vtss_init_data_t */
#include "vtss_module_id.h" /* For vtss_module_id_t */
#include "main.h"
#include "vns_registers.h"
#include "conf_xml_struct.h"
#include "ies_board_conf.h"
#include "time_input.h"
#include "ies_fpga_led.h"

/* int activate_epe(); */
int activate_epe_decoder(const vns_epe_conf_blk_t* epe_conf);
int activate_epe_encoder(const vns_epe_conf_blk_t* epe_conf);
void debug_time_input();

void clear_time_input_parameters();
void set_external_1pps_input();
void set_disable_time_input();
void set_gps_input( eGPSAntennaVoltage_t voltage );
int set_time_input_config( vns_time_input_src_t time_in_src );
void set_irig_dc_input( tsd_rx_irig_code_type_t type );
void set_irig_am_input( tsd_rx_irig_code_type_t type );

int Set_FPGA_IEEE_1588_Mode(vns_1588_type mode);
int Set_IEEE_1588_Mode_Mod_clk(vns_1588_type mode, BOOL modify_clock);
int Set_IEEE_1588_Mode(vns_1588_type mode);
int Get_IEEE_1588_Mode(vns_1588_type * mode);

void set_external_1pps_input();
vtss_rc vns_fpga_init(vtss_init_data_t *data);

int _GetRegister(vns_register_info reg_info, u32 * data);
int GetRegister(vns_registers_t reg, u32 * data);

int _SetRegister(vns_register_info reg_info, u32 data);
int SetRegister(vns_registers_t reg, u32 data);

/* int Set_TimeInConfig(vns_time_input time_in); */
int Set_TimeInConfig(vns_time_input_src_t time_in);
/* int Get_TimeInConfig(vns_time_input * time_in); */
int Get_TimeInConfig(vns_time_input_src_t * time_in);
/**
 * \brief Module initialization function
 */

void set_leds_debug(u32 led_red_blue, u32 led_green, u32 led_flashing, u32 discrete_out);
void init_led_subsystem();
// **** START Moved from vns_fpga_cli.c to access from both vns_fpga_web.c **********////
int GetSystemHealthReg(u32 * sys_health);
int GetDiscreteInHealthReg(u32 *di_health);
int GetDiscreteOutHealthReg(u32 *do_health);
int GetGPSHealthReg(u32 *gps_health);
int GetTimeInHealth(u32 *time_in_health);
/* Try to use GetTimeInHealth.  Not all health is located in the FPGA Register */
int GetTimeInHealthReg(u32 *time_in_health);
int GetTimeOutHealth(u32 *time_out_health);
int GetTimeOutHealthReg(u32 *time_out_health);
int GetPercentComplete(int * percent_complete);
// **** END Moved from vns_fpga_cli.c to access from both vns_fpga_web.c **********////

int Get_FPGAFirmwareVersionMajor(uint32_t * val);
int Get_FPGAFirmwareVersionMinor(uint32_t * val);

int Get_CurrentTimeInSource(vns_time_input_src_t * src);

void init_time_input_subsystem();

int Set_TimeOutTimeCode(vns_time_output_t tc);
int Get_TimeOutTimeCode(vns_time_output_t *tc);

eFrontPanelBoard_t get_front_panel_board();


int save_vns_config(void);//vns_fpga_conf_t config);

void delete_ptp_clock(void);

void create_ptp_clock(BOOL master);

vns_fpga_conf_t * get_shadow_config(void);


int Set_DiscreteInputConfig(discrete_in_config disc_in_cfg);
int Get_DiscreteInputConfig(discrete_in_config * disc_in_cfg);

int Set_DiscreteOutputConfig(discrete_out_config disc_out_cfg);
int Get_DiscreteOutputConfig(discrete_out_config * disc_out_cfg);

int Get_DiscreteInputState(u32 * di_state);

int Set_DiscreteOutputState(u32 do_state);
int Get_DiscreteOutputState(u32 * do_state);

int Set_TimeInCalibration(vns_time_in_cal time_in_cal);
int Get_TimeInCalibration(vns_time_in_cal * time_in_cal);

int Set_TimeInChannelId(u16 chid);
int Get_TimeInChannelId(u16 * chid);

int Set_TimeInSource(vns_time_input_src_t src);
int Get_TimeInSource(vns_time_input_src_t * src);

int Set_TimeInSignal(vns_time_input_signal_t sig);
int Get_TimeInSignal(vns_time_input_signal_t * sig);

int Set_TimeOutChannelId(u16 chid);
int Get_TimeOutChannelId(u16 * chid);


int Set_TimeOutCalibration(vns_time_out_cal time_out_cal);
int Get_TimeOutCalibration(vns_time_out_cal * time_out_cal);

int Set_TimeOutChannelId(u16 chid);
int Get_TimeOutChannelId(u16 * chid);

int Set_TimeOutMode(vns_time_output_mode_t mode);
int Get_TimeOutMode(vns_time_output_mode_t *mode);

int Set_Ttl1ppsMode(BOOL enable);
int Get_Ttl1ppsMode(BOOL *enable);

int Set_TimeOutConfig(vns_time_output time_out);
int Get_TimeOutConfig(vns_time_output * time_out);

int Start_TimeInput(void);
int Stop_TimeInput(void);

int Start_TimeOutput(void);
int Stop_TimeOutput(void);

vns_status_t GetStatus(void);

int Set_leds_and_discretes(u32 link, u32 activity, u32 speed);

int Get_TimeIn_Time(int *year, int *yday, int *hour, int *minute, int *second);

int Get_TimeOut_Time(int *year, int *yday, int *hour, int *minute, int *second);

int modify_input_time(ies_time_process_command_t cmd, u32 seconds);
int Get_PTP_SetTime(int *year, int *yday, int *hour, int *minute, int *second);

int set_1pps_time(int year, int yday, int hour, int minute, int second);
time_t FpgaTime_2_time_t(int year, int yday, int hour, int minute, int second);

void FPGA_Debug_Test_Mode(BOOL test_mode_on);
void FPGA_Debug_disable_timer(BOOL test_mode_on);

int Set_discretes_out(u32 value);

/* Remote Update Functions START*/
int Set_UpdateFactoryImg(u16 val);
int Get_UpdateFactoryImg(u16 * val);

int Set_UpdateLoadFactoryImg(u16 val);
int Get_UpdateLoadFactoryImg(u16 * val);

/*
 * Warning: chunk mode packet must be written to the register
 * there is no benefit right now; best kept disabled till additional functionality added
 *
 */
int Set_UpdateChunkModeEnable(u16 val);
int Get_UpdateChunkModeEnable(u16 * val);

int Set_UpdateData(unsigned int val);
int Get_UpdateData(unsigned int * val);

int Get_UpdateStatus(u16 * val);

int Get_IsUpdateFactoryImg(u16 * val);

int Get_IsUpdateDone(u16 * val);
int epe_config_disable_all(void);
int decoder_config_disable(void);
int encoder_config_disable(void);

int Get_IsUpdateEraseDone(u16 * val);

const char *fpga_firmware_status_get(void);

void fpga_firmware_status_set(const char *status);

typedef struct update_args
{
	FILE* file_ptr;
	unsigned char* buffer;
	u32 file_length;
	u32 version;

} update_args_t;
void* UpdateVNSFPGAFirmware(void* update_args  );
void start_fpga_upgrade( update_args_t* args);
char calc_checksum(void* file_ptr, u32 file_size);


/*
 * test to see if UpdateVNSFPGAFirmware has allocated memory upgrade
 * Used in conjuction with set_fpga_update_mode()
 */
int is_fpga_update_ready(void);
/*
 * verifies the file is valid with a signature, checksum of the header & file
 * it also point the file pointer to the correct loction of the update file, adjusts file length, and pulls the version from the header
 * Returns: 0 on success
 */
int fpga_firmware_check( update_args_t *args );

/* Remote Update Functions END*/

/* XML Config functions START */
/**
 * \brief Sets size value to the size of the modules configuration structure
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size will be set
 * \return TRUE when successful
 */
BOOL get_vns_fpga_master_conf_size(config_transport_t *conf);

/**
 * \brief Will duplicate the modules configuration structure in the provided buffer
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size will be used to verify buffer is large enough to hold buffer
 * 			*config conf->data.pointer.uc give the location of buffer the configuration structure can be copied to
 * \return TRUE when successful, FALSE if the buffer is not large enough
 */
BOOL get_vns_fpga_master_config(const config_transport_t *conf);

/**
 * \brief Sets the modules configuration structure restoring saved values
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size will be used to verify buffer is large enough to hold buffer
 * 		  	*config conf->data.pointer.uc give the location of buffer the configuration structure can be copied to
 * \return TRUE when successful, FALSE if the buffer is not the anticipated size or configuration block could not be accessed
 */
BOOL set_vns_fpga_master_config(const config_transport_t *conf);
/* XML Config functions END */

/* HDLC Config functions START */
/**
 * \brief Sets the configuration for EPE
 * \param Uses vns_epe_conf_blk_t
 *                      Setting Mode is a vns_epe_mode_t type and will enable HDLC, CH7, or disable
 * 		  	Setting bit rate is an unsigned long and will set to block boundary
 * \return TRUE when successful, FALSE if the buffer is not the anticipated size or configuration block could not be accessed
 */

BOOL set_vns_fpga_epe_conf(const vns_epe_conf_blk_t epe_conf);
BOOL set_vns_fpga_epe_conf_debug(const vns_epe_conf_blk_t epe_conf);
/**
 * \brief Gets the configuration for EPE
 * \param Uses vns_epe_conf_blk_t
 *                      Getting Mode is a vns_epe_mode_t type HDLC, CH7, or disable
 * 		  	Getting bit rate is an unsigned long and set to block boundary
 * \return TRUE when successful, FALSE if the buffer is not the anticipated size or configuration block could not be accessed
 */
BOOL get_vns_fpga_epe_decode_conf(vns_epe_conf_blk_t* epe_conf);
BOOL get_vns_fpga_epe_encode_conf(vns_epe_conf_blk_t* epe_conf);


/**
 * \brief verifies if the mac address is default
 * \returns true true if the MAC address is default 
 */
BOOL is_mac_default(void);

/**
 * \brief verifies if product is rev5 ies 12
 * \returns true if the the HARDWARE is rev 5 givin by the the feature bit
 */
BOOL is_epe_able(void);
BOOL is_epe_decoder_able(void);

/**
 * \brief verifies if product is capable of running irig as an input
 * \returns true if the the HARDWARE is rev 5 givin by the the feature bit
 */
BOOL is_irig_in_able(void);

BOOL is_1pps_in_able(void);

BOOL is_gps_locked(void);
/**
 * \brief verifies if product is pre rev5 ies 12
 * \returns true if the the HARDWARE is a previous rev 5 givin by the MAC address
 */
BOOL is_mac_ies12(void);

BOOL is_mac_ies6(void);

/* ************************************************** */
/* ** Functions below were built for defect 3641    * */
/* ** soft resets caused errors with Mutex          * */
/* ** this is to disable i2c writes during reset    * */
/* ************************************************** */
/**
 * \brief enables i2c reads and writes to the FPGA
 * \returns void
 */
void enable_i2c(void);
/**
 * \brief disables i2c reads and writes to the FPGA
 * \returns void
 */
void disable_i2c(void);
/**
 * \brief checks i2c read/write status
 * \returns 0 if i2c is enabled else i2c will be disabled
 */
int is_i2c_disabled(void);
void reset_leds_and_discretes(void);
char* get_fpga_update_invalid_string(void);
int set_board_id(u32 id);
int get_board_id(u32* id);
int set_epe_license(vns_fpga_epe_license_info_t license_info);
int get_epe_license(vns_fpga_epe_license_info_t* license_info);

#if false
vtss_rc vns_fpga_init(vtss_init_data_t *data);

int save_vns_config(void);//vns_fpga_conf_t config);

void delete_ptp_clock(void);

void create_ptp_clock(BOOL master);

vns_fpga_conf_t * get_shadow_config(void);


int Set_DiscreteInputConfig(discrete_in_config disc_in_cfg);
int Get_DiscreteInputConfig(discrete_in_config * disc_in_cfg);

int Set_DiscreteOutputConfig(discrete_out_config disc_out_cfg);
int Get_DiscreteOutputConfig(discrete_out_config * disc_out_cfg);

int Get_DiscreteInputState(u32 * di_state);
int Get_FPGAFirmwareVersionMajor(u8 * val);
int Get_FPGAFirmwareVersionMinor(u8 * val);

int Set_DiscreteOutputState(u32 do_state);
int Get_DiscreteOutputState(u32 * do_state);

int Set_TimeInCalibration(vns_time_in_cal time_in_cal);
int Get_TimeInCalibration(vns_time_in_cal * time_in_cal);

int Set_TimeInChannelId(u16 chid);
int Get_TimeInChannelId(u16 * chid);

int Set_TimeInSource(vns_time_input_src_t src);
int Get_TimeInSource(vns_time_input_src_t * src);

int Set_TimeInSignal(vns_time_input_signal_t sig);
int Get_TimeInSignal(vns_time_input_signal_t * sig);

int Set_TimeOutChannelId(u16 chid);
int Get_TimeOutChannelId(u16 * chid);

int Set_TimeInConfig(vns_time_input time_in);
int Get_TimeInConfig(vns_time_input * time_in);

int Set_TimeOutCalibration(vns_time_out_cal time_out_cal);
int Get_TimeOutCalibration(vns_time_out_cal * time_out_cal);

int Set_TimeOutChannelId(u16 chid);
int Get_TimeOutChannelId(u16 * chid);

int Set_TimeOutTimeCode(vns_time_output_t tc);
int Get_TimeOutTimeCode(vns_time_output_t *tc);

int Set_TimeOutMode(vns_time_output_mode_t mode);
int Get_TimeOutMode(vns_time_output_mode_t *mode);

int Set_Ttl1ppsMode(BOOL enable);
int Get_Ttl1ppsMode(BOOL *enable);

int Set_TimeOutConfig(vns_time_output time_out);
int Get_TimeOutConfig(vns_time_output * time_out);

int Start_TimeInput(void);
int Stop_TimeInput(void);

int Start_TimeOutput(void);
int Stop_TimeOutput(void);

vns_status_t GetStatus(void);

int Set_leds_and_discretes(u32 link, u32 activity, u32 speed);

int Get_TimeIn_Time(int *year, int *yday, int *hour, int *minute, int *second);

int Get_TimeOut_Time(int *year, int *yday, int *hour, int *minute, int *second);

int modify_input_time(ies_time_process_command_t cmd, u32 seconds);
int Get_PTP_SetTime(int *year, int *yday, int *hour, int *minute, int *second);

time_t FpgaTime_2_time_t(int year, int yday, int hour, int minute, int second);

void FPGA_Debug_Test_Mode(BOOL test_mode_on);
void FPGA_Debug_disable_timer(BOOL test_mode_on);

int Set_discretes_out(u32 value);
// **** START Moved from vns_fpga_cli.c to access from both vns_fpga_web.c **********////
int GetSystemHealthReg(u32 * sys_health);
int GetDiscreteInHealthReg(u32 *di_health);
int GetDiscreteOutHealthReg(u32 *do_health);
int GetGPSHealthReg(u32 *gps_health);
int GetTimeInHealth(u32 *time_in_health);
/* Try to use GetTimeInHealth.  Not all health is located in the FPGA Register */
int GetTimeInHealthReg(u32 *time_in_health);
int GetTimeOutHealth(u32 *time_out_health);
int GetTimeOutHealthReg(u32 *time_out_health);
int GetPercentComplete(int * percent_complete);
// **** END Moved from vns_fpga_cli.c to access from both vns_fpga_web.c **********////

/* Remote Update Functions START*/
int Set_UpdateFactoryImg(u16 val);
int Get_UpdateFactoryImg(u16 * val);

int Set_UpdateLoadFactoryImg(u16 val);
int Get_UpdateLoadFactoryImg(u16 * val);

/*
 * Warning: chunk mode packet must be written to the register
 * there is no benefit right now; best kept disabled till additional functionality added
 *
 */
int Set_UpdateChunkModeEnable(u16 val);
int Get_UpdateChunkModeEnable(u16 * val);

int Set_UpdateData(unsigned int val);
int Get_UpdateData(unsigned int * val);

int Get_UpdateStatus(u16 * val);

int Get_IsUpdateFactoryImg(u16 * val);

int Get_IsUpdateDone(u16 * val);

int Get_IsUpdateEraseDone(u16 * val);

const char *fpga_firmware_status_get(void);

void fpga_firmware_status_set(const char *status);



/*
 * test to see if UpdateVNSFPGAFirmware has allocated memory upgrade
 * Used in conjuction with set_fpga_update_mode()
 */
int is_fpga_update_ready(void);
/*
 * verifies the file is valid with a signature, checksum of the header & file
 * it also point the file pointer to the correct loction of the update file, adjusts file length, and pulls the version from the header
 * Returns: 0 on success
 */
int fpga_firmware_check( update_args_t *args );

/* Remote Update Functions END*/

/* XML Config functions START */
/**
 * \brief Sets size value to the size of the modules configuration structure
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size will be set
 * \return TRUE when successful
 */
BOOL get_vns_fpga_master_conf_size(config_transport_t *conf);

/**
 * \brief Will duplicate the modules configuration structure in the provided buffer
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size will be used to verify buffer is large enough to hold buffer
 * 			*config conf->data.pointer.uc give the location of buffer the configuration structure can be copied to
 * \return TRUE when successful, FALSE if the buffer is not large enough
 */
BOOL get_vns_fpga_master_config(const config_transport_t *conf);

/**
 * \brief Sets the modules configuration structure restoring saved values
 * \param Uses a pointer to the structure config_transport_t
 * 			*config conf->data.size will be used to verify buffer is large enough to hold buffer
 * 		  	*config conf->data.pointer.uc give the location of buffer the configuration structure can be copied to
 * \return TRUE when successful, FALSE if the buffer is not the anticipated size or configuration block could not be accessed
 */
BOOL set_vns_fpga_master_config(const config_transport_t *conf);
/* XML Config functions END */

/* HDLC Config functions START */
/**
 * \brief Sets the configuration for EPE
 * \param Uses vns_epe_conf_blk_t
 *                      Setting Mode is a vns_epe_mode_t type and will enable HDLC, CH7, or disable
 * 		  	Setting bit rate is an unsigned long and will set to block boundary
 * \return TRUE when successful, FALSE if the buffer is not the anticipated size or configuration block could not be accessed
 */

BOOL set_vns_fpga_epe_conf(const vns_epe_conf_blk_t epe_conf);
BOOL set_vns_fpga_epe_conf_debug(const vns_epe_conf_blk_t epe_conf);
/**
 * \brief Gets the configuration for EPE
 * \param Uses vns_epe_conf_blk_t
 *                      Getting Mode is a vns_epe_mode_t type HDLC, CH7, or disable
 * 		  	Getting bit rate is an unsigned long and set to block boundary
 * \return TRUE when successful, FALSE if the buffer is not the anticipated size or configuration block could not be accessed
 */
BOOL get_vns_fpga_epe_conf(vns_epe_conf_blk_t* epe_conf);


/**
 * \brief verifies if the mac address is default
 * \returns true true if the MAC address is default 
 */
BOOL is_mac_default(void);

/**
 * \brief verifies if product is rev5 ies 12
 * \returns true if the the HARDWARE is rev 5 givin by the the feature bit
 */
BOOL is_epe_able(void);
BOOL is_epe_decoder_able(void);

/**
 * \brief verifies if product is capable of running irig as an input
 * \returns true if the the HARDWARE is rev 5 givin by the the feature bit
 */
BOOL is_irig_in_able(void);

BOOL is_1pps_in_able(void);

/**
 * \brief verifies if product is pre rev5 ies 12
 * \returns true if the the HARDWARE is a previous rev 5 givin by the MAC address
 */
BOOL is_mac_ies12(void);

BOOL is_mac_ies6(void);
/* ************************************************** */
/* ** Functions below were built for defect 3641    * */
/* ** soft resets caused errors with Mutex          * */
/* ** this is to disable i2c writes during reset    * */
/* ************************************************** */
/**
 * \brief enables i2c reads and writes to the FPGA
 * \returns void
 */
void enable_i2c(void);
/**
 * \brief disables i2c reads and writes to the FPGA
 * \returns void
 */
void disable_i2c(void);
/**
 * \brief checks i2c read/write status
 * \returns 0 if i2c is enabled else i2c will be disabled
 */
int is_i2c_disabled(void);
void reset_leds_and_discretes(void);
char* get_fpga_update_invalid_string(void);
int set_board_id(u32 id);
int get_board_id(u32* id);
int set_epe_license(vns_fpga_epe_license_info_t license_info);
int get_epe_license(vns_fpga_epe_license_info_t* license_info);
/**
 * \brief Gets and Sets for board revisions
 * \returns error code 
 */
int set_board_revision(flash_conf_tags_t board, uint rev);
int get_board_revision(flash_conf_tags_t board, uint* rev);
/* int get_board_revision_info(ies_board_id_t board, ies_board_rev_info_t* info); */

int Set_IEEE_1588_Mode_Mod_clk(vns_1588_type mode, BOOL modify_clock);
int Set_IEEE_1588_Mode(vns_1588_type mode);
int Get_IEEE_1588_Mode(vns_1588_type * mode);


BOOL is_epe_license_valid(vns_fpga_epe_license_info_t* license_info);
BOOL is_epe_license_acitive(void);
int Time_t_2_Vns_bcd_time(time_t t, vns_bcd_time * bcd);

#define SYSTEM_BUS_WIDTH (32)


uint32_t IORD(uint32_t base, uint32_t offset);
void IOWR(uint32_t base, uint32_t offset, uint32_t val);

uint32_t IORD_32DIRECT(uint32_t base, uint32_t offset);
void IOWR_32DIRECT(uint32_t base, uint32_t offset, uint32_t data);
#endif


#endif /* VNS_FPGA_API_H_ */
