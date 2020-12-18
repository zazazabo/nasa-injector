.code
__image_base proc
    xor rax, rax
    mov rax, gs:[30h]
    mov rax, [rax + 60h]
    mov rax, [rax + 10h]
    ret
__image_base endp
end