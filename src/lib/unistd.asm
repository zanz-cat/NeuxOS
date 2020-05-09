[SECTION .text]
global _sleep_sec
global _sleep_usec

_sleep_sec:
    push ecx
    mov ecx, 01ffffffh
.next:
    loop .next
    pop ecx
    ret

_sleep_usec:
    push ecx
    mov ecx, 0fffffh
.next:
    loop .next
    pop ecx
    ret