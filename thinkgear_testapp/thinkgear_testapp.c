#include <stdlib.h>
#include <stdio.h>
#include "library/thinkgear.h"
#include "library/resampler.h"
typedef int boolean;
enum booleanVar{ FALSE_ , TRUE_ };

/**
* Prompts and waits for the user to press ENTER.
*/
void
	wait() {
		printf( "\n" );
		printf( "Press the ENTER key...\n" );
		fflush( stdout );
		getc( stdin );
}

///**
//* Program which prints ThinkGear EEG_POWERS values to stdout.
//*/
//int
//	main( void ) {
//
//		char *comPortName = NULL;
//		int   dllVersion = 0;
//		int   connectionId = 0;
//		int   packetsRead = 0;
//		int   errCode = 0;
//		int set_filter_flag = 0;
//		int count = 0;
//		int i=0;
//
//		/* send 0xAA 0xAA 0x04 0x00 0x32 0xF9 0xE0 0x00 0x62 0xC9 to BMD 200*/
//		boolean hasbeenSendBytes = FALSE_;
//		char sendBuffer[10];
//		int buf[10]={0xAA ,0xAA ,0x04 ,0x00 ,0x32 ,0xF9 ,0xE0 ,0x00 ,0x62 ,0xC9};
//		
//		/* Print driver version number */
//		dllVersion = TG_GetVersion();
//		printf( "ThinkGear DLL version: %d\n", dllVersion );
//
//		/* Get a connection ID handle to ThinkGear */
//		connectionId = TG_GetNewConnectionId();
//		printf( "connectionId: %d\n", connectionId );
//		if( connectionId < 0 ) {
//
//			fprintf( stderr, "ERROR: TG_GetNewConnectionId() returned %d.\n", 
//				connectionId );
//			wait();
//			exit( EXIT_FAILURE );
//		}
//
//		/* Set/open stream (raw bytes) log file for connection */
//		errCode = TG_SetStreamLog( connectionId, "streamLog.txt" );
//		printf( "errCode: %d for TG_SetStreamLog\n", errCode );
//		if( errCode < 0 ) {
//			fprintf( stderr, "ERROR: TG_SetStreamLog() returned %d.\n", errCode );
//			wait();
//			exit( EXIT_FAILURE );
//		}
//
//		/* Set/open data (ThinkGear values) log file for connection */
//		errCode = TG_SetDataLog( connectionId, "dataLog.txt" );
//		printf( "errCode: %d for TG_SetDataLog\n", errCode );
//		if( errCode < 0 ) {
//			fprintf( stderr, "ERROR: TG_SetDataLog() returned %d.\n", errCode );
//			wait();
//			exit( EXIT_FAILURE );
//		}
//
//		/* Attempt to connect the connection ID handle to serial port "COM5" */
//		comPortName = "\\\\.\\COM18";
//		errCode = TG_Connect( connectionId, 
//			comPortName, 
//			TG_BAUD_57600, 
//			TG_STREAM_PACKETS );
//
//		/* convert buf[] for 'int' to 'char',then send buffer to BMD 200*/		
//		for(;i<10;i++){
//			sendBuffer[i] =  (char)(buf[i] & 0xFF);
//		}
//
//		printf( "errCode: %d for TG_Connect\n", errCode );
//		if( errCode < 0 ) {
//			fprintf( stderr, "ERROR: TG_Connect() returned %d.\n", errCode );
//			wait();
//			exit( EXIT_FAILURE );
//		}
//
//		/* Read 10 ThinkGear Packets from the connection, 1 Packet at a time */
//		packetsRead = 0;
//		while( packetsRead < 10 ) {
//
//			/* Attempt to read a Packet of data from the connection */
//			errCode = TG_ReadPackets( connectionId, 1 );
//
//			/* If TG_ReadPackets() was able to read a complete Packet of data... */
//			if( errCode == 1 ) {
//				packetsRead++;
//
//				//if(TG_GetValueStatus(connectionId, MWM15_DATA_FILTER_TYPE) != 0 ) {
//				//             /* Get and print out the updated raw value */
//				//             printf("Find Filter type: %d  \n",(int)TG_GetValue(connectionId, MWM15_DATA_FILTER_TYPE));
//				//         }
//
//				/* If 'TG_DATA_RAW' value has been updated by TG_ReadPackets()... */
//				if( TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0 ) {
//
//					/* Get and print out the updated attention value */
//					fprintf( stdout, "New RAW value: %d\n",
//						(int)TG_GetValue(connectionId, TG_DATA_RAW) );
//					fflush( stdout );
//
//				} /* end "If raw value has been updated..." */
//
//				/* If 'TG_DATA_CON_OUT_4' value has been updated by TG_ReadPackets()... */
//				if ( TG_GetValueStatus(connectionId, TG_DATA_CON_OUT_4) != 0 && !hasbeenSendBytes) {
//					/* send bytes to chip,the TYPE_2 reset data reported from BMD 200 Starter Kit:
//					 "AA AA 10 84 07 60 F9 31 1C 4D 62 00 08 49 85 03 FF FF FF 49 "
//					*/
//					fprintf( stdout, "Get conOut4 = 0x%04x, couOut3 = 0x%04x\n",
//						(int)TG_GetValue(connectionId, TG_DATA_CON_OUT_4),(int)TG_GetValue(connectionId,TG_DATA_COU_OUT_3) );
//
//					errCode = TG_SendBytes(connectionId,sendBuffer,10);
//					if(errCode != 0){
//						printf( " send byte error,errCode = %d \n",errCode );
//						return;
//					}
//					else{
//						hasbeenSendBytes = TRUE;
//					}
//
//				} /* end "If conOut4 has been updated..."*/
//			}else {
//				//printf( "read packets error ,errCode = %d \n",errCode );
//
//			} /* end "If a Packet of data was read..." */
//
//		} /* end "Read 10 Packets of data from connection..." */
//
//		//printf("auto read test begin:\n");
//		//fflush(stdin);
//		//errCode = TG_EnableAutoRead(connectionId,1);
//		//if(errCode == 0){
//		//	packetsRead =0;
//		//	errCode = MWM15_setFilterType(connectionId,MWM15_FILTER_TYPE_60HZ);//MWM15_FILTER_TYPE_60HZ
//		//	printf("MWM15_setFilterType: %d\n",errCode);
//		//	while(packetsRead <3000){
//		//		/* If raw value has been updated ... */
//		//		if( TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0 ) {
//		//			/* Get and print out the updated raw value */
//		//			if(packetsRead % 100 == 0){
//		//				printf(  " %d ",
//		//					(int)TG_GetValue(connectionId, TG_DATA_RAW) );
//		//			}else{
//		//				TG_GetValue(connectionId, TG_DATA_RAW);
//		//			}
//		//			//fflush( stdout );
//		//			//TG_GetValue(connectionId, TG_DATA_RAW);
//		//			packetsRead ++;
//		//			//wait for a while to call MWM15_getFilterType
//		//			if(packetsRead == 600){// at lease 1s after MWM15_setFilterType cmd
//		//				set_filter_flag ++;
//		//				errCode = MWM15_getFilterType(connectionId);
//		//				printf(  " \n MWM15_getFilterType   result: %d  index: %d\n",errCode,packetsRead );
//		//			}
//		//		}
//		//		if(TG_GetValueStatus(connectionId, MWM15_DATA_FILTER_TYPE) != 0 ) {
//
//		//			/* Get and print out the updated raw value */
//		//			printf(  "\n#### @@@ Find Filter type: %d  set_filter_flag: %d  index: %d\n",
//		//				(int)TG_GetValue(connectionId, MWM15_DATA_FILTER_TYPE), set_filter_flag,packetsRead);
//
//		//		}
//		//	}
//
//		//	errCode = TG_EnableAutoRead(connectionId,0); //stop
//		//	printf("auto read test stoped: %d \n",errCode);
//		//}else{
//		//	printf("auto read test failed: %d \n",errCode);
//		//}
//
//		TG_Disconnect(connectionId); // disconnect test
//
//		/* Clean up */
//		TG_FreeConnection( connectionId );
//
//		/* End program */
//		wait();
//		return( EXIT_SUCCESS );
//}


typedef void (*event_cb_t)(const struct event *evt, void *userdata);
int event_cb_register(event_cb_t cb, void *userdata);
static void my_event_cb(const struct event *evt, void *data)
{
    /* do stuff and things with the event */
}

int eventRegister(TestProgressCallback cb, int *userdata);

void progresscallback(int v)
{
	printf("\n Progress call back v= %d ",v);
}


int	main( void ) {

	FILE *fp;
	char ch;
	if((fp=fopen("C:\\Users\\chear\\Desktop\\BMD200\\source project\\reSampleDemo\\reSampleDemo\\bin\\Debug\\testcallback.txt","rt"))==NULL){
		printf("\nCannot open file strike any key exit!");
		wait();
		exit(1);
	}
	ch=fgetc(fp);
	while(ch!=EOF){
		putchar(ch);
		ch=fgetc(fp);
		//DoWork(&progresscallback);
		//eventRegister(TestProgressCallback, &progresscallback);

		//event_cb_register(progresscallback, 2);
	}	

	fclose(fp);
	wait();
	return( EXIT_SUCCESS );
}