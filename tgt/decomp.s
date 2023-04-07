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
	.import		_tochr
	.import		_decomp_flat
	.import		_decomp_1bit
	.import		_decomp_2bit
	.import		_decomp_3bit
	.import		_decomp_rle
	.import		_decomp_hline
	.import		_decomp_vline
	.import		_decomp_commonbyte
	.import		_decomp_ancestor
	.import		_decomp_uncompressed
	.export		_stc1_decompress
	.export		_mbyte
	.exportzp	passin, passout

.segment	"RODATA"

_methods:
	.addr	_decomp_1bit
	.addr	_decomp_2bit
	.addr	_decomp_3bit
	.addr	_decomp_rle
	.addr	_decomp_hline
	.addr	_decomp_vline
	.addr	_decomp_commonbyte
	.word	$0000
	.addr	_decomp_uncompressed

.segment	"ZEROPAGE"

passin:
	.res 2
passout:
	.res 2

.segment	"BSS"

_tmp:
	.res	128,$00
_numtiles:
	.res	2,$00
_in:
	.res	2,$00
_out:
	.res	2,$00
_mbyte:
	.res	1,$00
_m:
	.res	1,$00
_tgt:
	.res	1,$00

; ---------------------------------------------------------------
; void stc1_decompress(const u8 *in, u8 *out);
; ---------------------------------------------------------------

.segment	"CODE"

launcher:
	ldx	_tgt
	jmp	(_methods-2,x)

.proc	_stc1_decompress: near

	sta	_out
	stx	_out+1

	jsr	popax
	sta	_in
	sta	ptr1
	stx	_in+1
	stx	ptr1+1
;
; memcpy(&numtiles, in, 2);
;
	lda	(ptr1)
	sta	_numtiles
	ldy	#1
	lda	(ptr1),y
	sta	_numtiles+1
;
; in += 2;
;
	lda     #$02
	clc
	adc     _in
	sta     _in
	bcc     L0015
	inc     _in+1
;
; for (; numtiles; numtiles--) {
;
L0015:	lda     _numtiles
	ora     _numtiles+1
	bne     L0071
;
; }
;
	rts
;
; mbyte = *in++;
;
L0071:	lda     _in
	ldx     _in+1
	sta     regsave
	stx     regsave+1
	ina
	bne     L001E
	inx
L001E:	sta     _in
	stx     _in+1
	lda     (regsave)
	sta     _mbyte
	tax
;
; m = mbyte & 15;
;
	and     #$0F
	sta     _m
	asl
	sta	_tgt
;
; mbyte >>= 4;
;
	txa
	lsr     a
	lsr     a
	lsr     a
	lsr     a
	sta     _mbyte
;
; if (m >= M_COMMONBYTE) {
;
	lda     _m
	cmp     #$07
	ldx     #$00
	bcc     L005A
;
; if (m == M_ANCESTOR)
;
	lda     _m
	cmp     #$08
	bne     L0059
;
; in += decomp_ancestor(in, out, mbyte);
;
	lda	_in
	ldx	_in+1
	sta	passin
	stx	passin+1
	lda	_out
	ldx	_out+1
	sta	passout
	stx	passout+1
	jsr     _decomp_ancestor
;
; else
;
	jmp     L0066
;
; in += methods[m](in, out);
;
L0059:
	lda     _in
	ldx     _in+1
	sta	passin
	stx	passin+1
	lda     _out
	ldx     _out+1
	sta	passout
	stx	passout+1
;
; } else if (m == M_FLAT) {
;
	bra     L0070
L005A:	lda     _m
	bne     L005B
;
; methods[M_FLAT](&mbyte, tmp);
;
	lda     #<(_tmp)
	ldx     #>(_tmp)
	sta	passout
	stx	passout+1
	jsr	_decomp_flat
;
; } else {
;
	bra     L0039
;
; in += methods[m](in, tmp);
;
L005B:
	lda     _in
	ldx     _in+1
	sta	passin
	stx	passin+1
	lda     #<(_tmp)
	ldx     #>(_tmp)
	sta	passout
	stx	passout+1
L0070:
	jsr	launcher

L0066:	clc
	adc     _in
	sta     _in
	lda     #$00
	adc     _in+1
	sta     _in+1
;
; if (m == M_VLINE || m == M_HLINE) {
;
L0039:	lda     _m
	cmp     #$06
	beq     L005C
	cmp     #$05
	bne     L005D
;
; memcpy(tmp + 64, tmp, 64);
;
L005C:	ldy     #$3F
L0046:	lda     _tmp,y
	sta     _tmp+64,y
	dey
	bpl     L0046
;
; tochr(tmp + 64 - mbyte, out);
;
	lda     #<(_tmp+64)
	sec
	sbc     _mbyte
	sta	passin
	lda     #>(_tmp+64)
	sbc     #$00
	sta	passin+1
;
; } else if (m < M_HLINE) {
;
	bra     L0063
L005D:	lda     _m
	cmp     #$05
	bcs     L004B
;
; tochr(tmp, out);
;
	lda     #<(_tmp)
	ldx     #>(_tmp)
	sta	passin
	stx	passin+1
L0063:
	lda     _out
	ldx     _out+1
	jsr     _tochr
;
; out += 32;
;
L004B:	lda     #$20
	clc
	adc     _out
	sta     _out
	bcc     L0052
	inc     _out+1
;
; for (; numtiles; numtiles--) {
;
L0052:	lda     _numtiles
	ldx     _numtiles+1
	sec
	sbc     #$01
	bcs     L001B
	dex
L001B:	sta     _numtiles
	stx     _numtiles+1
	jmp     L0015

.endproc
