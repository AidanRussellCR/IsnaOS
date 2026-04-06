.code
entry:
    loadapi
    loadbase

    mov a, 42 ; the meaning of life
    call func
    exit

func:
    push a
    show
    say nl
    pop a
    ret

.data
nl:
    asciz "\n"


; Outputs 42
