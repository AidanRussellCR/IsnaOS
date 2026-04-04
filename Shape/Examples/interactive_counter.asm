; An interactive counter to showcase a good use of globals in a user-interactive program

.code
entry:
    loadbase

loop:
    say prompt
    hear

    cmp a, '+'
    je inc

    cmp a, '-'
    je dec

    cmp a, 'q'
    je done

    jmp loop

inc:
    mov a, [counter]
    add a, 1
    mov [counter], a
    say nl
    mov a, [counter]
    show
    say nl
    jmp loop

dec:
    mov a, [counter]
    sub a, 1
    mov [counter], a
    say nl
    mov a, [counter]
    show
    say nl
    jmp loop

done:
    exit

.data
counter:
    dd 0

prompt:
    asciz "Press + to increment, - to decrement, q to quit: "

nl:
    asciz "\n"
