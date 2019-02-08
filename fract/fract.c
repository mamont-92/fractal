#pragma comment(linker, "/MERGE:.data=.text")
#pragma comment(linker, "/MERGE:.rdata=.text")
#pragma comment(linker, "/SECTION:.text,EWR")

#include "complex_fpu.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


void onDraw();
void onTimer();
void generatePalette();
void initFractBitmap();

HANDLE hHeap;

HWND hWnd;
HDC hDCWnd;

#define wnd_errCapt "Er"
#define WND_WIDTH  800
#define WND_HEIGHT 600
char wnd_className[] = "wc";

LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define TIMER_ID 1
#define TIMER_MS 20

int CurrKey = 0;

#define fract_real_k	1.07f
#define fract_img_k		0.0001f
#define fract_rng		50.0f
#define fract_iter_max	20
#define fract_power_max	8.0f
#define fract_power_min	2.0f

float fract_power = 2.0f;
float fract_dpower = 0.02f;

float fract_scale = 0.01f;
float fract_dscale = 0.000008f;

unsigned char * fract_bits = NULL;
BITMAPINFO fract_bitmap;

#define MAX_PALETTE_CLR 65535

struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
}typedef pltClr;

pltClr * palette = NULL;

#define R_W 13
#define G_W 11
#define B_W 7


void onDraw()
{
	int i;

#pragma omp parallel for private(i)
	for (i = 0; i < WND_HEIGHT; ++i) {
		short midX = WND_WIDTH >> 1;
		short midY = WND_HEIGHT >> 1;

		_Fcomplex z;
		_Fcomplex constant = { fract_real_k , fract_img_k };
		_Fcomplex f_power = { (double)fract_power , 0.0 };

		float re_z, im_z, abs_z;

		for (int j = 0; j < WND_WIDTH; ++j) {
			long ind = (i * WND_WIDTH + j) * 3;

			REAL(z) = ((i - midY) * fract_scale);
			IMAG(z) = ((j - midX) * fract_scale);

			re_z = REAL(z);
			im_z = IMAG(z);
			abs_z = fpu_cabs(z);

			for (int k = 0; (re_z < fract_rng || im_z < fract_rng || abs_z < fract_rng) && (k < fract_iter_max); k++) {
				z = fpu_complex_pow(z, f_power);
				ADD(z, constant);

				re_z = fpu_fabs(REAL(z));
				im_z = fpu_fabs(IMAG(z));
				abs_z = fpu_cabs(z);
			}

			float fabs_z = abs_z + 1.0f;
			float val = fpu_ln(fabs_z);
			unsigned long int clr_ind = fpu_f_to_i(val) % MAX_PALETTE_CLR;

			fract_bits[ind] = palette[clr_ind].red;
			fract_bits[ind + 1] = palette[clr_ind].green;
			fract_bits[ind + 2] = palette[clr_ind].blue;
		}
	}
	
	SetDIBitsToDevice(hDCWnd, 0, 0, WND_WIDTH, WND_HEIGHT, 0, 0, 0, WND_HEIGHT, fract_bits, &fract_bitmap, DIB_RGB_COLORS);
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow
)
{
	DEVMODE savedScreenParams, targetScreenParams;
	MSG msg;
	WNDCLASSEX windowclass;
	HDC hDCScreen = GetDC(NULL);

	savedScreenParams.dmSize = sizeof(DEVMODE);
	savedScreenParams.dmBitsPerPel = GetDeviceCaps(hDCScreen, BITSPIXEL)
		* GetDeviceCaps(hDCScreen, PLANES);
	savedScreenParams.dmPelsWidth = GetDeviceCaps(hDCScreen, HORZRES);
	savedScreenParams.dmPelsHeight = GetDeviceCaps(hDCScreen, VERTRES);
	savedScreenParams.dmDisplayFrequency = GetDeviceCaps(hDCScreen, VREFRESH);
	savedScreenParams.dmFields = DM_BITSPERPEL + DM_PELSWIDTH 
		+ DM_PELSHEIGHT + DM_DISPLAYFREQUENCY;
	
	targetScreenParams.dmSize = sizeof(DEVMODE);
	targetScreenParams.dmBitsPerPel = savedScreenParams.dmBitsPerPel;
	targetScreenParams.dmPelsWidth = WND_WIDTH;
	targetScreenParams.dmPelsHeight = WND_HEIGHT;
	targetScreenParams.dmDisplayFrequency = savedScreenParams.dmDisplayFrequency;
	targetScreenParams.dmFields = DM_BITSPERPEL + DM_PELSWIDTH
		+ DM_PELSHEIGHT + DM_DISPLAYFREQUENCY;

	ChangeDisplaySettings(&targetScreenParams, 0);
	

	windowclass.cbSize = sizeof(windowclass);
	windowclass.style = CS_HREDRAW | CS_VREDRAW;
	windowclass.lpfnWndProc = WindowProc;
	windowclass.cbClsExtra = 0;
	windowclass.cbWndExtra = 0;
	windowclass.hInstance = hInstance;
	windowclass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	windowclass.lpszMenuName = NULL;
	windowclass.lpszClassName = wnd_className;
	windowclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	windowclass.hCursor = LoadCursor(NULL, NULL);
	windowclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&windowclass)){
		return 1;
	}
	hWnd = CreateWindowA(wnd_className, "F", WS_POPUP | WS_VISIBLE, 0, 0, WND_WIDTH, WND_HEIGHT, (HWND)NULL, (HMENU)NULL, (HINSTANCE)hInstance, NULL);
	if (!hWnd){
		return 1;
	}
	
	ShowWindow(hWnd, SW_SHOW);

	hHeap = HeapCreate(0, 0x01000, 0);

	generatePalette();
	initFractBitmap();
	
	SetTimer(hWnd, TIMER_ID, TIMER_MS, NULL);
	SetCursor(NULL);

	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	HeapFree(hHeap, 0, fract_bits);
	HeapFree(hHeap, 0, palette);
	HeapDestroy(hHeap);
	
	ChangeDisplaySettings(&savedScreenParams, 0);
	
	ExitProcess(0);

	return 0;
}

LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return 1;
		break;
	case WM_PAINT:
		hDCWnd = BeginPaint(hWindow, &ps);
		onDraw();
		EndPaint(hWindow, &ps);
		break;
	case WM_CLOSE:
		DestroyWindow(hWindow);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_TIMER:
		onTimer();
		break;
	case WM_KEYDOWN:
		if((int)wParam ==27);
			SendMessage(hWnd, WM_CLOSE, 0, 0);
		break;
	default:
		return DefWindowProc(hWindow, uMsg, wParam, lParam);
	}
	return 0;
}

void onTimer() {
	fract_power += fract_dpower;
	fract_scale -= fract_dscale;

	if (fract_power >= fract_power_max || fract_power <= fract_power_min) {
		fract_dpower = -fract_dpower;
		fract_dscale = -fract_dscale;
	}
	InvalidateRect(hWnd, NULL, 0);
}

void generatePalette()
{
	int len = MAX_PALETTE_CLR;
	long size = (long)(MAX_PALETTE_CLR+1) * sizeof(pltClr);
	palette = (unsigned char*)HeapAlloc(hHeap, 0, size);

	float rHalf = R_W * 0.5;
	float gHalf = G_W * 0.5;
	float bHalf = B_W * 0.5;

	float rHalfInv_mul_255 = 1.0 / rHalf * 255.0;
	float gHalfInv_mul_255 = 1.0 / gHalf * 255.0;
	float bHalfInv_mul_255 = 1.0 / bHalf * 255.0;

	for (int i = 0; i < len; ++i) {
		int redVal = (i + 1) % R_W;
		int grenVal = (i + 4) % G_W;
		int blueVal = (i+3) % B_W;

		float rVal = fpu_fabs((float)redVal - rHalf) * rHalfInv_mul_255;
		float gVal = fpu_fabs((float)grenVal - gHalf) * gHalfInv_mul_255;
		float bVal = fpu_fabs((float)blueVal - bHalf) * bHalfInv_mul_255;

		palette[i].red = max(min((int)rVal, 255), 0);
		palette[i].green = max(min((int)gVal, 255), 0);
		palette[i].blue = max(min((int)bVal, 255), 0);
	}
}

void initFractBitmap()
{
	fract_bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	fract_bitmap.bmiHeader.biWidth = WND_WIDTH;
	fract_bitmap.bmiHeader.biHeight = WND_HEIGHT;
	fract_bitmap.bmiHeader.biPlanes = 1;
	fract_bitmap.bmiHeader.biBitCount = 24;
	fract_bitmap.bmiHeader.biCompression = BI_RGB;
	fract_bitmap.bmiColors[0].rgbBlue = 255;
	fract_bitmap.bmiColors[0].rgbBlue = 255;
	fract_bitmap.bmiColors[0].rgbGreen = 255;

	long size = (long)WND_WIDTH * (long)WND_HEIGHT * (long)3 * sizeof(char);
	fract_bits = (unsigned char*)HeapAlloc(hHeap, 0, size);
}
