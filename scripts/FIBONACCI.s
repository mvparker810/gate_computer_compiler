//FIB(X0) = x1
//FIB(24) = 46368
MOV X0 24

PRINT 0 'F'
PRINT 1 'i'
PRINT 2 'b'
PRINT 3 '('

MOV X1 4
LR X7
ADD X7 X7 3
B PRINT_NUMBER_UNSIGNED

ADD X1 X1 1
PRINT X1 ')'
ADD X1 X1 1
PRINT X1 '='


ADD X1 X1 1
WRITE X1 0x00
//at this point screen says : fib(x0)=

//caluclate fib x0

MOV X1 0x0000      
MOV X2 0x0001 
MOV X3 0x0001

fib_loop:
    CMP X3 X0       
    BGT fib_done     
    ADD X5 X1 X2      
    MOV X1 X2           
    MOV X2 X5          
    ADD X3 X3 0x0001   
    B fib_loop          
fib_done:

//x1 = fib(x0)

MOV X0 X1
READ X1 0x00

LR X7
ADD X7 X7 3
B PRINT_NUMBER_UNSIGNED


EXIT

/*
TODO description

Arguments:
X0 -> Number
X1 -> Screen Location
X7 -> Return Register
*/
PRINT_NUMBER_UNSIGNED:
    #ALIAS X0 NUMBER
    #ALIAS X1 SCREENLOC
    #ALIAS X2 BCDECIMAL
    #ALIAS X3 SHIFT
    #ALIAS X5 RUNSUM

    MOV SHIFT 12
    BCDH BCDECIMAL NUMBER

    MOV RUNSUM 0

.digit_loop:

    LSR X4 BCDECIMAL SHIFT
    AND X4 X4 0x000F
    ADD RUNSUM RUNSUM X4

    ADD X4 X4 '0'

    CMP RUNSUM 0
    beq .end_digit_print

    PRINT SCREENLOC X4
    ADD SCREENLOC SCREENLOC 1
.end_digit_print:
    SUB SHIFT SHIFT 4

    CMP SHIFT 0
    BLT .digit_loop_end

    B .digit_loop
.digit_loop_end:

    BCDL BCDECIMAL NUMBER
    AND BCDECIMAL BCDECIMAL 0x000F
    ADD BCDECIMAL BCDECIMAL '0'
    PRINT SCREENLOC BCDECIMAL
    
    b X7