#pragma once
#include "stdafx.h"
#include "ViewManager.h"
#include <list>
#include <vector>
#include <algorithm>

class LoadedFileInfo
{
public:
	~LoadedFileInfo(void);
	void LoadFiles(void);
	void Clear(void);
	static LoadedFileInfo& GetInstance();
	int GetVideoOffset() {return offset;}
	int GetMaxNumData();
	int GetNumData(int sensNum);
	float GetData(int sensor, int axis, int pos);
	float GetMinData(int sensor, int axis) {return minData[sensor][axis];}
	float GetMaxData(int sensor, int axis) {return maxData[sensor][axis];}
	bool FilesAreLoaded() {return filesLoaded;}
	//bool IndexMatch(int rangeStart, int rangeEnd, int index, int arr[2]);
	bool IndexMatch(int index, int &foot, int sensornum);
	void AddRightStep(int index);
	void AddLeftStep(int index);
	void AddRightEdgecase(int index);
	void AddLeftEdgecase(int index);
	void DeleteStep(int position, int maxRange);
	bool MatchesPeak(int sensor, int axis, int pos);
	void FreeData();
	void MakeStepSubsets(int startIndex, int endIndex);
	int GetLastStepIndex(int position, int condition);
	int GetNextStepIndex(int position, int condition);
	bool GetIfDomAxis(int sensor, int axis, int pos);
	int** matchSDA(int sensor, int* fp, int* fn, int* tp, int* maxlen);
	void deleteSDA(int** matches, int maxlen);

private:
	LoadedFileInfo(void);
	LoadedFileInfo(const LoadedFileInfo&);
	LoadedFileInfo& operator=(const LoadedFileInfo);
	bool LoadData(int sensNum);
	bool LoadSync(void);
	void LoadSteps(void);
	void SaveSteps(void);
	void CalculateRange(void);
	void DetectSteps(void);
	void FixData(int sensNum);
	bool SyncData(void);
	double GetTimeValue(int sensNum, int index);
	void GetMDY(int sensNum, int index, char* MDY);
	void DecrementTime(int sensNum, int index);
	void FindDomAxes(void);

	std::string participant;
	std::string trial;
	std::string folderPath;
	std::vector<std::string> filePaths;
	std::vector<std::string> fileNames;
	
	int totalData[NUM_SENSORS];
	int offset;
	std::string videoPath;
	float minData[NUM_SENSORS][NUM_AXES];
	float maxData[NUM_SENSORS][NUM_AXES];
	int numLocalMaxima[NUM_SENSORS][NUM_AXES];
	int localMaxima[NUM_SENSORS][NUM_AXES][MAX_DATA];
//	std::list<std::pair<int, int>> steps;
	std::list<int> rightStepIndices;
	std::list<int> leftStepIndices;
	std::list<int> rightStepSubset;
	std::list<int> leftStepSubset;
	std::list<int> rightEdgecaseIndices;
	std::list<int> leftEdgecaseIndices;
	std::list<int> rightEdgecaseSubset;
	std::list<int> leftEdgecaseSubset;
	std::list<int> predictedStepIndices1;
	std::list<int> predictedStepIndices2;
	std::list<int> predictedStepIndices3;
	std::list<int> predictedStepSubset1;
	std::list<int> predictedStepSubset2;
	std::list<int> predictedStepSubset3;
	std::list<int> gtStepIndices;
	float data[NUM_SENSORS][NUM_AXES][MAX_DATA];
	char timeData[NUM_SENSORS][MAX_DATA][64];
	int syncOffsets[NUM_SENSORS];
	// for each sensor, for each axis, record ranges to be bolded as "dominant axes" 
	int domAxisRanges[NUM_SENSORS][NUM_AXES][MAX_STEPS][2];
	int axisRangesRecorded[NUM_SENSORS][NUM_AXES];

	bool filesLoaded;
	bool dataSynced;
};

