.code
entry:
    ; The number to guess is 5
    say intro

menu:
    say input
    hear

    cmp a, 'x'
    jz done

    cmp a, '5'
    jz success
    jnz fail

done:
    say bye
    exit

success:
    say win
    jmp menu

fail:
    say lose
    jmp menu

.data
intro:
    asciz "Welcome to the guessing game!\n"
input:
    asciz "\nEnter your guess here: "
win:
    asciz "\nYou guessed correct!\n"
lose:
    asciz "\nYou guessed wrong.\n"
bye:
    asciz "\nThank you for playing!\n"