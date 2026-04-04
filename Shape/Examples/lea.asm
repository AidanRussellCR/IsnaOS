.code
entry:
    loadbase
    lea a, hello
    say hello
    exit

.data
hello:
    asciz "Hello\n"
