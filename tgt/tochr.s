;
; File generated by cc65 v 2.18 - Git 2f3955d
;
	.fopt		compiler,"cc65 v 2.18 - Git 2f3955d"
	.setcpu		"65C02"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
	.importzp	tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
	.macpack	longbranch
	.import		_memset
	.export		_tochr

_in = ptr2
_out = ptr3
_y = tmp3
_pix = tmp4

; ---------------------------------------------------------------
; void tochr(const u8 *in, u8 * const out);
; ---------------------------------------------------------------

.segment "RODATA"

invbits:
	.byte 1 << 7
	.byte 1 << 6
	.byte 1 << 5
	.byte 1 << 4
	.byte 1 << 3
	.byte 1 << 2
	.byte 1 << 1
	.byte 1 << 0

.segment	"CODE"

.proc	_tochr: near

	sta	_out
	stx	_out+1

	jsr	popax
	sta	_in
	stx	_in+1
;
; memset(out, 0, 32);
;
	lda     #$00
	ldy     #$1F
L003A:	sta     (_out),y
	dey
	bpl     L003A
;
; for (y = 0; y < 8; y++) {
;
	stz     _y
L003F:	lda     _y
	cmp     #$08
	bcc     L0048
;
; }
;
	rts
;
; for (x = 0; x < 8; x++) {
;
L0048:	ldx	#0
L0040:	cpx     #$08
	jcs     L0047
;
; const u8 pix = *in++;
;
	lda	(_in)
	sta	_pix

	inc	_in
	bne	@noinc
	inc	_in+1
@noinc:
;
; if (pix & 1)
;
	lda     _pix
	and     #$01
	beq     L0042
;
; out[y * 2] |= 1 << (7 - x);
;
	lda     _y
	asl     a
	tay

	lda	(_out),y
	ora	invbits,x
	sta	(_out),y
;
; if (pix & 2)
;
L0042:	lda     _pix
	and     #$02
	beq     L0044
;
; out[y * 2 + 1] |= 1 << (7 - x);
;
	lda     _y
	asl     a
	ina
	tay

	lda	(_out),y
	ora	invbits,x
	sta	(_out),y
;
; if (pix & 4)
;
L0044:	lda     _pix
	and     #$04
	beq     L0046
;
; out[16 + y * 2] |= 1 << (7 - x);
;
	lda     _y
	asl     a
	clc
	adc	#16
	tay

	lda	(_out),y
	ora	invbits,x
	sta	(_out),y
;
; if (pix & 8)
;
L0046:	lda     _pix
	and     #$08
	beq     L002E
;
; out[16 + y * 2 + 1] |= 1 << (7 - x);
;
	lda     _y
	asl     a
	clc
	adc	#17
	tay

	lda	(_out),y
	ora	invbits,x
	sta	(_out),y
;
; }
;
L002E:
;
; for (x = 0; x < 8; x++) {
;
	inx
	jmp     L0040
;
; for (y = 0; y < 8; y++) {
;
L0047:	inc     _y
	jmp     L003F

.endproc

