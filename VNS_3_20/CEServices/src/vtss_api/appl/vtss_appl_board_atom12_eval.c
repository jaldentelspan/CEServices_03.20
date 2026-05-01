/*

 Vitesse API software.

 Copyright (c) 2002-2012 Vitesse Semiconductor Corporation "Vitesse". All
 Rights Reserved.

 Unpublished rights reserved under the copyright laws of the United States of
 America, other countries and international treaties. Permission to use, copy,
 store and modify, the software and its source code is granted. Permission to
 integrate into other products, disclose, transmit and distribute the software
 in an absolute machine readable format (e.g. HEX file) is also granted.  The
 source code of the software may not be disclosed, transmitted or distributed
 without the written permission of Vitesse. The software and its source code
 may only be used in products utilizing the Vitesse switch products.

 This copyright notice must appear in any copy, modification, disclosure,
 transmission or distribution of the software. Vitesse retains all ownership,
 copyright, trade secret and proprietary rights in the software.

 THIS SOFTWARE HAS BEEN PROVIDED "AS IS," WITHOUT EXPRESS OR IMPLIED WARRANTY
 INCLUDING, WITHOUT LIMITATION, IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR USE AND NON-INFRINGEMENT.
 

*/


//***************************************************************************
//* This file contains board specific functions needed for running the PHY  *
//* API at a Atom12 evaluation board. The evaluation board is equipped with *
//* a Rabbit CPU board, which do the communication with the PHY using a     *
//* socket connection. The actual API is running on the host computer. The  *
//* API has been tested with both Linux (Red Hat) and Cygwin.               *
//***************************************************************************



#include <netdb.h>  // For socket
#include <stdarg.h> // For va_list


#include "vtss_api.h"   // For BOOL and friends
#include "vtss_appl.h"  // For board types


// Fixed socket port for the CPU board used 
#define CPU_BOARD_PORT "23" 

// Define which trace group to use for VTSS printout in this file
#define VTSS_TRACE_GROUP VTSS_TRACE_GROUP_PHY


/* ================================================================= *
 *  Misc. functions 
 * ================================================================= */

// Function defining the port interface.
vtss_port_interface_t port_interface(vtss_port_no_t port_no)
{
    return VTSS_PORT_INTERFACE_SGMII;
}

/* ================================================================= *
 *  Debug trace
 * ================================================================= */

vtss_trace_conf_t vtss_appl_trace_conf = {
    .level = VTSS_TRACE_LEVEL_ERROR
};


// Trace callout function - This function is called for printing out debug information from the API
// Different levels of trace are support. The level are :
// 1) VTSS_TRACE_LEVEL_NONE   - No information from the API will be printed
// 2) VTSS_TRACE_LEVEL_ERROR  - Printout of T_E/VTSS_E trace. Error messages for malfunctioning API
// 3) VTSS_TRACE_LEVEL_WARNING- Printout of T_W/VTSS_W trace. Warning messages for unexpected API behavior.
// 4) VTSS_TRACE_LEVEL_INFO   - Printout of T_I/VTSS_I trace. Debug messages.
// 5) VTSS_TRACE_LEVEL_DEBUG  - Printout of T_D/VTSS_D trace. Even more debug messages.
// 6) VTSS_TRACE_LEVEL_NOISE  - Printout of T_N/VTSS_N trace. Even more debug messages.
void vtss_callout_trace_printf(const vtss_trace_group_t group,
                               const vtss_trace_level_t level,
                               const char *file,
                               const int line,
                               const char *function,
                               const char *format,
                               ...)
{
    va_list va;
    printf("Lvl:%s file:%s func:%s line:%d - ", 
               level == VTSS_TRACE_LEVEL_ERROR ? "Error" :
               level == VTSS_TRACE_LEVEL_INFO ? "Info " :
               level == VTSS_TRACE_LEVEL_DEBUG ? "Debug" :
               level == VTSS_TRACE_LEVEL_NOISE ? "Noise" : "?????",
               file,
               function,
               line);

    va_start(va, format);
    vprintf(format, va);
    va_end(va);
    printf("\n");
}


/* ================================================================= *
 *  Board specific functions
 * ================================================================= */

int sockfd;

// Function for doing write access to the board CPU board via socket
// In :  Buffer - Pointer to the text to send over the socket
static void socket_write (char *buffer) {
    int n;
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
        T_E("ERROR writing to socket");
    } else {
        T_N("TX: %s\n", buffer);
    }
}


// Function for do read access from the board CPU board via socket
// In :  Buffer - Pointer for the data read via the socket.
static void socket_read (char *buffer) {
    
    int n;
    VTSS_MSLEEP(100);
    n = read(sockfd, buffer, 255);
    if (n < 0) 
        T_E("ERROR reading from socket");
}

// Function for initializing the socket connection to the CPU board.
// In : server_addr - IP address for the CPU board 
//      port        - Port used for the socket connection
static void socket_init (const char *server_addr, const char *port) {
    struct hostent *server;
    struct sockaddr_in serv_addr;
    
    int portno = atoi(port);


    server = gethostbyname(server_addr);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        T_E("ERROR opening socket");
    

    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);


    if (connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        T_E("ERROR connecting");
}




// Each board can have it own way of communicating with the chip. The miim read and write function are called by the API 
// when the API needs to do register access.


// Miim read access specific for this board.
// In : port_no - The port to access.
//      addr    - The address to access
//
// In/Out:  value   - Pointer to the value to be returned
vtss_rc miim_read(const vtss_port_no_t port_no,
                  const u8             addr,
                  u16                  *const value)
{

    char * pch;
    char *endptr;
    char buffer[255];
    char scan_buf1[255];
    char scan_buf[255];
    int i;

    char last[100], first[100], middle[100];

    sprintf(buffer, "MIIM READ %d %d \n", port_no, addr);
    socket_write(&buffer[0]);

    strcpy(&buffer[0], "");
    socket_read(&buffer[0]);
    

    const char *ptr = buffer;
    char field [255];
    int n;

    while ( sscanf(ptr, "%255[^0x]%n", field, &n) == 1 )
    {
        ptr += n; /* advance the pointer by the number of characters read */
        ++ptr; /* skip the delimiter */
    }
    ptr--;
    sscanf(ptr, "%s", field);

    pch = strtok (buffer, "\n"); // Skip the miim read 
    pch = strtok (NULL, "\n"); 
    *value = strtol(field, &endptr, 16);
    
    T_I("miim read port_no = %d, addr = %d, value = 0x%X, pch = %s", port_no, addr ,value, pch);
    return VTSS_RC_OK;
}

// Miim write access specific for this board.
// In : port_no - The port to access.
//      addr    - The address to access
///     value   - The value to written 
vtss_rc miim_write(const vtss_port_no_t port_no,
                   const u8             addr,
                   const u16            value)
{


    char buffer[255];
    sprintf(buffer, "MIIM WRITE %d %d %d\r", port_no, addr, value);
    T_I("miim_write port_no = %d, addr = %d, value = 0x%X", port_no, addr ,value);
    T_I("Buffer = %s", &buffer[0]);
    socket_write(&buffer[0]);
    socket_read(&buffer[0]);
    return VTSS_RC_OK;
}

#if defined(VTSS_CHIP_10G_PHY)
vtss_rc mmd_read(const vtss_port_no_t  port_no,
                        const u8              mmd,
                        u16                   addr,
                        u16                   *const value)
{
    /* Must be filled out by the user */
    T_I("mmd_read port_no = %d, mmd = %d addr = %d, value = 0x%X", port_no, mmd, addr);
    return VTSS_RC_OK;
}

vtss_rc mmd_write(const vtss_port_no_t  port_no,
                         const u8              mmd,
                         u16                   addr,
                         u16                   data)
{
    /* Must be filled out by the user */
    T_I("mmd_write port_no = %d, mmd = %d addr = %d, value = 0x%X", port_no, addr, data);
    return VTSS_RC_OK;
}
#endif /* VTSS_CHIP_10G_PHY */





// The Vitesse eval board has a CPLD that needs to be initialized. This is done with the CPLD function.

// Function for writing the CPLD.
// In : data - The data to be written to the CPLD
static void cpld_write(char * data) {
    char buffer[255];
    socket_write("spi select 1 \n\r");
    socket_write("spi gw ");
    socket_write(data);
    socket_write("\n");
    socket_read(&buffer[0]);
}

// Function for reading the CPLD.
static void cpld_read(char * addr) {
    char buffer[255];
    socket_write("spi select 1 \n \r");
    socket_write("spi grw ");
    socket_write(addr);
    socket_write("\n");
    socket_read(&buffer[0]);
}



// Function for initializing the hardware board.
int board_init(int argc, const char **argv, vtss_board_t *board)
{
    char buffer[255];
    board->port_count = VTSS_PORTS; //Setup the number of port used 

    board->port_interface = port_interface; // Define the port interface

    board->init.init_conf->miim_read =  miim_read; // Set pointer to the MIIM read function for this board.
    board->init.init_conf->miim_write = miim_write; // Set pointer to the MIIM write function for this board.
#if defined(VTSS_CHIP_10G_PHY)
    board->init.init_conf->mmd_read =  mmd_read; // Set pointer to the MIIM read function for this board.
    board->init.init_conf->mmd_write = mmd_write; // Set pointer to the MIIM write function for this board.
#endif /* VTSS_CHIP_10G_PHY */


    if (argc != 2) {
        printf("Usage  : %s <Rabbit IP Address> \n"  , argv[0]);
        printf("Example: %s 10.10.132.59 \n"  , argv[0]);
        exit(EXIT_SUCCESS);
    }

    socket_init(argv[1], CPU_BOARD_PORT); // Connect to the CPU board
    
    socket_read(&buffer[0]); // Clear the welcome message
    
    // Do the CPLD initialization
    cpld_write("80000200000141");
    cpld_write("80000200000140");
    cpld_write("80000300000000");
    cpld_write("80000300000001");
    cpld_read("000003 4");

    return 0;
}



/* ================================================================= *
 *  API lock/unlock functions - If the Application support threads 
 *  the API must be protected via Mutexes  
 * ================================================================= */
BOOL api_locked = FALSE;

// Function call by the API, when the API shall do mutex lock.
void vtss_callout_lock(const char *function)
{
    // For testing we don't get a deadlock. The API must be unlocked before "locking"
    if (api_locked) {
        T_E("API lock problem");
    }
    api_locked = TRUE;
}

// Function call by the API, when the API shall do mutex unlock.
void vtss_callout_unlock(const char *function)
{
    // For testing we don't get a deadlock. vtss_callout_lock must have been called before vtss_callout_unlock is called.
    if (!api_locked) {
        T_E("API unlock problem");
    }
    api_locked = FALSE;
}







/* ================================================================= *
 *  Board init. Function called by the application
 * ================================================================= */

// Function defining the board
// 
// In : Pointer to the board definition
void vtss_board_phy_init(vtss_board_t *board)
{
    board->descr = "PHY"; // Description
    board->target = VTSS_TARGET_CU_PHY; // Target
    board->board_init = board_init; // Pointer to function initializing the board
}







