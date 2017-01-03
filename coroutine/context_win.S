.CODE

save_context PROC
	mov  [rcx+64], rsi
	pop  rsi		
	xor  eax, eax
	mov  [rcx]   , rbx
	mov  [rcx+8] , rsp
	push rsi
	mov  [rcx+16], rbp
	push r11
	mov  r11, qword ptr gs:[10h]
	mov  [rcx+24], r11
	pop  r11
	mov  [rcx+32], r12
	mov  [rcx+40], r13
	mov  [rcx+48], r14
	mov  [rcx+56], r15
	push r11
	mov  r11, [rcx+64]
	mov  [rcx+64], rsi
	mov  rsi, r11
	pop  r11
	ret
save_context ENDP

restore_context PROC
	mov eax, edx
	mov rbx, [rcx]
	mov rsp, [rcx+8]
	mov rbp, [rcx+16]
	push r11
	mov r11, [rcx+24]
	mov qword ptr gs:[10h], r11
	pop  r11
	mov r12, [rcx+32]
	mov r13, [rcx+40]
	mov r14, [rcx+48]
	mov r15, [rcx+56]
	jmp qword ptr [rcx+64]
restore_context ENDP

END