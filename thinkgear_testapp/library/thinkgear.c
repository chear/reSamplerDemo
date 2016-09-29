/*
 * @(#)thinkgear.c    4.0    May 14, 2008
 *
 * Copyright (c) 2007 NeuroSky, Inc. All Rights Reserved
 * NEUROSKY PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/**
 * @file thinkgear.c
 *
 * @author Kelvin Soo
 * @version 4.6, Jan 24, 2013 Neraj Bobra
 * - Added TG_DATA_EEG_RAW1_PREVIOUS
 * - Added TG_DATA_EEG_RAW2_PREVIOUS
 * @version 4.5, Oct 30, 2012 Neraj Bobra
 * -Added the 0xB3 code to the parser
 * -Added TG_DATA_POOR_SIGNAL_CH1 and TG_DATA_POOR_SIGNAL_CH2
 * @version 4.4, Sep 7, 2011 Neraj Bobra
 * -updated the 0x90 code so that it will output the timestamp to the thinkgearlog file
 * @version 4.3, Feb 1, 2011 Neraj Bobra
 *	 - Updated the JNI to support the following functions:
 *	   writestreamlog
 *	   writedatalog
 *	   enablelowpass
 *	   enableblinkdetection
 *	   enableautoread
 * @version 4.2 Sep 28, 2009 Kelvin Soo
 *   - Updated TGCD driver version to 10.
 *   - Added TG_EnableLowPassFilter() function.
 *   - Added TG_HISTORY_BUFFER_SIZE constant.
 *   - Added lowPassFilterEnabled, lowPassFilterInitialized, rawHistoryIndex,
 *     and rawHistory[TG_HISTORY_BUFFER_SIZE] to ThinkGearConnection struct.
 *   - Added lowPassFilter() function, and updated thinkgearFunc() to
 *     use it when parsing codes 0x06 and 0x80.
 *   - Added some "Hidden" TG_DATA_ types that will be used in the near
 *     future.
 *   - Added declaration for TG_GetPreviousValue() function that will be added
 *     formally in the future.
 * @version 4.1 Jun 26, 2009 Kelvin Soo
 *   - Updated name of library from ThinkGear Connection API to
 *     ThinkGear Communications Driver (TGCD). The library still
 *     uses ThinkGear Connection objects and IDs to communicate.
 * @version 4.0 May 14, 2008 Kelvin Soo
 *   - Updated to match new 4.0 API to support new features of ThinkGear-EM.
 *   - Reworked log file system.
 *   - Uses new ThinkGearParser interface instead of ThinkGearData.
 *   - Removes handling of threading.
 *   - Removes all recalc functionality.
 * @version 1.0 Nov 29, 2007 Kelvin Soo
 *   - Initial version.
 */

#include "thinkgear.h"

/* Include libraries required specifically by this implementation of this
 * library and not already included by this library's header
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32_WCE)
int errno = 0;
#else
#include <errno.h>
#endif

#include "SerialComPort.h"
#include "ThinkGearStreamParser.h"
#include "ftime.h"



/* Define constants used in this implementation */
#define TG_DRIVER_VERSION 3

/* "Hidden" TG Data types not yet exposed to the public interface 
    Commentted out if defined in header file.*/
#define TG_DATA_BATTERY             0
//#define TG_DATA_POOR_SIGNAL         1
//#define TG_DATA_ATTENTION           2
//#define TG_DATA_MEDITATION          3
//#define TG_DATA_RAW                 4
#define TG_DATA_EEG_RAW1              4
//#define TG_DATA_DELTA               5
//#define TG_DATA_THETA               6
//#define TG_DATA_ALPHA1              7
//#define TG_DATA_ALPHA2              8
//#define TG_DATA_BETA1               9
//#define TG_DATA_BETA2              10
//#define TG_DATA_GAMMA1             11
//#define TG_DATA_GAMMA2             12
#define TG_DATA_EEG_RAW2             13
#define TG_DATA_EEG_RAW3             14
#define TG_DATA_EEG_RAW4             15
#define TG_DATA_EEG_RAW5             16
#define TG_DATA_EEG_RAW6             17
#define TG_DATA_EEG_RAW7             18
#define TG_DATA_EEG_RAW_TIMESTAMP1   19
#define TG_DATA_EEG_RAW_TIMESTAMP2   20
#define TG_DATA_EEG_RAW_TIMESTAMP3   21
#define TG_DATA_EEG_RAW_TIMESTAMP4   22
#define TG_DATA_EEG_RAW_TIMESTAMP5   23
#define TG_DATA_EEG_RAW_TIMESTAMP6   24
#define TG_DATA_EEG_RAW_TIMESTAMP7   25
#define TG_DATA_ACCEL_X              26
#define TG_DATA_ACCEL_Y              27
#define TG_DATA_ACCEL_Z              28
#define TG_DATA_TEMPERATURE_RAW      29
#define TG_DATA_GSR_RAW              30
#define TG_DATA_ATTENTION2           31
#define TG_DATA_MEDITATION2          32
#define TG_DATA_TRANSMITTER_ID       33
#define TG_DATA_SEP                  34
#define TG_DATA_SEP_TIMESTAMP        35
#define TG_DATA_EVENT                36
//#define TG_DATA_BLINK_STRENGTH     37
#define TG_DATA_READYZONE	         38
#define TG_DATA_OFFHEAD1             39
#define TG_DATA_OFFHEAD2             40
#define TG_DATA_OFFHEAD3             41
#define TG_DATA_EEG_RAW8             42
#define TG_DATA_EEG_RAW_TIMESTAMP8   43
#define TG_DATA_EEG_RAW_TIMESTAMP    44
#define TG_DATA_POOR_SIGNAL_CH1		 45
#define TG_DATA_POOR_SIGNAL_CH2		 46
#define TG_DATA_EEG_RAW1_PREVIOUS	 47
#define TG_DATA_EEG_RAW2_PREVIOUS	 48


/* Total number of TG Data types, including public and "Hidden" */
#define NUM_TG_DATA_TYPES          50

/**
 * Size of data history buffers, for use with TG_GetPreviousValue().
 */
#define TG_HISTORY_BUFFER_SIZE     33

/**
 * Number of milliseconds between background auto-read pulses.
 */
#define AUTOREAD_DELAY_MS            1

 //char get_cmd [] = {0xAA,0xAA,0x02,0xBC,0x03,0x40};
 //char set_cmd60 [] = {0xAA,0xAA,0x02,0xBC,0x02,0x41};
 //char set_cmd50 [] = {0xAA,0xAA,0x02,0xBC,0x01,0x42};

/* Define private data types and classes */
typedef struct _ThinkGearConnection {

	/* Output log files */
    FILE         *streamLog;
    FILE         *dataLog;

	/* Input stream source */
    SerialComPort serialComPort;
    FILE         *fileSrc;

	/* ThinkGear Parser */
    ThinkGearStreamParser parser;

	/* Data, and ReadPackets() reception status */
    float         data[NUM_TG_DATA_TYPES];
    unsigned char dataReceived[NUM_TG_DATA_TYPES];

    /* Low pass filter */
    //int           lowPassFilterEnabled;
    //int           lowPassFilterInitialized;
    //short         rawHistory[TG_HISTORY_BUFFER_SIZE]; /* older-to-newer */
    //int           rawHistoryIndex;  /* Index of oldest value/next insertion */

	/* Non-embedded Blink Detection */
	//int           blinkDetectionEnabled;

	/* Non-embedded Zone Calculation */
	//int           zoneCalculationEnabled;

	/* Auto-read thread feature */
    int           autoreadEnabled;
#if defined(_WIN32)
    UINT          autoreadTimerId;
#endif

} ThinkGearConnection;


/* Declare private function prototypes */
static void
thinkgearFunc( unsigned char extendedCodeLevel,
               unsigned char code, unsigned char numBytes,
               const unsigned char *value, void *customData );

void
TG_Init();

THINKGEAR_API int
TG_SetDataFormat( int connectionId, int serialDataFormat );

/**
 * (This is a planned future function, not yet revealed to the public API)
 *
 * Returns a previously-read value of the given @c dataType, which is
 * one of the TG_DATA_* constants defined above.  Use @c TG_ReadPackets()
 * to read more Packets in order to obtain updated values.  Afterwards, use
 * @c TG_GetValueStatus() to check if a call to @c TG_ReadPackets() actually
 * updated a particular @c dataType.
 *
 * The most-recently read value is at @c offset 0, so that calling
 * @c TG_GetPreviousValue() with an @c offset of 0 is equivalent to
 * calling @c TG_GetValue().  The next oldest is at @c offset 1, and so on.
 * The largest @c offset available is @c TG_HISTORY_BUFFER_SIZE-1.
 * Attempting to use a larger value will always return a 0.0 value.
 *
 * NOTE: This function will terminate the program with a message printed
 * to stderr if @c connectionId is not a valid ThinkGear Connection, or
 * if @c dataType is not a valid TG_DATA_* constant.
 *
 * @param connectionId The ID of the ThinkGear Connection to get a data
 *                     value from, as obtained from TG_GetNewConnectionId().
 * @param dataType     The type of data value desired.  Select from one of
 *                     the TG_DATA_* constants defined above.  Refer to the
 *                     documentation of each constant for details of how
 *                     to interpret its value.
 * @param offset       Specifies which previous value to retrieve.  0 is
 *                     the most-recently read value, 1 is the next-oldest,
 *                     etc.  Attempting to use an @c offset more than
 *                     @c TG_HISTORY_BUFFER_SIZE-1 or less than 0 will
 *                     always return 0.0.
 *
 * @return 0.0 if @c offset is more than @c TG_HISTORY_BUFFER_SIZE-1 or
 *         less than 0.
 *
 * @return The requested previous value of the requested @c dataType.
 */
THINKGEAR_API float
TG_GetPreviousValue( int connectionId, int dataType, int offset );

/**
 * Enables and disables the non-embedded zone calculation.
 *
 * Non-embedded zone detection is disabled by default.
 *
 * @param connectionId    The ID of the ThinkGear Connection to enable
 *                        low pass filtering for, as obtained from
 *                        TG_GetNewConnectionId().
 * @param enable          Enables or disables the non-embedded zone
 *                        calculation.
 *
 * @return -1 if @c connectionId does not refer to a valid ThinkGear
 *         Connection ID handle.
 *
 * @return 0 on success.
 */
THINKGEAR_API int
TG_EnableZoneCalculation( int connectionId, int enable );


/* Define globals used in this implementation */
ThinkGearConnection *connections[TG_MAX_CONNECTION_HANDLES];

/**
Equiripple low pass filter characteristics with density factor of 20.

Astop: 40dB
Apass: 1dB
Fstop: 50Hz
Fpass: 30Hz
Fs: 512Hz

 0.0096046
 0.0069921
 0.0070707
 0.0048744
 4.6154e-005
-0.0071154
-0.015583
-0.023602
-0.028951
-0.029343
-0.022887
-0.0086064
 0.013218
 0.04092
 0.071568
 0.10139
 0.12637
 0.14299
 0.14882
 0.14299
 0.12637
 0.10139
 0.071568
 0.04092
 0.013218
-0.0086064
-0.022887
-0.029343
-0.028951
-0.023602
-0.015583
-0.0071154
 4.6154e-005
 0.0048744
 0.0070707
 0.0069921
 0.0096046
*/
int LPF_FACTORS[] = {
0x00000275,
0x000001CA,
0x000001CF,
0x0000013F,
0x00000003,
0xFFFFFE2E,
0xFFFFFC03,
0xFFFFF9F6,
0xFFFFF897,
0xFFFFF87D,
0xFFFFFA25,
0xFFFFFDCC,
0x00000362,
0x00000A79,
0x00001252,
0x000019F4,
0x00002059,
0x0000249A,
0x00002619,
0x0000249A,
0x00002059,
0x000019F4,
0x00001252,
0x00000A79,
0x00000362,
0xFFFFFDCC,
0xFFFFFA25,
0xFFFFF87D,
0xFFFFF897,
0xFFFFF9F6,
0xFFFFFC03,
0xFFFFFE2E,
0x00000003,
0x0000013F,
0x000001CF,
0x000001CA,
0x00000275
};

/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_GetVersion() {
    return( TG_DRIVER_VERSION );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_GetNewConnectionId() {

    ThinkGearConnection *connection = NULL;
    int i = 0;

    /* Search for an unused Connection index in the global connections table */
    /* Return error if no unused Connection indices available */
    for( i=0; i<TG_MAX_CONNECTION_HANDLES; i++ ) if( !connections[i] ) break;
    if( i == TG_MAX_CONNECTION_HANDLES ) return( -1 );

    /* Allocate memory for a new ThinkGear Connection object */
    /* Return error if not enough memory for a new Connection object */
    connection =
        (ThinkGearConnection *)calloc( 1, sizeof(ThinkGearConnection) );
    if( !connection ) return( -2 );

    connection->fileSrc = NULL;

    /* Insert the new Connection object into the global connections table
     * at the unused Connection index
     */
    connections[i] = connection;

    /* Return the connection ID (Connection table index) */
    return( i );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_SetStreamLog( int connectionId, const char *filename ) {

    ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* If the connection's stream log is open, close it first */
    if( connection->streamLog ) {
        fclose( connection->streamLog );
        connection->streamLog = NULL;
    }

    /* If no file specified or emptry string, return success */
    if( !filename || (filename[0] == '\0') ) return( 0 );

   	/* Open the @c filename for writing as the connection's stream log */
	errno = 0;
	connection->streamLog = fopen( filename, "w" );
	if( !connection->streamLog ) return( -2 );

    /* Return success */
    return( 0 );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_SetDataLog( int connectionId, const char *filename ) {

    ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* If the connection's stream log is open, close it first */
    if( connection->dataLog ) {
        fclose( connection->dataLog );
        connection->dataLog = NULL;
    }

    /* If no file specified or emptry string, return success */
    if( !filename || (filename[0] == '\0') ) return( 0 );

    /* Open the @c filename for writing as the connection's stream log */
    errno = 0;
    connection->dataLog = fopen( filename, "w" );
    if( !connection->dataLog ) return( -2 );

    /* Return success */
    return( 0 );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_WriteStreamLog( int connectionId, int insertTimestamp, const char *msg ) {

    ThinkGearConnection *connection = NULL;
    FILE *log = NULL;
    double dValue;
    struct TIMEB time = {0,0,0,0};

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* Get local pointers, to improve code readability */
    log = connection->streamLog;

	/* If the connection's stream log isn't open, return an error */
    if( !log ) return( -2 );

   	/* Write @c msg into the connection's stream log */
    if( insertTimestamp ) {
        FTIME( &time );
        dValue = time.time + time.millitm/1000.0;
		fprintf( log, "\n%.3lf: ", dValue );
    }
    fprintf( log, msg );

    /* Return success */
    return( 0 );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_WriteDataLog( int connectionId, int insertTimestamp, const char *msg ) {

    ThinkGearConnection *connection = NULL;
    FILE *log = NULL;
    double dValue;
    struct TIMEB time = {0,0,0,0};

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* Get local pointers, to improve code readability */
    log = connection->dataLog;

	/* If the connection's stream log isn't open, return an error */
    if( !log ) return( -2 );

   	/* Write @c msg into the connection's stream log */
    if( insertTimestamp ) {
        FTIME( &time );
        dValue = time.time + time.millitm/1000.0;
		fprintf( log, "%.3lf: ", dValue );
    }
    fprintf( log, "%s\n", msg );

    /* Return success */
    return( 0 );
}


/*
 * See header file for interface documentation.
 */
//THINKGEAR_API int
//TG_EnableLowPassFilter( int connectionId, int enable ) {

    //ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    //connection = connections[connectionId];
    //if( !connection ) return( -1 );

    /* Set whether the low pass filter is enabled */
    //connection->lowPassFilterEnabled     = enable;
    //connection->lowPassFilterInitialized = enable;

    /* Return success */
   // return( 0 );
//}

/*
 * See header file for interface documentation.
 */
//THINKGEAR_API int
//TG_EnableBlinkDetection( int connectionId, int enable ) {

   // ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
   // connection = connections[connectionId];
  //  if( !connection ) return( -1 );

    /* Set whether the low pass filter is enabled */
  //  connection->blinkDetectionEnabled     = enable;

    /* Return success */
  //  return( 0 );
//}

/*
 * See header file for interface documentation.
 */
//THINKGEAR_API int
//TG_EnableZoneCalculation( int connectionId, int enable ) {

    //ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
   // connection = connections[connectionId];
   // if( !connection ) return( -1 );

    /* Set whether the low pass filter is enabled */
   // connection->zoneCalculationEnabled     = enable;

    /* Return success */
    //return( 0 );
//}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_Connect( int connectionId, const char *serialPortName, int serialBaudrate,
            int serialDataFormat ) {

    ThinkGearConnection *connection = NULL;
    int errCode = 0;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* Initialize the ThinkGearParser */
    errCode = TG_SetDataFormat( connectionId, serialDataFormat );
    if( errCode ) return( -4 );

    /* Open serialPortName based on serialDataFormat type... */
    switch( serialDataFormat ) {

        /* User requested the input data stream to be from a file... */
        case( TG_STREAM_FILE_PACKETS ):

            /* Open the input file by name */
            connection->fileSrc = fopen( serialPortName, "rb" );
            if( !connection->fileSrc ) return( -2 );
			//printf( "connection->fileSrc: %X\n", connection->fileSrc );
            break;

        /* User requested input data stream to be from serial COM port... */
        case( TG_STREAM_PACKETS ):
        //case( TG_STREAM_5VRAW ):

            /* Open the named serial communication (COM) port */
            errCode = SERIALCOM_openPort( &connection->serialComPort,
                                          serialPortName );
            if( errCode ) return( -2 );

            /* Set the serial communication (COM) baud rate */
            errCode = TG_SetBaudrate( connectionId, serialBaudrate );
            if( errCode ) return( -3 );

            break;

        /* Invalid serialDataFormat: return invalid serialDataFormat error */
        default: return( -4 );

    } /* end "Open serialPortName based on serialDataFormat type..." */

    /* Return success */
    return( 0 );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_ReadPackets( int connectionId, int numPackets ) {

    ThinkGearConnection *connection = NULL;
    ThinkGearStreamParser *parser = NULL;
    SerialComPort *serialComPort = NULL;
    FILE *log = NULL;
    double dValue;
    int bytesRead = 0;
    int packetsRead = 0;
    char buffer[2];
    int errCode = 0;
    int timestamped = 0;
    struct TIMEB time = {0,0,0,0};

	//char msg[256];  /* for debugging */

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* Get local pointers, to improve code readability */
    serialComPort = &connection->serialComPort;
    parser = &connection->parser;
    log = connection->streamLog;

    /* Clear dataReceived[] flags */
    memset( connection->dataReceived, 0, sizeof(connection->dataReceived) );

    /* Loop until no bytes are available, or until enough packets have been
     * read...
     *
     * IMPORTANT: The Connection's serialComPort MUST have been opened in
     *            non-blocking mode, otherwise this loop may take
     *            indefinitely long to complete as it waits for input
     *            values that may never arrive!
     */
    while( (numPackets == -1) || (packetsRead < numPackets) ) {

        /* Get a byte from the data input stream */
        if( !connection->fileSrc ) {

            /* Attempt to get the next byte from the serial stream */
            errCode = SERIALCOM_read( serialComPort, buffer, 2 );

            /* If no bytes were available, return an appropriate return code */
            if( errCode == 0 ) {
                if( !bytesRead ) return( -2 );
                else             return( packetsRead );
			} else {
				//fprintf( stderr, "SERIALCOM_read():%d\n", errCode );
				//fflush( stderr );
			}

        } else {
            /* Attempt to get the next byte from the input file */
			
            errCode = (int)fread( buffer, 1, 1, connection->fileSrc );
            buffer[1] = '\0';
        }

        /* If an error occurred getting the byte, return an I/O error */
        if( errCode != 1 ){
			
			return( -3 );
		}

        /* Record the byte in the log */
        bytesRead++;
        if( log ) {
            if( !timestamped ) {
                FTIME( &time );
		        dValue = time.time + time.millitm/1000.0;
        		fprintf( log, "\n%.3lf: ", dValue );
                //fprintf( log, "\n%ld.%03hu: ", currTime.time, currTime.millitm );
                timestamped = 1;
            }
            fprintf( log, " %02X", buffer[0] & 0xFF );
        }

        /* Feed the byte into the ThinkGear Parser */
        errCode = THINKGEAR_parseByte( parser, buffer[0] ); 
        if( errCode == 1 ){
			packetsRead++;
		}
    }

    if( log ) fflush( log );

    /* Return the actual number of packets read */
    return( packetsRead );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_GetValueStatus( int connectionId, int dataType ) {

    ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    /* Terminate if the connection is not valid */
    connection = connections[connectionId];
    if( !connection ) {
        fprintf( stderr, "ERROR: Invalid connectionId to TG_GetValue(): %d\n",
                 connectionId );
        exit( EXIT_FAILURE );
    }

    /* Terminate if the dataType is not valid */
    if( (dataType < 0) || (dataType >= NUM_TG_DATA_TYPES) ) {
        fprintf( stderr, "ERROR: Invalid dataType to TG_GetValue(): %d\n",
                 dataType );
        exit( EXIT_FAILURE );
    }

    /* Return the value of the dataType */
    return( connection->dataReceived[dataType] );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API float
TG_GetValue( int connectionId, int dataType ) {

    ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    /* Terminate if the connection is not valid */
    connection = connections[connectionId];
    if( !connection ) {
        fprintf( stderr, "ERROR: Invalid connectionId to TG_GetValue(): %d\n",
                 connectionId );
        exit( EXIT_FAILURE );
    }

    /* Terminate if the dataType is not valid */
    if( (dataType < 0) || (dataType >= NUM_TG_DATA_TYPES) ) {
        fprintf( stderr, "ERROR: Invalid dataType to TG_GetValue(): %d\n",
                 dataType );
        exit( EXIT_FAILURE );
    }

    /* Return the value of the dataType */
//	if(connection->autoreadEnabled != 0 ){ // Rico added for dismiss the dirty data
		connection->dataReceived[dataType] = 0;
//	}
    return( connection->data[dataType] );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_SendByte( int connectionId, int b ) {

    ThinkGearConnection *connection = NULL;
    char buffer = 0;
    int errCode = 0;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    if( connection->fileSrc ) return( -2 );

    /* Send the byte */
    buffer = (char)(b & 0xFF);
    errCode = SERIALCOM_write( &connection->serialComPort, &buffer, 1 );
    if( errCode != 1 ) return( -3 );

    /* Return success */
    return( 0 );
}

THINKGEAR_API int
TG_SendBytes( int connectionId, char buffer[], int len ) {

    ThinkGearConnection *connection = NULL;
    //char buffer = 0;
    int errCode = 0;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    if( connection->fileSrc ) return( -2 );

    /* Send the byte */
    //buffer = (char)(b & 0xFF);
    errCode = SERIALCOM_write( &connection->serialComPort, buffer, len );
	printf("TG_SendBytes origin: %d , write len: %d \n",len,errCode);
    if( errCode != len ) return( -3 );

    /* Return success */
    return( 0 );
}



THINKGEAR_API int
MWM15_getFilterType( int connectionId){
	char get_cmd [] = {0xAA,0xAA,0x02,0xBC,0x03,0x40};
	int i = 0;
	int sum = 0;
	for(;i<6;i++){
		sum += TG_SendByte(connectionId,get_cmd[i]);
	}
	return sum;
}

THINKGEAR_API int
MWM15_setFilterType( int connectionId, int filterType){
	int i;
	int sum = 0;
	if(filterType == MWM15_FILTER_TYPE_60HZ){
		char set_cmd60 [] = {0xAA,0xAA,0x02,0xBC,0x02,0x41};
		for(i = 0;i<6;i++){
			sum += TG_SendByte(connectionId,set_cmd60[i]);
		}
		//return 0;//TG_SendBytes(connectionId,set_cmd60,5); //TG_SendBytes doesn't work
	}else{
		char set_cmd50 [] = {0xAA,0xAA,0x02,0xBC,0x01,0x42};
		for(i = 0;i<6;i++){
			sum += TG_SendByte(connectionId,set_cmd50[i]);
		}
		//return sum;//TG_SendBytes(connectionId,set_cmd50,5);
	}
	return sum;
	
}

/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_SetBaudrate( int connectionId, int serialBaudrate ) {

    ThinkGearConnection *connection = NULL;
    int baudrate = 0;
    int errCode = 0;

    /* Get the requested Connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    if( connection->fileSrc ) return( -4 );

    /* Translate ThinkGear Connection serial baudrates to SerialComPort
     * baudrates
     */
    switch( serialBaudrate ) {
        case( TG_BAUD_1200   ): baudrate = BAUDRATE_1200; break;
        case( TG_BAUD_2400   ): baudrate = BAUDRATE_2400; break;
        case( TG_BAUD_4800   ): baudrate = BAUDRATE_4800; break;
        case( TG_BAUD_9600   ): baudrate = BAUDRATE_9600; break;
        case( TG_BAUD_57600  ): baudrate = BAUDRATE_57600; break;
        case( TG_BAUD_115200 ): baudrate = BAUDRATE_115200; break;
        default: return( -2 );
    }

    /* Set the serial communication (COM) baud rate */
    errCode = SERIALCOM_configurePort( &connection->serialComPort, baudrate );
    if( errCode ) return( -3 );

    return( 0 );
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_SetDataFormat( int connectionId, int serialDataFormat ) {

    ThinkGearConnection *connection = NULL;
    ThinkGearStreamParser *parser = NULL;
    unsigned char parserType = 0;
    int errCode = 0;

    /* Get the requested Connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return( -1 );

    /* Translate ThinkGear Connection serial data formats to ThinkGear
     * Stream Parser types
     */
    parser = &connection->parser;
    switch( serialDataFormat ) {
        case( TG_STREAM_FILE_PACKETS ): // fall through to TG_STREAM_PACKETS...
        case( TG_STREAM_PACKETS ): parserType = PARSER_TYPE_PACKETS; break;
        //case( TG_STREAM_5VRAW   ): parserType = PARSER_TYPE_2BYTERAW; break;
        default: return( -2 );
    }

    /* Initialize the Connection's ThinkGearParser */
    errCode = THINKGEAR_initParser( parser, parserType, thinkgearFunc,
                                    connection );
    if( errCode ) return( -2 );

    /* Return success */
    return( 0 );
}


#if defined(_WIN32)
/**
 * Simply calls TG_ReadPackets( -1 ).  Exists so that TG_ReadPackets()
 * can be used as a callback function to timeSetEvent().
 */
void CALLBACK
autoReadPackets( UINT uTimerID, UINT uMsg, DWORD_PTR dwUser,
                 DWORD_PTR dw1, DWORD_PTR dw2 ) {

    int connectionId = 0;
    int errCode = 0;

	/* Unused arguments */
	uTimerID = uTimerID;
	uMsg = uMsg;
	dw1 = dw1;
	dw2 = dw2;

    /* Read all available Packets in the connection */
    connectionId = (int)dwUser;
    errCode = TG_ReadPackets( connectionId, -1 );
    /* Ignore the error code */

    return;
} /* autoReadPackets() */
#endif /* _WIN32 */


/*
 * See header file for interface documentation.
 */
THINKGEAR_API int
TG_EnableAutoRead( int connectionId, int enable ) {

//    ThinkGearConnection *connection = NULL;
//
//    /* Get the requested connection from the global connections[] table */
//    connection = connections[connectionId];
//    if( !connection ) return( -1 );
//
//    /* If auto-read is not already enabled and the caller requested to
//     * turn on auto-read...
//     */
//    if( !connection->autoreadEnabled && enable ) {
//
//#if defined(_WIN32)
//        /* Create the auto-read thread (timer) that calls the
//         * autoReadPackets() function every AUTOREAD_DELAY_MS
//         * milliseconds.
//         */
//        connection->autoreadTimerId =
//            timeSetEvent( AUTOREAD_DELAY_MS, 0,
//                          autoReadPackets,
//                          connectionId,
//                          TIME_PERIODIC |
//                          TIME_CALLBACK_FUNCTION |
//                          TIME_KILL_SYNCHRONOUS );
//        if( !connection->autoreadTimerId ) return( -2 );
//#endif /* _WIN32 */
//        connection->autoreadEnabled = 1;
//
//    /* Else if auto-read is currently enabled and the caller requested to
//     * turn off auto-read...
//     */
//    } else if( connection->autoreadEnabled && !enable ) {
//
//#if defined(_WIN32)
//		/* Attempt to kill auto-read thread (timer) */
//        if( timeKillEvent(connection->autoreadTimerId) ==
//            MMSYSERR_INVALPARAM ) {
//
//			/* autoreadTimerId was not a valid Timer ID */
//	        connection->autoreadEnabled = 0;
//            return( -3 );
//        }
//#endif /* _WIN32 */
//        connection->autoreadEnabled = 0;
//
//    } /* Check if enabling or disabling... */

    return( 0 );

} /* TG_EnableAutoRead() */


/*
 * See header file for interface documentation.
 */
THINKGEAR_API void
TG_Disconnect( int connectionId ) {

    ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return;

    /* Stop the autoread thread, if it is running */
    if( connection->autoreadEnabled ) TG_EnableAutoRead( connectionId, 0 );

    /* Destroy the serial com port */
	if(connection->serialComPort.portName != NULL){// Rico: call TG_Disconnect then call TG_FreeConnection, the app will crash
        SERIALCOM_destroySerialComPort( &connection->serialComPort );
	}

    /* Close the input file stream, if applicable */
    if( connection->fileSrc ) {
        fclose( connection->fileSrc );
        connection->fileSrc = NULL;
    }
}


/*
 * See header file for interface documentation.
 */
THINKGEAR_API void
TG_FreeConnection( int connectionId ) {

    ThinkGearConnection *connection = NULL;

    /* Get the requested connection from the global connections[] table */
    connection = connections[connectionId];
    if( !connection ) return;

    /* Disconnect the Connection, if applicable */
    /* Automatically frees memory in the connection's serialComPort */
    TG_Disconnect( connectionId );

    /* Clean up any allocated memory within the Connection object */
    if( connection->streamLog ) fclose( connection->streamLog );
    if( connection->dataLog ) fclose( connection->dataLog );
    /* No allocated memory for parser, so nothing to free */

    /* Clean up the Connection object itself */
    free( connections[connectionId] );
    connections[connectionId] = NULL;
}


/*
 *
 * @param lowPassHistory Array of signed 16-bit values.  Older values
 *                       to the left, newer values to the right.
 * @param i              Oldest value in @c lowPassHistory[].  Therefore,
 *                       the newest value is at [i-1].
 *
 * @return Filtered value.
 */
//static short
//lowPassFilter( short* lowPassHistory, int i ) {

	//int value = 0;
	//int j = 0;

	//for( j=0; j<TG_HISTORY_BUFFER_SIZE; j++ ) {
	//	if( --i < 0 ) i = TG_HISTORY_BUFFER_SIZE-1;
	//	value += (LPF_FACTORS[i] * lowPassHistory[i]) >> 16;
	//}

	//return( (short)value );

//} /* lowPassFilter() */


/**
 * Processes a raw ADC value received on the @c connection.
 *
 * The value is saved into the @c connection's rawHistory[] buffer,
 * and a 30Hz low pass filter is applied to the value, if applicable.
 *
 * @param connection
 * @param adcValue
 *
 * @return The processed ADC value (if low pass filtering is not enabled,
 *         then the original @c adcValue is simply returned).
 */
//static short
//processRawAdcValue( ThinkGearConnection *connection, short adcValue ) {

	//int i = 0;

	/* Initialize raw history for low pass filter (only if it has not been
	 * initialized, or was disabled at some point)
	 */
	//if( !connection->lowPassFilterInitialized ) {
	//	for( i=0; i<TG_HISTORY_BUFFER_SIZE; i++ ) {
	//		connection->rawHistory[i] = adcValue;
	//	}
	//	connection->lowPassFilterInitialized = 1;
	//}

	/* Save raw ADC value for low pass filter */
	//connection->rawHistory[connection->rawHistoryIndex] = adcValue;
	//connection->rawHistoryIndex++;
	//if( connection->rawHistoryIndex >= TG_HISTORY_BUFFER_SIZE ) {
	//	connection->rawHistoryIndex = 0;
	//}

	/* Apply low pass filter to value, if applicable */
	//if( connection->lowPassFilterEnabled ) {
	//	adcValue = lowPassFilter( connection->rawHistory,
	//							  connection->rawHistoryIndex );
	//}

	//return( adcValue );
//}


/**
 * This function handles data values parsed from ThinkGear Packets.
 *
 * This function is not intended to be called directly; it is intended
 * to be invoked by a ThinkGearStreamParser as a callback function
 * whenever a data value is successfully parsed.  As such, it should
 * be used as the @c handleDataValueFunc argument to initialize the
 * ThinkGearStreamParser.
 *
 * The implementation of this handler function should be customized
 * to suit the needs of each application.  Refer to the ThinkGear Packet
 * Specification document and diagram for details.
 *
 * THIS IMPLEMENTATION: Handles each DataRow data value received by casting
 * to a float and saving it to the data[] array.
 *
 * If the dataLog is enabled, then also writes the DataRow to the
 * dataLog.
 *
 * @param extendedCodeLevel The EXtended CODE level of the @c code.
 * @param code              The CODE of the data value.
 * @param numBytes          The number of bytes in the @c value[].
 * @param value             An array of bytes comprising the data value.
 * @param customData        Pointer to arbitrary user-defined data.
 */
static void
thinkgearFunc( unsigned char extendedCodeLevel,
               unsigned char code, unsigned char numBytes,
               const unsigned char *value, void *customData ) {

    /* Union used to convert IEEE 4-byte floating point numbers
     * that have been encoded as arrays of 4 bytes (unsigned chars)
     * back into a C float value.  See usage further below for
     * an important implementation/portability note.
     */
    union _BYTES_TO_FLOAT {
        unsigned char bytes[4];
        float fValue;
    } floatConverter;

    struct TIMEB time = {0,0,0,0};
    double dValue = 0.0;
    float  fValue = 0.0;
    short  sValue = 0;
    int    iValue = 0;
    int    i = 0;

    /* Local pointers to data objects, for code readability */
    ThinkGearConnection *connection = NULL;
    FILE                *dataLog = NULL;
    float               *data = NULL;
    unsigned char       *dataReceived = NULL;

    /* Get local pointers to data objects, for code readability */
    connection   = (ThinkGearConnection *)customData;
    dataLog      = connection->dataLog;
    data         = connection->data;
    dataReceived = connection->dataReceived;

    /* Print timestamp into dataLog, if applicable */
    FTIME( &time );
    if( dataLog ) {
        dValue = time.time + time.millitm/1000.0;
        fprintf( dataLog, "%.3lf: ", dValue );
        //fprintf( dataLog, "%ld.%03hu: ", time.time, time.millitm );
        //fprintf( dataLog, "%I64d.%03d: ", time.time, time.millitm );
        fprintf( dataLog, "[%02X] ", code&0xFF );
    }

    /* Handle each type of value */
    if( extendedCodeLevel == 0 ) {

        switch( code ) {
						/* code: FILTER_TYPE */
            case( 0xBC ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[MWM15_DATA_FILTER_TYPE] = (float)iValue;
                dataReceived[MWM15_DATA_FILTER_TYPE] = 1;
                break;

           
            /* code: POOR_SIGNAL */
            case( 0x02 ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_POOR_SIGNAL] = (float)iValue;
                dataReceived[TG_DATA_POOR_SIGNAL] = 1;
                break;

            /* code: ATTENTION */
            case( 0x04 ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_ATTENTION] = (float)iValue;
                dataReceived[TG_DATA_ATTENTION] = 1;
                break;

            /* code: MEDITATION */
            case( 0x05 ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_MEDITATION] = (float)iValue;
                dataReceived[TG_DATA_MEDITATION] = 1;
                break;

            /* code: 8-bit RAW */
            case( 0x06 ):

                /* Interpret value byte into an ADC value */
                sValue = (*value)<<2;
				//processRawAdcValue( connection, sValue );

				/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
				/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
				/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
				fValue = (float)(sValue*.005865102639 + -3.0);
				if( dataLog ) fprintf( dataLog, "%6hd, %04X, %9f\n",
									   sValue, sValue&0xFFFF, fValue );

				/* Save ADC value */
				data[TG_DATA_RAW] = (float)sValue;
				dataReceived[TG_DATA_RAW] = 1;
				break;

            /* code: OFFHEAD, FLAT, EXCESS, LOWPOWER, HIGHPOWER, ERRFLAGS */
            case( 0x08 ):
            case( 0x09 ):
            case( 0x0A ):
            case( 0x0B ):
            case( 0x0C ):
            case( 0x0D ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                break;

            // zone meter
           // case( 0x0E ):
             //   iValue = *value & 0xFF;
             //   if( dataLog ) fprintf( dataLog, "%d\n", iValue );
				/* Will not update if non-embedded zone calcuation is enabled */
            //    if( !connection->zoneCalculationEnabled ) {
			//		data[TG_DATA_READYZONE] = (float)iValue;
			//		dataReceived[TG_DATA_READYZONE] = 1;
			//	}
			//	break;

			/*code: TG_DATA_EEG_RAW_TIMESTAMP */
			case (0x13):
				iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_EEG_RAW_TIMESTAMP] = (float)iValue;
                dataReceived[TG_DATA_EEG_RAW_TIMESTAMP] = 1;
                break;

			/* code: ATTENTION2 */
			case( 0x14 ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_ATTENTION2] = (float)iValue;
                dataReceived[TG_DATA_ATTENTION2] = 1;
                break;

			/* code: MEDITATION2 */
			case( 0x15 ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_MEDITATION2] = (float)iValue;
                dataReceived[TG_DATA_MEDITATION2] = 1;
                break;

			/* code: BLINK STRENGTH */
			case( 0x16 ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
				/* Will not update if non-embedded eye blink detection is enabled */
				//if( !connection->blinkDetectionEnabled ) {
				//	data[TG_DATA_BLINK_STRENGTH] = (float)iValue;
				//	dataReceived[TG_DATA_BLINK_STRENGTH] = 1;
				//}
				break;
            /* code: EVENT */
            case( 0x1E ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_EVENT] = (float)iValue;
                dataReceived[TG_DATA_EVENT] = 1;
                break;

			/* code: TRANSMITTER_ID */
			case( 0x7F ):
                iValue = *value & 0xFF;
                if( dataLog ) fprintf( dataLog, "%d\n", iValue );
                data[TG_DATA_TRANSMITTER_ID] = (float)iValue;
                dataReceived[TG_DATA_TRANSMITTER_ID] = 1;
                break;

            /* code: 10-bit RAW */
            case( 0x80 ):

                /* Interpret value[] bytes into raw ADC value */
                sValue = (value[0]<<8) | value[1];
				//processRawAdcValue( connection, sValue );

				/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
				/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
				/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
				fValue = (float)(sValue*.005865102639 + -3.0);
				if( dataLog ) fprintf( dataLog, "%6hd, %04X, %9f\n",
									   sValue, sValue&0xFFFF, fValue );

				/* Save ADC value */
                data[TG_DATA_RAW] = (float)sValue;
                dataReceived[TG_DATA_RAW] = 1;
                break;

			 /* code: 599-th sample for BMD 200 */
			 case( 0x8A ):

                /* Interpret value[] bytes into raw ADC value */
                sValue = (value[0]<<8) | value[1];
				//processRawAdcValue( connection, sValue );

				/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
				/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
				/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
				fValue = (float)(sValue*.005865102639 + -3.0);
				if( dataLog ) fprintf( dataLog, "%6hd, %04X, %9f\n",
									   sValue, sValue&0xFFFF, fValue );

				/* Save ADC value */
                data[TG_DATA_RAW] = (float)sValue;
                dataReceived[TG_DATA_RAW] = 1;

				data[TG_DATA_RAW_LAST_MARK] = 3;
				dataReceived[TG_DATA_RAW] = 1;
                break;

			/* code: Reset Data reported every one second */
            /*case( 0x84 ):
				printf("read 0x84 pocket.");
				data[TG_DATA_CON_OUT_4] = (float) value[3];
				dataReceived[TG_DATA_CON_OUT_4] = 1;

				data[TG_DATA_COU_OUT_3] = (float)value[8];
				dataReceived[TG_DATA_COU_OUT_3] = 1;
                break;*/

            /* code: EEG_POWERS (4-byte floats)*/
            case( 0x81 ):

                for( i=0; i<numBytes/4; i++ ) {
                    /* IMPLEMENTATION NOTE: Using a union like this to
                     * reconstruct an IEEE float from an array of bytes
                     * may not be completely safe, and at the least, may
                     * not be portable to little endian systems.  However,
                     * this is the best method I've been able to come up
                     * with.  Fortunately, as we move away from putting
                     * floats as arrays of bytes into the ThinkGear output
                     * stream, this should become less of a problem.
                     */
                    floatConverter.bytes[0] = value[4*i+0];
                    floatConverter.bytes[1] = value[4*i+1];
                    floatConverter.bytes[2] = value[4*i+2];
                    floatConverter.bytes[3] = value[4*i+3];
                    fValue = floatConverter.fValue;
                    if( dataLog ) fprintf( dataLog,
                                           "%7f, 0x%02X%02X%02X%02X, ",
                                           fValue,
                                           floatConverter.bytes[0] & 0xFF,
                                           floatConverter.bytes[1] & 0xFF,
                                           floatConverter.bytes[2] & 0xFF,
                                           floatConverter.bytes[3] & 0xFF );
                    data[TG_DATA_DELTA+i] = fValue;
                    dataReceived[TG_DATA_DELTA+i] = 1;
                }
                if( dataLog ) fprintf( dataLog, "\n" );
                break;

            /* code: ASIC_EEG_POWER_INT (3-byte ints) */
            case( 0x83 ):

                for( i=0; i<numBytes/3; i++ ) {
                    iValue = (value[3*i+0] << 16) |
                             (value[3*i+1] <<  8) |
                             (value[3*i+2] <<  0);
                    if( iValue & 0x00800000 ) iValue |= 0xFF000000;
                    if( dataLog ) fprintf( dataLog, "%7d, 0x%08X, ",
                                           iValue, iValue );
                    data[TG_DATA_DELTA+i] = (float)iValue;
                    dataReceived[TG_DATA_DELTA+i] = 1;
                }
                if( dataLog ) fprintf( dataLog, "\n" );
                break;

            /* code: ASIC_ESENSE_DETAILS */
            case( 0x85 ):
                if( dataLog ) {
                    if( value[0] == 0x01 ) fprintf( dataLog, "Att, " );
                    else if( value[0] == 0x02 ) fprintf( dataLog, "Med, " );
                    sValue = (value[1] << 8) | (value[2]);
                    fprintf( dataLog, "%6d, ", sValue );
                    sValue = (value[3] << 8) | (value[4]);
                    fprintf( dataLog, "%6d, ", sValue );
                    sValue = (value[5] << 8) | (value[6]);
                    fprintf( dataLog, "%6d, ", sValue );
                    sValue = (value[7] << 8) | (value[8]);
                    fprintf( dataLog, "%6d, ", sValue );
                    sValue = (value[9] << 8) | (value[10]);
                    fprintf( dataLog, "%6d, ", sValue );
                    fprintf( dataLog, "%3d\n", value[11] & 0xFF );
                }
                break;

            /* code: OFFHEAD_POINTS */
            case( 0x86 ):
                iValue = (value[0] << 8) | (value[1] << 0);
                if( dataLog ) fprintf( dataLog, "%7d\n", iValue );
                break;

			/* code: RAW_MULTI_CHANNEL_EEG */
			case( 0x90 ):
				if( numBytes >= 3 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[0]<<8) | value[1];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, "%6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[2]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW1] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW1] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP1] = (float)value[2];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP1] = 1;
				}
				if( numBytes >= 6 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[3]<<8) | value[4];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[5]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW2] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW2] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP2] = (float)value[5];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP2] = 1;
				}
				if( numBytes >= 9 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[6]<<8) | value[7];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[8]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW3] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW3] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP3] = (float)value[8];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP3] = 1;
				}
				if( numBytes >= 12 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[9]<<8) | value[10];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[11]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW4] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW4] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP4] = (float)value[11];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP4] = 1;
				}
				if( numBytes >= 15 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[12]<<8) | value[13];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[14]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW5] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW5] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP5] = (float)value[14];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP5] = 1;
				}
				if( numBytes >= 18 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[15]<<8) | value[16];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[17]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW6] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW6] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP6] = (float)value[17];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP6] = 1;
				}
				if( numBytes >= 21 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[18]<<8) | value[19];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f, %u",
										   sValue, sValue&0xFFFF, fValue, value[20]);

					/* Save ADC value */
					data[TG_DATA_EEG_RAW7] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW7] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP7] = (float)value[20];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP7] = 1;
				}
				fprintf( dataLog, "\n" );
                break;

			/* code: ACCELEROMETER_RAW */
			case( 0x91 ):
				if( numBytes >= 2 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[0]<<8) | value[1];
					if( dataLog ) fprintf( dataLog, "%04X", sValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_ACCEL_X] = (float)sValue;
					dataReceived[TG_DATA_ACCEL_X] = 1;
				}
				if( numBytes >= 4 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[2]<<8) | value[3];
					if( dataLog ) fprintf( dataLog, " %04X", sValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_ACCEL_Y] = (float)sValue;
					dataReceived[TG_DATA_ACCEL_Y] = 1;
				}
				if( numBytes >= 6 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[4]<<8) | value[5];
					if( dataLog ) fprintf( dataLog, " %04X", sValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_ACCEL_Z] = (float)sValue;
					dataReceived[TG_DATA_ACCEL_Z] = 1;
				}
				fprintf( dataLog, "\n" );
				break;

			/* code: TEMPERATURE_RAW */
			case( 0x92 ):
				if( numBytes >= 2 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[0]<<8) | value[1];
					if( dataLog ) fprintf( dataLog, " %04X", sValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_TEMPERATURE_RAW] = (float)sValue;
					dataReceived[TG_DATA_TEMPERATURE_RAW] = 1;
				}
				break;

			/* code: GSR_RAW */
			case( 0x93 ):
				if( numBytes >= 2 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[0]<<8) | value[1];
					if( dataLog ) fprintf( dataLog, " %04X", sValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_GSR_RAW] = (float)sValue;
					dataReceived[TG_DATA_GSR_RAW] = 1;
				}
				break;

			/* code: SEP_DATA */
			case( 0xA0 ):
				if( numBytes >= 2 ) {

					/* Save SEP data value */
					data[TG_DATA_SEP] = (float)value[0];
					dataReceived[TG_DATA_SEP] = 1;

					/* Save SEP timestamp */
					data[TG_DATA_SEP_TIMESTAMP] = (float)value[1];
					dataReceived[TG_DATA_SEP_TIMESTAMP] = 1;
				}
				break;

			/* code: RAW_MULTI_CHANNEL_EEG */
			case( 0xB0 ):
				if( numBytes >= 2 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[0]&0x03)<<8) | value[1];
					if( value[0] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, "%6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW1] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW1] = 1;
				}
				if( numBytes >= 4 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[2]&0x03)<<8) | value[3];
					if( value[2] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW2] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW2] = 1;
				}
				if( numBytes >= 6 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[4]&0x03)<<8) | value[5];
					if( value[4] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW3] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW3] = 1;
				}
				if( numBytes >= 8 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[6]&0x03)<<8) | value[7];
					if( value[6] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW4] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW4] = 1;
				}
				if( numBytes >= 10 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[8]&0x03)<<8) | value[9];
					if( value[8] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW5] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW5] = 1;
				}
				if( numBytes >= 12 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[10]&0x03)<<8) | value[11];
					if( value[10] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW6] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW6] = 1;
				}
				if( numBytes >= 14 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = ((value[12]&0x03)<<8) | value[13];
					if( value[12] & 0x10 ) sValue++;
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* sValue     0 to 1023: sValue/1024*6.0      + -3.0 */
					/* sValue -2048 to 2047: sValue+2048/4096*6.0 + -3.0 */
					fValue = (float)(sValue*.005865102639 + -3.0);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW7] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW7] = 1;
				}
				fprintf( dataLog, "\n" );
                break;

			/* code: RAW_MULTI_CHANNEL_EEG */
			case( 0xB1 ):
				if( numBytes >= 3 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[1]<<8) | value[2];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					/* voltage = (float)(((3.3 * (float)ADC) / 4096 - 1.65) / 500); */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, "%6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW1] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW1] = 1;
					data[TG_DATA_EEG_RAW_TIMESTAMP] = (float)value[0];
					dataReceived[TG_DATA_EEG_RAW_TIMESTAMP] = 1;
				}
				if( numBytes >= 5 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[3]<<8) | value[4];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW2] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW2] = 1;
				}
				if( numBytes >= 7 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[5]<<8) | value[6];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW3] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW3] = 1;
				}
				if( numBytes >= 9 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[7]<<8) | value[8];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW4] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW4] = 1;
				}
				if( numBytes >= 11 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[9]<<8) | value[10];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW5] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW5] = 1;
				}
				if( numBytes >= 13 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[11]<<8) | value[12];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW6] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW6] = 1;
				}
				if( numBytes >= 15 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[13]<<8) | value[14];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW7] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW7] = 1;
				}
				if( numBytes >= 17 ) {
					/* Interpret value[] bytes into raw ADC value */
					sValue = (value[15]<<8) | value[16];
					//processRawAdcValue( connection, sValue );

					/* Interpret ADC value into -3.0 to 3.0 range (fValue) */
					fValue = (float)(sValue*.000001611328125 - 0.0033);
					if( dataLog ) fprintf( dataLog, ", %6hd, %04X, %9f",
										   sValue, sValue&0xFFFF, fValue );

					/* Save ADC value */
					data[TG_DATA_EEG_RAW8] = (float)sValue;
					dataReceived[TG_DATA_EEG_RAW8] = 1;
				}
				fprintf( dataLog, "\n" );
                break;

			/* code: RAW_MULTI_CHANNEL_EEG */
			case( 0xB2 ):
				if( numBytes >= 2 ) {
					if (dataReceived[TG_DATA_EEG_RAW1_PREVIOUS] != 1) {				//it's RAW1_PREVIOUS
						/* Interpret value[] bytes into raw ADC value */
						sValue = (value[0]<<8) | value[1];
						
						/* Save ADC value */
						data[TG_DATA_EEG_RAW1_PREVIOUS] = (float)sValue;
						dataReceived[TG_DATA_EEG_RAW1_PREVIOUS] = 1;
					} else {												//it's RAW1
						/* Interpret value[] bytes into raw ADC value */
						sValue = (value[0]<<8) | value[1];
						
						/* Save ADC value */
						data[TG_DATA_EEG_RAW1] = (float)sValue;
						dataReceived[TG_DATA_EEG_RAW1] = 1;
					}
				}
				if( numBytes >= 4 ) {
					if (dataReceived[TG_DATA_EEG_RAW2_PREVIOUS] != 1) {				//it's RAW2_PREVIOUS
						/* Interpret value[] bytes into raw ADC value */
						sValue = (value[2]<<8) | value[3];
						
						/* Save ADC value */
						data[TG_DATA_EEG_RAW2_PREVIOUS] = (float)sValue;
						dataReceived[TG_DATA_EEG_RAW2_PREVIOUS] = 1;
					} else {												//it's RAW2
						/* Interpret value[] bytes into raw ADC value */
						sValue = (value[2]<<8) | value[3];
						
						/* Save ADC value */
						data[TG_DATA_EEG_RAW2] = (float)sValue;
						dataReceived[TG_DATA_EEG_RAW2] = 1;
					}
				}
				
				break;

			/* code: OFFHEAD_MULTI_CHANNEL_EEG */
			case( 0xC0 ):
				if( numBytes >= 2 ) {
					/* Interpret value[] bytes into raw ADC value */
					iValue = (value[0]<<8) | value[1];
					if( dataLog ) fprintf( dataLog, "%04X", iValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_OFFHEAD1] = (float)iValue;
					dataReceived[TG_DATA_OFFHEAD1] = 1;
				}
				if( numBytes >= 4 ) {
					/* Interpret value[] bytes into raw ADC value */
					iValue = (value[2]<<8) | value[3];
					if( dataLog ) fprintf( dataLog, " %04X", iValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_OFFHEAD2] = (float)iValue;
					dataReceived[TG_DATA_OFFHEAD2] = 1;
				}
				if( numBytes >= 6 ) {
					/* Interpret value[] bytes into raw ADC value */
					iValue = (value[4]<<8) | value[5];
					if( dataLog ) fprintf( dataLog, " %04X", iValue&0xFFFF );

					/* Save ADC value */
					data[TG_DATA_OFFHEAD3] = (float)iValue;
					dataReceived[TG_DATA_OFFHEAD3] = 1;
				}
				fprintf( dataLog, "\n" );
				break;
			
			
			/* code: MULTI_POOR_SIGNAL */
			case( 0xB3 ):
				if( numBytes >= 1 ) {
					sValue = value[0];
					
					if( dataLog ) fprintf( dataLog, "%6hd", sValue);

					/* Save ADC value */
					data[TG_DATA_POOR_SIGNAL_CH1] = (float)sValue;
					dataReceived[TG_DATA_POOR_SIGNAL_CH1] = 1;
				}
				if( numBytes >= 2 ) {
					sValue = value[1];

					if( dataLog ) fprintf( dataLog, ", %6hd", sValue);

					/* Save ADC value */
					data[TG_DATA_POOR_SIGNAL_CH2] = sValue;
					dataReceived[TG_DATA_POOR_SIGNAL_CH2] = 1;
				}
				
				fprintf( dataLog, "\n" );
                break;

				 /* code: BATTERY */
            case( 0x01 ):
                fValue = (float)(*value / 128.0);
                if( dataLog ) fprintf( dataLog, "%f\n", fValue );
                data[TG_DATA_BATTERY] = fValue;
                dataReceived[TG_DATA_BATTERY] = 1;
                break;


			/* code: unrecognized */
            default:

                /* Handle unrecognized codes */
                if( dataLog ) {
                    for( i=0; i<numBytes; i++ ) fprintf( dataLog, " %02X",
                                                         value[i] & 0xFF );
                    fprintf( dataLog, "\n" );
                }
                break;

        } /* end "Switch on CODE..." */

    } /* end "If EXCODE level 0..." */

	/*This is for non-embedded Blink Detection and it will overwrite the embedded Blink Detection*/
	//if( connection->blinkDetectionEnabled && dataReceived[TG_DATA_RAW] ) {
	//	unsigned char temp = BLINK_detect( (unsigned char)data[TG_DATA_POOR_SIGNAL], (signed short)data[TG_DATA_RAW]  );
	//	if(temp > 0) {
	//		data[TG_DATA_BLINK_STRENGTH] = temp; 
	//		dataReceived[TG_DATA_BLINK_STRENGTH] = 1;

	//	}
	//}

	/*This is for non-embedded Zone and it will overwrite the embedded Zone Calculation*/
	//if( connection->zoneCalculationEnabled && dataReceived[TG_DATA_MEDITATION] && dataReceived[TG_DATA_ATTENTION] ) {
	//	data[TG_DATA_READYZONE]         = ZONE_calculate( (unsigned char)data[TG_DATA_ATTENTION], (unsigned char)dataReceived[TG_DATA_MEDITATION] );
	//	dataReceived[TG_DATA_READYZONE] = 1;
	//}

    if( dataLog ) fflush( dataLog );

} /* end thinkgearFunc() */


/**
 * Clears the global connections[] array, initializing all its pointers
 * to NULL.  Intended to be called by DllMain().  Must be called explicitly
 * otherwise.
 */
void
TG_Init() {
    memset( connections, 0,
            TG_MAX_CONNECTION_HANDLES*sizeof(ThinkGearConnection *) );
}


#if defined(_WIN32) && !defined(_WIN32_WCE)
/**
 * Windows API callback function for DLLs.
 * Called on process creation, thread creation, thread destroy, and
 * process destroy. Use fdwReason to determine which of these four
 * triggered the callback.
 */
BOOL WINAPI
DllMain( HINSTANCE dllHandle, DWORD fdwReason, LPVOID lpvReserved ) {

    /* Unused parameters, so these lines silence benign compiler warnings */
    dllHandle = dllHandle;
    lpvReserved = lpvReserved;

    /* If this is the first time the DLL is being entered,
     * initialize the ThinkGear Connection library's globals
     */
    if( fdwReason == DLL_PROCESS_ATTACH ) TG_Init();
    return( TRUE );
}
#endif /* _WIN32 */

/* END FUNCTIONS FOR JNI SUPPORT */
