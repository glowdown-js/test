: COUNT-UP
    5 0 DO
        I .
    LOOP
    CR
;

COUNT-UP

: NESTED
    3 0 DO
        2 0 DO
            I . J . SPACE
        LOOP
        CR
    LOOP
;

NESTED

: TEST-CASE
    CASE
        1 OF ." one" ENDOF
        2 OF ." two" ENDOF
        3 OF ." three" ENDOF
        ." unknown"
    ENDCASE
    CR
;

1 TEST-CASE
2 TEST-CASE
5 TEST-CASE

: TEST-LEAVE
    10 0 DO
        I .
        I 5 = IF LEAVE THEN
    LOOP
    CR
;

TEST-LEAVE