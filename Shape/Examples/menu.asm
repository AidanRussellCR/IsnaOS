.code
entry:
menu:
    say options
    hear

    cmp a, '1'
    jz one

    cmp a, '2'
    jz two

    cmp a, '3'
    jz three

    cmp a, '4'
    jz done

    say bad
    jmp menu

one:
    say msg1
    jmp menu

two:
    say msg2
    jmp menu

three:
    say msg3
    jmp menu

done:
    say bye
    exit

.data
options:
    asciz "\nHello, select a line option.\nSelect lines 1-3, or 4 to exit: "
msg1:
    asciz "\nExample line 1.\n"
msg2:
    asciz "\nExample line 2.\n"
msg3:
    asciz "\nExample line 3.\n"
bad:
    asciz "\nPlease enter valid input (1-4).\n"
bye:
    asciz "\nGoodbye.\n"