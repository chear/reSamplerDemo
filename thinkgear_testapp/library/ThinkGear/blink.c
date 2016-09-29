/*
*Dyana Blink Algorithm
*Update 2
*11.12.10
*/



#include "blink.h"
#include <stdio.h>
#include <math.h>

#define ENABLE_INVERTED_BLINKS  1
#define ENABLE_MEAN_VARIABILITY 1
#define DEBUG                   0

/* Defines the Blink States*/
#define NO_BLINK                0
#define NORMAL_BLINK_UPPER      1
#define NORMAL_BLINK_LOWER      2
#define NORMAL_BLINK_VERIFY     3
#define INVERTED_BLINK_LOWER    4
#define INVERTED_BLINK_UPPER    5
#define INVERTED_BLINK_VERIFY   6

#define SHIFTING_TERM           4

#define BUFFER_SIZE 512

unsigned short i              = 0;
unsigned short  bufferCounter = 0;
signed short   buffer[BUFFER_SIZE];		//initialize an array of size BUFFER_SIZE

#define DATA_MEAN                33
#define POS_VOLT_THRESHOLD       230 //DATA_MEAN+265
#define NEG_VOLT_THRESHOLD      -200 //DATA_MEAN-265
#define DISTANCE_THRESHOLD       120

#define INNER_DISTANCE_THRESHOLD 45

#define MAX_LEFT_RIGHT           25

#define MEAN_VARIABILITY         200
#define BLINK_LENGTH             50
#define MIN_MAX_DIFF             500

#define POORSIGNAL_THRESHOLD     51

unsigned char  volatile blinkState = NO_BLINK;		//initialize the variable "blinkState" to NO_BLINK

//initialize various variables
signed short blinkStart = -1;
signed short outerLow   = -1;
signed short innerLow   = -1;
signed short innerHigh  = -1;
signed short outerHigh  = -1;
signed short blinkEnd   = -1;

signed short maxValue = 0;
signed short minValue = 0;

signed short blinkStrength = 0;

double meanVariablityThreshold = 0;
double average                 = 0;

/* FILE *out = NULL;	//FOR DEBUGGING */


unsigned char
BLINK_detect(unsigned char poorSignalQualityValue, signed short eegValue  )
{

#if DEBUG
	if( !out ) {
		out = fopen( "debug.txt", "w" );
		if( !out ) {
			exit( 0 );
		}
	}
#endif

	if(poorSignalQualityValue < POORSIGNAL_THRESHOLD)	//if poorsignal is less than POORSIGNAL_THERSHOLD, continue with algorithm
	{
		//update the buffer with the latest eegValue
		for(i = 0; i < BUFFER_SIZE - 1; i++)
		{
			buffer[i] = buffer[i+1];
		}
		buffer[BUFFER_SIZE - 1] = eegValue;

		//Counting the number of points in the buffer to make sure you have 512
		if( bufferCounter < 512 )
		{
			bufferCounter++;
		}

		if( bufferCounter > BUFFER_SIZE - 1 )	//if the buffer is full (it has BUFFER_SIZE points)
		{
			switch(blinkState)
			{
			case( NO_BLINK ):

				if(eegValue > POS_VOLT_THRESHOLD)
				{
					blinkStart = -1;
					innerLow   = -1;
					innerHigh  = -1;
					outerHigh  = -1;
					blinkEnd   = -1;

					outerLow = BUFFER_SIZE - 1;
					maxValue = eegValue;
					blinkState = NORMAL_BLINK_UPPER;

#if DEBUG
					fprintf( stdout, "NORMAL_BLINK_UPPER (outerLow:%d) \n", buffer[outerLow]);
					fflush( stdout );
#endif
				}

#if ENABLE_INVERTED_BLINKS

				if(eegValue < NEG_VOLT_THRESHOLD)
				{
					blinkStart = -1;
					innerLow   = -1;
					innerHigh  = -1;
					outerHigh  = -1;
					blinkEnd   = -1;

					outerLow   = BUFFER_SIZE - 1;
					minValue   = eegValue;
					blinkState = INVERTED_BLINK_LOWER;
#if DEBUG
					fprintf( stdout, "INVERTED_BLINK_LOWER (outerLow:%d) \n", buffer[outerLow]);
					fflush( stdout );
#endif
				}
#endif
				break;

			case( NORMAL_BLINK_UPPER ):

				//Monitors the DISTANCE_THRESHOLD
				if( ((BUFFER_SIZE - 1) - outerLow) > DISTANCE_THRESHOLD || outerLow < 1 )
				{
					blinkState = NO_BLINK;
#if DEBUG
					fprintf( stdout, "FAILED DISTANCE_THRESHOLD in NORMAL_BLINK_UPPER. \n");
					fflush( stdout );
#endif
					return 0;
				}

				outerLow--;		//decrement the index of outerlow to account for shifting of the buffer

				//Monitors the innerLow value.
				if(eegValue <= POS_VOLT_THRESHOLD && buffer[BUFFER_SIZE - 2] > POS_VOLT_THRESHOLD)	//if the current value is less than POS_VOLT_THRESH and the previous value is greater than POS_VOLT_THRESH
				{
					innerLow = BUFFER_SIZE - 2;		//then innerLow is defined to be the previous value
				}else
				{
					innerLow--;
				}

				//Monitors the maximum value
				if(eegValue > maxValue) maxValue = eegValue;

				//When it hits the negative threshold, set that to be the innerHigh and set the state to NORMAL_BLINK_LOWER.
				if(eegValue < NEG_VOLT_THRESHOLD)	//if we are below the NEG_VOLT_THRESH
				{
					innerHigh = BUFFER_SIZE - 1;	//innerHigh is the current value
					minValue = eegValue;

					//Verify the INNER_DISTANCE_THRESHOLD
					if ( (innerHigh - innerLow) < INNER_DISTANCE_THRESHOLD )	//if the distance btwn innerHigh and innerLow isn't too long
					{
						blinkState = NORMAL_BLINK_LOWER;
#if DEBUG
						fprintf( stdout, "NORMAL_BLINK_LOWER (outerLow:%d) \n", buffer[outerLow]);
						fflush( stdout );
#endif
					}
					else		//otherwise the distance btwn innerHigh and innerLow is too much and it wasn't actually a blink
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED INNER_DISTANCE_THRESHOLD_NORMAL. inner distance = %d)\n", innerHigh - innerLow);
						fflush( stdout );
#endif
					}
				}

				break;

			case( INVERTED_BLINK_LOWER ):

				//Monitors the DISTANCE_THRESHOLD
				if( ((BUFFER_SIZE - 1) - outerLow) > DISTANCE_THRESHOLD || outerLow < 1 )
				{
					blinkState = NO_BLINK;
#if DEBUG
					fprintf( stdout, "FAILED DISTANCE_THRESHOLD in INVERTED_BLINK_LOWER \n");
					fflush( stdout );
#endif
					return 0;
				}

				outerLow--;

				//Monitors the innerLow value.
				if(eegValue >= NEG_VOLT_THRESHOLD && buffer[BUFFER_SIZE - 2] < NEG_VOLT_THRESHOLD)
				{
					innerLow = BUFFER_SIZE - 2;
				}else
				{
					innerLow--;
				}

				//Monitors the minimum value
				if(eegValue < minValue) minValue = eegValue;

				//When it hits the positive threshold, set that to be innerHigh and set the state to INVERTED_BLINK_UPPER.
				if(eegValue > POS_VOLT_THRESHOLD)
				{
					innerHigh = BUFFER_SIZE - 1;
					maxValue = eegValue;

					//Verify the INNER_DISTANCE_THRESHOLD
					if(innerHigh - innerLow < INNER_DISTANCE_THRESHOLD)
					{
						blinkState = INVERTED_BLINK_UPPER;
#if DEBUG
						fprintf( stdout, "INVERTED_BLINK_UPPER (outerLow:%d) \n", buffer[outerLow]);
						fflush( stdout );
#endif
					}
					else
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED INNER_DISTANCE_THRESHOLD_INVERTED. inner distance = %d)\n", innerHigh - innerLow);
						fflush( stdout );
#endif
					}
				}

				break;

			case( NORMAL_BLINK_LOWER ):

				outerLow--;
				innerLow--;
				innerHigh--;

				//Monitors the outerHigh value.
				if(eegValue >= NEG_VOLT_THRESHOLD && buffer[BUFFER_SIZE - 2] < NEG_VOLT_THRESHOLD)	//if the current value is greater than NEG_VOLT_THRESH and the previous value is less than NEG_VOLT_THRESH
				{
					outerHigh = BUFFER_SIZE - 2;		//then the previous value is defined to be outerHigh
					blinkState = NORMAL_BLINK_VERIFY;
#if DEBUG
					fprintf( stdout, "NORMAL_BLINK_VERIFY (outerLow:%d) \n", buffer[outerLow]);
					fflush( stdout );
#endif
				}else
				{
					outerHigh--;
				}

				//Monitors the minimum value
				if(eegValue < minValue) minValue = eegValue;

				//Monitors the DISTANCE_THRESHOLD
				if( ((BUFFER_SIZE - 1) - outerLow) > DISTANCE_THRESHOLD ) //if the distance from the current point to outerLow is greater than DIST_THRESH
				{
					outerHigh = BUFFER_SIZE - 1;
					blinkState = NORMAL_BLINK_VERIFY;
#if DEBUG
					fprintf( stdout, "NORMAL_BLINK_VERIFY at DISTANCE_THRESHOLD (outerLow:%d) \n", buffer[outerLow]);
					fflush( stdout );
#endif
				}

				break;

			case( INVERTED_BLINK_UPPER ):

				outerLow--;
				innerLow--;
				innerHigh--;

				//Monitors the outerHigh value.
				if( (eegValue <= POS_VOLT_THRESHOLD) && (buffer[BUFFER_SIZE - 2] > POS_VOLT_THRESHOLD) )		//if the current value is less than POS_VOLT_THRESH and the previous value is greater than POS_VOLT_THRESH
				{
					outerHigh = BUFFER_SIZE - 2;			//then the previous value is defined as outerHigh
					blinkState = INVERTED_BLINK_VERIFY;
#if DEBUG
					fprintf( stdout, "INVERTED_BLINK_VERIFY (outerLow:%d) \n", buffer[outerLow]);
					fflush( stdout );
#endif
				}else
				{
					outerHigh--;
				}

				//Monitors the maximum value
				if(eegValue > maxValue) maxValue = eegValue;

				//Monitors the DISTANCE_THRESHOLD
				if( ((BUFFER_SIZE - 1) - outerLow) > DISTANCE_THRESHOLD )
				{
					outerHigh = BUFFER_SIZE - 1;
					blinkState = INVERTED_BLINK_VERIFY;
#if DEBUG
					fprintf( stdout, "INVERTED_BLINK_VERIFY at DISTANCE_THRESHOLD (outerLow:%d) \n", buffer[outerLow]);
					fflush( stdout );
#endif
				}
				break;

			case( NORMAL_BLINK_VERIFY ):

				outerLow--;
				innerLow--;
				innerHigh--;

				if(eegValue < NEG_VOLT_THRESHOLD) //if the current value is less than NEG_VOLT_THRES
				{
					blinkState = NORMAL_BLINK_LOWER;
				}else
				{
					outerHigh--;
				}

				//Set the endBlink to when it hits the mean or it hits MAX_LEFT_RIGHT.
				if( ((BUFFER_SIZE - 1) - outerHigh > MAX_LEFT_RIGHT) || (eegValue > DATA_MEAN) )
				{
					blinkEnd = BUFFER_SIZE - 1;
				}

				//Checks if the value is back at the DATA_MEAN
				if(blinkEnd > 0)
				{
					//Verifies the Blink
					//Sets the blinkStart to when it hits the mean or it hits MAX_LEFT_RIGHT.
					for(i = 0; i < MAX_LEFT_RIGHT; i++)
					{

						blinkStart = outerLow - i;

						if(buffer[outerLow - i] < DATA_MEAN)
						{
							break;
						}
					}

					//Verify the MIN_MAX_DIFF
					blinkStrength = maxValue - minValue;
					fflush( stdout );
					if(blinkStrength < MIN_MAX_DIFF)
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED MIN_MAX_DIFF. (BlinkStrength: %d)\n", blinkStrength);
						fflush( stdout );
#endif
						return 0;
					}

#if ENABLE_MEAN_VARIABILITY
					//Verify the MEAN_VARIABILITY
					meanVariablityThreshold = blinkStrength/993*MEAN_VARIABILITY;
					average = 0;
					for(i = blinkStart; i < blinkEnd + 1; i++)
					{
						average += buffer[i];
					}
					average /= (blinkEnd - blinkStart + 1);
					average = fabs(average);

					if(average > MEAN_VARIABILITY)
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED MEAN_VARIABILITY. Mean Variability: %f\n", average);
						fflush( stdout );
#endif
						return 0;
					}
#endif
					//Verify the BLINK_LENGTH
					if(blinkEnd - blinkStart < BLINK_LENGTH)
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED BLINK_LENGTH. (Blink Length: %f)\n", blinkEnd - blinkStart);
						fflush( stdout );
#endif
						return 0;
					}

					//verify that blinkStart is between POS_VOLT_THRESHOLD and NEG_VOLT_THRESHOLD
					if( (buffer[blinkStart] > POS_VOLT_THRESHOLD) || (buffer[blinkStart] < NEG_VOLT_THRESHOLD) )
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED BLINKSTART. (blinkStart: %d)\n", blinkStart);
						fflush( stdout );
#endif
						return 0;
					}

					//verify that blinkEnd is between POS_VOLT_THRESHOLD and NEG_VOLT_THRESHOLD
					if( (buffer[blinkEnd] > POS_VOLT_THRESHOLD) || (buffer[blinkEnd] < NEG_VOLT_THRESHOLD) )
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED BLINKEND. (blinkEnd: %d)\n", blinkEnd);
						fflush( stdout );
#endif
						return 0;
					}

					blinkState = NO_BLINK;
					return (unsigned char)(blinkStrength>>SHIFTING_TERM);  /* ksoo Mar 04, 2011: Added cast, but is this correct? */
				}

				break;

			case( INVERTED_BLINK_VERIFY ):

				outerLow--;
				innerLow--;
				innerHigh--;

				if(eegValue > POS_VOLT_THRESHOLD)
				{
					blinkState = INVERTED_BLINK_UPPER;
				}else
				{
					outerHigh--;
				}

				//Set the endBlink to when it hits the mean or it hits MAX_LEFT_RIGHT.
				if( ((BUFFER_SIZE - 1) - outerHigh > MAX_LEFT_RIGHT) || (eegValue < DATA_MEAN) )
				{
					blinkEnd = BUFFER_SIZE - 1;
				}

				//Checks if the value is back at the DATA_MEAN
				if(blinkEnd > 0)
				{
					//Verifies the Blink
					//Sets the blinkStart to when it hits the mean or it hits MAX_LEFT_RIGHT.
					for(i = 0; i < MAX_LEFT_RIGHT; i++)
					{
						blinkStart = outerLow - i;

						if(buffer[outerLow - i] > DATA_MEAN)
						{
							break;
						}
					}

					//Verify the MIN_MAX_DIFF
					blinkStrength = maxValue - minValue;
					fflush( stdout );
					if(blinkStrength < MIN_MAX_DIFF)
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED MIN_MAX_DIFF. (BlinkStrength: %d)\n", blinkStrength);
						fflush( stdout );
#endif
						return 0;
					}

#if ENABLE_MEAN_VARIABILITY

					//Verify the MEAN_VARIABILITY
					meanVariablityThreshold = blinkStrength/993*MEAN_VARIABILITY;
					average = 0;
					for(i = blinkStart; i < blinkEnd + 1; i++)
					{
						average += buffer[i];
					}
					average /= (blinkEnd - blinkStart + 1);
					average = fabs(average);

					if(average > MEAN_VARIABILITY)
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED MEAN_VARIABILITY. Mean Variability: %f\n", average);
						fflush( stdout );
#endif
						return 0;
					}
#endif
					//Verify the BLINK_LENGTH
					if(blinkEnd - blinkStart < BLINK_LENGTH)
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED BLINK_LENGTH. (Blink Length: %f)\n", blinkEnd - blinkStart);
						fflush( stdout );
#endif
						return 0;
					}

					//verify that blinkStart is between POS_VOLT_THRESHOLD and NEG_VOLT_THRESHOLD
					if( (buffer[blinkStart] > POS_VOLT_THRESHOLD) || (buffer[blinkStart] < NEG_VOLT_THRESHOLD) )
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED BLINKSTART. (blinkStart: %d)\n", blinkStart);
						fflush( stdout );
#endif
						return 0;
					}

					//verify that blinkEnd is between POS_VOLT_THRESHOLD and NEG_VOLT_THRESHOLD
					if( (buffer[blinkEnd] > POS_VOLT_THRESHOLD) || (buffer[blinkEnd] < NEG_VOLT_THRESHOLD) )
					{
						blinkState = NO_BLINK;
#if DEBUG
						fprintf( stdout, "FAILED BLINKEND. (blinkEnd: %d)\n", blinkEnd);
						fflush( stdout );
#endif
						return 0;
					}

					blinkState = NO_BLINK;
					return (unsigned char)(blinkStrength>>SHIFTING_TERM);  /* ksoo Mar 04, 2011: Added cast, but is this correct? */
				}

				break;

			default:
				blinkState = NO_BLINK;

				break;
			}
		}

	}
	else	//poorsignal is greater than 51 and do not evaluate the algorithm
	{
		bufferCounter = 0;
		outerLow  = -1;
		innerLow  = -1;
		innerHigh = -1;
		outerHigh = -1;

		blinkState = NO_BLINK;
	}

	return 0;
}


unsigned char
ZONE_calculate( unsigned char attentionValue, unsigned char meditationValue ) {

	return ( (unsigned char)(0.85*attentionValue + 0.15*meditationValue) );

}
