# timetemplate.S
	.data
	.align 2
mytime:	.word 	0x5957
timstr:	.asciz 	"text more text lots of text\0"
	.text
	.globl timetemplate, tick, time2string, display_string, delay

# Function for displaying a string with a newline at the end	
display_string:	
	li a7,4
	ecall
	li a0, 10
	li a7,11
	ecall
	jr ra
	
timetemplate:
	la	a0, timstr
	jal     display_string
	
	# wait a little
	li	a0, 1000	# ms
	jal	delay
	
	# call tick
	la 	a0, mytime
	jal	tick
	
	# call your function time2string
	la	a0, timstr
	la	t0, mytime
	lw	a1, 0(t0)
	jal	time2string
	
	# go back and do it all again
	j	timetemplate

	
# tick: update time pointed to by $a0
tick:	lw	t0, 0(a0)	# get time
	addi	t0, t0, 1	# increase
	andi	t1, t0, 0xf	# check lowest digit
	sltiu	t2, t1, 0xa	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x6	# adjust lowest digit
	
	andi	t1, t0, 0xf0	# check next digit
	sltiu	t2, t1, 0x60	# if digit < 6, okay
	bnez	t2, tiend
	addi	t0, t0, 0xa0	# adjust digit
	
	li	t3, 0xF
	slli	t3, t3, 0x8
	and	t1, t0, t3	# check minute digit
	addi	t3, x0, 0xA
	slli	t3, t3, 0x8
	slt	t2, t1, t3	# if digit < a, okay
	bnez	t2, tiend
	addi	t0, t0, 0x600	# adjust digit - this one's okay, it's lower than 0x7FF 
	
	li	t3, 0xF
	slli	t3, t3, 0xC
	and	t1, t0, t3	# check last digit
	addi	t3, x0, 0x6
	slli	t3, t3, 0xC
	slt	t2, t1, t3	# if digit < 6, okay
	bnez	t2, tiend
	
	li	t3, 0xA
	slli	t3, t3, 0xC
	add	t0, t0, t3	# adjust last digit
	
	# Now check for hour overflow (if hours >= 24:00:00, reset to 00:00:00)
	# Extract tens of hours (bits 20-23)
	srli    t1, t0, 20
	andi    t1, t1, 0xF
	# Extract ones of hours (bits 16-19)
	srli    t2, t0, 16
	andi    t2, t2, 0xF
	# If hours >= 24, reset to 0
	li      t3, 2
	blt     t1, t3, tiend_hours
	li      t3, 4
	blt     t2, t3, tiend_hours
	# Set hours to 00, keep minutes and seconds
	li      t3, 0xFF0000
	not     t3, t3
	and     t0, t0, t3
tiend_hours:
tiend:	sw	t0,0(a0)	# save updated result
	jr	ra		# return

#########################################################
# Place for your functions: time2string, hex2asc, delay.#
#########################################################	
# Transforms hexadecimal to ASCII.
# Easily replaced by "addi a0, a0, 0x30" instead of "jal hexasc" since no letters in clock
hexasc:
	li	t0, 10
	bge 	a0, t0, letters
	addi	a0, a0, 0x30
	jr	ra
letters:
	addi	a0, a0, 0x47
	jr 	ra

# Delay, causes CPU to be busy, does nothing
# Takes a0 as argument for amount of while-loops
delay:
	addi t0, a0, 0
# While loop
while:
	blez t0, delay_done
	li t1, 15000
# For loop
for:
	addi t1, t1, -1
	bnez t1, for
	addi t0, t0, -1
	j while
delay_done:
	jr ra


# Makes inputted time ready to print
# Calls upon hexasc to transform time from hexadecimal to ASCII
time2string:
	addi	sp, sp, -8
	sw	ra, 4(sp)
	sw	s0, 0(sp)
	
	mv	s0, a0
	# tens of hours
	srli 	t0, a1, 20
	andi	a0, t0, 0xF
	jal ra, hexasc
	sb	a0, 0(s0)

	# ones of hours
	srli 	t0, a1, 16
	andi	a0, t0, 0xF
	jal ra, hexasc
	sb	a0, 1(s0)

	# add colon
	li	t0, 0x3A
	sb	t0, 2(s0)

	# tens of minutes
	srli	t0, a1, 12
	andi	a0, t0, 0xF
	jal	ra, hexasc
	sb	a0, 3(s0)
	
	# ones of minutes
	srli	t0, a1, 8
	andi	a0, t0, 0xF
	jal	ra, hexasc
	sb	a0, 4(s0)
	
	# add colon
	li	t0, 0x3A
	sb	t0, 5(s0)
	
	# tens of seconds
	srli	t0, a1, 4
	andi	a0, t0, 0xF
	jal	ra, hexasc
	sb	a0, 6(s0)
	
	# ones of seconds
	andi	a0, a1, 0xF
	jal	ra, hexasc
	sb	a0, 7(s0)
	
	# terminator
	sb	zero, 8(s0)
	
	# restore and cleanup
	lw	s0, 0(sp)
	lw	ra, 4(sp)
	addi	sp, sp, 8
	jr 	ra
