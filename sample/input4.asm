     addi r1,r1,2    ;set r1=2, r1 is counter
test addi r2,r2,2    ;set r2=2, r2 is counter
     addi r1,r1,-1    ;r1=r1-1
     beqz r1,end
loop addi r2,r2,-1    ;r2=r2-1
     beqz r2,test
     j loop
end halt