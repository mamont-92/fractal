#ifndef COMPLEX_FPU
#define COMPLEX_FPU

//#include <complex.h>

#define REAL(cmplx) cmplx._Val[0]
#define IMAG(cmplx) cmplx._Val[1]

#define ADD(cmplx1, cmplx2) REAL(cmplx1) = REAL(cmplx1) + REAL(cmplx2), IMAG(cmplx1) = IMAG(cmplx1) + IMAG(cmplx2)

static const float half = 0.49999;

typedef struct complex_fpu_f {
	float _Val[2];
} complex_fpu_f;

typedef complex_fpu_f _Fcomplex;

float fpu_cabs(_Fcomplex _Fc) {
	float _abs = REAL(_Fc)*REAL(_Fc) + IMAG(_Fc)*IMAG(_Fc);
	__asm {
		fld _abs
		fsqrt 
		fstp _abs
	}
	return _abs;
}

_declspec(naked) float __stdcall fpu_fabs(float x) {
	__asm {
		push	ebp
		mov ebp, esp

		mov eax, x
		and eax, 0x7fffffff
		push eax
		fld dword ptr[esp]
		add esp, 4

		pop ebp
		ret 4
	}
}

_declspec(naked) unsigned int __stdcall fpu_f_to_i(float x) {
	__asm {
		fst dword ptr[esp + 4]
		mov eax, [esp + 4]

		and eax, 080000000H
		xor eax, 0BEFFFFFFH

		push eax

		; mov eax, dword ptr[esp]
		fadd dword ptr[esp]
		fistp dword ptr[esp]

		pop eax

		ret 4
	}
}

_declspec(naked) float __stdcall fpu_ln(float x) {
	__asm {
		push    ebp
		mov     ebp, esp


		fld1
		fldl2e
		fdivp st(1), st
		fld x
		fyl2x

		pop     ebp
		ret     4
	}
}

_declspec(naked) float __stdcall fpu_pow(float x, float y) {
	__asm {
		push    ebp
		mov     ebp, esp

		fld y
		fld x
		fyl2x
		fld half
		fadd st, st(1)
		frndint;//st1 = a.b | st0 = a
		fsub st(1), st;//st1 = 0.b | st0 = a
		fxch st(1);//st1 = a, st0 = 0.b
		f2xm1; //st1 = a, st0 = 2 ^(0.b) -1
		fld1
			faddp st(1), st; //st1 = a, st0 = 2 ^(0.b)
		fscale

			pop     ebp
			ret     8
	}
}

_declspec(naked) float __cdecl fpu_exp(float x) {
	__asm {
		finit


		fld [esp+4]
		fldl2e
		fmulp st(1), st
		fld half
		fadd st, st(1)
		frndint;//st1 = a.b | st0 = a
		fsub st(1), st;//st1 = 0.b | st0 = a
		fxch st(1);//st1 = a, st0 = 0.b
		f2xm1; //st1 = a, st0 = 2 ^(0.b) -1
		fld1
			faddp st(1), st; //st1 = a, st0 = 2 ^(0.b)
		fscale
		
			fclex

			ret     0
	}
}

_declspec(naked) float __stdcall fpu_sin(float x) {
	__asm {
		push	ebp
		mov ebp, esp
		fld x
		fsin
		pop	ebp
		ret 4
	}
}

_declspec(naked) float __stdcall fpu_cos(float x) {
	__asm {
		push	ebp
		mov ebp, esp
		fld x
		fcos
		pop	ebp
		ret 4
	}
}

_declspec(naked) float __stdcall fpu_atan(float x, float y) {
	__asm {
		push	ebp
		mov ebp, esp

		fld x
		fld y
		fpatan

		pop ebp
		ret 8
	}
}



_declspec(naked) float __stdcall fpu_sqrt(float x) {
	__asm {
		push	ebp
		mov ebp, esp

		fld x
		fsqrt

		pop ebp
		ret 4
	}
}

_Fcomplex fpu_complex_pow(_Fcomplex x, _Fcomplex y) {
	_Fcomplex res;
	float ln_x1y1 = fpu_ln(fpu_sqrt(REAL(x)*REAL(x) + IMAG(x)*IMAG(x)));
	float a = fpu_atan(IMAG(x), REAL(x));

	float e1 = fpu_exp(REAL(y)*ln_x1y1 - IMAG(y)*a);
	float a_y2lnx1y1 = REAL(y) * a + IMAG(y) * ln_x1y1;

	REAL(res) = fpu_cos(a_y2lnx1y1)*e1;
	IMAG(res) = fpu_sin(a_y2lnx1y1)*e1;

	return res;
}

#endif //COMPLEX_FPU