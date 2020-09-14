// StepcounterVIEW.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "StepcounterVIEW.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_STEPCOUNTERVIEW, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_STEPCOUNTERVIEW));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STEPCOUNTERVIEW));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_STEPCOUNTERVIEW);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   filesLoaded = false;
   jumpType = 0;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	LoadedFileInfo& currentFile = LoadedFileInfo::GetInstance();
	ViewManager& vManager = ViewManager::GetInstance();
	PAINTSTRUCT ps;
	HDC hdc;
	bool isPlaying;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
			case ID_FILE_LOAD:
				//if (Playing == 1)
			  //break;
				
			vManager.SetWindowHandle(hWnd);
			currentFile.Clear();
			currentFile.LoadFiles();
			break;

			case ID_FILE_EXIT:
				vManager.FreeData();
				Sleep(100);
				DestroyWindow(hWnd);
				break;
			case IDM_ABOUT:
				DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
				break;
			case IDM_EXIT:
				vManager.FreeData();
				Sleep(100);
				DestroyWindow(hWnd);
				break;
			case ID_JUMP_BUTTON:
				vManager.JumpIndex();
				break;
			case ID_DISPLAYSENSOR_WRIST:
				vManager.SetDisplaySensor(1);
				vManager.Paint();
				break;
			case ID_DISPLAYSENSOR_HIP:
				vManager.SetDisplaySensor(2);
				vManager.Paint();
				break;
			case ID_DISPLAYSENSOR_ANKLE:
				vManager.SetDisplaySensor(3);
				vManager.Paint();
				break;
			case ID_DISPLAYSENSOR_ALL:
				vManager.SetDisplaySensor(0);
				vManager.Paint();
				break;
			case ID_JUMPTO_ALL:
				jumpType = 0;
				break;
			case ID_JUMPTO_STEPSONLY:
				jumpType = 1;
				break;
			case ID_JUMPTO_EDGECASESONLY:
				jumpType = 2;
				break;
			case ID_JUMPTO_ALLRIGHT:
				jumpType = 3;
				break;
			case ID_JUMPTO_ALLLEFT:
				jumpType = 4;
				break;
			case ID_JUMPTO_RIGHTSTEPS:
				jumpType = 5;
				break;
			case ID_JUMPTO_LEFTSTEPS:
				jumpType = 6;
				break;
			case ID_JUMPTO_RIGHTEDGECASES:
				jumpType = 7;
				break;
			case ID_JUMPTO_LEFTEDGECASES:
				jumpType = 8;
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:
		SetFocus(hWnd); 
		break;

	case WM_MOUSEWHEEL:
		if(GET_KEYSTATE_WPARAM(wParam) == MK_CONTROL)
		{
			if(GET_WHEEL_DELTA_WPARAM(wParam) > 0)
			{
				vManager.UpdateDataScale(true);
			}
			else
			{	
				vManager.UpdateDataScale(false);
			}
		}
		break;

	case WM_HSCROLL:
		/*if (Playing == 1)
		  break;*/
		if (LOWORD(wParam) == SB_LINELEFT)  
		{
			vManager.ShiftPosition(-SMALL_LEAP);
		}
		if (LOWORD(wParam) == SB_LINERIGHT) 
		{
			vManager.ShiftPosition(SMALL_LEAP);
		}
		if (LOWORD(wParam) == SB_PAGELEFT) 
		{
			vManager.ShiftPosition(-vManager.GetDataWidth());
		}
		if (LOWORD(wParam) == SB_PAGERIGHT)
		{	
			vManager.ShiftPosition(vManager.GetDataWidth());
		}
		if (LOWORD(wParam) == SB_THUMBPOSITION)
		{
			vManager.SetPosition(HIWORD(wParam));
		}
		vManager.Paint();
		break;

			/* keyboard command */
	case WM_CHAR:			
		if (((TCHAR)wParam == 'a') || ((TCHAR)wParam == 'A'))
 		{			
			vManager.ShiftPosition(-BIG_LEAP);
			vManager.SetPlayMode(1);
		}
		if (((TCHAR)wParam == 's') || ((TCHAR)wParam == 'S'))
		{
			vManager.ShiftPosition(-SMALL_LEAP);
			vManager.SetPlayMode(2);
		}
		if (((TCHAR)wParam == 'd') || ((TCHAR)wParam == 'D'))
		{	
			vManager.TogglePlay();
		}
		if (((TCHAR)wParam == 'f') || ((TCHAR)wParam == 'F'))
		{	
			vManager.ShiftPosition(SMALL_LEAP);
			vManager.SetPlayMode(3);
		}
		if (((TCHAR)wParam == 'g') || ((TCHAR)wParam == 'G'))
		{		
			vManager.ShiftPosition(BIG_LEAP);
			vManager.SetPlayMode(4);
		}
		// Allow user to label a new step
		if (((TCHAR)wParam == 'r') || ((TCHAR)wParam == 'R') || ((TCHAR)wParam == 'j') || ((TCHAR)wParam == 'J'))
		{
			vManager.LeftStepButtonPress();
			//vManager.LabelButtonPress();
		}
		if (((TCHAR)wParam == 't') || ((TCHAR)wParam == 'T') || ((TCHAR)wParam == 'k') || ((TCHAR)wParam == 'K'))
		{
			vManager.RightStepButtonPress();
		}
		if (((TCHAR)wParam == 'l') || ((TCHAR)wParam == 'L'))
		{
			vManager.LeftEdgecaseButtonPress();
			//vManager.LabelButtonPress();
		}
		if (((TCHAR)wParam == ';') || ((TCHAR)wParam == ':'))
		{
			vManager.RightEdgecaseButtonPress();
		}
		if (((TCHAR)wParam == ',') || ((TCHAR)wParam == '<'))
		{
			// jump options: all steps and all edgecases, all steps, all edgecases, nondom steps, nondom edge
			vManager.JumpToLastStep(jumpType);
		}
		if (((TCHAR)wParam == '.') || ((TCHAR)wParam == '>'))
		{
			vManager.JumpToNextStep(jumpType);
		}
		if (((TCHAR)wParam == '-') || ((TCHAR)wParam == '_'))
		{
				vManager.UpdateDataScale(false);
		}
		if (((TCHAR)wParam == '=') || ((TCHAR)wParam == '+'))
		{
				vManager.UpdateDataScale(true);
		}

		// add function to allow user to delete step with delete within step boundaries (vmanager should tell file info to delete)
		vManager.Paint();
		break;

	case WM_KEYDOWN:		  /* the delete key only generates KEYDOWN message, not WM_CHAR */

		if (wParam == VK_DELETE)
		{
			vManager.DeleteStep();
			vManager.Paint();
		}
		if(wParam == VK_ESCAPE)
		{
			// if user hits ESCAPE, cancel any word currently being written
			vManager.CancelCurrentStep();
			vManager.Paint();
		}
		if(wParam == VK_UP)
		{
			if(vManager.GetDisplaySensor()-1 < 0)
			{
				vManager.SetDisplaySensor(3);
			}
			else
			{
				vManager.SetDisplaySensor(vManager.GetDisplaySensor()-1);
			}
			vManager.Paint();
		}
		if(wParam == VK_DOWN)
		{
			if(vManager.GetDisplaySensor()+1 >3)
			{
				vManager.SetDisplaySensor(0);
			}
			else
			{
				vManager.SetDisplaySensor(vManager.GetDisplaySensor()+1);
			}
			vManager.Paint();
		}

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		vManager.Paint();
		EndPaint(hWnd, &ps);
		break;

	case WM_TIMER:
		if (wParam == TIMER_SECOND)
		{
			switch (vManager.GetPlayMode())
			{
				case 1: 
					vManager.ShiftPosition(-BIG_LEAP);
					vManager.Paint();
					break;
				case 2: 
					vManager.ShiftPosition(-SMALL_LEAP);
					vManager.Paint();
					break;
				case 3: 
					vManager.ShiftPosition(SMALL_LEAP);
					vManager.Paint();
					break;
				case 4: 
					vManager.ShiftPosition(BIG_LEAP);
					vManager.Paint();
					break;
			}
		}
		break;

	case WM_DESTROY:
		vManager.FreeData();
		Sleep(100);
		DestroyWindow(hWnd);
		// memory leak report
		_CrtDumpMemoryLeaks();
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
