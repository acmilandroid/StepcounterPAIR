#include "StdAfx.h"
#include "ViewManager.h"
#include <algorithm>


ViewManager::ViewManager(void): 
	playMode(-1), 
	displaySensor(0),
	curPos(0), 
	leftGap(10), 
	lineGap(20),
	infoGap(200),
	videoCols(0),
	videoRows(0),
	titleSpace(20), 
	windowRect(), 
	dataRect(), 
	hWnd(),
	currentVideo(),
	videoDisplayRows(640),
	videoDisplayCols(480),
	videoLoaded(false),
	isPlaying(false),
	labelStep(0),
	stepStartIndex(-1),
	dataScale(6)
{

}

ViewManager& ViewManager::GetInstance()
{
	static ViewManager theManager;
	return theManager;
}

ViewManager::~ViewManager(void)
{
}

void ViewManager::SetPlayMode(int i)
{
	playMode = i;
}

void ViewManager::SetDisplaySensor(int value)
{
	displaySensor = value;
}

void ViewManager::SetWindowHandle(HWND h)
{
	hWnd = h;
	if(GetClientRect(hWnd,&windowRect))
	{
		dataRect.left = windowRect.left;
		dataRect.right = windowRect.right - videoDisplayCols;
		dataRect.top = windowRect.top + titleSpace*NUM_SENSORS;
		dataRect.bottom = windowRect.bottom - titleSpace;
	}
}

void ViewManager::LoadVideoFile(std::string filename)
{
	videoLoaded = currentVideo.OpenVideoFile(filename.c_str(), videoRows, videoCols);	
}

void ViewManager::Paint(void)
{
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	
	ClearDataWindow();
	
	if(fInfo.FilesAreLoaded())
	{
		DisplayData();
		if(videoLoaded)
		{
			DisplayFrame();
		}
	}
}

void ViewManager::ClearDataWindow(void)
{
	HDC hDC;
	HBRUSH brush;
	RECT rect;

	rect = dataRect;
	rect.top = rect.top - 10;
	rect.left = rect.left - 10;
	rect.right = rect.right + 10;
	rect.bottom = rect.bottom + 10;
	hDC=GetDC(hWnd);
	brush=CreateSolidBrush(RGB(255,255,255));
	FillRect(hDC,&rect,brush);
	DeleteObject(brush);
	ReleaseDC(hWnd,hDC);

	rect.top = videoDisplayRows;
	rect.left = windowRect.right - videoDisplayCols;
	rect.right = windowRect.right;
	rect.bottom = windowRect.bottom;
	hDC=GetDC(hWnd);
	brush=CreateSolidBrush(RGB(255,255,255));
	FillRect(hDC,&rect,brush);
	DeleteObject(brush);
	ReleaseDC(hWnd,hDC);
}

void ViewManager::DisplayData(void)
{
	DrawScrollBar();
	DrawDataInfo();
	DrawData();
}

void ViewManager::DrawScrollBar(void)
{
	SCROLLINFO bar_pos;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	bar_pos.cbSize=sizeof(SCROLLINFO);
	bar_pos.fMask=SIF_ALL;
	bar_pos.nMin=0; 
	bar_pos.nMax= fInfo.GetMaxNumData() - 1 + dataRect.right-dataRect.left - 1;
	bar_pos.nPage=dataRect.right-dataRect.left;
	bar_pos.nPos=curPos;
	SetScrollInfo(hWnd,SB_HORZ,&bar_pos,TRUE);
	ShowScrollBar(hWnd,SB_HORZ,TRUE);
}

void ViewManager::DrawDataInfo(void)
{
	char	text[320], sensStr[320];
	int		pos[2], sens, axis, HH, MM, SS;
	HDC		hDC;
	HFONT	hFont, hOldFont;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	//int startSensor;
	//int stopSensor;

	hFont = CreateFont(0,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_ROMAN,"");

	//if(displaySensor == 0)
	//{
	//	stopSensor = NUM_SENSORS;
	//	startSensor = 0;
	//}
	//else
	//{
	//	stopSensor = startSensor = displaySensor - 1;
	//}

	if(fInfo.GetMaxNumData() > 0)
	{
		hDC=GetDC(hWnd);
		for(sens=0; sens<NUM_SENSORS; sens++)
		{
			pos[1] = 0 + lineGap*sens;
			if(sens == 0)
			{
				strcpy(sensStr,"Wrist");
			}
			else if(sens == 1)
			{
				strcpy(sensStr,"Hip");
			}
			else if(sens == 2)
			{
				strcpy(sensStr,"Ankle");
			}
			for(axis=0;axis<NUM_AXES;axis++)
			{
				pos[0] = leftGap + infoGap*axis;
		
				
				if(axis == 0)
				{
					sprintf(text,"%s X: %7.3f",sensStr,fInfo.GetData(sens,axis,curPos));
				}
				else if(axis == 1)
				{
					sprintf(text,"%s Y: %7.3f",sensStr,fInfo.GetData(sens,axis,curPos));
				}
				else if(axis == 2)
				{
					sprintf(text,"%s Z: %7.3f",sensStr,fInfo.GetData(sens,axis,curPos));
				}

				/*if(axis == 0)
				{
					sprintf(text,"Sensor%d X: %7.3f",sens,fInfo.GetData(sens,axis,curPos));
				}
				else if(axis == 1)
				{
					sprintf(text,"Sensor%d Y: %7.3f",sens,fInfo.GetData(sens,axis,curPos));
				}
				else if(axis == 2)
				{
					sprintf(text,"Sensor%d Z: %7.3f",sens,fInfo.GetData(sens,axis,curPos));
				}*/
				if (hOldFont = (HFONT)SelectObject(hDC, hFont)) 
				{
					TextOut(hDC,pos[0],pos[1],text,strlen(text));      
					SelectObject(hDC, hOldFont); 
				}

			}
		}
		pos[1] = 0;

		pos[0] = leftGap + infoGap*NUM_AXES;
		calculateTime(curPos, HH, MM, SS);
		sprintf(text,"Time: %02d:%02d:%02d", HH, MM, SS);
		if (hOldFont = (HFONT)SelectObject(hDC, hFont)) 
		{
			TextOut(hDC,pos[0],pos[1],text,strlen(text));      
			SelectObject(hDC, hOldFont); 
		}

		pos[0] = leftGap + infoGap*(NUM_AXES+1);
		sprintf(text,"Index: %d", curPos);
		if (hOldFont = (HFONT)SelectObject(hDC, hFont)) 
		{
			TextOut(hDC,pos[0],pos[1],text,strlen(text));      
			SelectObject(hDC, hOldFont); 
		}

		pos[1] = 0 + lineGap;
		pos[0] = leftGap + infoGap * NUM_AXES;
		sprintf(text, "Time: %02d:%02d:%02d", HH, MM, SS);
		if (hOldFont = (HFONT)SelectObject(hDC, hFont))
		{
			TextOut(hDC, pos[0], pos[1], text, strlen(text));
			SelectObject(hDC, hOldFont);
		}

		ReleaseDC(hWnd,hDC);
	}
}

void ViewManager::calculateTime(int pos, int &HH, int &MM, int &SS)
{
	pos = pos / SAMPLE_RATE;
	HH = pos / 3600;
	pos = pos - HH * 3600;
	MM = pos / 60; 
	pos = pos - MM * 60;
	SS = pos / 1;
}

void ViewManager::DrawData(void)
{
	int		startHeight, endHeight, dataIndex, dataWidth, dataHeight, numData, curSensor, screenXCoord, h;
	float	data, nextData;
	HDC		hDC;
	// data boundaries: [0] = screen X coord corresponding to first data drawn
	// data boundaries: [1] = screen X coord corresponding to last data drawn
	// data boundaries: [2] = data index corresponding to first data drawn
	// data boundaries: [3] = data index corresponding to last data drawn
	int		dataBoundaries[4];
	int		foot;
	int		axisNum;
	bool	result;
	bool	bolded;
	float	maxMax, minMin;
	POINT	  linearray[2];
	HBRUSH	  brush;
	HPEN	  pen;
	HGDIOBJ	  prevObj;
	RECT	  rect, p_rect;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	int startSensor;
	int stopSensor;
	int fp, fn, tp, maxlen; // stats
	int** matches = fInfo.matchSDA(&fp, &fn, &tp, &maxlen); // get array of matching indices
	int matchCount = 0; // counter for number of matches
	int predictionMatch = 0, gtMatch = 0;

	if(displaySensor == 0)
	{
		startSensor = 0;
		stopSensor = NUM_SENSORS;
	}
	else
	{
		startSensor = displaySensor - 1;
		stopSensor = displaySensor;
	}

	for(int i=0;i<NUM_SENSORS;i++)
	{
		for(int j=0;j<NUM_AXES;j++)
		{
			if(i==0 && j==0)
			{
				maxMax = fInfo.GetMaxData(i,j);
				minMin = fInfo.GetMinData(i,j);
			}
			else
			{
				if(fInfo.GetMaxData(i,j) > maxMax)
				{
					maxMax = fInfo.GetMaxData(i,j);
				}
				if(fInfo.GetMinData(i,j) < minMin)
				{
					minMin = fInfo.GetMinData(i,j);
				}
			}
		}
	}

	hDC = GetDC(hWnd); 

	dataWidth = (dataRect.right - dataRect.left)/dataScale;
	dataHeight = (dataRect.bottom - dataRect.top);
	fInfo.MakeStepSubsets(curPos - dataWidth/2, curPos + dataWidth/2);

	if(displaySensor == 0)
	{
		TextOut(hDC,windowRect.right - videoDisplayCols+2,videoDisplayRows+2,"Wrist,Hip,Ankle",15);
	}
	else if(displaySensor == 1)
	{
		TextOut(hDC,windowRect.right - videoDisplayCols+2,videoDisplayRows+2,"Wrist",5);
	}
	else if(displaySensor == 2)
	{
		TextOut(hDC,windowRect.right - videoDisplayCols+2,videoDisplayRows+2,"Hip",3);
	}
	else
	{
		TextOut(hDC,windowRect.right - videoDisplayCols+2,videoDisplayRows+2,"Ankle",5);
	}

	// draw upper boundary for data window
	linearray[0].x = dataRect.left;
	linearray[1].x = dataRect.right;
	linearray[0].y = dataRect.top; 
	linearray[1].y = dataRect.top; 
	pen=CreatePen(PS_SOLID,1,RGB(0,0,0));
	prevObj = SelectObject(hDC,pen);
	Polyline(hDC,linearray,2);	
	SelectObject(hDC,prevObj);
	DeleteObject(pen);

	for(curSensor=startSensor; curSensor<stopSensor; curSensor++)
	{
		for(axisNum=0;axisNum<NUM_AXES;axisNum++)
		{
			numData = fInfo.GetNumData(curSensor);

			// height reserved for data from each sensor
			h = dataHeight/((stopSensor-startSensor)*NUM_AXES);
			startHeight = dataRect.top + h*(curSensor-startSensor)*3 + h*axisNum;
			endHeight = startHeight + h;
			screenXCoord = 0; 
			dataIndex = curPos - dataWidth/2;

			dataBoundaries[0] = screenXCoord;
			dataBoundaries[1] = screenXCoord + dataWidth*dataScale;
			dataBoundaries[2] = dataIndex;
			dataBoundaries[3] = dataIndex + dataWidth;

			while (screenXCoord < dataWidth*dataScale)
			{
				if (dataIndex < 0 || dataIndex > numData - 1)
				{
					screenXCoord += dataScale;
					dataIndex++;
					continue;
				}

				linearray[0].x = screenXCoord;
				linearray[1].x = screenXCoord + dataScale;

				data = fInfo.GetData(curSensor, axisNum, dataIndex);
				bolded = fInfo.GetIfDomAxis(curSensor, axisNum, dataIndex);

				linearray[0].y = (static_cast<long>(startHeight) + static_cast<long>(h) * (maxMax - data) / (maxMax - minMin)); 
				linearray[1].y = (static_cast<long>(startHeight) + static_cast<long>(h) * (maxMax - data) / (maxMax - minMin)); 

				result = false;

				// draw green box to indicate current index
				if (dataIndex == curPos)
				{
					rect.left=linearray[0].x-5;
					rect.right=linearray[1].x+5;
					rect.bottom=(static_cast<long>(startHeight) + static_cast<long>(h) * (maxMax - data) / (maxMax - minMin)) +5;
					rect.top=(static_cast<long>(startHeight) + static_cast<long>(h) * (maxMax - data) / (maxMax - minMin)) -5;
					brush=CreateSolidBrush(RGB(0,255,0));
					FillRect(hDC,&rect,brush);
					DeleteObject(brush);
				}

				// draw the raw data points
				if (dataIndex <= numData - 1)
				{
					nextData = fInfo.GetData(curSensor,axisNum,dataIndex+1);
					linearray[0].x = screenXCoord;
					linearray[1].x = screenXCoord + dataScale;
					linearray[0].y = (long) (startHeight + h * (maxMax - data) / (maxMax - minMin)); 
					linearray[1].y = (long) (startHeight + h * (maxMax - nextData) / (maxMax - minMin)); 
		
					if(bolded == false)
					{
						pen=CreatePen(PS_SOLID,2,RGB(0,0,0));
					}
					else
					{
						// *** RYAN *** change 2 to 4
						pen=CreatePen(PS_SOLID,2,RGB(0,0,0));
					}
					prevObj = SelectObject(hDC,pen);
					Polyline(hDC,linearray,2);	
					SelectObject(hDC,prevObj);
					DeleteObject(pen);
				}
			
				// if a step is found, mark it
				result = fInfo.IndexMatch(dataIndex, foot);
				if (result)
				{
					rect.left = screenXCoord;
					rect.right = screenXCoord + (dataScale + 1) / 2;
					rect.bottom = linearray[0].y;
					rect.top = startHeight + 5;

					p_rect.left = screenXCoord;
					p_rect.right = screenXCoord + (dataScale + 1) / 2;
					p_rect.bottom = endHeight - 5;
					p_rect.top = linearray[0].y;

					// check if indices match with predicted or gt indices
					predictionMatch = -1;
					gtMatch = -1;
					if (dataIndex == 632) {
						printf("hello world\n");
					}
					for (int i = 0; i < maxlen; i++)
					{
						if (dataIndex == matches[i][0])
						{
							predictionMatch = i;
							break;
						}
					}
					for (int i = 0; i < maxlen; i++)
					{
						if (dataIndex == matches[i][1])
						{
							gtMatch = i;
							break;
						}
					}

					// index matches with any step
					if (predictionMatch != -1 || gtMatch != -1)
					{
						// index matches a step prediction
						if (predictionMatch != -1)
						{
							if (predictionMatch % 2 == 0) brush = CreateSolidBrush(RGB(0, 230, 0));
							else brush = CreateSolidBrush(RGB(0, 100, 0));
							FillRect(hDC, &p_rect, brush);
						}
						// index matches a gt step
						if (gtMatch != -1)
						{
							if (gtMatch % 2 == 0) brush = CreateSolidBrush(RGB(0, 230, 0));
							else brush = CreateSolidBrush(RGB(0, 100, 0));
							FillRect(hDC, &rect, brush);
						}
					}
					// prediction with no match
					else if (foot == 4)
					{
						brush = CreateSolidBrush(RGB(255, 0, 0));
						FillRect(hDC, &p_rect, brush);
					}
					// gt step with no match
					else {
						brush = CreateSolidBrush(RGB(255, 0, 0));
						FillRect(hDC, &rect, brush);
					}

					// if left foot step, color blue
					//if (foot == 0)
					//{
					//	brush = CreateSolidBrush(RGB(0, 0, 255));
					//	FillRect(hDC, &rect, brush);
					//}
					//// if right foot step, color red
					//else if (foot == 1)
					//{
					//	brush = CreateSolidBrush(RGB(255, 0, 0));
					//	FillRect(hDC, &rect, brush);
					//}
					//// if left edgecase, color light blue
					//else if (foot == 2)
					//{
					//	brush = CreateSolidBrush(RGB(0, 0, 255));
					//	FillRect(hDC, &rect, brush);
					//}
					//// if right edgecase step, color light red
					//else if (foot == 3)
					//{
					//	brush = CreateSolidBrush(RGB(255, 0, 0));
					//	FillRect(hDC, &rect, brush);
					//}
					//// if predicted step, color dark green
					//else if (foot == 4)
					//{
					//	brush = CreateSolidBrush(RGB(0, 160, 0));
					//	FillRect(hDC, &p_rect, brush);
					//}

					DeleteObject(brush);
				}

				screenXCoord+=dataScale;
				dataIndex++;
			}
		}
	}
	for (int i = 0; i < maxlen; i++) {
		free(matches[i]);
	}
	free(matches);
	ReleaseDC(hWnd,hDC);
}


void ViewManager::DisplayFrame(void)
{
	BITMAPINFO		*bm_info;
	unsigned char	*dispImage = NULL;
	HDC				hDC;
	int				i;
	RECT			VideoRect;
	HBRUSH			brush;
	//int				Frame_Index;
	double				Time_Seconds;
	int				dataWidth;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	
	dataWidth = dataRect.right - dataRect.left;

		// do float math to determine approximate frame to display for the current data sample based on fps of the video and sample rate of the sensor
	
	// this would be the frame index to grab if it has a frame rate of FRAME_RATE
	//temp = (static_cast<float>(fInfo.GetVideoOffset()) + static_cast<float>(curPos)) / static_cast<float>(SAMPLE_RATE) * FRAME_RATE;
	// round to the nearest frame to display
	//Frame_Index = (int)(temp+0.5);
	
	// this version is the time in seconds that we would want
	Time_Seconds = (static_cast<double>(fInfo.GetVideoOffset()) + static_cast<double>(curPos)) / static_cast<double>(SAMPLE_RATE);

	if (videoLoaded == true)
	{
		dispImage=(unsigned char *)calloc(videoRows*videoCols*3,1);
		currentVideo.ReadVideoFrame(Time_Seconds,dispImage);

		hDC=GetDC(hWnd);
		bm_info=(BITMAPINFO *)malloc(sizeof(BITMAPINFO)+256*sizeof(RGBQUAD));
		bm_info->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		//bm_info->bmiHeader.biWidth=videoCols;
		//bm_info->bmiHeader.biHeight=-videoRows;
		// if rotated
		bm_info->bmiHeader.biWidth=videoRows;
		bm_info->bmiHeader.biHeight=-videoCols;
		bm_info->bmiHeader.biPlanes=1;
		bm_info->bmiHeader.biBitCount=24;
		bm_info->bmiHeader.biCompression=BI_RGB;
		bm_info->bmiHeader.biSizeImage=0;
		bm_info->bmiHeader.biXPelsPerMeter=0;
		bm_info->bmiHeader.biYPelsPerMeter=0;
		bm_info->bmiHeader.biClrUsed=0;
		bm_info->bmiHeader.biClrImportant=0;
		for (i=0; i<256; i++)
		{
			bm_info->bmiColors[i].rgbBlue=bm_info->bmiColors[i].rgbGreen=bm_info->bmiColors[i].rgbRed=i;
			bm_info->bmiColors[i].rgbReserved=0;
		}

//		StretchDIBits(hDC,windowRect.right - videoDisplayCols, 0, videoDisplayCols, videoDisplayRows, 0, 0, videoCols, videoRows,
//								dispImage, bm_info, DIB_RGB_COLORS, SRCCOPY);

		// if rotated
		StretchDIBits(hDC,windowRect.right - videoDisplayCols, 0, videoDisplayCols, videoDisplayRows, 0, 0, videoRows, videoCols,
								dispImage, bm_info, DIB_RGB_COLORS, SRCCOPY);
		
		free(bm_info);
		ReleaseDC(hWnd,hDC);
	}
	else
	{
		VideoRect.left = dataWidth - videoDisplayCols + leftGap;
		VideoRect.right = VideoRect.left + videoDisplayCols;
		VideoRect.top = 0;
		VideoRect.bottom = videoDisplayRows;
		hDC=GetDC(hWnd);
		brush=CreateSolidBrush(RGB(255,255,255));
		FillRect(hDC,&VideoRect,brush);
		DeleteObject(brush);
		ReleaseDC(hWnd,hDC);
	}

	if(dispImage != NULL)
	{
		free(dispImage);
	}
	return;
}

void ViewManager::OneTimePaints(void)
{
	//Jump to Index Button
	hwndJumpStatic = CreateWindow(TEXT("STATIC"),TEXT("Jump To Index:"), WS_VISIBLE | WS_BORDER | WS_CHILD, leftGap+(NUM_AXES+2)*infoGap, 0, 110, 20, hWnd, (HMENU) ID_JUMP_STEXT, NULL,NULL); 
	hwndJumpInput = CreateWindow(TEXT("EDIT"),TEXT(""), WS_CHILD | WS_BORDER | WS_VISIBLE, leftGap+(NUM_AXES+2)*infoGap+110, 0, 60, 20, hWnd, (HMENU) ID_JUMP_INDEX, NULL,NULL);
	hwndJumpButton = CreateWindow(TEXT("BUTTON"),TEXT("Jump"), WS_CHILD | WS_VISIBLE | WS_BORDER, leftGap+(NUM_AXES+2)*infoGap+110+60, 0, 60, 20, hWnd, (HMENU) ID_JUMP_BUTTON, NULL,NULL);


	// instructions
	//videoInstructionHandle = CreateWindow(TEXT("STATIC"),TEXT("Video Controls: \n\nA: Large Back\nS: Small Back\nD: Play/Pause\nF: Small Forward\nG: Large Forward"),WS_VISIBLE | WS_CHILD,leftGap + 210,titleSpace+280,180,120,hWnd,(HMENU) ID_VIDEO_CONTROLS,NULL,NULL);

	//		/* Note: commented out to simplify interface.  Turned on smoothing by default, with no option to turn off built in
	//		//Checkbox and Editbox for Gaussian smoothing
	//		hwndGaussChk = CreateWindow(TEXT("BUTTON"), TEXT("Smooth ["), BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD, LEFT_GAP, TITLE_SPACE, 100, 20, hWnd, (HMENU) ID_GAUSS_CHKBOX, NULL, NULL);
	//		CreateWindow(TEXT("STATIC"),TEXT("Sigma:"),WS_VISIBLE | WS_CHILD,LEFT_GAP + 100, TITLE_SPACE , 50,20, hWnd, (HMENU) NULL, NULL,NULL); 
	//		hwndGaussSigmaEditbox = CreateWindow(TEXT("EDIT"),TEXT("10.0"), WS_VISIBLE | WS_CHILD | WS_BORDER, LEFT_GAP + 157,TITLE_SPACE, 50, 20, hWnd, (HMENU) ID_GAUSS_SIGMA_EDITBOX, NULL,NULL);
	//		CreateWindow(TEXT("STATIC"),TEXT("]"),WS_VISIBLE | WS_CHILD,LEFT_GAP + 220,TITLE_SPACE, 10,20,hWnd, (HMENU) NULL, NULL,NULL);
	//		*/

	//		//Include Ability to Swap betweek Auto and Manual Labeling
	//		hwndAutoChk = CreateWindow(TEXT("BUTTON"), TEXT("Auto"), BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD, LEFT_GAP, TITLE_SPACE + 260, 50, 20, hWnd, (HMENU) ID_AUTO_CHKBOX, NULL, NULL);

	//		//Scale Data Consistency settings
	//		hwndConsistencyRangeStatictext = CreateWindow(TEXT("STATIC"),TEXT("Range:"), WS_VISIBLE | WS_BORDER | WS_CHILD, LEFT_GAP, TITLE_SPACE + 280, 70, 20, hWnd, (HMENU) ID_CONSISTENCY_RANGE_STEXT, NULL,NULL); 
	//		hwndConsistencyRangeBox = CreateWindow(TEXT("EDIT"),TEXT(""), WS_CHILD | WS_BORDER | WS_VISIBLE, LEFT_GAP + 70,TITLE_SPACE + 280, 60, 20, hWnd, (HMENU) ID_CONSISTENCY_RANGE_VALUE, NULL,NULL);
	//		hwndSetConsistencyRangeButton = CreateWindow(TEXT("BUTTON"),TEXT("Update"), WS_CHILD | WS_VISIBLE | WS_BORDER, LEFT_GAP + 130,TITLE_SPACE + 280, 70, 20, hWnd, (HMENU) ID_CONSISTENCY_RANGE_BUTTON, NULL,NULL);
	//		hwndConsistencyTimeStatictext = CreateWindow(TEXT("STATIC"),TEXT("Indexes:"), WS_VISIBLE | WS_BORDER | WS_CHILD, LEFT_GAP, TITLE_SPACE + 300, 70, 20, hWnd, (HMENU) ID_CONSISTENCY_TIME_STEXT, NULL,NULL); 
	//		hwndConsistencyTimeBox = CreateWindow(TEXT("EDIT"),TEXT(""), WS_CHILD | WS_BORDER | WS_VISIBLE, LEFT_GAP + 70,TITLE_SPACE + 300, 60, 20, hWnd, (HMENU) ID_CONSISTENCY_TIME_VALUE, NULL,NULL);
	//		hwndConsistencyTimeButton = CreateWindow(TEXT("BUTTON"),TEXT("Update"), WS_CHILD | WS_VISIBLE | WS_BORDER, LEFT_GAP + 130,TITLE_SPACE + 300, 70, 20, hWnd, (HMENU) ID_CONSISTENCY_TIME_BUTTON, NULL,NULL);
	//	
	

	//		//Label List
	//		CreateWindow(TEXT("STATIC"),TEXT("Jump to Label:"),WS_VISIBLE | WS_CHILD,LEFT_GAP + 320, TITLE_SPACE , 180,20, hWnd, (HMENU) NULL, NULL,NULL); 
	//		hwndLabelListbox = CreateWindow(TEXT("LISTBOX"), TEXT(""), WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL | WS_BORDER , LEFT_GAP + 320, TITLE_SPACE + 20, 260, 200, hWnd, (HMENU) ID_LABELLIST_LSTBOX, NULL, NULL);

	//		//Delete button
	//		hwndDeleteBtn = CreateWindow(TEXT("BUTTON"), TEXT("Delete Label"), WS_VISIBLE | WS_CHILD, LEFT_GAP + 320, TITLE_SPACE + 220, 180, 20, hWnd, (HMENU) ID_DELETE_BTN, NULL, NULL);

	//		//Label Checkbox
	//		//hwndLabelModeChk = CreateWindow(TEXT("BUTTON"), TEXT("Label data"), BS_AUTOCHECKBOX | WS_VISIBLE | WS_CHILD, LEFT_GAP + 450, TITLE_SPACE + 20, 100, 20, hWnd, (HMENU) ID_LABELDATA_CHKBOX, NULL, NULL);
	//		hwndB1Statictext = CreateWindow(TEXT("STATIC"),TEXT("Boundary1:"),WS_CHILD,LEFT_GAP + 520, TITLE_SPACE + 50 , 100,20, hWnd, (HMENU) ID_BOUNDARY1_STEXT, NULL,NULL); 
	//		hwndB2Statictext = CreateWindow(TEXT("STATIC"),TEXT("Boundary2:"),WS_CHILD,LEFT_GAP + 520, TITLE_SPACE + 70 , 100,20, hWnd, (HMENU) ID_BOUNDARY2_STEXT, NULL,NULL); 
	//		hwndB1Editbox = CreateWindow(TEXT("EDIT"),TEXT(""), WS_CHILD | WS_BORDER | WS_DISABLED, LEFT_GAP + 540,TITLE_SPACE + 50, 80, 20, hWnd, (HMENU) ID_B1_EDITBOX, NULL,NULL);
	//		hwndB2Editbox = CreateWindow(TEXT("EDIT"),TEXT(""), WS_CHILD | WS_BORDER | WS_DISABLED, LEFT_GAP + 540,TITLE_SPACE + 70, 80, 20, hWnd, (HMENU) ID_B2_EDITBOX, NULL,NULL);
	//	
	//		//Instructions
	//		hwndLabelingControlsInfo = CreateWindow(TEXT("STATIC"),TEXT("Labeling Controls: \n\nQ: Label Scale\nESC: Cancel Label"),WS_VISIBLE | WS_CHILD,LEFT_GAP+420,TITLE_SPACE+280,180,120,hWnd,(HMENU) ID_LABEL_CONTROLS,NULL,NULL);
	//		hwndVideoControlsInfo = CreateWindow(TEXT("STATIC"),TEXT("Video Controls: \n\nA: Large Back\nS: Small Back\nD: Play/Pause\nF: Small Forward\nG: Large Forward"),WS_VISIBLE | WS_CHILD,LEFT_GAP + 210,TITLE_SPACE+280,180,120,hWnd,(HMENU) ID_VIDEO_CONTROLS,NULL,NULL);

	//		//Labels button
	//		//hwndLabelsBtn = CreateWindow(TEXT("BUTTON"), TEXT("Label Data"), WS_VISIBLE | WS_CHILD, LEFT_GAP + 275, TITLE_SPACE + 220, 135, 20, hWnd, (HMENU) ID_LABELS_BTN, NULL, NULL);

	//		SendMessage(hwndAutoChk, BM_SETCHECK, BST_CHECKED, 0);
}

void ViewManager::FreeData(void)
{
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	fInfo.FreeData();
	if(videoLoaded)
	{
		currentVideo.CloseVideoFile();
		videoLoaded = 0;
	}
}

void ViewManager::ShiftPosition(int delta)
{
	int magnitude, direction;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	magnitude = abs(delta);
	if(delta < 1)
	{
		direction = -1;
	}
	else
	{
		direction = 1;
	}

	if(direction == -1)
	{
		if((curPos - magnitude) < 0)
		{
			curPos = 0;
			if(isPlaying)
			{
				KillTimer(hWnd, TIMER_SECOND);
				isPlaying = false;
			}
		}
		else
		{
			curPos = curPos - magnitude;
		}
	}
	else
	{
		if((curPos + magnitude) > fInfo.GetMaxNumData() - 1)
		{
			curPos = fInfo.GetMaxNumData() - 1;
			if(isPlaying)
			{
				KillTimer(hWnd, TIMER_SECOND);
				isPlaying = false;
			}
		}
		else
		{
			curPos = curPos + magnitude;
		}
	}
}

void ViewManager::TogglePlay(void)
{
	isPlaying = !isPlaying;
	if (isPlaying)
		SetTimer(hWnd,TIMER_SECOND,10,NULL);	/* 10 ms timer; basically run fast */
	else
		KillTimer(hWnd,TIMER_SECOND);
}

void ViewManager::LeftStepButtonPress(void)
{
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	fInfo.AddLeftStep(curPos);
}
	
void ViewManager::RightStepButtonPress(void)
{
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	fInfo.AddRightStep(curPos);
}

void ViewManager::LeftEdgecaseButtonPress(void)
{
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	fInfo.AddLeftEdgecase(curPos);
}
	
void ViewManager::RightEdgecaseButtonPress(void)
{
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	fInfo.AddRightEdgecase(curPos);
}
//void ViewManager::LabelButtonPress(void)
//{
//	labelStep ++;
//	if(labelStep == 1)
//	{
//		stepStartIndex = curPos;
//	}
//	if(labelStep == 2)
//	{
//		CompleteStep();
//		stepStartIndex = -1;
//		labelStep = 0;
//	}
//}
//
//void ViewManager::CompleteStep(void)
//{
//	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
//
//	fInfo.AddStep(stepStartIndex,curPos);
//}
	
void ViewManager::DeleteStep(void)
{
	int dataWidth;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();
	
	dataWidth = (dataRect.right - dataRect.left)/dataScale;

	// will delete next step marked from curPos up to the end of the displayed data
	fInfo.DeleteStep(curPos,curPos+dataWidth/2);
}

void ViewManager::CancelCurrentStep(void)
{
	labelStep = 0;
	stepStartIndex = -1;
}

void ViewManager::JumpIndex(void)
{
	BOOL validInt;
	int index;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	index = GetDlgItemInt(hWnd,ID_JUMP_INDEX,&validInt,FALSE);

	if(validInt)
	{
		if(index >= 0 && index <= fInfo.GetMaxNumData()-1)
		{
			curPos = index;
		}
		else if(index > fInfo.GetMaxNumData()-1)
		{
			curPos = fInfo.GetMaxNumData() - 1;
		}
		else if(index < 0)
		{
			curPos = 0;
		}
		if (isPlaying == 1)
		{
			KillTimer(hWnd,TIMER_SECOND);
			isPlaying=0;
		}
		Paint();
	}
	SetWindowText(hwndJumpInput,"");
}

void ViewManager::JumpToLastStep(int condition)
{
	int temp;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	temp = fInfo.GetLastStepIndex(curPos, condition);
	if(temp != -1)
	{
		curPos = temp;
	}
}

void ViewManager::JumpToNextStep(int condition)
{
	int temp;
	LoadedFileInfo &fInfo = LoadedFileInfo::GetInstance();

	temp = fInfo.GetNextStepIndex(curPos, condition);
	if(temp != -1)
	{
		curPos = temp;
	}
}

void ViewManager::UpdateDataScale(bool positive)
{
	if(positive)
	{
		if(dataScale < 48)
		{
			dataScale += 2;
		}
		else
		{
			dataScale = 50;
		}
	}
	else
	{
		if(dataScale > 3)
		{
			dataScale -= 2;
		}
		else
		{
			dataScale = 1;
		}
	}
	Paint();
}


void ViewManager::ResetCurPos()
{
	curPos = 0;
}

int ViewManager::GetDisplaySensor()
{
	return displaySensor;
}