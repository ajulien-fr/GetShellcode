#include <windows.h>
#include <stdio.h>
#include <windowsx.h>

typedef FILE *PFILE;

#define APPLICATIONNAME "Get Shellcode\0"
#define CLASSNAME       "GetShellcode\0"

#define IDC_FILENAME_EDIT   101
#define IDC_LOAD_BUTTON     102
#define IDC_SHELLCODE_EDIT  103

#define IDA_SHELLCODE_EDIT 201

HINSTANCE hInst;
HWND hWnd;

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void				ResizeControls(HWND, HWND, HWND, HWND);
void				OnButtonClick(HWND, HWND, HWND);
void                GetShellcode(HWND, HWND, LPSTR);
void                DumpTextSegment(PFILE, IMAGE_SECTION_HEADER, HWND, HWND);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	ACCEL accel;
	HACCEL haccel;

	MyRegisterClass(hInstance);

	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	accel.fVirt = FCONTROL | FVIRTKEY;
	accel.key = 0x41;
	accel.cmd = IDA_SHELLCODE_EDIT;

	haccel = CreateAcceleratorTable(&accel, 1);

	if (haccel == NULL)
	{
		return FALSE;
	}

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, haccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = CLASSNAME;
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Stocke le handle d'instance dans la variable globale.

	hWnd = CreateWindow(CLASSNAME, APPLICATIONNAME, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 800, 500, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	static HWND hwndFilenameEdit;
	static HWND hwndLoadButton;
	static HWND hwndShellcodeEdit;

	switch (message)
	{
	case WM_CREATE:
		hwndFilenameEdit = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY | ES_LEFT,
			0, 0, 0, 0, hWnd, (HMENU)IDC_FILENAME_EDIT, hInst, NULL);
		hwndLoadButton = CreateWindow("BUTTON", "Load...", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
			0, 0, 0, 0, hWnd, (HMENU)IDC_LOAD_BUTTON, hInst, NULL);
		hwndShellcodeEdit = CreateWindow("EDIT", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | ES_READONLY | ES_LEFT | ES_MULTILINE | ES_NOHIDESEL,
			0, 0, 0, 0, hWnd, (HMENU)IDC_SHELLCODE_EDIT, hInst, NULL);
		ResizeControls(hwndFilenameEdit, hwndLoadButton, hwndShellcodeEdit, hWnd);
		break;
	case WM_SIZE:
		ResizeControls(hwndFilenameEdit, hwndLoadButton, hwndShellcodeEdit, hWnd);
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Analyse les sélections de menu :
		switch (wmId)
		{
		case IDC_LOAD_BUTTON:
			OnButtonClick(hwndFilenameEdit, hwndShellcodeEdit, hWnd);
			break;
		case IDA_SHELLCODE_EDIT:
			Edit_SetSel(hwndShellcodeEdit, 0, Edit_GetTextLength(hwndShellcodeEdit));
			SetFocus(hwndShellcodeEdit);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO : ajoutez ici le code de dessin...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

void ResizeControls(HWND hwndFilenameEdit, HWND hwndLoadButton, HWND hwndShellcodeEdit, HWND hWnd)
{
	RECT rcClient;

	GetClientRect(hWnd, &rcClient);

	MoveWindow(hwndFilenameEdit, 15, 15, rcClient.right - rcClient.left - (15 + 80 + 15 + 15), 25, TRUE);

	MoveWindow(hwndLoadButton, rcClient.right - (15 + 80), 15, 80, 25, TRUE);

	MoveWindow(hwndShellcodeEdit, 15, 15 + 25 + 15, rcClient.right - rcClient.left - (15 + 15),
		rcClient.bottom - rcClient.top - (15 + 25 + 15 + 15), TRUE);
}

void OnButtonClick(HWND hwndFilenameEdit, HWND hwndShellcodeEdit, HWND hWnd)
{
	OPENFILENAME ofn;
	TCHAR szFile[1024];

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = TEXT("Executable Files\0*.exe\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		Edit_SetText(hwndFilenameEdit, ofn.lpstrFile);
		return GetShellcode(hWnd, hwndShellcodeEdit, ofn.lpstrFile);
	}
}

void GetShellcode(HWND hWnd, HWND hwndShellcodeEdit, LPSTR lpstrFile)
{
	PFILE pfile = NULL;
	IMAGE_DOS_HEADER iDosHeader;
	IMAGE_NT_HEADERS iNtHeaders;
	IMAGE_SECTION_HEADER iSectionHeader;

	pfile = fopen(lpstrFile, "rb");

	if (pfile == NULL)
	{
		MessageBox(hWnd, "Impossible d'ouvrir le fichier.", "Erreur", MB_OK | MB_ICONERROR);
		return;
	}

	fread(&iDosHeader, sizeof(IMAGE_DOS_HEADER), 1, pfile);

	fseek(pfile, iDosHeader.e_lfanew, SEEK_SET);

	fread(&iNtHeaders, sizeof(IMAGE_NT_HEADERS), 1, pfile);

	for (WORD w = 0; w < iNtHeaders.FileHeader.NumberOfSections; w++)
	{
		fread(&iSectionHeader, sizeof(IMAGE_SECTION_HEADER), 1, pfile);

		if (!strcmp((char*)iSectionHeader.Name, ".text"))
		{
			return DumpTextSegment(pfile, iSectionHeader, hwndShellcodeEdit, hWnd);
		}
	}

	MessageBox(hWnd, "Impossible de trouver le segment text.", "Erreur", MB_OK | MB_ICONERROR);

	return;
}

void DumpTextSegment(PFILE pfile, IMAGE_SECTION_HEADER iSectionHeader, HWND hwndShellcodeEdit, HWND hWnd)
{
	BYTE by = 0;
	int nLength = 0;
	char szText[5];

	Edit_SetText(hwndShellcodeEdit, "");

	fseek(pfile, iSectionHeader.PointerToRawData, SEEK_SET);

	for (DWORD dw = 0; dw < iSectionHeader.Misc.VirtualSize; dw++)
	{
		fread(&by, sizeof(BYTE), 1, pfile);
		sprintf(szText, "\\x%.2X", by);

		nLength = Edit_GetTextLength(hwndShellcodeEdit);
		Edit_SetSel(hwndShellcodeEdit, nLength, nLength);
		Edit_ReplaceSel(hwndShellcodeEdit, szText);
	}

	fclose(pfile);

	Edit_SetSel(hwndShellcodeEdit, 0, Edit_GetTextLength(hwndShellcodeEdit));

	SetFocus(hwndShellcodeEdit);
}
