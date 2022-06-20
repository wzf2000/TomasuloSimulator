     addi r9,r9,1     ;set r9=1
     addi r1,r1,10    ;set r1=10, r1 is counter
loop add r10,r8,r9    ;r10 = r8+r9
     addi r8,r9,0     ;r8 = r9
     sw r10,r2,0      ;store r10, use r2 as offset
     lw r9,r2,0       ;load r9, use r2 as offset
     sub r3,r9,r10    ;r3=r9-r10
     j test
cont addi r2,r2,1     ;offset ++
     addi r1,r1,-1    ;counter --
     beqz r1,end      ;loop
     j loop
test beqz r3,cont
end halt