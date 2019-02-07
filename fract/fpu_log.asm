.486
.model flat, stdcall
option casemap :none

.code

;--------------------------------------------
;Addup PROC, Arg1:DWORD, Arg2:DWORD, Arg3:DWORD

;mov eax, Arg1
;add eax, Arg2
;add eax, Arg3

;ret

;Addup ENDP

log_fpu PROC, Arg1:REAL4, Arg2:REAL4

fld Arg2
fld Arg1
fyl2x
sub esp, 4; or use space you already reserved
fstp dword ptr  [esp]
mov eax, [esp]; or better, pop eax
add esp, 4

ret

log_fpu ENDP
;----------------------------------------------
END