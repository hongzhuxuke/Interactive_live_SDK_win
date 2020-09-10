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

#include <string.h>
#include <stdio.h>
#include "VhallAudioMixing.h"

#define QDOMIN 16		 
#define LIMIT_MAX 32767
#define LIMIT_MIN -32768

static const int baseValue[5] = {
	0, 28672, 32256, 32704, 32760
};
static const int rightShift[5] = {
	3, 6, 9, 12, 15
};


int Vhall_AudioMixingCore(short *inputData, short *outputData, int channelNum, int dataLength, int isSingleOut){
	int sum;
	int i,j;
	int noDebuffData[MAX_CHANNEL_NUM + 1], noDebuffDataAbs[MAX_CHANNEL_NUM + 1], signFlag[MAX_CHANNEL_NUM + 1];
	int nLevel[MAX_CHANNEL_NUM + 1], temp1[MAX_CHANNEL_NUM + 1], temp2[MAX_CHANNEL_NUM + 1];
	short dataIn[MAX_CHANNEL_NUM][441 * PROCUNIT / 10], dataOut[MAX_CHANNEL_NUM + 1][441 * PROCUNIT / 10];

	if (!inputData || !outputData){
		return -1;
	}
	if (channelNum > MAX_CHANNEL_NUM || channelNum < 1){
		return -1;
	}
	else if (channelNum == 1){
		if (isSingleOut == 0){
			memset(outputData, 0, sizeof(short)*dataLength);
			memcpy(outputData + dataLength, inputData, sizeof(short)*dataLength);
		}
		else{
			memcpy(outputData, inputData, sizeof(short)*dataLength);
		}
		
		return 0;
	}
	if (dataLength < 1 || dataLength > 441 * PROCUNIT / 10){
		return -1;
	}
	//isSingleOut = !!isSingleOut;
	
	//copy input data into temp buffer
	for (i = 0; i < channelNum; i++){
		memcpy(dataIn[i], inputData + i * dataLength, sizeof(inputData[0])*dataLength);
	}
	
	//Big Loop
	for (j = 0; j < dataLength; j++){
		sum = 0;
		
		// sum and its absolute value
		for (i = 0; i < channelNum; i++){
			sum += dataIn[i][j];
		}
		if (sum < 0){
			signFlag[channelNum] = 1;
			noDebuffDataAbs[channelNum] = -sum;
		}
		else{
			signFlag[channelNum] = 0;
			noDebuffDataAbs[channelNum] = sum;
		}

		// the true mixing result for every participant
		if (isSingleOut == 0){
			for (i = 0; i < channelNum; i++){
				noDebuffData[i] = sum - dataIn[i][j];

				if (noDebuffData[i] < 0){
					signFlag[i] = 1;
					noDebuffDataAbs[i] = -noDebuffData[i];
				}
				else{
					signFlag[i] = 0;
					noDebuffDataAbs[i] = noDebuffData[i];
				}
			}
		}
		
		// nLevel:intensity level
		if (isSingleOut == 0){
			for (i = 0; i < channelNum; i++){
				nLevel[i] = noDebuffDataAbs[i] >> (QDOMIN - 1);
				if (nLevel[i] > 4){
					nLevel[i] = 4;
				}
			}
		}
		nLevel[channelNum] = noDebuffDataAbs[channelNum] >> (QDOMIN - 1);
		if (nLevel[channelNum] > 4){
			nLevel[channelNum] = 4;
		}
		

		//compress the data to prevent overflow
		if (isSingleOut == 0){
			for (i = 0; i < channelNum; i++){
				temp1[i] = noDebuffDataAbs[i] & LIMIT_MAX;
				temp2[i] = (temp1[i] << 2) + (temp1[i] << 1) + temp1[i];
				dataOut[i][j] = baseValue[nLevel[i]] + (temp2[i] >> rightShift[nLevel[i]]);
				if (signFlag[i] == 1){
					dataOut[i][j] = -dataOut[i][j];
				}
			}
		}

		temp1[channelNum] = noDebuffDataAbs[channelNum] & LIMIT_MAX;
		temp2[channelNum] = (temp1[channelNum] << 2) + (temp1[channelNum] << 1) + temp1[channelNum];
		dataOut[channelNum][j] = baseValue[nLevel[channelNum]] + (temp2[channelNum] >> rightShift[nLevel[channelNum]]);
		if (signFlag[channelNum] == 1){
			dataOut[channelNum][j] = -dataOut[channelNum][j];
		}
	}//end big loop

	//copy mixed data into output buffer
	if (isSingleOut == 0){
		for (i = 0; i < channelNum + 1; i++){
			memcpy(outputData + i * dataLength, dataOut[i], sizeof(short)*dataLength);
		}
	}
	else{
		memcpy(outputData, dataOut[channelNum], sizeof(short)*dataLength);		
	}
	

	return 0;
};

int Vhall_AudioMixing(void *inputData, void *outputData, int channelNum, int sampleRate, int accuracyType, int isSingleOut){
	short inputDataSafe[MAX_CHANNEL_NUM * 441 * PROCUNIT / 10];
	short outputDataSafe[(MAX_CHANNEL_NUM + 1) * 441 * PROCUNIT / 10];
	int i;
	int dataLength;
	int ret;
	int temp;
	
	if (!inputData || !outputData){
		return -1;
	}
	if (channelNum > MAX_CHANNEL_NUM || channelNum < 1){
		return -1;
	}
	if (sampleRate != 8000 && sampleRate != 16000 && sampleRate != 32000 && sampleRate != 44100){
		return -1;
	}
	if (accuracyType != DD_SHORT && accuracyType != DD_FLOAT){
		return -1;
	}

	dataLength = (sampleRate * PROCUNIT) / 1000;
	// If it is not short format, convert it!!
	if (accuracyType == DD_FLOAT){
		for (i = 0; i < channelNum * dataLength; i++){
			temp = ((float*)inputData)[i] * LIMIT_MAX;
			if (temp > LIMIT_MAX){
				inputDataSafe[i] = LIMIT_MAX;
			}
			else if (temp < LIMIT_MIN){
				inputDataSafe[i] = LIMIT_MIN;
			}
			else{
				inputDataSafe[i] = temp;
			}

		}
	}
	else{
		memcpy(inputDataSafe, (short *)inputData, channelNum * dataLength * sizeof(short));
	}
	ret = Vhall_AudioMixingCore(inputDataSafe, outputDataSafe, channelNum, dataLength, isSingleOut);

	if (isSingleOut){
		if (ret == 0 && accuracyType == DD_FLOAT){
			for (i = 0; i < dataLength; i++){
				((float*)outputData)[i] = (float)(outputDataSafe[i]) / LIMIT_MAX;
				if (((float*)outputData)[i] > 0.99999999){
					((float*)outputData)[i] = 0.99999999;
				}
            else if (((float*)outputData)[i] < -0.99999999){
					((float*)outputData)[i] = -0.99999999;
				}
			}
		}
		else if (ret == 0){
			memcpy((short *)outputData, outputDataSafe, dataLength * sizeof(short));
		}
	}
	else{
		if (ret == 0 && accuracyType == DD_FLOAT){
			for (i = 0; i < (channelNum + 1) * dataLength; i++){
				((float*)outputData)[i] = (float)(outputDataSafe[i]) / LIMIT_MAX;
			}
		}
		else if (ret == 0){
			memcpy((short *)outputData, outputDataSafe, (channelNum + 1) * dataLength * sizeof(short));
		}
	}
	
	return ret;
}
