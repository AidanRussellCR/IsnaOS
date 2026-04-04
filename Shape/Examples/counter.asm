.code
entry:
    loadbase
    mov a, [count]
    add a, 1
    mov [count], a
    show
    exit

.data
count:
    dd 41

; outputs 42
