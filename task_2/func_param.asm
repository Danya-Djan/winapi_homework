IFDEF RAX

.code

extern printParameters: proto

addParameters PROC
    ; Save the base pointer and set up a new stack frame
    push rbp
    mov rbp, rsp

    push rax
    push rbx

    mov rax, [rbp + 48]
    mov rbx, [rbp + 56]

    push rbx
    push rax
    push r8
    push r9
    push rdx
    push rcx

    ; Call the stdcall function
    call printParameters

    push rax
    push rbx

    ; Restore the stack frame and return
    pop rbp
    ret
addParameters ENDP

ELSE

.386
.MODEL FLAT

.CODE

extern _printParameters@24: proto

@addParameters@24 PROC
    ; Save the base pointer and set up a new stack frame
    push ebp
    mov ebp, esp

    ; Preserve registers
    push edx
    push ecx

    ; Get fastcall parameters
    mov eax, [ebp + 8]   ; Example parameter access, adjust based on your actual parameters
    mov ebx, [ebp + 12]
    mov edi, [ebp + 16]
    mov esi, [ebp + 20]

    ; Prepare for stdcall call
    push esi
    push edi
    push ebx
    push eax
    push edx
    push ecx

    ; Call the stdcall function
    call _printParameters@24

    ; Restore registers
    pop edi
    pop esi

    ; Restore the stack frame and return
    pop ebp
    ret
@addParameters@24 ENDP

ENDIF

END
