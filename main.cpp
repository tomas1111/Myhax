#include <windows.h>
#include <dwmapi.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <conio.h>
#include <vector>
#include <iostream>
#include <fstream>

#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"dwmapi.lib")
#pragma warning(disable: 4018)
#pragma warning(disable: 4244)


#define Entity_Table  0xDFBD98
#define ARMA_CLIENT 0xDFCDD8
#define ARMA_PLAYERINFO 0xDEEAE8
#define ARMA_TRANSFORMATION 0xE27D94
#define SAFE_RELEASE(x) if(x) { x->Release(); x = NULL; } 

char g_szRandom[25] = { 0 };



IDirect3D9Ex* g_pDirect3D;
IDirect3DDevice9Ex* g_pDirect3DDevice;
ID3DXFont* g_pFont;
ID3DXLine* g_pLine;

HWND hWnd;
HWND g_ArmaHWND;
DWORD g_ArmaPID;
HANDLE g_ArmaHANDLE;
float g_fResolution[2] = { 100, 100 };

D3DXVECTOR3 InvViewTranslation;
D3DXVECTOR3 InvViewRight;
D3DXVECTOR3 InvViewUp;
D3DXVECTOR3 InvViewForward;
D3DXVECTOR3 ViewPortMatrix;
D3DXVECTOR3 ProjD1;
D3DXVECTOR3 ProjD2;

D3DXVECTOR3 g_LocalPos;

LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void Render();
void RenderGame();
void SideMenu();
void InitDirectX();
void ShutdownDirectX();
void HotKeyz();
template <class T> T Read(DWORD dwAddress);
D3DXVECTOR3 WorldToScreen(D3DXVECTOR3 in);
void DrawBox(int x, int y, int w, int h, DWORD dwColor);
void DrawText(int x, int y, DWORD dwColor, char* szText, ...);
void DrawTextBorder(int x, int y, DWORD dwColor, char* szText, ...);
static BOOL CALLBACK ArmaEnumFunc(HWND hwnd, LPARAM lParam);
bool SeePlayers = true;
bool SeeVehicles = true;
bool SeeBodies = true;
bool SeeTents = true;
bool nofatigue = false;
bool infiAmmo = false;
bool repairV = false;
bool fullfuel = false;
bool datfire = false;
bool wepdmg = false;
bool wepdmg1 = false;
bool alwaysday = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	srand((unsigned int)time(NULL));
	for(int i = 0; i < 25; i++)
		g_szRandom[i] = (char)(rand()%255);

	CreateMutex(0, false, "GFZFLDLSLDLJFDKL");
	if(GetLastError() == ERROR_ALREADY_EXISTS)
	{
		MessageBox(NULL, "Error: #1", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	BOOL bComp = FALSE;
	DwmIsCompositionEnabled(&bComp);
	if(!bComp)
	{
		MessageBox(NULL, "Error: #2", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	WNDCLASSEX wc = {
		sizeof(WNDCLASSEX),
		NULL,
		WindowProc,
		NULL,
		NULL,
		hInstance,
		LoadIcon(NULL, IDI_APPLICATION),
		LoadCursor(NULL, IDC_ARROW),
		NULL,
		NULL,
		g_szRandom,
		LoadIcon(NULL, IDI_APPLICATION)
	};
	RegisterClassEx(&wc);

	while(!g_ArmaHWND)
	{
		EnumWindows(ArmaEnumFunc, NULL);
		if(g_ArmaHWND)
		{
			GetWindowThreadProcessId(g_ArmaHWND, &g_ArmaPID);
			if(g_ArmaPID != 0)
			{
				g_ArmaHANDLE = OpenProcess(PROCESS_ALL_ACCESS|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_QUERY_INFORMATION, false, g_ArmaPID);
				if(g_ArmaHANDLE == NULL)
				{
					g_ArmaHWND = NULL;
					g_ArmaPID = NULL;
					g_ArmaHANDLE = NULL;
					continue;
				}
			}
		}
	}

	while(GetForegroundWindow() != g_ArmaHWND)
		Sleep(100);

	Sleep(100);

	RECT armaClientRect;
	GetClientRect(g_ArmaHWND, &armaClientRect);
	g_fResolution[0] = armaClientRect.right;
	g_fResolution[1] = armaClientRect.bottom;

	hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_COMPOSITED | WS_EX_TRANSPARENT | WS_EX_LAYERED, g_szRandom, g_szRandom, WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, g_fResolution[0], g_fResolution[1], NULL, NULL, hInstance, NULL);
	SetWindowLong(hWnd, GWL_EXSTYLE, ((int)GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT));
	SetLayeredWindowAttributes(hWnd, RGB(0, 0, 0), 0, ULW_COLORKEY);
	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);

	MARGINS margins = { -1 };
	if(FAILED(DwmExtendFrameIntoClientArea(hWnd, &margins)))
	{
		MessageBox(NULL, "Error: #3", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	InitDirectX();

	SetWindowPos(g_ArmaHWND, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	MSG uMsg;
	while(true)
	{
		if(!g_ArmaHANDLE)
			g_ArmaHANDLE = OpenProcess(PROCESS_ALL_ACCESS|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_QUERY_INFORMATION, false, g_ArmaPID);

		if(!g_ArmaHANDLE)
			ExitProcess(0);

		if(!g_ArmaHWND)
			ExitProcess(0);

		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		RECT armaClientRect;
		RECT armaWindowRect;

		GetClientRect(g_ArmaHWND, &armaClientRect);
		GetWindowRect(g_ArmaHWND, &armaWindowRect);

		if(g_fResolution[0] != armaClientRect.right && g_fResolution[1] != armaClientRect.bottom)
		{
			MoveWindow(hWnd, armaWindowRect.left + 8, armaWindowRect.top + 30, armaClientRect.right, armaClientRect.bottom, FALSE);
			ShutdownDirectX();
			InitDirectX();
			g_fResolution[0] = armaClientRect.right;
			g_fResolution[1] = armaClientRect.bottom;
		}
		else
			MoveWindow(hWnd, armaWindowRect.left + 8, armaWindowRect.top + 30, g_fResolution[0], g_fResolution[1], FALSE);

		// Render
		Render();

		while(PeekMessage(&uMsg, NULL, 0, 0, PM_REMOVE))
		{
			if(uMsg.message == WM_QUIT)
				break;

			TranslateMessage(&uMsg);
			DispatchMessage(&uMsg);
		}
	}
	return 1;
}

LRESULT WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_DESTROY:
		{
			PostQuitMessage(0);
			ExitProcess(0);
			return 0;
		}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void Render()
{
	if(g_pDirect3DDevice == NULL)
	{
		MessageBox(NULL, "Error: #8", g_szRandom, MB_OK);
		ExitProcess(0);
	}
	g_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
	if(FAILED(g_pDirect3DDevice->BeginScene()))
	{
		MessageBox(NULL, "Error: #9", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	RenderGame();
	SideMenu();

	g_pDirect3DDevice->EndScene();
	g_pDirect3DDevice->PresentEx(NULL, NULL, NULL, NULL, NULL);

}

void SideMenu()
{
	{DrawTextBorder(10, 50, D3DCOLOR_ARGB(255, 178, 34, 34), "Vehicles are: %s",(SeeVehicles)?"on":"off"); }
	{DrawTextBorder(10, 60, D3DCOLOR_ARGB(255, 178, 34, 34), "Players are: %s",(SeePlayers)?"on":"off"); }
	{DrawTextBorder(10, 70, D3DCOLOR_ARGB(255, 178, 34, 34), "Dead bodies are: %s",(SeeBodies)?"on":"off"); }
	{DrawTextBorder(10, 80, D3DCOLOR_ARGB(255, 178, 34, 34), "Tents are: %s",(SeeTents)?"on":"off"); }
	{DrawTextBorder(10, 90, D3DCOLOR_ARGB(255, 178, 34, 34), "No fatigue: %s",(nofatigue)?"on":"off"); }
	{DrawTextBorder(g_fResolution[0] - 100 , 5, D3DCOLOR_ARGB(255, 255, 185, 15), "Unlimited ammo: %s",(infiAmmo)?"on":"off"); }
	{DrawTextBorder(10, 100, D3DCOLOR_ARGB(255, 255, 69, 0), "Repair is: %s",(repairV)?"active!":"off"); }
	{DrawTextBorder(10, 110, D3DCOLOR_ARGB(255, 255, 69, 0), "Refuel is: %s",(fullfuel)?"active!":"off"); }
	if(datfire){{DrawTextBorder(g_fResolution[0] - 100 , 50, D3DCOLOR_ARGB(255, 255, 0, 0), "Maximum firerate"); }}
	if(wepdmg){{DrawTextBorder(g_fResolution[0] - 100 , 60, D3DCOLOR_ARGB(255, 255, 0, 0), "high damage"); }}
	if(wepdmg1){{DrawTextBorder(g_fResolution[0] - 100 , 70, D3DCOLOR_ARGB(255, 255, 0, 0), "VERY HIGH DAMAGE"); }}
	if(alwaysday){{DrawTextBorder(g_fResolution[0] - 100 , 70, D3DCOLOR_ARGB(255, 255, 0, 0), "ALways Day - activated"); }}
}

void RenderGame()
{
	g_ArmaHANDLE = OpenProcess(PROCESS_ALL_ACCESS|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_QUERY_INFORMATION, false, g_ArmaPID);
	if(g_ArmaHANDLE)
	{
		DWORD dwTransformations = Read<DWORD>(ARMA_TRANSFORMATION);
		DWORD dwObjectTable = Read<DWORD>(ARMA_CLIENT);
		DWORD dwPlayerInfo = Read<DWORD>(ARMA_PLAYERINFO);
		DWORD dwLocalPlayer = Read<DWORD>(Read<DWORD>(dwObjectTable + 0x13A8) + 0x4);

		// Update W2S
		{
			DWORD dwTransData = Read<DWORD>(dwTransformations + 0x90);
			InvViewRight = Read<D3DXVECTOR3>(dwTransData + 0x4);
			InvViewUp = Read<D3DXVECTOR3>(dwTransData + 0x10);
			InvViewForward = Read<D3DXVECTOR3>(dwTransData + 0x1C);
			InvViewTranslation = Read<D3DXVECTOR3>(dwTransData + 0x28);
			ViewPortMatrix = Read<D3DXVECTOR3>(dwTransData + 0x54);
			ProjD1 = Read<D3DXVECTOR3>(dwTransData + 0xCC);
			ProjD2 = Read<D3DXVECTOR3>(dwTransData + 0xD8);
		}
		//shortcuts

		if (GetAsyncKeyState(VK_LEFT) && GetAsyncKeyState(VK_LSHIFT))
		{
			SeePlayers = !SeePlayers;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_RIGHT)&& GetAsyncKeyState(VK_LSHIFT))
		{
			SeeBodies = !SeeBodies;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_UP)&& GetAsyncKeyState(VK_LSHIFT))
		{
			SeeVehicles = !SeeVehicles;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_DOWN)&& GetAsyncKeyState(VK_LSHIFT))
		{
			SeeTents = !SeeTents;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_PRIOR))
		{
			DWORD viewdist = Read<DWORD>(0x0E25718);
			long viewdist1 = viewdist+3846004;
			WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)0x0E25718, &viewdist1, sizeof(viewdist1), NULL);
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_NEXT) )
		{
			DWORD viewdist = Read<DWORD>(0x0E25718);
			long viewdist1 = viewdist-3846004;
			WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)0x0E25718, &viewdist1, sizeof(viewdist1), NULL);
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_HOME))
		{
			DWORD grass = Read<DWORD>(ARMA_CLIENT);
			int grasspt = grass + 0x14F0;
			long value = 1112014848;
			WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)grasspt, &value, sizeof(value), NULL);
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_END))
		{
			nofatigue = !nofatigue;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_SUBTRACT))
		{
			infiAmmo = !infiAmmo;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_INSERT))
		{
			fullfuel = !fullfuel;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_DELETE))
		{
			repairV = !repairV;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_DELETE))
		{
			repairV = !repairV;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LCONTROL) && GetAsyncKeyState(VK_END))
		{
			datfire = !datfire;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_F2))
		{
			wepdmg = !wepdmg;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_F3))
		{
			wepdmg1 = !wepdmg1;
			Sleep(150);
		}
		else if (GetAsyncKeyState(VK_LSHIFT) && GetAsyncKeyState(VK_F9))
		// LocalPlayer
		{
			alwaysday = !alwaysday;
			SLeep(150);
		}
		{
			if(dwLocalPlayer)
			{
				g_LocalPos = Read<D3DXVECTOR3>(Read<DWORD>(dwLocalPlayer + 0x18) + 0x28);
				DrawTextBorder(10, 22, D3DCOLOR_ARGB(255, 255, 0, 0), "Position: %0.1f/%0.1f", g_LocalPos.x/100, g_LocalPos.z/100);
			}
		}
		// Range to Rectile
		{
			float fRange = Read<float>(Read<DWORD>(dwObjectTable + 0x8) + 0x30);
			DrawTextBorder(10, 10, D3DCOLOR_ARGB(255, 255, 0, 0), "Range: %0.fm", fRange);
		}

		// Loop Object Table
		if (SeeVehicles || SeePlayers || SeeBodies)
		{
			DWORD dwObjectTablePtr = Read<DWORD>(dwObjectTable + 0x5FC);
			DWORD dwObjectTableArray = Read<DWORD>(dwObjectTablePtr + 0x0);
			DWORD dwObjectTableSize = Read<DWORD>(dwObjectTablePtr + 0x4);
			DrawTextBorder(10, 34, D3DCOLOR_ARGB(255, 255, 0, 0), "ObjectTableSize: %u", dwObjectTableSize);

			for(int i = 0; i < dwObjectTableSize; i++)
			{
				DWORD dwObjectPtr = Read<DWORD>(dwObjectTableArray + (i * 0x34));
				DWORD dwEntity = Read<DWORD>(dwObjectPtr + 0x4);
				if(dwEntity == dwLocalPlayer && fullfuel)
				{
					DWORD fleveladdy = Read<DWORD>(dwEntity + 0x18);
					int fleveladdy1 = fleveladdy + 0xAC;
					DWORD fcapaddy = Read<DWORD>(dwEntity + 0x3C);
					int fcapaddy1 = fcapaddy + 0x600;

					DWORD fuelcap = Read<DWORD>(fcapaddy1);
					DWORD fuellevel = Read<DWORD>(fleveladdy1);
					WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)(fleveladdy1), &fuelcap, sizeof(fuelcap), NULL);
				}
				if(dwEntity == dwLocalPlayer && repairV)
				{
					DWORD dwPartsAmount = Read<DWORD>(dwEntity + 0xC4);
					DWORD dwParts = Read<DWORD>(dwEntity + 0xC0);

					for(int i = 0; i < dwPartsAmount; i++)
					{
						long value = 0;
						WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)(dwParts + i * 0x4), &value, sizeof(value), NULL);
					}
					continue;
				}
				if(dwEntity == dwLocalPlayer)
					continue;

				DWORD dwEntity1 = Read<DWORD>(dwEntity + 0x3C);
				DWORD dwEntity2 = Read<DWORD>(dwEntity1 + 0x30);

				char szObjectName[32];
				ReadProcessMemory(g_ArmaHANDLE, (LPCVOID)(dwEntity2 + 0x8), &szObjectName, 32, 0);

				char szObjectType[25];
				ReadProcessMemory(g_ArmaHANDLE, (LPCVOID)(Read<DWORD>(dwEntity1 + 0x6C) + 0x8), &szObjectType, 25, 0);

				D3DXVECTOR3 pos = Read<D3DXVECTOR3>(Read<DWORD>(dwEntity + 0x18) + 0x28);

				float dx = (g_LocalPos.x - pos.x);
				float dy = (g_LocalPos.y - pos.y);
				float dz = (g_LocalPos.z - pos.z);

				float dist = sqrt((dx*dx) + (dy*dy) + (dz*dz));
				if(dist >= 2000.0f)
					continue;

				pos = WorldToScreen(pos);
				if(pos.z > 0.01)
				{
					if(!strcmp(szObjectType, "soldier"))
					{
						char szWeaponName[25] = { 0 };
						DWORD dwWeaponID = Read<DWORD>(dwEntity + 0x6E0);
						if(dwWeaponID != -1)
						{
							DWORD dwWeaponNameAddress_a = Read<DWORD>(dwEntity + 0x694);
							DWORD dwWeaponNameAddress_b = Read<DWORD>(dwWeaponNameAddress_a + (0x24 * dwWeaponID + 8)) ;
							DWORD dwWeaponNameAddress_c = Read<DWORD>(dwWeaponNameAddress_b + 0x10);
							DWORD dwWeaponNameAddress = Read<DWORD>(dwWeaponNameAddress_c + 0x4);
							ReadProcessMemory(g_ArmaHANDLE, (LPCVOID)(dwWeaponNameAddress + 0x8), &szWeaponName, 25, 0);
						}

						//alive players
						if(Read<BYTE>(dwEntity + 0x20C) != 1 && SeePlayers)
						{

							if (dist < 50.0f)
							{DrawBox(pos.x-20, pos.y-60, 20, 60, D3DCOLOR_ARGB(255, 255, 0, 0));
							DrawTextBorder(pos.x, pos.y, D3DCOLOR_ARGB(255, 255, 0, 0), "%s[%0.fm]\n%s", szObjectName, dist, szWeaponName);}
							else if(dist < 400.0f)
							{DrawBox(pos.x, pos.y, 20, 20, D3DCOLOR_ARGB(255, 255, 99, 71));
							DrawTextBorder(pos.x, pos.y, D3DCOLOR_ARGB(255, 255, 99, 71), "%s[%0.fm]\n%s", szObjectName, dist, szWeaponName);}
							else 
							{DrawBox(pos.x, pos.y, 10, 10, D3DCOLOR_ARGB(255, 255, 160, 122));
							DrawTextBorder(pos.x, pos.y, D3DCOLOR_ARGB(255, 255, 160, 122), "%s[%0.fm]\n%s", szObjectName, dist, szWeaponName);}
						}
						//bodies
						if(Read<BYTE>(dwEntity + 0x20C) == 1 && SeeBodies)
						{
							DrawTextBorder(pos.x, pos.y, D3DCOLOR_ARGB(255, 0, 0, 0), "%s[%0.fm]\n%s", szObjectName, dist, szWeaponName);
							DrawBox(pos.x, pos.y, 10, 10, D3DCOLOR_ARGB(255, 0, 0, 0));
						}
					}
					else if(SeeVehicles &&( !strcmp(szObjectType, "car") || !strcmp(szObjectType, "helicopter") || !strcmp(szObjectType, "motorcycle") || !strcmp(szObjectType, "ship") || !strcmp(szObjectType, "airplane")))
					{
						DWORD dwPartsAmount = Read<DWORD>(dwEntity + 0xC4);
						DWORD dwParts = Read<DWORD>(dwEntity + 0xC0);

						float fHealth = 0.0f;
						for(int i = 0; i < dwPartsAmount; i++)
						{
							fHealth += Read<float>(dwParts + i * 0x4) * 100;
						}
						fHealth = 100.0 - (fHealth / dwPartsAmount);

						DrawTextBorder(pos.x, pos.y, D3DCOLOR_ARGB(255, 0, 0, 255), "%s[%0.fm]\n%0.1f", szObjectName, dist, fHealth);
					}
				}
			}
		}
		if(SeeTents)
		{
			int MasterO[] = { 0x880, 0xb24, 0xdc8 };
			int SlaveO[] = { 0x8, 0xB0, 0x158, 0x200 };
			for(int master : MasterO) 
			{ 
				DWORD MasterA = Read<DWORD>(ARMA_CLIENT) + master;
				for(int slave : SlaveO)
				{
					DWORD SlaveA = Read<DWORD>(MasterA + slave);
					DWORD SlaveS = Read<DWORD>(MasterA + slave + 8);
					for (int i = 0; i < SlaveS; i++)
					{
						DWORD One = Read<DWORD>(SlaveA + (i * 4));
						DWORD Two = Read<DWORD>(One + 0x3C);
						DWORD Tri = Read<DWORD>(Two + 0x30);
						char pavad[25];
						ReadProcessMemory(g_ArmaHANDLE, (LPCVOID)(Tri + 0x8), &pavad, 25, 0);
						if (SeeTents && (!strcmp(pavad, "TentStorage")))
						{
							D3DXVECTOR3 pos = Read<D3DXVECTOR3>(Read<DWORD>(One + 0x18) + 0x28);
							float dx = (g_LocalPos.x - pos.x);
							float dy = (g_LocalPos.y - pos.y);
							float dz = (g_LocalPos.z - pos.z);
							float dist = sqrt((dx*dx) + (dy*dy) + (dz*dz));
							if(dist <= 2000.0f)
								pos = WorldToScreen(pos);
							if(pos.z > 0.01)
							{
								DrawTextBorder(pos.x, pos.y, D3DCOLOR_ARGB(255, 0, 200, 0), "%s[%0.fm]", pavad, dist);
								DrawBox(pos.x, pos.y, 10, 10, D3DCOLOR_ARGB(255, 255, 255, 255));
							}
						}
					}
				}
			}
		}
		//writing functions
		if (nofatigue)
		{
			DWORD one = Read<DWORD>(ARMA_CLIENT);
			DWORD two = Read<DWORD>(one + 0x13A8);
			DWORD three = Read<DWORD>(two + 0x4);
			int fatpt = three + 0xC44;
			long value = 0;
			WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)fatpt, &value, sizeof(fatpt), NULL);
		}
		if (alwaysday)
		if (infiAmmo)
		{
			// i dont have the code atm, im at school and i'll push at home
		}
		{
			DWORD objTable = Read<DWORD>(ARMA_CLIENT);
			DWORD objTablePtr = Read<DWORD>(objTable + 0x13A8);
			DWORD objTablePtr1 = Read<DWORD>(objTablePtr + 0x4);
			DWORD objTablePtr2;
			DWORD weaponID = Read<DWORD>(objTablePtr1 + 0x6E0);
			objTablePtr1 = Read<DWORD>(objTablePtr1 + 0x694);
			objTablePtr2 = Read<DWORD>(objTablePtr1 + (weaponID * 0x24 + 0x4));

			DWORD maxCntPtr = Read<DWORD>(objTablePtr2 + 8);
			DWORD currentMuzzleVelocity = Read<DWORD>(maxCntPtr + 0x34);
			DWORD maxCnt = Read<DWORD>(maxCntPtr + 0x2C);
			if (wepdmg)
			{
				DWORD dmgpt = Read<DWORD>(maxCntPtr + 0x200);
				DWORD dmgptr = dmgpt + 0x140;
				long newvalue = 1099713344;
				WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)dmgptr, &newvalue, sizeof(newvalue), NULL);
			}
			else if (wepdmg1)
			{
				DWORD dmgpt = Read<DWORD>(maxCntPtr + 0x200);
				DWORD dmgptr = dmgpt + 0x140;
				long newvalue = 1199713344;
				WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)dmgptr, &newvalue, sizeof(newvalue), NULL);
			}
			if (maxCnt <= 2)
			{
				long newvalue = 2;
				WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)maxCntPtr, &newvalue, sizeof(newvalue), NULL);
			}
			int timeOut;
			timeOut = objTablePtr2 + 0x14;
			if (datfire)
			{
				long value = 0;
				WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)timeOut, &value, sizeof(value), NULL);
			
			}
			int value = (int)(maxCnt);
			DWORD ammo1 = objTablePtr2 + 0xc;
			DWORD ammo2 = objTablePtr2 + 0x24;
			DWORD tempint;
			int int1 = (DWORD)(value ^ 0xBABAC8B6);
			tempint = int1;
			int1 = int1 << 1;
			DWORD int2 = tempint - (int1);
			WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)ammo1, &int1, sizeof(int1), NULL);
			WriteProcessMemory(g_ArmaHANDLE,  (LPVOID*)ammo2, &int2, sizeof(int2), NULL);
		}
	}
}
void InitDirectX()
{
	if(FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION, &g_pDirect3D)))
	{
		MessageBox(NULL, "Error: #4", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	D3DPRESENT_PARAMETERS pp = { 0 };
	pp.Windowed = TRUE;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.hDeviceWindow = hWnd;
	pp.BackBufferFormat = D3DFMT_A8R8G8B8;
	pp.BackBufferWidth = g_fResolution[0];
	pp.BackBufferHeight = g_fResolution[1];
	pp.EnableAutoDepthStencil = TRUE;
	pp.AutoDepthStencilFormat = D3DFMT_D16;

	if(FAILED(g_pDirect3D->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, NULL, &g_pDirect3DDevice)))
	{
		MessageBox(NULL, "Error: #5", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	if(FAILED(D3DXCreateFont(g_pDirect3DDevice, 12, 0, FW_NORMAL, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &g_pFont)))
	{
		MessageBox(NULL, "Error: #6", g_szRandom, MB_OK);
		ExitProcess(0);
	}

	if(FAILED(D3DXCreateLine(g_pDirect3DDevice, &g_pLine)))
	{
		MessageBox(NULL, "Error: #7", g_szRandom, MB_OK);
		ExitProcess(0);
	}
	g_pLine->SetWidth(1);
	g_pLine->SetPattern(0xFFFFFFFF);
	g_pLine->SetAntialias(FALSE);

}

void ShutdownDirectX()
{
	SAFE_RELEASE(g_pLine)
		SAFE_RELEASE(g_pFont)
		SAFE_RELEASE(g_pDirect3DDevice)
		SAFE_RELEASE(g_pDirect3D)
}

template <class T> T Read(DWORD dwAddress)
{
	T t;
	ReadProcessMemory(g_ArmaHANDLE, (LPCVOID)dwAddress, &t, sizeof(t), 0);
	return t;
}

D3DXVECTOR3 WorldToScreen(D3DXVECTOR3 in)
{
	D3DXVECTOR3 out, temp;

	D3DXVec3Subtract(&temp, &in, &InvViewTranslation);
	float x = D3DXVec3Dot(&temp, &InvViewRight);
	float y = D3DXVec3Dot(&temp, &InvViewUp);
	float z = D3DXVec3Dot(&temp, &InvViewForward);

	out.x = ViewPortMatrix.x * (1 + (x / ProjD1.x / z));
	out.y = ViewPortMatrix.y * (1 - (y / ProjD2.y / z));
	out.z = z;

	return out;
}

void DrawBox(int x, int y, int w, int h, DWORD dwColor)
{
	D3DXVECTOR2 lines[5];
	lines[0] = D3DXVECTOR2(x, y);
	lines[1] = D3DXVECTOR2(x+w, y);
	lines[2] = D3DXVECTOR2(x+w, y+h);
	lines[3] = D3DXVECTOR2(x, y+h);
	lines[4] = D3DXVECTOR2(x, y);
	g_pLine->Draw(lines, 5, dwColor);
}

void DrawText(int x, int y, DWORD dwColor, char* szText, ...)
{
	RECT rect = { x, y, x + 800, y + 800 };
	char buffer[MAX_PATH] = { 0 };
	va_list va_alist;
	va_start(va_alist, szText);
	vsprintf_s(buffer, szText, va_alist);
	va_end(va_alist);

	g_pFont->DrawText(NULL, buffer, -1, &rect, DT_NOCLIP, dwColor);
}

void DrawTextBorder(int x, int y, DWORD dwColor, char* szText, ...)
{

	char buffer[MAX_PATH] = { 0 };
	va_list va_alist;
	va_start(va_alist, szText);
	vsprintf_s(buffer, szText, va_alist);
	va_end(va_alist);

	DrawText(x, y - 1, 0xFF000000, buffer);
	DrawText(x, y + 1, 0xFF000000, buffer);
	DrawText(x - 1, y, 0xFF000000, buffer);
	DrawText(x + 1, y, 0xFF000000, buffer);

	DrawText(x, y, dwColor, buffer);
}

static BOOL CALLBACK ArmaEnumFunc(HWND hwnd, LPARAM lParam)
{
	char title[MAX_PATH];
	GetWindowText(hwnd, title, MAX_PATH);

	if(!strncmp(title, "ArmA 2 OA", strlen("ArmA 2 OA")))
	{
		g_ArmaHWND = hwnd;
		return FALSE;
	}
	return TRUE;
}
