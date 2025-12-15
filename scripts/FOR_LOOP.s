MOV X0 0  
MOV X1 5

loop:
CMP X0 X1 
BLE loop_end
ADD X0, X0, 1
B loop:
  
loop_end:
MOV X2 0xFFFF


EXIT
