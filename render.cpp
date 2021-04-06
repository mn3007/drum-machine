/*
 * assignment2_drums
 * ECS7012 Music and Audio Programming
 * Student ID: 161205285
 * Second assignment, to create a sequencer-based
 * drum machine which plays sampled drum sounds in loops.
 *
 * This code runs on the Bela embedded audio platform (bela.io).
 *
 * Andrew McPherson, Becky Stewart and Victor Zappi
 * 2015-2020
 */

#include <Bela.h>
#include <cmath>
#include "drums.h"

/* Variables which are given to you: */

/* Drum samples are pre-loaded in these buffers. Length of each
 * buffer is given in gDrumSampleBufferLengths.
 */
extern float *gDrumSampleBuffers[NUMBER_OF_DRUMS];
extern int gDrumSampleBufferLengths[NUMBER_OF_DRUMS];

int gIsPlaying = 0;			/* Whether we should play or not. Implement this in Step 4b. */

/* Read pointer into the current drum sample buffer.
 *
 * TODO (step 3): you will replace this with two arrays, one
 * holding each read pointer, the other saying which buffer
 * each read pointer corresponds to.
 */
//this array tells us which readpointer points to which buffer.
int gDrumBufferForReadPointer[NUMBER_OF_READPOINTERS];
//the actual read pointers
int gReadPointers[NUMBER_OF_READPOINTERS];

/* Patterns indicate which drum(s) should play on which beat.
 * Each element of gPatterns is an array, whose length is given
 * by gPatternLengths.
 */
extern int *gPatterns[NUMBER_OF_PATTERNS];
extern int gPatternLengths[NUMBER_OF_PATTERNS];

/* These variables indicate which pattern we're playing, and
 * where within the pattern we currently are. Used in Step 4c.
 */
int gCurrentPattern = 0;
int gCurrentIndexInPattern = 0;

/* This variable holds the interval between events in **milliseconds**
 * To use it (Step 4a), you will need to work out how many samples
 * it corresponds to.
 */
int gEventIntervalMilliseconds = 250; // 50 - 1000ms
int gEventInterval = 2205; // in samples 2205 - 44100

int printCounter = 0;
int boardCounter = 0;
int orientationCounter = 0;

/* This variable indicates whether samples should be triggered or
 * not. It is used in Step 4b, and should be set in gpio.cpp.
 */
extern int gIsPlaying;

/* This indicates whether we should play the samples backwards.
 */
int gPlaysBackwards = 0;

/* For bonus step only: these variables help implement a fill
 * (temporary pattern) which is triggered by tapping the board.
 */
int gShouldPlayFill = 0;
int gPreviousPattern = 0;

/* TODO: Declare any further global variables you need here */
const int kButtonPin2 = 2;
const int kButtonPin = 1;				
const int kLedPin = 0;
int gLastButtonStatus = HIGH;
//int gLastButtonStatus2 = HIGH;
const unsigned int kInputTempo = 0;

unsigned int gSequencerBuffer = 0;
unsigned int gSequencerLocation = 0;

unsigned int gMetronomeInterval = 11025; //Interval between events
unsigned int gMetronomeCounter = 0; //NUmber of elapsed samples
unsigned int gLedInterval = 0;

const unsigned int x = 1;
const unsigned int y = 2;
const unsigned int z = 3;

enum boardState {
	flat = 0,
	tiltLeft,
	tiltRight,
	tiltFront,
	tiltBack,
	upsideDown
};

// Initialise accelerometer state 
int boardState = flat;
float gThresholdHighpos = 9;
float gThresholdLowpos = 0.3;
float gThresholdHighneg = -9;
float gThresholdLowneg = -0.3;

//variable that keeps track of the beats of the bar (8 beats from 0 to 7)
int gCurrentBeat = 0;

// setup() is called once before the audio rendering starts.
// Use it to perform any initialisation and allocation which is dependent
// on the period size or sample rate.
//
// userData holds an opaque pointer to a data structure that was passed
// in from the call to initAudio().
//
// Return true on success; returning false halts the program.

bool setup(BelaContext *context, void *userData) {
	
	// Set every index in aray to zero
	for (unsigned int i=0; i < NUMBER_OF_READPOINTERS;i++) {
	
		gReadPointers[i]=-1;
	}
	for (unsigned int i=0; i < NUMBER_OF_PATTERNS; i++) {
		
		*gPatterns[i] = -1;
	}
	
//	gMetronomeInterval = 0.25 * context->audioSampleRate;
	gLedInterval = 0.05 * context -> audioSampleRate;


	// initialise GPIO pins 
	pinMode(context, 0, kButtonPin2, INPUT);
	pinMode(context, 0, kButtonPin, INPUT);
	pinMode(context, 0, kLedPin, OUTPUT);
	return true;
}

// render() is called regularly at the highest priority by the audio engine.
// Input and output are given from the audio hardware and the other
// ADCs and DACs (if available). If only audio is available, numMatrixFrames
// will be 0.

void render(BelaContext *context, void *userData) {
	/* TODO: your audio processing code goes here! */
    
    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	
    	// Reset audio buffer
    	float out = 0;
		// Read the digital input to start or stop playing
    	int status = digitalRead(context, n, kButtonPin);
		// Read the analog input to get the current tempo
		float input = analogRead(context, n/2, kInputTempo);
		float bpm = map(input, 0, 1, 1000, 50);
		
		// Read analog pins to get accelerometer values
		float X = analogRead(context, n/2, x);
		float Y = analogRead(context, n/2, y);
		float Z = analogRead(context, n/2, z);
		
		// Map accelerometer reading to acceleration
		float gX_mapped = map(X, 0, 0.805, -1, 1);
		float gY_mapped = map(Y, 0, 0.9, -1, 1);
		float gZ_mapped = map(Z, 0, 0.6, -1, 1);
		
		// Apply hysteresis
		if (gX_mapped < gThresholdLowpos && gX_mapped > gThresholdLowneg)
		     gX_mapped = 0;
		else if (gX_mapped >= gThresholdHighpos)
	         gX_mapped = 9.8;
		else if (gX_mapped <gThresholdHighneg)
	         gX_mapped = -9.8;	         
	         
		if (gY_mapped < gThresholdLowpos && gY_mapped > gThresholdLowneg)
		     gY_mapped = 0;
		else if (gY_mapped >= gThresholdHighpos)
	         gY_mapped = 9.8;
		else if (gX_mapped < gThresholdHighneg)
	         gY_mapped = -9.8;	 	         
	         
		if (gZ_mapped < gThresholdLowpos && gZ_mapped > gThresholdLowneg) {
		     gZ_mapped = 0;
		}     
		else if (gZ_mapped >= gThresholdHighpos) {
	         gZ_mapped = 9.8;
		}     
		else if (gZ_mapped < gThresholdHighneg) {
	         gZ_mapped = -9.8;	
	    }

	     // Detecting orientation
		     if (gX_mapped == 0 && gY_mapped == 0 && gZ_mapped > 0) {
//		     	rt_printf("Flat");
		     	gCurrentPattern = 0;
		     }
		     else if (gX_mapped < 0 && gY_mapped == 0 && gZ_mapped == 0) {
//		     	rt_printf("TiltLeft");
		     	gCurrentPattern = 1;
		     }
		     else if (gX_mapped > 0 && gY_mapped == 0 && gZ_mapped == 0) {
//		     	rt_printf("TiltRight");
		     	gCurrentPattern = 2;
		     }
		     else if (gX_mapped == 0 && gY_mapped < 0 && gZ_mapped == 0) {
//		     	rt_printf("TiltFront");
		     	gCurrentPattern = 3;
		     }
		     else if (gX_mapped == 0 && gY_mapped > 0 && gZ_mapped == 0) {
//		     	rt_printf("TiltBack");
		     	gCurrentPattern = 4;
		     }
		     else if (gX_mapped == 0 && gY_mapped == 0 && gZ_mapped < 0) {
//		     	rt_printf("upsideDown");
		     	gPlaysBackwards = 1;
		     }

	    	
		// Print accelerometer values to console
		if (printCounter++ == context->audioSampleRate*0.1) {		
	//		rt_printf("X: %f\n", gX_mapped);
   // 		rt_printf("Y: %f\n", gY_mapped);
   // 		rt_printf("Z: %f\n", gZ_mapped);
       		// reset to zero
			printCounter = 0;
		}
       	
   //     	// Button was pressed - advance the sequence
			// /* Step 2: use gReadPointer to play a drum sound */
       if(status == LOW && gLastButtonStatus == HIGH) {
       		if (gIsPlaying==0)
       	       gIsPlaying=1;
       		else
       			gIsPlaying=0;
       }

        gMetronomeInterval = 60.0 * context->audioSampleRate / bpm;

       
         gLastButtonStatus = status;
         
				if(++gMetronomeCounter >= gMetronomeInterval) {
				//counter reached the target
				gMetronomeCounter = 0; // Reset the counter	
				   if(gIsPlaying==1){
					 startNextEvent(); // start a new tick
				   }
				}

    			//we loop through gDrumBufferForReadPointer array, looking for a free readpointer (i.e. with value = -1)
    			for (int i=0;i<16;i++) {
    				
    			// Read pointers that are active have a value between 0 and the buffer length
    				if (gDrumBufferForReadPointer[i]>=0) {
    				// Move read pointers to the next frame
    				if (gReadPointers[i] < gDrumSampleBufferLengths[gDrumBufferForReadPointer[i]]) {
   						// read through the samples in audioOutputBuffer with the read pointer at index i, and add them to out
    					out += gDrumSampleBuffers[gDrumBufferForReadPointer[i]][gReadPointers[i]];
    					gReadPointers[i]++;
    				}
    					// If the read pointer reaches the limit of the buffer, deactivate it
    					else {
    					// Reset the read pointer
    					gReadPointers[i]=-1;
    					// Free the read pointer
    					gDrumBufferForReadPointer[i]=-1;
    					}
    				}
    			}

				// TODO: turn on the LED on if we are early enough in the tick
    			if(gMetronomeCounter < gLedInterval) {
    			digitalWriteOnce(context, n, kLedPin, HIGH);
    			} else {
    			digitalWriteOnce(context, n, kLedPin, LOW);
    			}
				
		//reduce amplitude to avoid clipping
		                out /= 2;

		// Write the sample to every audio output channel            
    	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
    		audioWrite(context, n, channel, out);
    	}  

    }
}



/* Start playing a particular drum sound given by drumIndex.
 */
void startPlayingDrum(int drumIndex) {
	/* TODO in Steps 3a and 3b */
	for (int i=0; i < 16; i++){
		//if readpointer "i" is not in use, that means that gDrumBufferForReadPointer[i]==-1
		if (gDrumBufferForReadPointer[i] == -1) {
			//we assign drum (buffer) with index "drumIndex" to readpointer "i"
			gDrumBufferForReadPointer[i]=drumIndex;
			// reset readpointer
			gReadPointers[i]=0;
			//exit for loop
			break;
		}

	}

}

/* Start playing the next event in the pattern */
void startNextEvent() {
	// If gPattern[i] is 1, that means drum "i" should be played
	for (int i = 0; i < NUMBER_OF_PATTERNS; i++){
		if (gPatterns[gCurrentPattern][gCurrentIndexInPattern]) {
			startPlayingDrum(i);
		}
		
	for (int j = 0; j < NUMBER_OF_DRUMS; j++) {
		//if this drum is contained in the current event of the chosen pattern, play it.
		int currentEvent = gPatterns[gCurrentPattern][gCurrentIndexInPattern];
		if (eventContainsDrum(currentEvent, j) == 1) {
			startPlayingDrum(j);
		}
	}		
	//update the beat index (between 0 and 15), and send it to the GUI
	gCurrentIndexInPattern++;
	if(gCurrentIndexInPattern > gPatternLengths[i] )
	gCurrentIndexInPattern = 0;
	}
}


/* Returns whether the given event contains the given drum sound */
int eventContainsDrum(int event, int drum) {
	if(event & (1 << drum))
		return 1;
	return 0;
}

// cleanup_render() is called once at the end, after the audio has stopped.
// Release any resources that were allocated in initialise_render().

void cleanup(BelaContext *context, void *userData)
{

}
