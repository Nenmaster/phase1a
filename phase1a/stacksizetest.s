	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 15, 0	sdk_version 15, 1
	.globl	_add                            ; -- Begin function add
	.p2align	2
_add:                                   ; @add
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #32
	.cfi_def_cfa_offset 32
	str	x0, [sp, #24]
	str	x1, [sp, #16]
	add	x8, sp, #15
	adrp	x9, _p1@PAGE
	str	x8, [x9, _p1@PAGEOFF]
	ldr	x8, [sp, #24]
	ldr	w8, [x8]
	ldr	x9, [sp, #16]
	ldr	w9, [x9]
	add	w8, w8, w9
	str	w8, [sp, #8]
	ldr	w0, [sp, #8]
	add	sp, sp, #32
	ret
	.cfi_endproc
                                        ; -- End function
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #48
	stp	x29, x30, [sp, #32]             ; 16-byte Folded Spill
	add	x29, sp, #32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	w8, #0                          ; =0x0
	str	w8, [sp, #8]                    ; 4-byte Folded Spill
	stur	wzr, [x29, #-4]
	mov	w8, #10                         ; =0xa
	stur	w8, [x29, #-8]
	stur	w8, [x29, #-12]
	sub	x9, x29, #13
	adrp	x8, _p2@PAGE
	str	x9, [x8, _p2@PAGEOFF]
	ldr	x8, [x8, _p2@PAGEOFF]
	adrp	x9, _p1@PAGE
	ldr	x9, [x9, _p1@PAGEOFF]
	subs	x8, x8, x9
                                        ; kill: def $w8 killed $w8 killed $x8
	str	w8, [sp, #12]
	ldr	w9, [sp, #12]
                                        ; implicit-def: $x8
	mov	x8, x9
	mov	x9, sp
	str	x8, [x9]
	adrp	x0, l_.str@PAGE
	add	x0, x0, l_.str@PAGEOFF
	bl	_printf
	ldr	w0, [sp, #8]                    ; 4-byte Folded Reload
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	add	sp, sp, #48
	ret
	.cfi_endproc
                                        ; -- End function
.zerofill __DATA,__bss,_p1,8,3          ; @p1
.zerofill __DATA,__bss,_p2,8,3          ; @p2
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"sum of total is %d"

.subsections_via_symbols
