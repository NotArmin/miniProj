  # hexmain.S
  # Written 2015-09-04 by F Lundevall
  # Copyright abandonded - this file is in the public domain.

	.text
	.globl hex2asc

hex2asc:
	li	a0, 16		# test number (from 0 to 15)
	
	addi    sp,sp,-4
	sw      ra,0(sp)
	
	jal	hexasc		# call hexasc
	
	li	a7, 11	# write a0 to stdout
	ecall

	lw      ra,0(sp)
	addi    sp,sp,4
	jr      ra	

  # You can write your own code for hexasc here
hexasc:
	li	t0, 10
	andi	a0, a0, 0xf
	bge 	a0, t0, letters
	addi	a0, a0, 0x30
	jr	ra
	
	letters:
	addi	a0, a0, 0x37
	jr 	ra
