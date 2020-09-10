/**************************************************************************/
/* VHALL Audio Mixing Project                                             */
/* Used to mixing multi-channels audio signal into one channel. When M    */
/* channels are input to this module, up to N channels mixed result could */
/* be output.(N = M+1) Now only short and float format data are available.*/
/* If you want to deal with other format, please convert the data's format*/
/* first.                                                                 */
/* Created BY Neil Xia, 2017-10-15                                          */
/* Version 1.0.0                                                          */
/**************************************************************************/

#ifndef VHALL_AUDIO_MIXING_H__
#define VHALL_AUDIO_MIXING_H__

#define MAX_CHANNEL_NUM 16	//support 16 channels data to mix together at most

#define PROCUNIT  20		//Decide how many number of samples will be processed.
							//If PROCUNIT = 40, then Samplerate/1000*PROCUNIT
							//sample points will be processed at once. Here 40
							//means 40ms mono data(or 20ms stereo data) .
							//NOTICE: PROCUNIT must be a multiplier of 10

//Data depth
#define DD_SHORT 0
#define DD_FLOAT 1

/**************************************************************************/
/* VhallAudioMixing(...)                                                  */
/*                                                                        */
/* Mixing different channels audio data                                   */
/*                                                                        */
/* Input:                                                                 */
/*     - inputData		: input data which storage format should be planar*/
/*                        11...1122...2233...33...nnnn...n                */
/*                        one input unit is a frame(10ms for mono or 5ms  */
/*						  for stereo)								      */
/*             (11...11 is the data of channel 1, and n channels in total)*/
/*     - channelNum		: Total number of input channels, and 8 is max    */
/*     - sampleRate		: sampling rate of input data, only 8000£¬ 32000  */
/*                        and 44100Hz is supported                        */
/*     - accuracyType	: type of input data                              */
/*                        0 - short, 1 - float                            */
/*	   - isSingleOut	: if only a single output, mixing all together    */
/*						  0 - complete output, 1 - single output		  */
/* Output:                                                                */
/*     - outputData		: storage format is same with input data          */
/*                        11...1122...2233...33...nnnn...nmmm...mm        */
/*                        1~n is the data for channel 1 to n, m is the    */
/*                        data that mixed all of channels together        */
/* Return value			: 0 - OK, -1 - Error                              */
/**************************************************************************/
int Vhall_AudioMixing(void *inputData, 
					 void *outputData,
					 int channelNum,
					 int sampleRate,
					 int accuracyType,
					 int isSingleOut );


#endif //VHALL_AUDIO_MIXING_H