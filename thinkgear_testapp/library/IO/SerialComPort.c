/*
 * @(#)SerialComPort.c    1.0    Jul 03, 2007
 *
 * Copyright (c) 2007 NeuroSky, Inc. All Rights Reserved
 * NEUROSKY PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/**
 * @file SerialComPort.c
 *
 * A portable implementation of the SerialComPort class for opening,
 * reading from, and writing to a serial COM port.
 *
 * @author Kelvin Soo
 * @version 1.0 Jul 03, 2007 Kelvin Soo
 *   - Initial version.
 */

#include "SerialComPort.h"

/* Include libraries required specifically by this implementation of this
 * library and not already included by this library's header
 */
#include <stdlib.h>
#include <string.h>

/*
 * Include headers for non-Windows implementations
 */
#if !defined(_WIN32)
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

/* Define constants used in this implementation */
#define SERIAL_COM_PORT_DEBUG_ENABLED 0
#define MAX_ERR_MSG_SIZE 1024

/* Define the port name format based on the platform we are compiling for */
#if defined( _WIN32 )
#if defined( _WIN32_WCE )
#define PORT_NAME_FORMAT "COM%d:"
#else /* !_WIN32_WCE */
#define PORT_NAME_FORMAT "\\\\.\\COM%d"
#endif /* !_WIN32_WCE */
#else /* !_WIN32 */
#define PORT_NAME_FORMAT "/dev/ttys%d"
#endif /* !_WIN32 */


/**
 * See header file for interface documentation.
 */
int
SERIALCOM_portNumberToName( int portNumber, char *portNameBuffer,
                            int portNameBufferSize ) {

    int printResult = 0;

    /* Not currently used since Visual Studio doesn't support snprintf() */
    portNameBufferSize = portNameBufferSize;

    printResult = sprintf( portNameBuffer, PORT_NAME_FORMAT, portNumber );
    return( printResult );
}

#if defined(_WIN32)
void
Win32_outputSystemMsg( DWORD msgId ) {

    TCHAR msg[128];
    DWORD formatOk = 0;

    /* Attempt to use FormatMessage() to get system message... */
    msg[0] = '\0';
    formatOk = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL, msgId, 0, msg, 128, NULL );

#if !defined(UNICODE)

    if( !formatOk ) sprintf( msg, TEXT("Unknown msgID: %lu"), msgId );
    fprintf( stderr, "%s", msg );

#else /* UNICODE */

    /* NOTE: MS Visual Studio 2005 complains about levels of indirection 
     * and types for the length 128 argument here.  Not sure why.
     */
    if( !formatOk ) swprintf( msg, (size_t)128, TEXT("Unknown msgID: %lu"), msgId );

    /* WinCE doesn't show stderr, so output msg to MessageBox() too */
#if defined(_WIN32_WCE)
    MessageBox( NULL, msg, TEXT(""), MB_OK );
#endif /* _WIN32_WCE */

    /* Output msg */
    fprintf( stderr, "%ls", msg );

#endif /* UNICODE */
}
#endif /* _WIN32 */

/**
 * See header file for interface documentation.
 */
int
SERIALCOM_openPort( SerialComPort *serialComPort, const char *portName ) {

    int errCode = 0;

#if SERIAL_COM_PORT_DEBUG_ENABLED
    char msg[256];
#endif /* SERIAL_COM_PORT_DEBUG_ENABLED */

#if defined(_WIN32)
    int          returnSuccess = 0;
    COMMTIMEOUTS comPortTimeouts;
    DWORD        errId = 0;
#if defined(_WIN32_WCE)
    wchar_t *portNameWC = NULL;
    size_t   length = 0;
    size_t   returnVal = 0;
#endif /* _WIN32_WCE */
#endif /* _WIN32 */

    if( !serialComPort ) return( -1 );
    if( !portName ) return( -2 );

    serialComPort->readLog = NULL;

#if SERIAL_COM_PORT_DEBUG_ENABLED
    fprintf( stderr, "Attempting to open port '%s'...\n", portName );
#endif

/* openPort() for Win32 systems */
#if defined(_WIN32)

    /* Open a handle to the named serial COM port */
#if defined(_WIN32_WCE)

    /* Allocate memory for the port name as a wide character string */
    length = mbstowcs( NULL, portName, 0 );
    portNameWC = calloc( length+1, sizeof(wchar_t) );
    if( !portNameWC ) {
        MessageBox( NULL, L"calloc()", L"SERIALCOM_openPort()", MB_OK );
        return( -8 );
    }

    /* Convert portName into a wide character string */
    returnVal = mbstowcs( portNameWC, portName, length );
    if( returnVal < 0 ) {
        MessageBox( NULL, L"calloc()", L"SERIALCOM_openPort()", MB_OK );
        return( -9 );
    }

    /* Attempt to open port */
    serialComPort->handle = CreateFile( portNameWC,
                                        GENERIC_READ | GENERIC_WRITE,
                                        0, NULL, OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL, NULL );

#else

    /* Attempt to open port */
    serialComPort->handle = CreateFileA( portName,
                                         GENERIC_READ | GENERIC_WRITE,
                                         0, NULL, OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL, NULL );
#endif

    /* If an error occurred attempting to open port... */
    if( serialComPort->handle == INVALID_HANDLE_VALUE ) {
        errId = GetLastError();

#if SERIAL_COM_PORT_DEBUG_ENABLED
        fprintf( stderr, "CreateFile( \"%s\" ) returned '%lu': ",
                 portName, errId );
        Win32_outputSystemMsg( errId );
        fprintf( stderr, "\n" );
#endif /* SERIAL_COM_PORT_DEBUG_ENABLED */

        switch( errId ) {
            case( ERROR_FILE_NOT_FOUND ): return( -3 );
            case( ERROR_SEM_TIMEOUT    ): return( -3 );
            case( ERROR_ACCESS_DENIED  ): /* fall through to default*/
            default:
#if SERIAL_COM_PORT_DEBUG_ENABLED
                sprintf( msg, "CreateFileA( '%s' ) returned %d.", portName, errId );
                MessageBox( NULL, msg, "ERROR!", MB_OK );
#endif /* SERIAL_COM_PORT_DEBUG_ENABLED */

                return( -4 );
        }
    }

    /* Configure serial COM port settings (such as baud rate) */
    errCode = SERIALCOM_configurePort( serialComPort, BAUDRATE_9600 );
    if( errCode ) {
        CloseHandle( serialComPort->handle );
        return( -5 );
    }

    /* Update serial COM port read and write timeouts */
    comPortTimeouts.ReadIntervalTimeout = MAXDWORD;  /* in ms, or MAXDWORD */
    comPortTimeouts.ReadTotalTimeoutConstant = 0;    /* in ms, or 0 for instant */
    comPortTimeouts.ReadTotalTimeoutMultiplier = 0;  /* in ms, or 0 for instant */
    comPortTimeouts.WriteTotalTimeoutConstant = 0;   /* in ms */
    comPortTimeouts.WriteTotalTimeoutMultiplier = 0; /* in ms */

    /* Apply updated timeouts to the open serial COM port handle */
    returnSuccess = SetCommTimeouts( serialComPort->handle, &comPortTimeouts );
    if( !returnSuccess ) {
        CloseHandle( serialComPort->handle );
        return( -6 );
    }


/* openPort() for non-Win32 systems */
#else /* !_WIN32 */
    serialComPort->handle = open(portName, O_RDWR | O_NOCTTY | O_NDELAY);
	
	if(serialComPort->handle == -1){
		switch(errno){
			case ENOENT:	// file does not exist
				return -3;
			case EACCES:	// file is inaccessible
			default:
				return -4;
		}
	}
	
	errCode = SERIALCOM_configurePort(serialComPort, BAUDRATE_9600);
	
	if(errCode){
		close(serialComPort->handle);
		return -5;
	}

#endif /* !_WIN32 */

    SERIALCOM_purge( serialComPort );

    /* Record the opened port name */
    serialComPort->portName = (char *)malloc( (strlen(portName)+1)*sizeof(char) );
    if( !serialComPort->portName ) return( -7 );
    strcpy( serialComPort->portName, (char *)portName );

    /* Return success */
    return( 0 );

} /* end SERIALCOM_openPort() */


/**
 * See header file for interface documentation.
 */
int
SERIALCOM_openPortNumber( SerialComPort *serialComPort, int portNumber ) {

    char portName[128];
    int  numPrintedChars = 0;
    int  errCode = 0;

    if( !serialComPort ) return( -1 );

    /* Convert the port number to a string name */
    numPrintedChars = SERIALCOM_portNumberToName( portNumber, portName, 128 );
    if( numPrintedChars >= 128 ) return( -2 );
    /* CHECK PRINT RESULT! */

    /* Attempt to open the port by name */
    errCode = SERIALCOM_openPort( serialComPort, portName );
    return( errCode );
}


/**
 * See header file for interface documentation.
 */
int
SERIALCOM_scanAndOpen( SerialComPort *serialComPort, int startingPortNumber,
                       int endingPortNumber ) {

    int portNumber = 0;
    int errCode = 0;

    if( !serialComPort ) return( -1 );

    /* If user chose not to specify start and end port numbers, default to
     * some reasonable default values
     */
    if( startingPortNumber < 0 ) startingPortNumber = 0;
    if( endingPortNumber   < 0 ) endingPortNumber   = 256;

    /* For each port number from startingPortNumber to endingPortNumber... */
    for( portNumber = startingPortNumber;
         portNumber <= endingPortNumber;
         portNumber++ ) {

        /* Attempt to open the port */
        errCode = SERIALCOM_openPortNumber( serialComPort, portNumber );
        switch( errCode ) {
            case(  0 ): return( portNumber ); /* Port opened successfully */
            case( -1 ): return( -2 );
            case( -2 ): return( -3 );
            /* Do nothing for errCode -3, that port simply couldn't be opened */
            case( -4 ): return( -4 );
            case( -5 ): return( -5 );
            case( -6 ): return( -6 );
        }
    }

    /* No ports could be opened */
    return( -7 );
}


/**
 * See header file for interface documentation.
 */
int
SERIALCOM_configurePort( SerialComPort *serialComPort, int baudRate ) {
#if defined(_WIN32)
	DCB comPortSettings;
#endif
	
	int returnSuccess;
	
	if( !serialComPort ) return( -1 );
	
	returnSuccess = 0;

#if defined(_WIN32)

    /* Fetch current configuration settings from the serial COM port handle */
    comPortSettings.DCBlength = sizeof( comPortSettings );
    returnSuccess = GetCommState( serialComPort->handle, &comPortSettings );
    if( !returnSuccess ) return( -2 );

    /* Choose new baud rate (if applicable) */
    switch( baudRate ) {
        case( BAUDRATE_KEEP_EXISTING ): break;
        case( BAUDRATE_1200   ): comPortSettings.BaudRate = CBR_1200;   break;
        case( BAUDRATE_2400   ): comPortSettings.BaudRate = CBR_2400;   break;
        case( BAUDRATE_4800   ): comPortSettings.BaudRate = CBR_4800;   break;
        case( BAUDRATE_9600   ): comPortSettings.BaudRate = CBR_9600;   break;
        case( BAUDRATE_57600  ): comPortSettings.BaudRate = CBR_57600;  break;
        case( BAUDRATE_115200 ): comPortSettings.BaudRate = CBR_115200; break;
        default: return( -4 );
    }

    /* Choose new settings (if applicable) */
    comPortSettings.ByteSize = 8;
    comPortSettings.StopBits = ONESTOPBIT;
    comPortSettings.Parity   = NOPARITY;

    /* Apply updated settings choices to the serial COM port handle */
    returnSuccess = SetCommState( serialComPort->handle, &comPortSettings );
    if( !returnSuccess ) return( -3 );

#else /* !_WIN32 */
	struct termios options;
	
	int appliedBaudRate = B9600;
	
	// grab the existing settings for this com port
	returnSuccess = tcgetattr(serialComPort->handle, &options);
	
	if(returnSuccess == -1)
		return -2;
	
	// figure out which baud rate to apply
	switch(baudRate){
		case BAUDRATE_KEEP_EXISTING:
			break;
		case BAUDRATE_1200:
			appliedBaudRate = B1200;
			break;
		case BAUDRATE_9600:
			appliedBaudRate = B9600;
			break;
		case BAUDRATE_57600:
			appliedBaudRate = B57600;
			break;
		default:
			return -4;
	}
	
	// set the new baud rate in the options struct
	cfsetispeed(&options, appliedBaudRate);	// input port speed
    cfsetospeed(&options, appliedBaudRate);	// output port speed
	
	// CLOCAL	- local connection, no modem control
	// CREAD	- enable receiving characters
	// PARENB   - enable parity (not set)
	// PARODD   - odd parity (not set; even parity)
	options.c_cflag |= (CLOCAL | CREAD);
	
	// set the options
    returnSuccess = tcsetattr(serialComPort->handle, TCSANOW, &options);
	
	if(returnSuccess == -1)
		return -3;

#endif /* !_WIN32 */
	
	// save the new baud rate
	serialComPort->baudrate = baudRate;
	
	return 0;
}

/**
 * See header file for interface documentation.
 */
int
SERIALCOM_read( SerialComPort *serialComPort, char *buffer, int bufferSize ) {

    int bytesRead = 0;
    int i = 0;

#if defined(_WIN32)
#if !defined(_WIN32_WCE)
    /*struct TIMEB msTime;*/
#endif /* !_WIN32_WCE */
    int     returnSuccess = 0;
    DWORD   dwBytesRead = 0;

    COMSTAT comStat;
    DWORD   dwErrors;
    BOOL    fOOP, fOVERRUN, fPTO, fRXOVER, fRXPARITY, fTXFULL;
    BOOL    fBREAK, fDNS, fFRAME, fIOE, fMODE;
#endif /* _WIN32 */

    if( !serialComPort ) return( -1 );
    if( !buffer ) return( -2 );

/* read() for Win32 systems */
#if defined(_WIN32)

    /* Get and clear current errors on the port */
	if( !ClearCommError(serialComPort->handle, &dwErrors, &comStat) ) return( -4 );

    /* Get error flags. */
    fDNS      = dwErrors & CE_DNS;

    fIOE      = dwErrors & CE_IOE;
    fOOP      = dwErrors & CE_OOP;
    fPTO      = dwErrors & CE_PTO;
    fMODE     = dwErrors & CE_MODE;
    fBREAK    = dwErrors & CE_BREAK;
    fFRAME    = dwErrors & CE_FRAME;
    fRXOVER   = dwErrors & CE_RXOVER;
    fTXFULL   = dwErrors & CE_TXFULL;
    fOVERRUN  = dwErrors & CE_OVERRUN;
    fRXPARITY = dwErrors & CE_RXPARITY;

	if( fIOE ) fprintf( stderr, "\nfIOE\n" );
	if( fOOP ) fprintf( stderr, "\nfOOP\n" );
	if( fPTO ) fprintf( stderr, "\nfPTO\n" );
	if( fMODE ) fprintf( stderr, "\nfMODE\n" );
	if( fBREAK ) fprintf( stderr, "\nfBREAK\n" );
	if( fFRAME ) fprintf( stderr, "\nfFRAME\n" );
	if( fRXOVER ) fprintf( stderr, "\nfRXOVER\n" );
	if( fTXFULL ) fprintf( stderr, "\nfTXFULL\n" );
	if( fOVERRUN ) fprintf( stderr, "\nfOVERRUN\n" );
	if( fRXPARITY ) fprintf( stderr, "\nfRXPARITY\n" );


	// COMSTAT structure contains information regarding
    // communications status.
    if (comStat.fCtsHold) fprintf( stderr, "\nfCtsHold\n" );
        // Tx waiting for CTS signal

    if (comStat.fDsrHold) fprintf( stderr, "\nfDsrHold\n" );
        // Tx waiting for DSR signal

    if (comStat.fRlsdHold) fprintf( stderr, "\nfRlsdHold\n" );
        // Tx waiting for RLSD signal

    if (comStat.fXoffHold) fprintf( stderr, "\nfXoffHold\n" );
        // Tx waiting, XOFF char rec'd

    if (comStat.fXoffSent) fprintf( stderr, "\nfXoffSent\n" );
        // Tx waiting, XOFF char sent
    
    if (comStat.fEof) fprintf( stderr, "\nfEof\n" );
        // EOF character received
    
    if (comStat.fTxim) fprintf( stderr, "\nfTxim\n" );
        // Character waiting for Tx; char queued with TransmitCommChar

	//if (comStat.cbInQue) fprintf( stderr, " %d ", comStat.cbInQue );
        // comStat.cbInQue bytes have been received, but not read

    if (comStat.cbOutQue) fprintf( stderr, "\ncbOutQue\n" );
        // comStat.cbOutQue bytes are awaiting transfer

	fflush( stderr );

    returnSuccess = ReadFile( serialComPort->handle, buffer, bufferSize-1,
                              &dwBytesRead, NULL );
    if( !returnSuccess ) return( -3 );
    buffer[bufferSize-1] = '\0';
    bytesRead = (int)dwBytesRead;

/* read() for non-Win32 systems */
#else /* !_WIN32 */

	bytesRead = read(serialComPort->handle, buffer, bufferSize - 1);
	
	// an error was found, so let's figure out which one
	if(bytesRead == -1){
		switch(errno){
			case EIO:			// I/O error
			case EBADF:			// invalid file descriptor, or fp was not opened in read mode
			case EINVAL:		// unreadable object
				return -3;
			case EAGAIN:		// no data available for reading
			default:
				bytesRead = 0;
				break;
		}
	}
	
	buffer[bufferSize - 1] = '\0';

#endif /* !_WIN32 */

	/* If readLog is defined, write the bytes read out to the log */
    if( serialComPort->readLog && bytesRead ) {
#if defined(_WIN32_WCE)
#else /* !_WIN32_WCE */
        /*
        FTIME( &msTime );
        fprintf( serialComPort->readLog, "%d.%03hu",
                 (int)msTime.time, msTime.millitm );
         */
#endif /* !_WIN32_WCE */

        for( i=0; i<bytesRead; i++ ) {
            fprintf( serialComPort->readLog, " %02X", buffer[i] & 0xFF );
        }
        fprintf( serialComPort->readLog, "\n" );
        fflush( serialComPort->readLog );
    } /* end "Write bytes out to the readLog..." */

    /* Return the number of bytes successfully read */
    return( bytesRead );

} /* end SERIALCOM_read() */


/**
 * See header file for interface documentation.
 */
int
SERIALCOM_write( SerialComPort *serialComPort, char *buffer, int bufferSize ) {

    int bytesWritten = 0;

#if defined(_WIN32)
    int     returnSuccess = 0;
    DWORD   dwBytesWritten = 0;

    COMSTAT comStat;
    DWORD   dwErrors;
    BOOL    fOOP, fOVERRUN, fPTO, fRXOVER, fRXPARITY, fTXFULL;
    BOOL    fBREAK, fDNS, fFRAME, fIOE, fMODE;
#endif /* _WIN32 */

    if( !serialComPort ) return( -1 );
    if( !buffer ) return( -2 );

/* write() for Win32 systems */
#if defined(_WIN32)

    /* Get and clear current errors on the port */
	if( !ClearCommError(serialComPort->handle, &dwErrors, &comStat) ) return( -4 );

    /* Get error flags. */
    fDNS      = dwErrors & CE_DNS;
    fIOE      = dwErrors & CE_IOE;
    fOOP      = dwErrors & CE_OOP;
    fPTO      = dwErrors & CE_PTO;
    fMODE     = dwErrors & CE_MODE;
    fBREAK    = dwErrors & CE_BREAK;
    fFRAME    = dwErrors & CE_FRAME;
    fRXOVER   = dwErrors & CE_RXOVER;
    fTXFULL   = dwErrors & CE_TXFULL;
    fOVERRUN  = dwErrors & CE_OVERRUN;
    fRXPARITY = dwErrors & CE_RXPARITY;

    returnSuccess = WriteFile( serialComPort->handle, buffer, bufferSize,
                               &dwBytesWritten, NULL );
    if( !returnSuccess ) return( -3 );
    bytesWritten = (int)dwBytesWritten;

/* write() for non-Win32 systems */
#else /* !_WIN32 */
	bytesWritten = write(serialComPort->handle, buffer, bufferSize);
	
   if(bytesWritten == -1)
	   return -3;

#endif /* !_WIN32 */

    /* Return the number of bytes successfully written */
    return( bytesWritten );
}

/**
 * See header file for interface documentation.
 */
int
SERIALCOM_purge( SerialComPort *serialComPort ) {

#if defined(_WIN32)
    /* Clear any existing contents of the port's read/write buffers */
    PurgeComm( serialComPort->handle,
               PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT );
#else
	tcflush(serialComPort->handle, TCIOFLUSH);
#endif

    return( 0 );
}


/**
 * See header file for interface documentation.
 */
int
SERIALCOM_destroySerialComPort( SerialComPort *serialComPort ) {

	int    errCode = 0;
	/*
	char   msg[256];
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
	LPTSTR lpszFunction;
    DWORD  dw = 0;
	*/

    if( !serialComPort ) return( -1 );

#if defined(_WIN32)
    errCode = CloseHandle( serialComPort->handle );
	/*
	sprintf( msg, "CloseHandle() returned %d.", errCode );
	MessageBox( NULL, msg, "SerialComPort", 0 );
	if( errCode == 0 ) {
		dw = GetLastError(); 
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) &lpMsgBuf,
			0, NULL );

	    // Display the error message and exit the process
		lpszFunction = TEXT( "CloseHandle" );

	    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
					   (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
	    StringCchPrintf( (LPTSTR)lpDisplayBuf, 
						 LocalSize(lpDisplayBuf) / sizeof(TCHAR),
						 TEXT("%s failed with error %d: %s"), 
						 lpszFunction, dw, lpMsgBuf ); 
		MessageBox( NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK ); 

	    LocalFree( lpMsgBuf );
	    LocalFree( lpDisplayBuf );
	}
	*/
#else /* !_WIN32 */
	close(serialComPort->handle);
#endif /* !_WIN32 */

    serialComPort->baudrate = 0;
    if( serialComPort->portName ) {
        free( serialComPort->portName );
        serialComPort->portName = NULL;
    }

    return( 0 );
}
