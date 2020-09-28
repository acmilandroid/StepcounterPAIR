#pragma once

#include "StdAfx.h"
#include "LoadedFileInfo.h"


LoadedFileInfo& LoadedFileInfo::GetInstance()
{
	static LoadedFileInfo theInfo;
	return theInfo;
}

LoadedFileInfo::LoadedFileInfo(void): 
	participant(), 
	trial(), 
	folderPath(), 
	filePaths(),
	fileNames(),
	totalData(),
	offset(),
	videoPath(),
	filesLoaded(false),
	dataSynced(false)
{
}

LoadedFileInfo::~LoadedFileInfo(void)
{
}

void LoadedFileInfo::LoadFiles(void)
{
	OPENFILENAME ofn;
	int i, j;
	bool result;
	TCHAR tempFile[MAX_PATH_LEN];
	ViewManager& vManager = ViewManager::GetInstance();
	char* pch;
	//std::string temp;

	for(i=0;i<MAX_PATH_LEN;i++)
	{
		tempFile[i] = 0;
	}

	memset(&(ofn),0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFile=tempFile;
	ofn.nMaxFile=MAX_PATH_LEN;
	ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
	ofn.lpstrFilter = TEXT("Data files\0*.csv\0");
	if (!( GetOpenFileName(&ofn))){
		filesLoaded = false;
		return;
	}
	else{
		for(int sensNum=0; sensNum<NUM_SENSORS; sensNum++)
		{
			// set file to the path to the file that was loaded
			//temp.assign(tempFile,MAX_PATH_LEN);
			folderPath.assign(tempFile,MAX_PATH_LEN);
		
			// remove filename from path
			i = folderPath.find_last_of("/\\");
			folderPath.erase(i,folderPath.length() - i);

			pch = strrchr(tempFile,'.');//filePaths[sensNum].find_last_of(".");

			// save filename for later usage (NOTE: without file extension)
			pch[-1] = '1' + sensNum;
			//filePaths[sensNum][j-1] = filePaths[sensNum][j-1] + sensNum;
			//temp.assign(filePaths[sensNum],i+1,j-i-1);
			
			fileNames.push_back(tempFile);
			filePaths.push_back(tempFile);

			// set trial to the name of the directory at the end of the path
			i = folderPath.find_last_of("/\\");
			trial.assign(folderPath,i+1,folderPath.length() - i - 1);

			// set participant to the name of 
			j = folderPath.find_last_of("/\\",i-1);
			participant.assign(folderPath,j+1,i-j-1);
			
			//MessageBox(NULL,filePaths[sensNum].c_str(),"Loop",MB_OK | MB_APPLMODAL);

			result = LoadData(sensNum);
			if(result == 0)
			{
				filesLoaded = false;
				return;
			}
		}

		filesLoaded = true;
		dataSynced = SyncData();
		if(dataSynced == true)
		{
			//MessageBox(NULL,"Data was synchronized.","Sync Successful",MB_OK | MB_APPLMODAL);
		}
		CalculateRange();
		result = LoadSync();
		if(result)
		{
			LoadSteps();
			DetectSteps();
			vManager.LoadVideoFile(videoPath);
			vManager.OneTimePaints();
			vManager.Paint();
		}

		return;
	}
}

void LoadedFileInfo::Clear(void)
{
	ViewManager& vManager = ViewManager::GetInstance();
	if(filesLoaded)
	{
		filePaths.clear();
		fileNames.clear();
		folderPath.clear();
		participant.clear();
		rightStepIndices.clear();
		leftStepIndices.clear();
		rightEdgecaseIndices.clear();
		leftEdgecaseIndices.clear();
		for(int sensNum = 0; sensNum < NUM_SENSORS; sensNum++)
		{
			totalData[sensNum] = 0;
		}	
		trial.clear(); 
		videoPath.clear();
		vManager.ResetCurPos();
		filesLoaded = false;
		dataSynced = false;
	}
}

// this is a mess because the data logger used puts null characters between each
// sensor's reading (eg: 134\0932\0\n) would indicate that sensor 1 read 134 and sensor 2 read 932.
// each sensor can have from 1 to 3(4?) bytes. Using the null char separator makes fscanf end
// after reading the first entry
bool LoadedFileInfo::LoadData(int sensNum)
{
	FILE *fpt;
	char tempS[1024];
	size_t result;
	int i; 
	float tempF = 0;
	
	if(filePaths.size() >= sensNum)
	{
		fpt = fopen(filePaths[sensNum].c_str(),"r");
//		MessageBox(NULL,filePaths[sensNum].c_str(),"Opened file",MB_OK | MB_APPLMODAL);
	}
	else
	{
		return false;
	}

	if (fpt==NULL)
	{
		MessageBox(NULL,"Unable to open the data file.",filePaths[sensNum].c_str(),MB_OK | MB_APPLMODAL);
		return false;
	}

	totalData[sensNum] = 0; 
	while(fscanf(fpt,"%s\n",tempS) == 1)
	{
		if(strcmp(tempS,"dd_MMM_yyyy_HH:MM:SS.FFF") == 0)
		{
			break;
		}
	}
	while (1)
	{
		i = 0;
		// skip past unwanted shimmer data
		for(i=0;i<10;i++)
		{
			result = fscanf(fpt,"%s",tempS);
			if(result != 1)
			{			
				break;
			}
		}
		if(result != 1) break;
		
		// save acceleration in X (axis 0), Y (axis 1), and Z (axis 2)
		for(int axis=0; axis<NUM_AXES; axis++)
		{
			result = fscanf(fpt,"%s",tempS);
			data[sensNum][axis][totalData[sensNum]] = atof(tempS);
			if(result != 1)
			{			
				break;
			}
		}

		// skip past additional unwanted data
		for(i=0;i<3;i++)
		{
			result = fscanf(fpt,"%s",tempS);
			if(result != 1)
			{			
				break;
			}
		}

		//save date and time info
		result = fscanf(fpt,"%s",tempS);
		strncpy(timeData[sensNum][totalData[sensNum]],tempS,64);

		if(result != 1) 
			break;

		totalData[sensNum] ++;
		if (totalData[sensNum] == MAX_DATA)
		{	
			MessageBox(NULL,"Too many data!","Data File",MB_OK | MB_APPLMODAL);
			return true;
		}
	}
	fclose(fpt);

	//fill in missing samples and delete double samples
	FixData(sensNum);

	return true;
}

void LoadedFileInfo::FixData(int sensNum)
{
	double theoreticalCorrectTime;
	double startTime;
	double theoreticalMinTime;
	double theoreticalMaxTime;
	double sampleTime;
	// expected time between samples
	double msPerSample = 1000.0/15.0;
	
	char* loc;
	int cnt;
	int lineNum = 0;
	int lineWritten = 0;

	bool readAndDiscard = false;

	startTime = GetTimeValue(sensNum,lineNum);
	lineNum++;
	
	/* Read actual lines */
    while (lineNum < totalData[sensNum])
    {
		theoreticalCorrectTime = startTime + msPerSample*lineNum/1000.0;
		// going to compare each sample to the theoretically correct time
		// samples should be within 1 of a sample of the theoretically correct time
		theoreticalMinTime = startTime + msPerSample*lineNum/1000.0 - msPerSample/(1000.0);
		theoreticalMaxTime = startTime + msPerSample*lineNum/1000.0 + msPerSample/(1000.0);

		sampleTime = GetTimeValue(sensNum,lineNum);

		// 2 problem cases: 
		// 1) if sampleTime < theoretical Min, then need to remove a sample
        // 2) if sampleTime > theoretical Max, then need to insert a sample
		if (sampleTime < theoreticalMinTime)
		{
			for(int axis=0; axis<NUM_AXES; axis++)
			{
				memmove(&data[sensNum][axis][lineNum],&data[sensNum][axis][lineNum+1],MAX_DATA*sizeof(float) - (lineNum+1)*sizeof(float));
			}
			memmove(timeData[sensNum][lineNum],timeData[sensNum][lineNum+1],MAX_DATA*sizeof(char)*64 - (lineNum+1)*sizeof(char)*64);
			totalData[sensNum] --;
			lineNum++;
		}
		else if(sampleTime >= theoreticalMaxTime)
		{
			for(int axis=0; axis<NUM_AXES; axis++)
			{
				// move all the data one spot forward
				memmove(&data[sensNum][axis][lineNum+1],&data[sensNum][axis][lineNum],MAX_DATA*sizeof(float) - (lineNum+1)*sizeof(float));
				// ensure current entry is copied back to its correct location
				data[sensNum][axis][lineNum] = data[sensNum][axis][lineNum+1];
			}
			memmove(timeData[sensNum][lineNum+1],timeData[sensNum][lineNum],MAX_DATA*sizeof(char)*64 - (lineNum+1)*sizeof(char)*64);
			strcpy(timeData[sensNum][lineNum],timeData[sensNum][lineNum+1]);
			DecrementTime(sensNum,lineNum);
			totalData[sensNum] ++;
			// because I decrement the time by one sample size, should not increment line number to allow
			// for multiple missed samples
		}
		// if sample time is within the expected range, simply imcrement counter and move to next sample
		else
		{
			lineNum++;
		}        
		// this triggers if some bug is causing line number to get out of control. Problem if this break is triggered.
		if(lineNum >= MAX_DATA)
		{
			break;
		}
    }
}

// load the sync file, which contains frame offset and name of video file
bool LoadedFileInfo::LoadSync()
{
	FILE *fpt;
	std::string temp;
	char c_temp[MAX_PATH_LEN];
	int i;

	temp.assign(folderPath.c_str());
	//temp.append("\\");
	//temp.append(fileNames[0].c_str());
	temp.append("\\Sensor01_sync.txt");
	//temp.append("_sync.txt");

	fpt = fopen(temp.c_str(),"r");
	if (fpt==NULL)
	{
		MessageBox(NULL,"Unable to open the sync file.","Sync File",MB_OK | MB_APPLMODAL);
		return false;
	}
	
	i = fscanf(fpt, "%d\n", &offset);
	if(i!=1)
	{
		fclose(fpt);
		return false;
	}
	i = fscanf(fpt, "%s\n", c_temp);
	if(i!=1)
	{
		fclose(fpt);
		return false;
	}

	videoPath.append(folderPath.c_str());
	videoPath.append("\\");
	videoPath.append(c_temp);

	fclose(fpt);
	
	return true;
}


// load step indices (ground truth and predictions)
void LoadedFileInfo::LoadSteps()
{
	FILE *fpt;
	std::string temp;
	int i;
	int index;
	char foot[64];
	
	// read ground truth steps
	temp.append(folderPath.c_str());
	temp.append("\\steps.txt");

	fpt = fopen(temp.c_str(),"r");
	if (fpt==NULL)
	{
		return;
	}
	else
	{
		i = fscanf(fpt, "%d %s\n", &index, foot);
		while(i == 2)
		{
			// put indices in master ground truth list
			gtStepIndices.push_back(index);
			// put indices in correct sublist
			if(!strcmp(foot,"r"))
			{
				rightStepIndices.push_back(index);
			}
			else if(!strcmp(foot,"l"))
			{
				leftStepIndices.push_back(index);
			}
			else if(!strcmp(foot,"redge"))
			{
				rightEdgecaseIndices.push_back(index);
			}
			else if(!strcmp(foot,"ledge"))
			{
				leftEdgecaseIndices.push_back(index);
			}
			else
			{
				i = 2;
			}
			i = fscanf(fpt, "%d %s\n", &index, foot);
		}
		fclose(fpt);
	}
	
	// read predicted steps for sensor 01
	temp.clear();
	temp.append(folderPath.c_str());
	temp.append("\\predicted_steps_sensor01.txt");

	// put predicted steps in list 1
	fpt = fopen(temp.c_str(), "r");
	if (fpt == NULL)
	{
		return;
	}
	else
	{
		i = fscanf(fpt, "%d\n", &index);
		while (i == 1)
		{
			predictedStepIndices1.push_back(index);
			i = fscanf(fpt, "%d\n", &index);
		}
		fclose(fpt);
	}

	// read predicted steps for sensor 02
	temp.clear();
	temp.append(folderPath.c_str());
	temp.append("\\predicted_steps_sensor02.txt");

	// put predicted steps in list 2
	fpt = fopen(temp.c_str(), "r");
	if (fpt == NULL)
	{
		return;
	}
	else
	{
		i = fscanf(fpt, "%d\n", &index);
		while (i == 1)
		{
			predictedStepIndices2.push_back(index);
			i = fscanf(fpt, "%d\n", &index);
		}
		fclose(fpt);
	}

	// read predicted steps for sensor 03
	temp.clear();
	temp.append(folderPath.c_str());
	temp.append("\\predicted_steps_sensor03.txt");

	// put predicted steps in list
	fpt = fopen(temp.c_str(), "r");
	if (fpt == NULL)
	{
		return;
	}
	else
	{
		i = fscanf(fpt, "%d\n", &index);
		while (i == 1)
		{
			predictedStepIndices3.push_back(index);
			i = fscanf(fpt, "%d\n", &index);
		}
		fclose(fpt);
	}

	return;
}

void LoadedFileInfo::AddRightStep(int index)
{		
	rightStepIndices.push_back(index);
	SaveSteps();
}

void LoadedFileInfo::AddLeftStep(int index)
{
	leftStepIndices.push_back(index);
	SaveSteps();
}

void LoadedFileInfo::AddRightEdgecase(int index)
{		
	rightEdgecaseIndices.push_back(index);
	SaveSteps();
}

void LoadedFileInfo::AddLeftEdgecase(int index)
{
	leftEdgecaseIndices.push_back(index);
	SaveSteps();
}

//void LoadedFileInfo::AddStep(int start, int end)
//{
//	std::pair<int, int> step;
//
//	if(end > start)
//	{
//		step.first = start;
//		step.second = end;
//		steps.push_back(step);
//		steps.sort();
//		SaveSteps();
//	}
//}

void LoadedFileInfo::DeleteStep(int position, int maxRange)
{
	bool rightIndexMatched = false;
	bool leftIndexMatched = false;
	bool rightEdgecaseIndexMatched = false;
	bool leftEdgecaseIndexMatched = false;
	bool predictedIndexMatched1 = false;
	bool predictedIndexMatched2 = false;
	bool predictedIndexMatched3 = false;
	int rightIndex = -1;
	int leftIndex = -1;
	int rightEdgecaseIndex = -1;
	int leftEdgecaseIndex = -1;
	int predictedIndex1 = -1;
	int predictedIndex2 = -1;
	int predictedIndex3 = -1;
	int firstTypeMatched = -1;
	int firstIndexMatched = 500000;
	//use iterator, match start and end index
	rightStepIndices.sort();
	leftStepIndices.sort();
	rightEdgecaseIndices.sort();
	leftEdgecaseIndices.sort();

	std::list<int>::iterator ptr = rightStepIndices.begin();

	while(ptr != rightStepIndices.end() && !rightIndexMatched)
	{
		if((*ptr) >= position)
		{
			rightIndexMatched = true;
			rightIndex = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	ptr = leftStepIndices.begin();

	while(ptr != leftStepIndices.end() && !leftIndexMatched)
	{
		if((*ptr) >= position)
		{
			leftIndexMatched = true;
			leftIndex = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	ptr = rightEdgecaseIndices.begin();

	while(ptr != rightEdgecaseIndices.end() && !rightEdgecaseIndexMatched)
	{
		if((*ptr) >= position)
		{
			rightEdgecaseIndexMatched = true;
			rightEdgecaseIndex = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	ptr = leftEdgecaseIndices.begin();

	while(ptr != leftEdgecaseIndices.end() && !leftEdgecaseIndexMatched)
	{
		if((*ptr) >= position)
		{
			leftEdgecaseIndexMatched = true;
			leftEdgecaseIndex = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	ptr = predictedStepIndices1.begin();

	while (ptr != predictedStepIndices1.end() && !predictedIndexMatched1)
	{
		if ((*ptr) >= position)
		{
			predictedIndexMatched1 = true;
			predictedIndex1 = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	ptr = predictedStepIndices2.begin();

	while (ptr != predictedStepIndices2.end() && !predictedIndexMatched2)
	{
		if ((*ptr) >= position)
		{
			predictedIndexMatched2 = true;
			predictedIndex2 = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	ptr = predictedStepIndices3.begin();

	while (ptr != predictedStepIndices3.end() && !predictedIndexMatched3)
	{
		if ((*ptr) >= position)
		{
			predictedIndexMatched3 = true;
			predictedIndex3 = (*ptr);
		}
		else
		{
			ptr++;
		}
	}

	// find first index matched
	if(rightIndexMatched)
	{
		if(rightIndex < firstIndexMatched)
		{
			firstIndexMatched = rightIndex;
			firstTypeMatched = 0;
		}
	}
	if(leftIndexMatched)
	{
		if(leftIndex < firstIndexMatched)
		{
			firstIndexMatched = leftIndex;
			firstTypeMatched = 1;
		}
	}
	if(rightEdgecaseIndexMatched)
	{
		if(rightEdgecaseIndex < firstIndexMatched)
		{
			firstIndexMatched = rightEdgecaseIndex;
			firstTypeMatched = 2;
		}
	}
	if(leftEdgecaseIndexMatched)
	{
		if(leftEdgecaseIndex < firstIndexMatched)
		{
			firstIndexMatched = leftEdgecaseIndex;
			firstTypeMatched = 3;
		}
	}
	if (predictedIndexMatched1)
	{
		if (predictedIndex1 < firstIndexMatched)
		{
			firstIndexMatched = predictedIndex1;
			firstTypeMatched = 4;
		}
	}
	if (predictedIndexMatched2)
	{
		if (predictedIndex2 < firstIndexMatched)
		{
			firstIndexMatched = predictedIndex2;
			firstTypeMatched = 5;
		}
	}

	if (predictedIndexMatched3)
	{
		if (predictedIndex3 < firstIndexMatched)
		{
			firstIndexMatched = predictedIndex3;
			firstTypeMatched = 6;
		}
	}


	if(firstIndexMatched < maxRange)
	{
		if(firstTypeMatched == 0)
		{
			rightStepIndices.remove(firstIndexMatched);
		}
		if(firstTypeMatched == 1)
		{
			leftStepIndices.remove(firstIndexMatched);
		}
		if(firstTypeMatched == 2)
		{
			rightEdgecaseIndices.remove(firstIndexMatched);
		}
		if(firstTypeMatched == 3)
		{
			leftEdgecaseIndices.remove(firstIndexMatched);
		}
		if (firstTypeMatched == 4)
		{
			predictedStepIndices1.remove(firstIndexMatched);
		}
		if (firstTypeMatched == 5)
		{
			predictedStepIndices2.remove(firstIndexMatched);
		}
		if (firstTypeMatched == 6)
		{
			predictedStepIndices3.remove(firstIndexMatched);
		}
	}

	SaveSteps();
}

void LoadedFileInfo::SaveSteps(void)
{
	// save to steps.txt in format %d %d\n
	FILE *fpt;
	std::string temp;
	std::list<int>::iterator ptr = rightStepIndices.begin();
	
	rightStepIndices.sort();
	leftStepIndices.sort();
	rightEdgecaseIndices.sort();
	leftEdgecaseIndices.sort();
	
	temp.append(folderPath.c_str());
	temp.append("\\steps.txt");
	
	fpt = fopen(temp.c_str(),"w");
	if(fpt == NULL)
	{
		MessageBox(NULL,"Unable to open the steps save file.","Sync File",MB_OK | MB_APPLMODAL);
		return;
	}
	while(ptr != rightStepIndices.end())
	{
		fprintf(fpt,"%d r\n", (*ptr));
		ptr++;
	}

	ptr = leftStepIndices.begin();
	while(ptr != leftStepIndices.end())
	{
		fprintf(fpt,"%d l\n", (*ptr));
		ptr++;
	}

	ptr = leftEdgecaseIndices.begin();
	while(ptr != leftEdgecaseIndices.end())
	{
		fprintf(fpt,"%d ledge\n", (*ptr));
		ptr++;
	}

	ptr = rightEdgecaseIndices.begin();
	while(ptr != rightEdgecaseIndices.end())
	{
		fprintf(fpt,"%d redge\n", (*ptr));
		ptr++;
	}

	fclose(fpt);
	return;
}

void LoadedFileInfo::CalculateRange()
{
	int i, j, k;

	for (j = 0; j < NUM_SENSORS; j++) {
		for (k = 0; k < NUM_AXES; k++) {
			maxData[j][k] = MIN_RANGE;
			minData[j][k] = MAX_RANGE;
		}
	}
	for (j = 0; j < NUM_SENSORS; j++) {
		for (k = 0; k < NUM_AXES; k++) {
			for (i = 0; i < totalData[j]; i++) {
				if (maxData[j][k] < data[j][k][i])
					maxData[j][k] = data[j][k][i];
				if (minData[j][k] > data[j][k][i])
					minData[j][k] = data[j][k][i];
			}
		}
	}
}

float LoadedFileInfo::GetData(int sensor, int axis, int pos)
{
	if(data[sensor][axis][pos] > maxData[sensor][axis])
	{
		return maxData[sensor][axis];
	}
	else if(data[sensor][axis][pos] < minData[sensor][axis])
	{
		return minData[sensor][axis];
	}
	else
	{
		return data[sensor][axis][pos];
	}
}

bool LoadedFileInfo::GetIfDomAxis(int sensor, int axis, int pos)
{
	for(int i=0; i<axisRangesRecorded[sensor][axis]; i++)
	{
		if(domAxisRanges[sensor][axis][i][0] <= pos && domAxisRanges[sensor][axis][i][1] >= pos) 
		{
			return true;
		}
	}
	return false;
}

bool LoadedFileInfo::IndexMatch(int index, int &foot, int sensornum)
{
	if(rightStepIndices.size() == 0 && leftStepIndices.size() == 0 && leftEdgecaseIndices.size() == 0 && rightEdgecaseIndices.size() == 0)
	{
		foot = -1;
		return false;
	}
	else
	{
		std::list<int>::iterator ptr = leftStepSubset.begin();
		while(ptr != leftStepSubset.end())
		{
			if(index == (*ptr))
			{
				foot = 0;
				return true;
			}
			ptr++;
		}

		ptr = rightStepSubset.begin();
		while (ptr != rightStepSubset.end())
		{
			if (index == (*ptr))
			{
				foot = 1;
				return true;
			}
			ptr++;
		}

		ptr = leftEdgecaseSubset.begin();
		while (ptr != leftEdgecaseSubset.end())
		{
			if (index == (*ptr))
			{
				foot = 2;
				return true;
			}
			ptr++;
		}

		ptr = rightEdgecaseSubset.begin();
		while(ptr != rightEdgecaseSubset.end())
		{
			if(index == (*ptr))
			{
				foot = 3;
				return true;
			}
			ptr++;
		}

		// change what predicted step subset to match based on current sensor number
		if (sensornum == 1)
		{
			// Predicted Sensor 01 is foot 4
			ptr = predictedStepSubset1.begin();
			while (ptr != predictedStepSubset1.end())
			{
				if (index == (*ptr))
				{
					foot = 4;
					return true;
				}
				ptr++;
			}
		}
		else if (sensornum == 2)
		{
			// Predicted Sensor 02 is foot 5
			ptr = predictedStepSubset2.begin();
			while (ptr != predictedStepSubset2.end())
			{
				if (index == (*ptr))
				{
					foot = 5;
					return true;
				}
				ptr++;
			}
		}
		else if (sensornum == 3)
		{
			// Predicted Sensor 03 is foot 6
			ptr = predictedStepSubset3.begin();
			while (ptr != predictedStepSubset3.end())
			{
				if (index == (*ptr))
				{
					foot = 6;
					return true;
				}
				ptr++;
			}
		}

		// no match with anything
		foot = -1;
		return false;
	
	}
}

//bool LoadedFileInfo::IndexMatch(int rangeStart, int rangeEnd, int index, int arr[2])
//{
//	std::list<std::pair<int, int>>::iterator ptr = steps.begin();
//
//	while(ptr != steps.end())
//	{
//		if(rangeStart > ((*ptr).second))
//		{
//			ptr++;
//			continue;
//		}
//		if(rangeEnd < ((*ptr).first))
//		{
//			return false;
//		}
//		if(index == (*ptr).first)
//		{
//			arr[0] = (*ptr).first;
//			arr[1] = (*ptr).second;
//			return true;
//		}
//		else if(index == (*ptr).second)
//		{
//			arr[0] = (*ptr).first;
//			arr[1] = (*ptr).second;
//			return true;
//		}
//		ptr++;
//	}
//	return false;
//}

// modified peak detector for this sensor
void LoadedFileInfo::DetectSteps()
{
	//int sensor, axis, pos;
	//int lowThresh, highThresh;
	//int peakLoc, peakMax;
	//bool peakStart;
	//int delta = 700;
	//int cur;
	//bool findMax = true;

	//for(sensor=0;sensor<NUM_SENSORS;sensor++)
	//{
	//	for(axis=0;axis<NUM_AXES;axis++)
	//	{
	//		numLocalMaxima[sensor][axis] = 0;
	//		// high threshold is 80% of maximum data for the sensor beinge examined
	//		highThresh = (maxData[sensor][axis] - minData[sensor][axis])*0.8 + minData[sensor][axis];
	//		// low threshold set to 20% of maximum data
	//		lowThresh =  (maxData[sensor][axis] - minData[sensor][axis])*0.2 + minData[sensor][axis];

	//		// loop through all data
	//		pos = 0;
	//		peakStart = false;
	//		while (pos < totalData[sensor])
	//		{
	//			// update current value being examined
	//			cur = data[sensor][axis][pos];
	//			// in order to detect a peak, a value must exceed the high threshold
	//			if(cur > highThresh)
	//			{
	//				peakLoc = pos;
	//				peakMax = cur;
	//				peakStart = true;
	//			}
	//			// while data stays above low threshold, continue examining to find highest value "true peak"
	//			while(peakStart)
	//			{
	//				pos++;
	//				cur = data[sensor][axis][pos];
	//				if(cur > peakMax)
	//				{
	//					peakLoc = pos;
	//					peakMax = cur;
	//				}
	//				// when data falls below threshold, record peak value, begin looking for next peak
	//				if(cur < lowThresh)
	//				{
	//					localMaxima[sensor][axis][numLocalMaxima[sensor][axis]] = peakLoc;
	//					numLocalMaxima[sensor][axis] ++;
	//					peakStart = false;
	//				}
	//				if(pos >= totalData[sensor])
	//				{
	//					break;
	//				}
	//			}
	//			pos++;
	//		}
	//	}
	//}
}
//

// Surya's peak detector (slightly altered)
//void LoadedFileInfo::DetectSteps()
//{
//	int i,j;
//	int peakMax, peakIndex;
//	float lowThresh,highThresh,minThresh;
//	float hysteresisRatio;
//
//	minThresh = 200;
//	hysteresisRatio = 3.0;
//
//	/* use hysteresis approach to find peaks */
//	for(j=0;j<NUM_SENSORS;j++)
//	{
//		i = 0;
//		while(i < totalData)
//		{
//			lowThresh = data[i][j];						/* set initial thresholds based upon current value */
//			if (lowThresh < minThresh){
//				lowThresh = minThresh;
//			}
//			highThresh = hysteresisRatio * lowThresh;
//			while(data[i][j] < highThresh)				/* the signal has to go 2x larger than its previous minimum */
//			{
//				if (data[i][j] < lowThresh)				/* if signal goes lower, adjust thresholds down */
//				{
//					lowThresh = data[i][j];
//					if (lowThresh < minThresh){
//						lowThresh = minThresh;
//					}
//					highThresh = hysteresisRatio * lowThresh;
//				}
//				
//				i++;
//				
//				if(i >= totalData){
//				  break;
//				}
//			}
//			
//			if(i >= totalData){
//				break;
//			}
//
//			peakMax = data[i][j];
//
//			while (data[i][j] > lowThresh  &&  i < totalData)
//			{									/* peak lasts while larger than the minimum */
//				if (data[i][j] > peakMax)
//				{
//					peakIndex = i;
//					peakMax = data[i][j];
//					lowThresh = peakMax / hysteresisRatio;	/* readjust lower bound as peak gets higher */
//				}
//				i++;
//			}
//			
//			if(j == 0){
//				localMaxima0.push_back(peakIndex);
//			}
//			else{
//				localMaxima1.push_back(peakIndex);
//			}
//			i++;
//		}
//	}
//}

// very simplistic peak detector
//void LoadedFileInfo::DetectSteps()
//{
//	int i,j;
//	int min = 100000;
//	int max = -1;
//	int minLoc,maxLoc;
//	int delta = 700;
//	int cur;
//	bool findMax = true;
//
//	localMaxima0.clear();
//	localMaxima1.clear();
//
//	for(i=0;i<NUM_SENSORS;i++)
//	{
//		for(j=0;j<totalData;j++)
//		{
//			cur = data[j][i];
//			if(cur > max)
//			{
//				max = cur;
//				maxLoc = j;
//			}
//			if(cur < min)
//			{
//				min = cur;
//				minLoc = j;
//			}
//			if(findMax)
//			{
//				if(cur < max - delta)
//				{
//					if(i == 0){
//						localMaxima0.push_back(maxLoc);
//					}	
//					else{
//						localMaxima1.push_back(maxLoc);
//					}
//					min = cur;
//					minLoc = j;
//					findMax = false;
//				}
//			}
//			else
//			{
//				if(cur > min + delta)
//				{
//					max = cur;
//					maxLoc = j;
//					findMax = true;
//				}
//			}
//		}
//	}
//
//}
//

bool LoadedFileInfo::MatchesPeak(int sensor, int axis, int pos)
{
	std::vector<int> Maxima;
	for(int i=0;i<numLocalMaxima[sensor][axis];i++)
	{
		if(pos == localMaxima[sensor][axis][i])
		{
			return true;
		}
	}
	return false;
}

int LoadedFileInfo::GetNumData(int sensNum)
{
	if(sensNum < NUM_SENSORS)
	{
		return totalData[sensNum];
	}
	else
	{
		return -1;
	}
}

int LoadedFileInfo::GetMaxNumData() 
{
	int maxNum = 0;
	for(int i=0;i<NUM_SENSORS;i++)
	{
		if(totalData[i] > maxNum)
		{
			maxNum = totalData[i];
		}
	}
	return maxNum;
}

bool LoadedFileInfo::SyncData(void)
{
	char syncMDY[64];
	char comp1MDY[64];
	char comp2MDY[64];
	
	double syncTime;
	double comp1Time;
	double comp2Time;
	
	char* loc;
	int cnt;

	float synced[NUM_SENSORS];

	int syncUpdateDiff;

	for(int i=0; i<NUM_SENSORS; i++)
	{
		synced[i] = false;
		syncOffsets[i] = -1;
	}

	// run through sensor 1 and synchronize its time data with data from the remaining sensors
	for(int data1=0; data1<totalData[0] && (!synced[0] || !synced[1] || !synced[2]); data1++)
	{
  		syncTime = GetTimeValue(0,data1);
		GetMDY(0,data1,syncMDY);
		GetMDY(0,data1,comp1MDY);

		comp1Time = -1;
		comp2Time = -1;
		// beginning with sensor 2, try to sync data
		for(int j=1; j<NUM_SENSORS; j++)
		{
			// parse date/time string to verify date and time are synced as closely as possible
			for(int data2=0; data2<totalData[j] && synced[j] == false; data2++)
			{
				if(syncTime < comp1Time)
				{
					continue;
				}
				comp2Time = GetTimeValue(j,data2);
				GetMDY(j,data2,comp2MDY);

				// ensure two times have been added for comparison
				if(comp1Time > 0 && comp2Time > 0)
				{
					// ensure that the data were recorded on the same day
					if(strcmp(syncMDY,comp1MDY) == 0 && strcmp(syncMDY,comp2MDY) == 0)
					{
						// check if sensor 1's time is between two times on sensor2 or sensor 3
						if(syncTime > comp1Time && syncTime <= comp2Time)
						{
							synced[0] = true;
							synced[j] = true;
							// if this is the first time times were able to be synced, update
							// sensor 1 and the current sensor with synced data
							if(syncOffsets[0] == -1)
							{
								syncOffsets[0] = data1;
								syncOffsets[j] = data2-1;
							}
							// if this is not the first sync, then, previous sensors must be shifted
							// to meet current sensor's sync
							/* This chunk of code may be confusing, but here's the rundown:
							Imagine we have 3 sensors. Sensor 1 started recording before sensor 2
							which in turn started recording before sensor 3. The algorithm we are performing
							will seek if the first data point in sensor 1 is between 2 data points in sensor 2.
							Becuase sensor 1 started before sensor 2, this is not the case, so sensor 1 checks its 
							next data point and so on until we find a data point collected while sensor 2 was on.
							Then, these data points match, and a sync offset is found, eg: sensor 1 being 1500 data points
							after initialization, sensor 2 being 0. This process is repeated for sensor 3. If sensor 3
							was started 150 samples after sensor 2, then both sensor 1 and sesnor 2 would have to update their 
							offsets: to 1650 and 150, respectively, with sensor 3's offset being 0. NOTE:this would work
							if sensor 3 was started before sensor 2, because the time stamp for sample 1500 from sensor 1
							would be found between 2 of sensor 3's time stamps, making data1 = 1500 still, and adding 0
							to sensor 1 and sensor 2's offsets. NOTE2: Due to the structure of the loops, sensors 2 - N
							will not necessarily be synchronized in numerical order. They will actually be synchronized
							in order of earliest turned on.*/
							else
							{
								// find the number of samples after any previous sensors were recording before
								// the next sensor began recording and update the already recorded sync times.
								syncUpdateDiff = data1 - syncOffsets[0];
								for(int k=0; k<NUM_SENSORS && k!= j; k++)
								{
									syncOffsets[k] = syncOffsets[k] + syncUpdateDiff;
								}
								// store sync time for most recent sensor
								syncOffsets[j] = data2-1;
							}
						}
					}
				}
					
				// store this iteration of data for future comparison
				strncpy(comp1MDY,comp2MDY,64);
				comp1Time = comp2Time;
			}
		}
	}

	for(int i = 0; i < NUM_SENSORS; i++)
	{
		if(synced[i] == false)
		{
			return false;
		}
	}

	for(int i=0; i < NUM_SENSORS; i++)
	{
		for(int j=0; j<NUM_AXES; j++)
		{
			memmove(data[i][j], &data[i][j][syncOffsets[i]], MAX_DATA*sizeof(float) - syncOffsets[i]*sizeof(float));
		}
		totalData[i] = totalData[i] - syncOffsets[i];
	}
	return true;
}

double LoadedFileInfo::GetTimeValue(int sensNum, int index)
{
	char temp[64];
	char* loc;
	int cnt;

	char hour[64];
	char minute[64];
	char second[64];
	char milli[64];

	// get first time stamp from sensor 1
	strncpy(temp,timeData[sensNum][index],64);
	// break time string into each component
	loc = strtok(temp," _:.");
	cnt = 0;
	while(loc != NULL)
	{
		if(cnt == 3)
		{
			strncpy(hour,loc,64);
		}
		else if(cnt == 4)
		{
			strncpy(minute,loc,64);
		}
		else if(cnt == 5)
		{
			strncpy(second,loc,64);
		}
		else if(cnt == 6)
		{
			strncpy(milli,loc,64);
		}
		loc = strtok(NULL," _:.");
		cnt ++;		
	}

	return atof(hour)*3600+atof(minute)*60+atof(second)+atof(milli)/1000.0;
}

void LoadedFileInfo::GetMDY(int sensNum, int index, char* MDY)
{
	char temp[64];
	char* loc;
	int cnt;

	char month[64];
	char day[64];
	char year[64];
	
	// get first time stamp from sensor 1
	strncpy(temp,timeData[sensNum][index],64);
	// break time string into each component
	loc = strtok(temp," _:.");
	cnt = 0;
	
	while(loc != NULL)
	{
		if(cnt == 0)
		{
			strncpy(day,loc,64);
		}
		else if(cnt == 1)
		{
			strncpy(month,loc,64);
		}
		else if(cnt == 2)
		{
			strncpy(year,loc,64);
		}
		loc = strtok(NULL," _:.");
		cnt ++;		
	}

	strncpy(MDY,month,64);
	strncat(MDY,day,64);
	strncat(MDY,year,64);

	return;
}

void LoadedFileInfo::DecrementTime(int sensNum, int index)
{
	char temp[64];
	char* loc;
	int cnt;
	
	char month[64];
	char day[64];
	char year[64];
	char hour[64];
	char minute[64];
	char second[64];
	char milli[64];

	int d_hour;
	int d_minute;
	int d_second;
	int d_milli;

	double sampleTime = (int)1000/15.0;

	// get first time stamp from sensor 1
	strncpy(temp,timeData[sensNum][index],64);
	// break time string into each component
	loc = strtok(temp," _:.");
	cnt = 0;
	while(loc != NULL)
	{
		if(cnt == 0)
		{
			strncpy(day,loc,64);
		}
		else if(cnt == 1)
		{
			strncpy(month,loc,64);
		}
		else if(cnt == 2)
		{
			strncpy(year,loc,64);
		}
		if(cnt == 3)
		{
			strncpy(hour,loc,64);
		}
		else if(cnt == 4)
		{
			strncpy(minute,loc,64);
		}
		else if(cnt == 5)
		{
			strncpy(second,loc,64);
		}
		else if(cnt == 6)
		{
			strncpy(milli,loc,64);
		}
		loc = strtok(NULL," _:.");
		cnt ++;		
	}

	d_hour = atoi(hour);
	d_minute = atoi(minute);
	d_second = atoi(second);
	d_milli = atoi(milli);

	if(d_milli - sampleTime >= 0)
	{
		d_milli = d_milli - sampleTime;
		itoa(d_milli,milli,10);
	}
	else
	{
		d_milli = d_milli - sampleTime + 1000;
		itoa(d_milli,milli,10);
		if(d_second - 1 >= 0)
		{
			d_second = d_second - 1;
			itoa(d_second,second,10);
		}
		else
		{
			d_second = d_second - 1 + 60;
			itoa(d_second,second,10);
			if(d_minute - 1 >= 0)
			{
				d_minute = d_minute - 1;
				itoa(d_minute, minute,10);
			}
			else
			{
				d_minute = d_minute - 1 + 60;
				itoa(d_minute, minute,10);
				if(d_hour - 1 >= 0)
				{
					d_hour = d_hour - 1;
					itoa(d_hour, hour,10);
				}
				else
				{
					d_hour = d_hour - 1 + 24;
					itoa(d_hour, hour,10);
				}
			}
		}
	}
	strcpy(temp,day);
	strcat(temp,"_");
	strcat(temp,month);
	strcat(temp,"_");
	strcat(temp,year);
	strcat(temp,"_");
	strcat(temp,hour);
	strcat(temp,":");
	strcat(temp,minute);
	strcat(temp,":");
	strcat(temp,second);
	strcat(temp,".");
	strcat(temp,milli);
	strncpy(timeData[sensNum][index],temp,64);
}

void LoadedFileInfo::FreeData()
{
	filePaths.clear();
	fileNames.clear();
	rightStepIndices.clear();
	leftStepIndices.clear();
	leftEdgecaseIndices.clear();
	rightEdgecaseIndices.clear();
	predictedStepIndices1.clear();
	predictedStepIndices2.clear();
	predictedStepIndices3.clear();
}

void LoadedFileInfo::MakeStepSubsets(int startIndex, int endIndex)
{
	std::list<int>::iterator ptr = rightStepIndices.begin();

	rightStepSubset.clear();
	leftStepSubset.clear();
	rightEdgecaseSubset.clear();
	leftEdgecaseSubset.clear();
	predictedStepSubset1.clear();
	predictedStepSubset2.clear();
	predictedStepSubset3.clear();

	while(ptr != rightStepIndices.end())
	{
		if((*ptr) >= startIndex && (*ptr) <= endIndex )
		{
			rightStepSubset.push_back((*ptr));
		}
		ptr++;
	}

	ptr = leftStepIndices.begin();
	
	while(ptr != leftStepIndices.end())
	{
		if((*ptr) >= startIndex && (*ptr) <= endIndex )
		{
			leftStepSubset.push_back((*ptr));
		}
		ptr++;
	}

	ptr = rightEdgecaseIndices.begin();

	while(ptr != rightEdgecaseIndices.end())
	{
		if((*ptr) >= startIndex && (*ptr) <= endIndex )
		{
			rightEdgecaseSubset.push_back((*ptr));
		}
		ptr++;
	}

	ptr = leftEdgecaseIndices.begin();

	while(ptr != leftEdgecaseIndices.end())
	{
		if((*ptr) >= startIndex && (*ptr) <= endIndex )
		{
			leftEdgecaseSubset.push_back((*ptr));
		}
		ptr++;
	}

	ptr = predictedStepIndices1.begin();

	while (ptr != predictedStepIndices1.end())
	{
		if ((*ptr) >= startIndex && (*ptr) <= endIndex)
		{
			predictedStepSubset1.push_back((*ptr));
		}
		ptr++;
	}

	ptr = predictedStepIndices2.begin();

	while (ptr != predictedStepIndices2.end())
	{
		if ((*ptr) >= startIndex && (*ptr) <= endIndex)
		{
			predictedStepSubset2.push_back((*ptr));
		}
		ptr++;
	}

	ptr = predictedStepIndices3.begin();

	while (ptr != predictedStepIndices3.end())
	{
		if ((*ptr) >= startIndex && (*ptr) <= endIndex)
		{
			predictedStepSubset3.push_back((*ptr));
		}
		ptr++;
	}

	FindDomAxes();
	return;
}

int LoadedFileInfo::GetLastStepIndex(int position, int condition)
{
	bool tooFar = false;
	int index = -1;

	//use iterator, match start and end index
	rightStepIndices.sort();
	leftStepIndices.sort();
	rightEdgecaseIndices.sort();
	leftEdgecaseIndices.sort();

	std::list<int>::iterator ptr = rightStepIndices.begin();
	//conditions: 0 = all | 1 = steps | 2 = edgecases | 3 = right all | 4 = left all |
	//            5 = right steps | 6 = left steps | 7 = right edgecases | 8 = left edgecases

	while(ptr != rightStepIndices.end() && !tooFar && (condition == 0 || condition == 1 || condition == 3 || condition == 5))
	{
		if((*ptr) >= position)
		{
			tooFar = true;
		}
		else
		{
			if((*ptr) > index)
			{
				index = (*ptr);
			}
		}
		ptr++;
	}

	ptr = leftStepIndices.begin();
	tooFar = false;

	while(ptr != leftStepIndices.end() && !tooFar && (condition == 0 || condition == 1 || condition == 4 || condition == 6))
	{
		if((*ptr) >= position)
		{
			tooFar = true;
		}
		else
		{
			if((*ptr) > index)
			{
				index = (*ptr);
			}
		}
		ptr++;
	}

	ptr = rightEdgecaseIndices.begin();
	tooFar = false;

	while(ptr != rightEdgecaseIndices.end() && !tooFar && (condition == 0 || condition == 2 || condition == 3 || condition == 7))
	{
		if((*ptr) >= position)
		{
			tooFar = true;
		}
		else
		{
			if((*ptr) > index)
			{
				index = (*ptr);
			}
		}
		ptr++;

	}

	ptr = leftEdgecaseIndices.begin();
	tooFar = false;

	while(ptr != leftEdgecaseIndices.end() && !tooFar && (condition == 0 || condition == 2 || condition == 4 || condition == 8))
	{
		if((*ptr) >= position)
		{
			tooFar = true;
		}
		else
		{
			if((*ptr) > index)
			{
				index = (*ptr);
			}
		}
		ptr++;
	}

	return index;
}

int LoadedFileInfo::GetNextStepIndex(int position, int condition)
{
	bool indexMatched = false;
	int index = 500000;
	//use iterator, match start and end index
	rightStepIndices.sort();
	leftStepIndices.sort();
	rightEdgecaseIndices.sort();
	leftEdgecaseIndices.sort();

	std::list<int>::iterator ptr = rightStepIndices.begin();

	while(ptr != rightStepIndices.end() && !indexMatched && (condition == 0 || condition == 1 || condition == 3 || condition == 5))
	{
		if((*ptr) > position)
		{
			indexMatched = true;
			if((*ptr) < index)
			{
				index = (*ptr);
			}
		}
		ptr++;

	}

	ptr = leftStepIndices.begin();
	indexMatched = false;

	while(ptr != leftStepIndices.end() && !indexMatched && (condition == 0 || condition == 1 || condition == 4 || condition == 6))
	{
		if((*ptr) > position)
		{
			indexMatched = true;
			if((*ptr) < index)
			{
				index = (*ptr);
			}
		}
		ptr++;
	}

	ptr = rightEdgecaseIndices.begin();
	indexMatched = false;

	while(ptr != rightEdgecaseIndices.end() && !indexMatched && (condition == 0 || condition == 2 || condition == 3 || condition == 7))
	{
		if((*ptr) > position)
		{
			indexMatched = true;
			if((*ptr) < index)
			{
				index = (*ptr);
			}
		}
		ptr++;
	}

	ptr = leftEdgecaseIndices.begin();
	indexMatched = false;

	while(ptr != leftEdgecaseIndices.end() && !indexMatched && (condition == 0 || condition == 2 || condition == 4 || condition == 8))
	{
		if((*ptr) > position)
		{
			indexMatched = true;
			if((*ptr) < index)
			{
				index = (*ptr);
			}
		}
		ptr++;
	}

	if(index == 500000)
	{
		index = -1;
	}
	return index;
}

void LoadedFileInfo::FindDomAxes(void)
{
	int i, j, k;
	std::list<int>::iterator ptr = rightStepSubset.begin();
	int tempStart;
	int tempEnd;
	float tempMeanByAxis[NUM_AXES];
	float tempIntegByAxis[NUM_AXES];
	float maxDevByAxis[NUM_AXES];
	int tempDomAxis;
	
	for(i=0;i<NUM_SENSORS;i++)
	{
		for(j=0;j<NUM_AXES;j++)
		{
			axisRangesRecorded[i][j] = 0;
		}
	}

	while(ptr != rightStepSubset.end())
	{
		for(i=0; i<NUM_SENSORS; i++)	
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if(tempStart < 0)
			{
				tempStart = 0;
			}
			if(tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for(j=0;j<NUM_AXES;j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for(k=tempStart; k<=tempEnd;k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j]/(tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for(k=tempStart;k<=tempEnd;k++)
				{
					if(abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis,maxDevByAxis+NUM_AXES-1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr ++;
	}

	ptr = leftStepSubset.begin();
	while(ptr != leftStepSubset.end())
	{
		for(i=0; i<NUM_SENSORS; i++)	
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if(tempStart < 0)
			{
				tempStart = 0;
			}
			if(tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for(j=0;j<NUM_AXES;j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for(k=tempStart; k<=tempEnd;k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j]/(tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for(k=tempStart;k<=tempEnd;k++)
				{
					if(abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis,maxDevByAxis+NUM_AXES-1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr ++;
	}

	ptr = rightEdgecaseSubset.begin();
	while(ptr != rightEdgecaseSubset.end())
	{
		for(i=0; i<NUM_SENSORS; i++)	
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if(tempStart < 0)
			{
				tempStart = 0;
			}
			if(tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for(j=0;j<NUM_AXES;j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for(k=tempStart; k<=tempEnd;k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j]/(tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for(k=tempStart;k<=tempEnd;k++)
				{
					if(abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis,maxDevByAxis+NUM_AXES-1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr ++;
	}

	ptr = leftEdgecaseSubset.begin();
	while(ptr != leftEdgecaseSubset.end())
	{
		for(i=0; i<NUM_SENSORS; i++)	
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if(tempStart < 0)
			{
				tempStart = 0;
			}
			if(tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for(j=0;j<NUM_AXES;j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for(k=tempStart; k<=tempEnd;k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j]/(tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for(k=tempStart;k<=tempEnd;k++)
				{
					if(abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis,maxDevByAxis+NUM_AXES-1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr ++;
	}

	ptr = predictedStepSubset1.begin();
	while (ptr != predictedStepSubset1.end())
	{
		for (i = 0; i < NUM_SENSORS; i++)
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if (tempStart < 0)
			{
				tempStart = 0;
			}
			if (tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for (j = 0; j < NUM_AXES; j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for (k = tempStart; k <= tempEnd; k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j] / (tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for (k = tempStart; k <= tempEnd; k++)
				{
					if (abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis, maxDevByAxis + NUM_AXES - 1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr++;
	}

	ptr = predictedStepSubset2.begin();
	while (ptr != predictedStepSubset2.end())
	{
		for (i = 0; i < NUM_SENSORS; i++)
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if (tempStart < 0)
			{
				tempStart = 0;
			}
			if (tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for (j = 0; j < NUM_AXES; j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for (k = tempStart; k <= tempEnd; k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j] / (tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for (k = tempStart; k <= tempEnd; k++)
				{
					if (abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis, maxDevByAxis + NUM_AXES - 1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr++;
	}

	ptr = predictedStepSubset3.begin();
	while (ptr != predictedStepSubset3.end())
	{
		for (i = 0; i < NUM_SENSORS; i++)
		{
			tempStart = (*ptr) - 7;
			tempEnd = (*ptr) + 7;
			if (tempStart < 0)
			{
				tempStart = 0;
			}
			if (tempEnd > totalData[i])
			{
				tempEnd = totalData[i];
			}
			for (j = 0; j < NUM_AXES; j++)
			{
				tempMeanByAxis[j] = 0;
				tempIntegByAxis[j] = 0;
				for (k = tempStart; k <= tempEnd; k++)
				{
					tempMeanByAxis[j] += data[i][j][k];
				}
				tempMeanByAxis[j] = tempMeanByAxis[j] / (tempEnd - tempStart + 1);
				maxDevByAxis[j] = -1;
				for (k = tempStart; k <= tempEnd; k++)
				{
					if (abs(data[i][j][k] - tempMeanByAxis[j]) > maxDevByAxis[j])
					{
						maxDevByAxis[j] = abs(data[i][j][k] - tempMeanByAxis[j]);
					}
				}
				/*for(k=tempStart; k<=tempEnd;k++)
				{
					tempIntegByAxis[j] += abs(data[i][j][k] - tempMeanByAxis[j]);
				}*/
			}
			//tempDomAxis = std::distance(tempIntegByAxis, std::max_element(tempIntegByAxis,tempIntegByAxis+NUM_AXES-1));
			tempDomAxis = std::distance(maxDevByAxis, std::max_element(maxDevByAxis, maxDevByAxis + NUM_AXES - 1));
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][0] = tempStart;
			domAxisRanges[i][tempDomAxis][axisRangesRecorded[i][tempDomAxis]][1] = tempEnd;
			axisRangesRecorded[i][tempDomAxis] ++;
		}
		ptr++;
	}
}


// function to create match pairs for SDA
// requires pointers to hold returned stats, as well as sensor # to match metrics with
// returns 2d array of matched gtStepIndices and predictedStepIndices pairings
// return NULL if sensor argument incorrect
int** LoadedFileInfo::matchSDA(int sensor, int* fp, int* fn, int* tp, int* maxlen) 
{
	*fp = 0, *fn = 0, *tp = 0;
	int RANGE = 7;
	gtStepIndices.sort();
	std::list<int> *predictedStepIndices;
	
	// set predictedStepIndices to point to the correct sensor predicted index array
	if (sensor == 1)
	{
		predictedStepIndices = &predictedStepIndices1;
	}
	else if (sensor == 2)
	{
		predictedStepIndices = &predictedStepIndices2;
	}
	else if (sensor == 3)
	{
		predictedStepIndices = &predictedStepIndices3;
	}
	else
	{
		return NULL;
	}

	// find SDA
	std::list<int>::iterator i = (*predictedStepIndices).begin();
	std::list<int>::iterator j = gtStepIndices.begin();
	*maxlen = (*predictedStepIndices).size();
	if (gtStepIndices.size() > *maxlen)
	{
		*maxlen = gtStepIndices.size();
	}
	int** matches = (int**)calloc(*maxlen, sizeof(int*));
	for (int a = 0; a < *maxlen; a++)
	{
		matches[a] = (int*)calloc(2, sizeof(int));
	}

	while (i != (*predictedStepIndices).end() && j != gtStepIndices.end())
	{
		if (*i < (*j - RANGE))
		{
			i++;
			(*fp)++;
		}
		else if (*i > (*j + RANGE)) 
		{
			j++;
			(*fn)++;
		}
		else {
			matches[*tp][0] = *i; // predicted step index
			matches[*tp][1] = *j; // gt step index
			(*tp)++;
			i++;
			j++;
		}
	}

	// get remaining fp and fn if they do not match in count
	int diff = (*predictedStepIndices).size() - gtStepIndices.size();
	if (diff < 0) {
		(*fp) -= diff;
	}
	else {
		(*fn) += diff;
	}

	return matches;
}


// function to free memory allocated for SDA index match array
// requires pointer to matched index array and maxlen argument
void LoadedFileInfo::deleteSDA(int** matches, int maxlen)
{
	for (int i = 0; i < maxlen; i++) {
		free(matches[i]);
	}
	free(matches);
	return;
}