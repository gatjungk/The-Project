label1:
label2:
SLT $s6, $t5, $s1
BEQ $t9, $sp, label1
BNE $k1, $s1, label1
JR $t4
BEQ $s5, $t4, label1
J label1
BNE $s6, $sp, label1
BNE $v1, $s6, label2
BEQ $s3, $fp, label2
JR $at
J label2
J label2
J label2
SUB $at, $t5, $a0
ADD $s7, $s2, $t5
SLLV $s3, $t2, $t5
JR $ra
SUB $ra, $gp, $t8
SLLV $a2, $t8, $s7
