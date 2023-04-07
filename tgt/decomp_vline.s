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
	.export		_decomp_vline
	.importzp	passin, passout

_in = passin
_out = passout
_orig = ptr4
_val = tmp1
_i = tmp2
_highline0 = regsave
_highline1 = regsave+1
_highline2 = regsave+2
_highline3 = regsave+3
_highline4 = ptr1
_highline5 = ptr1+1
_highline6 = ptr2
_highline7 = ptr2+2

; ---------------------------------------------------------------
; u8 decomp_vline(const u8 *in, u8 *out);
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_decomp_vline: near

	lda	_in
	sta	_orig
	ldx	_in+1
	stx	_orig+1
;
; orig = in;
;
;
; val = *in++;
;
	lda	(_in)
	sta	_val
	inc	_in
	bne	:+
	inc	_in+1
:
;
; memcpy(highline, in, 4);
;
	lda	(_in)
	tax
	and	#$0F
	sta	_highline0
	txa
	lsr
	lsr
	lsr
	lsr
	sta	_highline1

	ldy	#1
	lda	(_in),y
	tax
	and	#$0F
	sta	_highline2
	txa
	lsr
	lsr
	lsr
	lsr
	sta	_highline3

	iny
	lda	(_in),y
	tax
	and	#$0F
	sta	_highline4
	txa
	lsr
	lsr
	lsr
	lsr
	sta	_highline5

	iny
	lda	(_in),y
	tax
	and	#$0F
	sta	_highline6
	txa
	lsr
	lsr
	lsr
	lsr
	sta	_highline7
;
; in += 4;
;
	lda     #$04
	clc
	adc     _in
	sta     _in
	bcc     L000E
	inc     _in+1
;
; for (i = 0; i < 8; i++) {
;
L000E:	stz     _i
L005F:	lda     _i
	cmp     #$08
	jcs     L0010
;
; if (val & (1 << i)) {
;
	lsr     _val
	bcc     L0017
;
; out[0 * 8 + i] = highline[0] & 15;
;
	lda     _highline0
	sta	(_out)
;
; out[1 * 8 + i] = highline[0] >> 4;
;
	lda	_highline1
	ldy	#8
	sta	(_out),y
;
; out[2 * 8 + i] = highline[1] & 15;
;
	lda	_highline2
	ldy	#16
	sta	(_out),y
;
; out[3 * 8 + i] = highline[1] >> 4;
;
	lda	_highline3
	ldy	#24
	sta	(_out),y
;
; out[4 * 8 + i] = highline[2] & 15;
;
	lda	_highline4
	ldy	#32
	sta	(_out),y
;
; out[5 * 8 + i] = highline[2] >> 4;
;
	lda	_highline5
	ldy	#40
	sta	(_out),y
;
; out[6 * 8 + i] = highline[3] & 15;
;
	lda	_highline6
	ldy	#48
	sta	(_out),y
;
; out[7 * 8 + i] = highline[3] >> 4;
;
	lda	_highline7
	ldy	#56
	sta	(_out),y
;
; } else {
;
	jmp     L0060
;
; out[0 * 8 + i] = in[0] & 15;
;
L0017:	lda     (_in)
	tax
	and	#$0F
	sta	(_out)
;
; out[1 * 8 + i] = in[0] >> 4;
;
	txa
	lsr
	lsr
	lsr
	lsr
	ldy	#8
	sta	(_out),y
;
; out[2 * 8 + i] = in[1] & 15;
;
	ldy	#1
	lda	(_in),y
	tax
	and	#$0F
	ldy	#16
	sta	(_out),y
;
; out[3 * 8 + i] = in[1] >> 4;
;
	txa
	lsr
	lsr
	lsr
	lsr
	ldy	#24
	sta	(_out),y
;
; out[4 * 8 + i] = in[2] & 15;
;
	ldy	#2
	lda	(_in),y
	tax
	and	#$0F
	ldy	#32
	sta	(_out),y
;
; out[5 * 8 + i] = in[2] >> 4;
;
	txa
	lsr
	lsr
	lsr
	lsr
	ldy	#40
	sta	(_out),y
;
; out[6 * 8 + i] = in[3] & 15;
;
	ldy	#3
	lda	(_in),y
	tax
	and	#$0F
	ldy	#48
	sta	(_out),y
;
; out[7 * 8 + i] = in[3] >> 4;
;
	txa
	lsr
	lsr
	lsr
	lsr
	ldy	#56
	sta	(_out),y
;
; in += 4;
;
	lda     #$04
	clc
	adc     _in
	sta     _in
	bcc     L0060
	inc     _in+1
;
; for (i = 0; i < 8; i++) {
;
L0060:	inc     _i
	inc	_out
	bne	:+
	inc	_out+1
:
	jmp     L005F
;
; return in - orig;
;
L0010:	lda     _in
	sec
	sbc     _orig
	ldx     #$00

	rts

.endproc
