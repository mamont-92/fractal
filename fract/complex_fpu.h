#ifndef COMPLEX_FPU
#define COMPLEX_FPU

#define REAL(cmplx) cmplx._Val[0]
#define IMAG(cmplx) cmplx._Val[1]

#define ADD(cmplx1, cmplx2) REAL(cmplx1) = REAL(cmplx1) + REAL(cmplx2), IMAG(cmplx1) = IMAG(cmplx1) + IMAG(cmplx2)

static const float half = 0.49999;
static const float minus_half = 0.49999;

typedef struct complex_fpu_f {
	float _Val[2];
} complex_fpu_f;

typedef complex_fpu_f _Fcomplex;

_declspec(naked) float __stdcall fpu_cabs(_Fcomplex * _Fc) {
	__asm {
		mov eax, dword ptr[esp + 4]
		fld dword ptr[eax]
		fmul st, st
		fld dword ptr[eax + 4]
		fmul st, st
		faddp st(1), st
		fsqrt
		ret 4
	}
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
		fld[esp + 4]

		sub esp, 4
		fistp dword ptr[esp]
		pop eax

		ret 4
	}
}

_declspec(naked) float __stdcall fpu_ln(float x) {
	__asm {
		fld1
		fldl2e
		fdivp st(1), st
		fld dword ptr [esp + 4]
		fyl2x

		ret     4
	}
}

_declspec(naked) float __stdcall fpu_exp(float x) {
	__asm {
		fninit
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
		ret     4
	}
}

_declspec(naked) float __stdcall fpu_atan(float x, float y) {
	__asm {
		fld dword ptr [esp+4]
		fld dword ptr [esp+8]
		fpatan
		ret 8
	}
}

_declspec(naked) float __stdcall fpu_sqrt(float x) {
	__asm {
		fld dword ptr [esp+4]
		fsqrt
		ret 4
	}
}

_declspec(naked) complexFromPolar(float angle, float len, _Fcomplex * cmplx) {
	_asm {
		mov eax, dword ptr[esp + 12] ; cmplx ptr
		fld dword ptr[esp + 8] ;load len 
		fld dword ptr[esp + 4] ;load angle
		fsincos
		fmul st, st(2)
		fstp dword ptr[eax] ;real_part
		fmul st, st(1)
		fstp dword ptr[eax + 4] ; imag_part
		fstp  st; just pop st(0) to nothing
		ret 12
	}
}

_Fcomplex fpu_complex_pow(_Fcomplex x, _Fcomplex y) {
	_Fcomplex res;
	float ln_x1y1 = fpu_ln(fpu_sqrt(REAL(x)*REAL(x) + IMAG(x)*IMAG(x)));
	float a = fpu_atan(IMAG(x), REAL(x));

	float e1 = fpu_exp(REAL(y)*ln_x1y1 - IMAG(y)*a);
	float a_y2lnx1y1 = REAL(y) * a + IMAG(y) * ln_x1y1;
	complexFromPolar(a_y2lnx1y1, e1, &res);

	return res;
}

//bonus
/*_declspec(naked) float __stdcall fpu_pow(float x, float y) {
	__asm {
		push    ebp
		mov     ebp, esp

		fld y
		fld x
		fyl2x
		fld minus_half
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
}*/

#endif //COMPLEX_FPU