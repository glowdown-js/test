VARIABLE SECRET
VARIABLE TRIES
VARIABLE WON

: NEW-GAME
    100 RANDOM 1+ SECRET !
    0 TRIES !
    0 WON !
;

: CHECK
    DUP SECRET @ = IF
        DROP
        1 WON !
    ELSE
        DROP
        TRIES @ 1+ TRIES !
    THEN
;

NEW-GAME
." Secret: " SECRET @ . CR
." Tries: " TRIES @ . CR