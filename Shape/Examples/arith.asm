.code
entry:
    say res
    mov a, 10
    add a, 5
    mul a, 2
    sub a, 4
    div a, 2
    show
    say nl
    exit

.data
res:
    asciz "The result is: "
nl:
    asciz "\n"