VARIABLE SECRET
VARIABLE TRIES
VARIABLE WON

: NEW-GAME
    100 RANDOM 1+ SECRET !
    0 TRIES !
    0 WON !
    ." === Number Guess ===" CR
    ." I'm thinking of 1-100." CR
    ." You have 10 tries." CR CR
;

: CHECK ( guess -- )
    DUP SECRET @ = IF
        ." Correct!" CR
        DROP
        1 WON !
        DROP
    ELSE
        DUP SECRET @ < IF
            ." Too low!" CR
        ELSE
            ." Too high!" CR
        THEN
        DROP
        DROP
        TRIES @ 1+ TRIES !
    THEN
;

: PLAY
    NEW-GAME
    BEGIN
        TRIES @ 10 <
        WON @ 0=
        AND
    WHILE
        ." Guess #" TRIES @ 1+ . ." : "
        INPUT
        DUP
        CHECK
    REPEAT
    WON @ 1 = IF
        ." You win!" CR
    ELSE
        ." Game over. Number was " SECRET @ . CR
    THEN
;

PLAY