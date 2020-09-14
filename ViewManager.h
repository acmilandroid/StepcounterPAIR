#pragma once
#include "Video.h"
#include "LoadedFileInfo.h"
#include "Resource.h"
#include <list>

class ViewManager
{
	public:
	~ViewManager(void);
	static ViewManager& GetInstance(void);
	//int GetPlayMode(void);
	void SetPlayMode(int i);
	void SetWindowHandle(HWND h);
	void LoadVideoFile(std::string);
	void Paint(void);
	void OneTimePaints(void);
	void FreeData(void);
	void ShiftPosition(int delta);
	void SetPosition(int pos) {curPos = pos;}
	int GetDataWidth(void) {return dataRect.right - dataRect.left;}
	void TogglePlay(void);
	int	GetPlayMode(void) {return playMode;}
	void LeftStepButtonPress(void);
	void RightStepButtonPress(void);
	void LeftEdgecaseButtonPress(void);
	void RightEdgecaseButtonPress(void);
	void DeleteStep(void);
	void CancelCurrentStep(void);
	void JumpIndex(void);
	void JumpToLastStep(int condition);
	void JumpToNextStep(int condition);
	void SetDisplaySensor(int value);
	int GetDisplaySensor(void);
	void UpdateDataScale(bool positive);
	void ResetCurPos(void);

private:
	void ClearDataWindow(void);
	void DisplayData(void);
	void DisplayFrame(void);
	void DrawInstruction(void);
	void DrawScrollBar(void);
	void DrawDataInfo(void);
	void DrawData(void);
	void calculateTime(int pos, int &HH, int &MM, int &SS);
	void CompleteStep(void);
	ViewManager(void);
	ViewManager(const ViewManager&);
	ViewManager& operator=(const ViewManager);

	int playMode;
	int displaySensor;
	int curPos;
	const int leftGap;
	const int lineGap;
	const int infoGap;
	int videoCols;
	int videoRows;
	const int titleSpace;
	RECT windowRect;
	RECT dataRect;
	HWND hWnd;
	Video currentVideo;
	const int videoDisplayRows;
	const int videoDisplayCols;
	long int videoIndices[120];
	int numVideoIndices;
	HWND videoInstructionHandle;
	bool dataLoaded;
	bool videoLoaded;
	bool isPlaying;
	int labelStep;
	int stepStartIndex;
	HWND hwndJumpStatic;
	HWND hwndJumpInput;
	HWND hwndJumpButton;
	int dataScale;
};

