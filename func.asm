section .data
distance2:    equ  4900                 ; constant value defining distance
                                        ; that is affected with alpha blending
section .text

; Make function visible outside this file
global alphaFunc

alphaFunc:

    ; PROLOGUE
	push rbp                            ; save caller's address
	mov rbp, rsp                        ; move current stack pointer

    ; =================== Arguments passed to function =====================
    ;   rdi  ->  pointer to primary bitmap ; first pixel in top left corner
    ;   rsi  ->  pointer to secondary bitmap
    ;   rdx  ->  Clicked coordinate X
    ;   rcx  ->  Clicked coordinate Y
    ;   r8   ->  bitmap width
    ;   r9   ->  bitmap height
    ;   xmm0 ->  sine argument
    ; ======================================================================

    ; =================== Registers used in algorithm ======================
    ;	xmm1 : 1.0
    ;	xmm2 : 2.0
    ;	xmm3 : 1 + sine
    ;	xmm6 : 1 - sine
    ;	xmm4 : sine argument
    ;	xmm5 : sine value
    ;	xmm7 : color from primary bitmap
    ;	xmm8 : color from secondary bitmap
    ; 	xmm9 : sineRatio
    ; ======================================================================

    ; ===================== ALPHA BLENDING Equation ========================
    ;
    ;   (1 + sine) * IM1_pixel_value + (1 - sine) * IM2_pixel_value
    ; _________________________________________________________________ = new pixel value
    ;                                2
    ;
    ;   sine is equal to sin( distance/sineRatio )
    ;=======================================================================

; Prepare for operations
    %idefine    pixel_counter           dword [rbp - 8]
    %idefine    number_of_pixels        dword [rbp - 16]

	finit					            ; initiate Floating-Point Unit

; set coordinates of first pixel
	mov         r10, 1                  ; start from first column
	mov		    r11, 1		            ; start from first row

	cvtsi2sd	xmm1, r11d 		        ; xmm1 = 1.0

	movsd		xmm2, xmm1
	addsd		xmm2, xmm1 		        ; xmm2 = 2.0

    ; push on the stack counter that stores how many pixels have been read
	mov         rax, 1
    push        rax

    ; push on the stack bitmap width * bitmap height
    imul        r9, r8
    push        r9

    jmp         calculateDistance       ; jump to calculate distance of the current pixel

; ======================================================== ;
;               MAIN ALGORITHM AND LOOPS
; ======================================================== ;

nextRow:
    ; move images pointer to the next row
    sub         rdi, 4096               ; move primary image pointer
    sub         rsi, 4096               ; move secondary image pointer
    ; why 4096? It is dependant on how allegro5 stores bitmap in the memory
    ; see https://liballeg.org/a5docs/trunk/graphics.html#allegro_locked_region

	add		    r11d, 1                 ; increment Y coordinate
	mov         r10d, 1                 ; set X coordinate to 1 start from first column
    inc         pixel_counter           ; increment pixel counter

    add         rdi, 4                  ; increment primary image pointer by one pixel
    add         rsi, 4                  ; increment secondary image pointer by one pixel

	jmp         calculateDistance

nextPixel:
    inc         r10d                    ; increment X coordinate
    inc         pixel_counter           ; increment counter

    ; if number of pixels iterated bigger than bitmap pixel quantity
    mov         r9d, number_of_pixels
    cmp         pixel_counter, r9d
    je          end

    add         rdi, 4                  ; increment primary image pointer by one pixel
    add         rsi, 4                  ; increment secondary image pointer by one pixel

; ================================================ ;
;       r10d - current X coordinate of pixel
;       r11d - current Y coordinate of pixel
; ================================================ ;

; CHECK IF NEXT ROW
	; if x coordinate is greater or equal to width
	cmp         r10d, r8d
    jae         nextRow

; calculate distance from the current pixel to the clicked position
calculateDistance:

    neg		    r10d			        ; negative current X
	neg		    r11d                    ; negative current Y

	lea		    r12d, [edx + r10d]      ; mouse X - actual X
	lea		    r13d, [ecx + r11d]      ; mouse Y - actual Y

	imul		r12d, r12d		        ; (mouse X - actual X)^2
	imul		r13d, r13d              ; (mouse Y - actual Y)^2

	lea		    r14d, [r12d + r13d]     ; (mouse X - actual X)^2 + (mouse Y - actual Y)^2

	neg		    r10d			        ; current X
    neg		    r11d                    ; current Y

; r14d holds the (distance between clicked point and current pixel)^2

; if distance^2 greater than constant value, jump to the next pixel
    cmp         r14, distance2
    ja          nextPixel

; ================================================ ;
;    From this point, alpha blending is in move
; ================================================ ;

    ; cvtsi2sd - Convert Dword Integer to Scalar Double-Precision FP Value
	cvtsi2sd	xmm4, r14d
	sqrtsd		xmm4, xmm4              ; calculate the square root of distance^2
	; sqrtsd - Compute Square Root of Scalar Double-Precision Floating-Point Value

; xmm4 holds now the distance

    ; xmm0 holds sine argument
	divsd 		xmm4, xmm0              ; divide distance by sine argument

; now we have distance/sineRatio = sine argument in xmm4

; make sine
    sub         rsp, 8                  ; allocate space on stack
	movq		qword [rbp - 24], xmm4  ; move xmm4 on stack
	fld		    qword [rbp - 24]        ; load floating point value to the fpu register stack
	fsin                                ; calculate sine
	fstp		qword [rbp - 24]        ; store floating point value from fpu register stack
	movq		xmm5, qword [rbp - 24]  ; move quad word sine to xmm5

; xmm5 holds sine value

	movsd 		xmm3, xmm1              ; Move Scalar Double-Precision Floating-Point Value
	movsd		xmm6, xmm1              ; Move Scalar Double-Precision Floating-Point Value

	addsd 		xmm3, xmm5	            ; xmm3 = 1 + sine
	subsd 		xmm6, xmm5	            ; xmm6 = 1 - sine

; ============================================= ;
;           Do operations on pixels
; ============================================= ;

; create counter to iterate through every pixel value R, G, B
; this loop could be replaced with 3 blocks of the same code

    mov r9d, 1                          ; initiate counter

change_pixel:
; get byte from picture

	mov 		al, [rdi]               ; move byte from the primary bitmap
	mov 		ah, [rsi]               ; move byte from the secondary bitmap

; extend byte to 32 bits
	movzx		r15d, al                ; move extending bits
	movzx 		ebx, ah

	cvtsi2sd	xmm7, r15d              ; convert
	cvtsi2sd	xmm8, ebx

	mulsd 		xmm7, xmm3              ; multiply by 1 + sine
	mulsd 		xmm8, xmm6              ; multiply by 1 - sine

	addsd 		xmm7, xmm8              ; (1 + sine) * IM1_color + IM2_color * (1 - sine)

	divsd 		xmm7, xmm2              ; divide by 2.0

	cvtsd2si	rax, xmm7               ; convert and send to rax

; save modified pixel in al
	mov		    [rdi], al               ; move lower 8 bits of rax

	inc		    rdi                     ; increment pixel value pointer
	inc		    rsi

	inc         r9d                     ; increment counter

; jump if all pixel values R, G, B were replaced
	cmp         r9d, 3
	jle         change_pixel

; do nothing with alpha channel
	inc 	    rdi
	inc		    rsi

    sub         rdi, 4
    sub         rsi, 4

	jmp		    nextPixel

end:
    ; EPILOGUE
    add         rsp, 8
    pop         r9
    pop         rax

	mov 		rsp, rbp
	pop 		rbp

	ret
