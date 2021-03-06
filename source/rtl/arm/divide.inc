{
    This file is part of the Free Pascal run time library.
    Copyright (c) 2014 by the Free Pascal development team.

    Implementation of division helpers

    See the file COPYING.FPC, included in this distribution,
    for details about the copyright.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 **********************************************************************}


{$if defined(CPUARM_HAS_UMULL) and defined(CPUARM_HAS_CLZ)}

{ ARM division helpers using umull to do a 32-bit division based on
  this paper: http://research.microsoft.com/pubs/70645/tr-2008-141.pdf

  For future optimization and testing, this file is compilable outside
  the system unit. }

{$ifndef FPC_SYSTEM_HAS_DIV_DWORD}
{$define FPC_SYSTEM_HAS_DIV_DWORD}

function fpc_div_dword(n,z:dword):dword;assembler;nostackframe;
{$ifdef FPC_IS_SYSTEM}[public,alias: 'FPC_DIV_DWORD'];{$endif}
asm
  // Handle division by zero
  cmp   r0, #0
  beq   .Lhandle_div_by_zero

  stmfd r13!, {r4,lr}
  // r1 = divisor
  // r0 = dividend
  // r2 = k, z
  // r3 = ty, t, my
  // ip = temp
  // r4 = scratch

  // unsigned k = clz(y);
  clz   r2, r0

  // unsigned ty = lsr( lsl(y,k), W-9 ); // prescaling
  // r3/ty will ALWAYS give a result between 256 and 511
  mov   r3, r0, lsl r2
  mov   r3, r3, lsr #23

  // unsigned t = unrt[ ty - 256 ] + 256; // table lookup
  adr   r4, .LLeading9BitTable - 256
  ldrb  ip, [r4, r3]
  rsb   r3, r2, #31
  and   r3, r3, #255
  add   ip, ip, #256

  // unsigned z = lsr( lsl(t,W-9), W-k-1 );
  mov   ip, ip, lsl #23
  mov   r2, ip, lsr r3

  // unsigned my = 0-y;
  rsb   r3, r0, #0

  // z = z + umulh(z,mul(my,z));
  mul   ip, r3, r2
  umull r4, ip, r2, ip
  add   r2, r2, ip
  // z = z + umulh(z,mul(my,z));
  mul   ip, r3, r2
  umull r4, ip, r2, ip
  add   r2, r2, ip

  // q estimate
  // q = umulh(x,z);
  // ip = q
  umull r4, ip, r1, r2

  // r = x - mul(y,q);
  // r4 = r
  mul r4, r0, ip
  sub r4, r1, r4

  // q refinement
  // if (r >= y) { r = r - y; q = q + 1; }
  cmp r0,r4
  subls r4, r4, r0
  addls ip, ip, #1

  // if (r >= y) { r = r - y; q = q + 1; }
  cmp r0,r4
  subls r4, r4, r0
  addls ip, ip, #1

  mov r0, ip
  mov r1, r4

  ldmfd r13!, {r4,pc}
.LLeading9BitTable:
  .byte 254, 252, 250, 248, 246, 244, 242, 240, 238, 236, 234, 233, 231, 229, 227, 225
  .byte 224, 222, 220, 218, 217, 215, 213, 212, 210, 208, 207, 205, 203, 202, 200, 199
  .byte 197, 195, 194, 192, 191, 189, 188, 186, 185, 183, 182, 180, 179, 178, 176, 175
  .byte 173, 172, 170, 169, 168, 166, 165, 164, 162, 161, 160, 158, 157, 156, 154, 153
  .byte 152, 151, 149, 148, 147, 146, 144, 143, 142, 141, 139, 138, 137, 136, 135, 134
  .byte 132, 131, 130, 129, 128, 127, 126, 125, 123, 122, 121, 120, 119, 118, 117, 116
  .byte 115, 114, 113, 112, 111, 110, 109, 108, 107, 106, 105, 104, 103, 102, 101, 100
  .byte 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 88, 87, 86, 85
  .byte 84, 83, 82, 81, 80, 80, 79, 78, 77, 76, 75, 74, 74, 73, 72, 71
  .byte 70, 70, 69, 68, 67, 66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 59
  .byte 58, 57, 56, 56, 55, 54, 53, 53, 52, 51, 50, 50, 49, 48, 48, 47
  .byte 46, 46, 45, 44, 43, 43, 42, 41, 41, 40, 39, 39, 38, 37, 37, 36
  .byte 35, 35, 34, 33, 33, 32, 32, 31, 30, 30, 29, 28, 28, 27, 27, 26
  .byte 25, 25, 24, 24, 23, 22, 22, 21, 21, 20, 19, 19, 18, 18, 17, 17
  .byte 16, 15, 15, 14, 14, 13, 13, 12, 12, 11, 10, 10, 9, 9, 8, 8
  .byte 7, 7, 6, 6, 5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0

.Lhandle_div_by_zero:
  mov r0, #200
  mov r1, r11
{$ifdef FPC_IS_SYSTEM}
  b   handleerrorframe
{$endif}
end;

{It is a compilerproc (systemh.inc), make an alias for internal use.}
{$ifdef FPC_IS_SYSTEM}
function fpc_div_dword(n,z:dword):dword;external name 'FPC_DIV_DWORD';
{$endif}
{$endif}

{$ifndef FPC_SYSTEM_HAS_DIV_LONGINT}
{$define FPC_SYSTEM_HAS_DIV_LONGINT}
function fpc_div_longint(n,z:longint):longint;assembler;nostackframe;
{$ifdef FPC_IS_SYSTEM}[public,alias: 'FPC_DIV_LONGINT'];{$endif}

asm
  stmfd sp!, {r4,lr}
  ands r4, r0, #1<<31       (* r12:=r0 and $80000000 *)
  rsbmi r0, r0, #0           (* if signed(r0) then r0:=0-r0 *)
  eors r4, r4, r1, ASR#32  (* r12:=r12 xor (r1 asr 32) *)
  rsbcs r1, r1, #0           (* if signed(r12) then r1:=0-r1 *)
  bl fpc_div_dword
  movs r4, r4, LSL#1       (* carry:=sign(r12) *)
  rsbcs r0, r0, #0
  rsbmi r1, r1, #0
  ldmfd sp!, {r4,pc}
end;

{It is a compilerproc (systemh.inc), make an alias for internal use.}
{$ifdef FPC_IS_SYSTEM}
function fpc_div_longint(n,z:longint):longint;external name 'FPC_DIV_LONGINT';
{$endif}
{$endif}

{$ifndef FPC_SYSTEM_HAS_MOD_DWORD}
{$define FPC_SYSTEM_HAS_MOD_DWORD}
function fpc_mod_dword(n,z:dword):dword;assembler;nostackframe;
{$ifdef FPC_IS_SYSTEM}[public,alias: 'FPC_MOD_DWORD'];{$endif}

asm
  stmfd sp!, {ip,lr}
  bl fpc_div_dword
  mov r0, r1
  ldmfd sp!, {ip,pc}
end;

{It is a compilerproc (systemh.inc), make an alias for internal use.}
{$ifdef FPC_IS_SYSTEM}
function fpc_mod_dword(n,z:dword):dword;external name 'FPC_MOD_DWORD';
{$endif}
{$endif}

{$ifndef FPC_SYSTEM_HAS_MOD_LONGINT}
{$define FPC_SYSTEM_HAS_MOD_LONGINT}
function fpc_mod_longint(n,z:longint):longint;assembler;nostackframe;
{$ifdef FPC_IS_SYSTEM}[public,alias: 'FPC_MOD_LONGINT'];{$endif}

asm
  stmfd sp!, {ip,lr}
  bl fpc_div_longint
  mov r0, r1
  ldmfd sp!, {ip,pc}
end;

{It is a compilerproc (systemh.inc), make an alias for internal use.}
{$ifdef FPC_IS_SYSTEM}
function fpc_mod_longint(n,z:longint):longint;external name 'FPC_MOD_LONGINT';
{$endif}
{$endif}

{$endif}
