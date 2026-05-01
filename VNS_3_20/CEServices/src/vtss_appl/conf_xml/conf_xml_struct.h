/*
 * conf_xml_struct.h
 *
 *  Created on: Mar 8, 2017
 *      Author: jalden
 */

#ifndef VTSS_APPL_CONF_XML_CONF_XML_STRUCT_H_
#define VTSS_APPL_CONF_XML_CONF_XML_STRUCT_H_


typedef struct buffer_info {
	union data_location {
		char * c;
		unsigned char* uc;
		void* v;
	}pointer;
	size_t size;
}buffer_info_t;

typedef struct config_transport{
	buffer_info_t data;
	buffer_info_t hex_string;
}config_transport_t;


#endif /* VTSS_APPL_CONF_XML_CONF_XML_STRUCT_H_ */
