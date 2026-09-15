; Z80-Highlighting: Dieses Beispiel im doseditor oeffnen.
; TODO: Hier dein eigenes Z80-Programm ergaenzen.

        ORG     $8000
PORT    EQU     10H
MASK    EQU     %10101010

start:  LD      SP,0FFFFH
        LD      IX,buffer
        LD      B,16
        XOR     A
.loop:  LD      (IX+0),A
        INC     IX
        INC     A
        DJNZ    .loop

        EX      AF,AF'
        BIT     7,A
        CALL    NZ,output
        EX      AF,AF'
        JR      done

output: OUT     (PORT),A
        RET

done:   HALT
        JP      done

message: DB     "Hallo Z80! ; dies bleibt ein String",13,10,0
values:  DB     0FFH,$FF,#FF,0xFF,10101010B,0b10101010
buffer:  DS     16
         END
