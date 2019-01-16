#include <windows.h>
#include <complex>

void onDraw();
void onTimer();
void onKeyDown();
void generatePalette();
void initFractBitmap();
void releaseBitmap();

HWND hWnd;
HDC hDCWnd;

#define wnd_errCapt "Error"
unsigned short wnd_width = 800;
unsigned short wnd_height = 600;
char wnd_className[] = "wndClass";

LRESULT CALLBACK WindowProc(HWND hWindow, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define TIMER_ID 1
#define TIMER_MS 20

int CurrKey = 0;

#define fract_real_k	1.07f
#define fract_img_k		0.0001f
#define fract_rng		50.0f
#define fract_iter_max	10
#define fract_power_max	10.0f
#define fract_power_min	2.0f

float fract_power = 2.0f;
float fract_dpower = 0.02f;

float fract_scale = 0.01f;
float fract_dscale = 0.000008f;

unsigned char * fract_bits = NULL;
BITMAPINFO fract_bitmap;

#define MAX_PALETTE_CLR 65535

struct pltClr {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
};

pltClr * palette = NULL;

int horRes;
int vertRes;
int refresh;

void onDraw()
{
	#pragma omp parallel for
	for (int i = 0; i < wnd_height; ++i) {
		std::complex <double> z; //z будет возводиться в куб

		short midX = wnd_width >> 1;
		short midY = wnd_height >> 1;

		const std::complex <double> constant(fract_real_k, fract_img_k);
		double re_z, im_z, abs_z;

		for (int j = 0; j < wnd_width; ++j) {
			long ind = (i * wnd_width + j) * 3;

			z.real((i - midY) * fract_scale);
			z.imag((j - midX) * fract_scale);

			re_z = fabs(z.real());
			im_z = fabs(z.imag());
			abs_z = abs(z);

			for (int k = 0; (re_z < fract_rng || im_z < fract_rng || abs_z < fract_rng) && k < fract_iter_max; k++) {
				z = pow(z, fract_power) + constant;
				re_z = fabs(z.real());
				im_z = fabs(z.imag());
				abs_z = abs(z);
			}

			double val = log(abs_z + 1.0);
			unsigned long int clr_ind = (unsigned long)val % MAX_PALETTE_CLR;

			fract_bits[ind + 0] = palette[clr_ind].red;
			fract_bits[ind + 1] = palette[clr_ind].green;
			fract_bits[ind + 2] = palette[clr_ind].blue;
		}
	}
	
	if (!SetDIBitsToDevice(hDCWnd, 0, 0, wnd_width, wnd_height, 0, 0, 0, wnd_height, fract_bits, &fract_bitmap, DIB_RGB_COLORS))
	{
		MessageBox(NULL, "Water bits error ", wnd_errCapt, MB_OK);
	}

}

void setResolution()
{
	HDC hDCScreen = GetDC(NULL);
	horRes = GetDeviceCaps(hDCScreen, HORZRES);
	vertRes = GetDeviceCaps(hDCScreen, VERTRES);
	refresh = GetDeviceCaps(hDCScreen, VREFRESH);
	ReleaseDC(NULL, hDCScreen);
	
	DEVMODE DM;
	DM.dmSize = sizeof(DEVMODE);
	DM.dmBitsPerPel = 32; 
	DM.dmPelsWidth = wnd_width;
	DM.dmPelsHeight = wnd_height;
	DM.dmFields = DM_BITSPERPEL +
		DM_PELSWIDTH +
		DM_PELSHEIGHT +
		DM_DISPLAYFREQUENCY;
	DM.dmDisplayFrequency = refresh; // частота обновления экрана
	ChangeDisplaySettings(&DM, 0);
}

void restoreResolution()
{
	DEVMODE DM;
	DM.dmSize = sizeof(DEVMODE);
	DM.dmBitsPerPel = 32; 
	DM.dmPelsWidth = horRes; 
	DM.dmPelsHeight = vertRes; 
	DM.dmFields = DM_BITSPERPEL +
		DM_PELSWIDTH +
		DM_PELSHEIGHT +
		DM_DISPLAYFREQUENCY;
	DM.dmDisplayFrequency = refresh; // частота обновления экрана
	ChangeDisplaySettings(&DM, 0);
}

int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow
)
{
	setResolution();

	MSG msg;

	WNDCLASSEX windowclass;

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
		MessageBox(NULL, "Can't register window class !", wnd_errCapt, MB_OK);
		return 1;
	}
	//hWnd = CreateWindow(wnd_className, "Fract", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 0, 0, wndWidth, wndHeight, (HWND)NULL, (HMENU)NULL, (HINSTANCE)hInstance, NULL);
	hWnd = CreateWindowA(wnd_className, "Fract", WS_POPUP | WS_VISIBLE, 0, 0, wnd_width, wnd_height, (HWND)NULL, (HMENU)NULL, (HINSTANCE)hInstance, NULL);
	if (!hWnd){
		MessageBox(NULL, "Can't create windows", wnd_errCapt, MB_OK);
		return 1;
	}
	
		
	ShowWindow(hWnd, SW_SHOW);
	generatePalette();
	initFractBitmap();
	
	SetTimer(hWnd, TIMER_ID, TIMER_MS, NULL);
	SetCursor(NULL);

	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	releaseBitmap();
	restoreResolution();

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
		CurrKey = (int)wParam;
		onKeyDown();
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

void exit()
{
	SendMessage(hWnd, WM_CLOSE, 0, 0);
}

void onKeyDown()
{
	switch (CurrKey)
	{
	case 27:
		exit();
		break;
	}
}

void generatePalette()
{
	int len = MAX_PALETTE_CLR;
	palette = new pltClr[MAX_PALETTE_CLR];

	float rHalf = 23.0 / 2.0;
	float gHalf = 11 / 2.0;
	float bHalf = 7 / 2.0;

	for (int i = 0; i < len; ++i) {
		int redVal = i % 17;
		int grenVal = i % 11;
		int blueVal = i % 7;

		float rVal = fabs((float)redVal - rHalf) / rHalf * 255.0;
		float gVal = fabs((float)grenVal - gHalf) / gHalf * 255.0;
		float bVal = fabs((float)blueVal - bHalf) / bHalf * 255.0;

		redVal = max(min((int)rVal, 255), 0);
		grenVal = max(min((int)gVal, 255), 0);
		blueVal = max(min((int)bVal, 255), 0);

		palette[i].blue = blueVal;
		palette[i].green = grenVal;
		palette[i].red = redVal;
	}

}

void initFractBitmap()
{
	fract_bitmap.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	fract_bitmap.bmiHeader.biWidth = wnd_width;
	fract_bitmap.bmiHeader.biHeight = wnd_height;
	fract_bitmap.bmiHeader.biPlanes = 1;
	fract_bitmap.bmiHeader.biBitCount = 24;
	fract_bitmap.bmiHeader.biCompression = BI_RGB;
	fract_bitmap.bmiColors[0].rgbBlue = 255;
	fract_bitmap.bmiColors[0].rgbBlue = 255;
	fract_bitmap.bmiColors[0].rgbGreen = 255;

	fract_bits = new unsigned char[(long)wnd_width * (long)wnd_height * (long)3];
}

void releaseBitmap()
{
	delete[] fract_bits;
}