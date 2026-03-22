.code
entry:
    speak wake
    yield
    speak bind
    ret

.data
wake:
    asciz "The golem awakens...\n"
bind:
    asciz "It awaits your command.\n"