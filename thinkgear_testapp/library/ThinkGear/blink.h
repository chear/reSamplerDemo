/*
*This is a blink detection algorithm that is written in the format of a state machine. It is written to detect
*both regular eyeblinks as well as inverted eyeblinks. This algorithm is optimized for MindWave (not MindSet).
*Written by Dyana Szibbo, implemented by Masa Someha and modified by Neraj Bobra
*
*/

#ifndef BLINK_H
#define BLINK_H

#define ENABLE_BLINK_DETECTION_DEBUG 0

#if ENABLE_BLINK_DETECTION_DEBUG
extern unsigned char  volatile aboveCount; 
extern unsigned char  volatile blinkCount0; 
extern unsigned char  volatile blinkCount1; 
extern unsigned char  volatile blinkCount2; 
extern signed   short volatile blinkMin; 
extern signed   short volatile blinkMax; 
extern unsigned char  volatile blinkState; 
#endif

unsigned char
BLINK_detect(unsigned char poorSignalQualityValue, signed short eegValue  );

unsigned char
ZONE_calculate( unsigned char attentionValue, unsigned char meditationValue );

#endif /* BLINK_H */
