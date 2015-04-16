#ifdef BUILD_NEON
#ifdef BUILD_NEON_INTRINSICS
#include <arm_neon.h>
#endif
#endif
/* blend pixel x color --> dst */
#ifdef BUILD_NEON

#include <arm_neon.h>

/* Note: Optimisation is based on keeping _dest_ aligned: else it's a pair of
 * reads, then two writes, a miss on read is 'just' two reads */
static void
_op_blend_p_c_dp_neon(DATA32 * __restrict s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 * __restrict d, int l) {
#ifdef BUILD_NEON_INTRINSICS
   uint16x8_t ad0_16x8;
   uint16x8_t ad1_16x8;
   uint16x8_t sc0_16x8;
   uint16x8_t sc1_16x8;
   uint16x8_t x255_16x8;
   uint32x2_t c_32x2;
   uint32x4_t ad_32x4;
   uint32x4_t alpha_32x4;
   uint32x4_t cond_32x4;
   uint32x4_t d_32x4;
   uint32x4_t s_32x4;
   uint32x4_t sc_32x4;
   uint32x4_t x0_32x4;
   uint32x4_t x1_32x4;
   uint8x16_t ad_8x16;
   uint8x16_t alpha_8x16;
   uint8x16_t d_8x16;
   uint8x16_t s_8x16;
   uint8x16_t sc_8x16;
   uint8x16_t x0_8x16;
   uint8x16_t x1_8x16;
   uint8x8_t ad0_8x8;
   uint8x8_t ad1_8x8;
   uint8x8_t alpha0_8x8;
   uint8x8_t alpha1_8x8;
   uint8x8_t c_8x8;
   uint8x8_t d0_8x8;
   uint8x8_t d1_8x8;
   uint8x8_t s0_8x8;
   uint8x8_t s1_8x8;
   uint8x8_t sc0_8x8;
   uint8x8_t sc1_8x8;

   c_32x2 = vdup_n_u32(c);
   c_8x8 = vreinterpret_u8_u32(c_32x2);
   x255_16x8 = vdupq_n_u16(0xff);
   x0_8x16 = vdupq_n_u8(0x0);
   x0_32x4 = vreinterpretq_u32_u8(x0_8x16);
   x1_8x16 = vdupq_n_u8(0x1);
   x1_32x4 = vreinterpretq_u32_u8(x1_8x16);
   DATA32 *start = d;
   int size = l;
   DATA32 *end = start + (size & ~3);
   while (start < end)
   {

      s_32x4 = vld1q_u32(s);
      s_8x16 = vreinterpretq_u8_u32(s_32x4);

      d_32x4 = vld1q_u32(start);
      d_8x16 = vreinterpretq_u8_u32(d_32x4);
      d0_8x8 = vget_low_u8(d_8x16);
      d1_8x8 = vget_high_u8(d_8x16);

      s0_8x8 = vget_low_u8(s_8x16);
      s1_8x8 = vget_high_u8(s_8x16);

      sc0_16x8 = vmull_u8(s0_8x8, c_8x8);
      sc1_16x8 = vmull_u8(s1_8x8, c_8x8);
      sc0_16x8 = vaddq_u16(sc0_16x8, x255_16x8);
      sc1_16x8 = vaddq_u16(sc1_16x8, x255_16x8);
      sc0_8x8 = vshrn_n_u16(sc0_16x8, 8);
      sc1_8x8 = vshrn_n_u16(sc1_16x8, 8);
      sc_8x16 = vcombine_u8(sc0_8x8, sc1_8x8);

      alpha_32x4 = vreinterpretq_u32_u8(sc_8x16);
      alpha_32x4 = vshrq_n_u32(alpha_32x4, 24);
      alpha_32x4 = vmulq_u32(x1_32x4, alpha_32x4);
      alpha_8x16 = vreinterpretq_u8_u32(alpha_32x4);
      alpha_8x16 = vsubq_u8(x0_8x16, alpha_8x16);
      alpha0_8x8 = vget_low_u8(alpha_8x16);
      alpha1_8x8 = vget_high_u8(alpha_8x16);

      ad0_16x8 = vmull_u8(alpha0_8x8, d0_8x8);
      ad1_16x8 = vmull_u8(alpha1_8x8, d1_8x8);
      ad0_8x8 = vshrn_n_u16(ad0_16x8,8);
      ad1_8x8 = vshrn_n_u16(ad1_16x8,8);
      ad_8x16 = vcombine_u8(ad0_8x8, ad1_8x8);
      ad_32x4 = vreinterpretq_u32_u8(ad_8x16);

      alpha_32x4 = vreinterpretq_u32_u8(alpha_8x16);
      cond_32x4 = vceqq_u32(alpha_32x4, x0_32x4);
      ad_32x4 = vbslq_u32(cond_32x4, d_32x4 , ad_32x4);

      sc_32x4 = vreinterpretq_u32_u8(sc_8x16);
      d_32x4 = vaddq_u32(sc_32x4, ad_32x4);

      vst1q_u32(start, d_32x4);

      s+=4;
      start+=4;
   }
   end += (size & 3);
   while (start <  end)
   {
      DATA32 sc = MUL4_SYM(c, *s);
      DATA32 alpha = 256 - (sc >> 24);
      *start = sc + MUL_256(alpha, *start);
      start++;
      s++;
   }
#else
#define AP	"blend_p_c_dp_"
   asm volatile (
		".fpu neon				\n\t"
		// Load 'c'
		"vdup.u32	q7, %[c]		\n\t"
		"vmov.i8	q6, #1			\n\t"

		// Choose a loop
		"andS		%[tmp], %[d], $0xf	\n\t"
		"beq		"AP"quadstart		\n\t"

		"andS		%[tmp],%[d], $0x4	\n\t"
		"beq		"AP"dualloop		\n\t"

	AP"singleloop:"
		"vld1.32	d0[0],  [%[s]]!		\n\t"
		"vld1.32	d2[0],	[%[d]]		\n\t"
		//  Mulitply s * c (= sc)
		"vmull.u8	q4,	d0,d14		\n\t"
		// sc in d8
		"vqrshrn.u16	d4,	q4, #8		\n\t"

		// sca in d9
		"vmvn.u32	d6,	d4		\n\t"
		"vshr.u32	d6,	d6, #24		\n\t"

		"vmul.u32	d6,     d12, d6 	\n\t"

		/* d * alpha */
		"vmull.u8	q4,	d6, d2 	 	\n\t"
		"vqrshrn.u16	d0,	q4, #8		\n\t"

		"vqadd.u8	d2,	d0, d4		\n\t"

		// Save dsc + sc
		"vst1.32	d2[0],	[%[d]]!		\n\t"

		// Now where?
		// Can we go the fast path?
		"andS		%[tmp], %[d],$0xf	\n\t"
		"beq		"AP"quadstart		\n\t"

	AP"dualloop:					\n\t"
		// Check we have enough to bother with!
		"sub		%[tmp], %[e], %[d]	\n\t"
		"cmp		%[tmp], #16		\n\t"
		"blt		"AP"loopout		\n\t"

		//  load 's' -> q0, 'd' -> q1
		"vldm		%[s]!,	{d0}		\n\t"
		"vldm		%[d], 	{d2}		\n\t"
		//  Mulitply s * c (= sc)
		"vmull.u8	q4,	d0,d14		\n\t"
		// sc in d8
		"vqrshrn.u16	d4,	q4, #8		\n\t"

		// sca in d9
		"vmvn.u32	d6,	d4		\n\t"
		"vshr.u32	d6,	d6, #24		\n\t"

		"vmul.u32	d6,     d12, d6 	\n\t"

		/* d * alpha */
		"vmull.u8	q4,	d6, d2 	 	\n\t"
		"vqrshrn.u16	d0,	q4, #8		\n\t"

		"vqadd.u8	d2,	d0, d4		\n\t"

		// Save dsc + sc
		"vst1.32	d2,	[%[d]]!		\n\t"

	AP"quadstart:					\n\t"
		"sub		%[tmp], %[e], %[d]	\n\t"
		"cmp		%[tmp], #16		\n\t"
		"blt		"AP"loopout		\n\t"

		"sub		%[tmp], %[e], #15	\n\t"

	AP"quadloop:\n\t"
		//  load 's' -> q0, 'd' -> q1
		"vldm	%[s]!, {d0,d1}		\n\t"
		"vldm	%[d], {d2,d3}		\n\t"
		//  Mulitply s * c (= sc)
		"vmull.u8	q4,	d0,d14	\n\t"
		"vmull.u8	q5,	d1,d14	\n\t"

		// Get sc & sc alpha
		"vqrshrn.u16	d4,	q4, #8		\n\t"
		"vqrshrn.u16	d5,	q5, #8		\n\t"
			// sc is now in q2, 8bpp
		// Shift out, then spread alpha for q2
		"vmvn.u32	q3,	q2		\n\t"
		"vshr.u32	q3,	q3, $0x18	\n\t"
		"vmul.u32	q3,	q6,q3		\n\t"

		//  Multiply 'd' by sc.alpha (dsca)
		"vmull.u8	q4,	d6,d2		\n\t"
		"vmull.u8	q5,	d7,d3		\n\t"

		"vqrshrn.u16	d0,	q4, #8		\n\t"
		"vqrshrn.u16	d1,	q5, #8		\n\t"

		"vqadd.u8	q1,	q0, q2		\n\t"

		// Save dsc + sc
		"vstm		%[d]!,	{d2,d3}		\n\t"

		"cmp 		%[tmp], %[d]		\n\t"

		"bhi 		"AP"quadloop		\n\t"

	/* Trailing stuff */
	AP"loopout:					\n\t"

		"cmp 		%[d], %[e]		\n\t"
                "beq 		"AP"done\n\t"
		"sub		%[tmp],%[e], %[d]	\n\t"
		"cmp		%[tmp],$0x04		\n\t"
		"beq		"AP"singleloop2		\n\t"

		"sub		%[tmp], %[e], #7	\n\t"
	/* Dual loop */
	AP"dualloop2:					\n\t"
		"vldm		%[s]!, {d0}		\n\t"
		"vldm		%[d], {d2}		\n\t"
		//  Mulitply s * c (= sc)
		"vmull.u8	q4,	d0,d14		\n\t"
		// sc in d8
		"vqrshrn.u16	d4,	q4, #8		\n\t"

		// sca in d9
		// XXX: I can probably squash one of these 3
		"vmvn.u32	d6,	d4		\n\t"
		"vshr.u32	d6,	d6, #24		\n\t"
		"vmul.u32	d6,     d6, d12 	\n\t"

		/* d * alpha */
		"vmull.u8	q4,	d6, d2 	 	\n\t"
		"vqrshrn.u16	d0,	q4, #8		\n\t"

		"vqadd.u8	d2,	d0, d4		\n\t"

		// Save dsc + sc
		"vstm		%[d]!,	{d2}		\n\t"

		"cmp 		%[tmp], %[d]		\n\t"
		"bhi 		"AP"dualloop2		\n\t"

		"cmp 		%[d], %[e]		\n\t"
                "beq 		"AP"done		\n\t"

	AP"singleloop2:					\n\t"
		"vld1.32	d0[0],  [%[s]]!		\n\t"
		"vld1.32	d2[0],	[%[d]]		\n\t"
		//  Mulitply s * c (= sc)
		"vmull.u8	q4,	d0,d14		\n\t"
		// sc in d8
		"vqrshrn.u16	d4,	q4, #8		\n\t"

		// sca in d6
		"vmvn.u32	d6,	d4		\n\t"
		"vshr.u32	d6,	d6, #24		\n\t"
		"vmul.u32	d6,     d12,d6	 	\n\t"

		/* d * alpha */
		"vmull.u8	q4,	d6, d2 	 	\n\t"
		"vqrshrn.u16	d0,	q4, #8		\n\t"

		"vqadd.u8	d2,	d0, d4		\n\t"

		// Save dsc + sc
		"vst1.32	d2[0],	[%[d]]!		\n\t"


	AP"done:"
		: // No output
		//
		: [s] "r" (s), [e] "r" (d + l), [d] "r" (d), [c] "r" (c),
			[tmp] "r" (12)
		: "q0","q1","q2","q3","q4","q5","q6","q7","memory"
	);
#undef AP
#endif
}

static void
_op_blend_pan_can_dp_neon(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
   DATA32 *e;
   UNROLL8_PLD_WHILE(d, l, e,
                     {
                        *d++ = 0xff000000 + MUL3_SYM(c, *s);
                        s++;
                     });
}

static void
_op_blend_pan_caa_dp_neon(DATA32 *s, DATA8 *m EINA_UNUSED, DATA32 c, DATA32 *d, int l) {
#if 1
   DATA32 *e;
   DATA32 sc;
   int alpha;
   c = 1 + (c & 0xff);
   UNROLL8_PLD_WHILE(d, l, e,
                    {
                       sc = MUL_256(c, *s);
                       alpha = 256 - (sc >> 24);
                       *d = sc + MUL_256(alpha, *d);
                       d++;
                       s++;
                    });
#else // the below neon is buggy!! misses rendering of spans, i think with alignment. quick - just disable this.
#define AP	"_op_blend_pan_caa_dp_"
   DATA32 *e = d + l, *tmp = (void*)73;
      asm volatile (
	".fpu neon					\n\t"
		/* Set up 'c' */
		"vdup.u8     d14, %[c]		\n\t"
		"vmov.i8     d15, #1		\n\t"
		"vaddl.u8   q15, d14, d15	\n\t"
		"vshr.u8	q15,#1		\n\t"

		// Pick a loop
		"andS		%[tmp], %[d], $0xf	\n\t"
		"beq		"AP"quadstart		\n\t"

		"andS		%[tmp], %[d], $0x4	\n\t"
		"beq		"AP"dualstart		\n\t"

	AP"singleloop:					\n\t"
		"vld1.32	d4[0],  [%[d]]		\n\t"
		"vld1.32	d0[0],  [%[s]]!		\n\t"

		// Long version of 'd'
		"vmovl.u8	q8, d4			\n\t"

		// Long version of 's'
		"vmovl.u8	q6, d0			\n\t"

		// d8 = s -d
		"vsub.s16	d8, d12, d16		\n\t"

		// Multiply
		"vmul.s16	d8, d8, d30		\n\t"

		// Shift down
		"vshr.s16	d8, #7			\n\t"

		// Add 'd'
		"vqadd.s16	d8, d8, d16		\n\t"

		// Shrink to save
		"vqmovun.s16	d0,  q4			\n\t"
		"vst1.32	d0[0], [%[d]]!		\n\t"

		// Now where?
		"andS		%[tmp], %[d], $0xf	\n\t"
		"beq		"AP"quadstart		\n\t"

	AP"dualstart:					\n\t"
		// Check we have enough
		"sub		%[tmp], %[e], %[d]	\n\t"
		"cmp		%[tmp], #16		\n\t"
		"blt		"AP"loopout		\n\t"

	AP"dualloop:"
		"vldm		%[d], 	{d4}		\n\t"
		"vldm		%[s]!,	{d0}		\n\t"

		// Long version of d
		"vmovl.u8	q8, d4		\n\t"

		// Long version of s
		"vmovl.u8	q6, d0		\n\t"

		// q4/q5 = s-d
		"vsub.s16 	q4, q6, q8 	\n\t"

		// Multiply
		"vmul.s16	q4,  q4,q15	\n\t"

		// Shift down
		"vshr.s16	q4, #7		\n\t"

		// Add d
		"vqadd.s16	q4, q4, q8	\n\t"

		// Shrink to save
		"vqmovun.s16	d0,  q4		\n\t"

		"vstm		%[d]!,	{d0}	\n\t"
	AP"quadstart:					\n\t"
		"sub		%[tmp], %[e], %[d]	\n\t"
		"cmp		%[tmp], #16		\n\t"
		"blt		"AP"loopout		\n\t"

		"sub		%[tmp], %[e], #15	\n\t"

	AP"quadloop:				\n\t"
		//  load 's' -> q0, 'd' -> q2
		"vldm	%[d],  {d4,d5}		\n\t"
		"vldm	%[s]!, {d0,d1}		\n\t"

		// Long version of d
		"vmovl.u8	q8, d4		\n\t"
		"vmovl.u8	q9, d5		\n\t"

		// Long version of s
		"vmovl.u8	q6, d0		\n\t"
		"vmovl.u8	q7, d1		\n\t"

		// q4/q5 = s-d
		"vsub.s16 	q4, q6, q8 	\n\t"
		"vsub.s16 	q5, q7, q9 	\n\t"

		// Multiply
		"vmul.s16	q4,  q4,q15	\n\t"
		"vmul.s16	q5,  q5,q15	\n\t"

		// Shift down
		"vshr.s16	q4, #7		\n\t"
		"vshr.s16	q5, #7		\n\t"

		// Add d
		"vqadd.s16	q4, q4, q8	\n\t"
		"vqadd.s16	q5, q5, q9	\n\t"

		// Shrink to save
		"vqmovun.s16	d0,  q4		\n\t"
		"vqmovun.s16	d1,  q5		\n\t"
		"vstm		%[d]!,	{d0,d1}	\n\t"
		"cmp		%[tmp], %[d] 		\n\t"

		"bhi "AP"quadloop\n\t"


		"b "AP"done\n\t"
	AP"loopout:					\n\t"
		"cmp 		%[d], %[e]		\n\t"
                "beq 		"AP"done\n\t"
		"sub		%[tmp],%[e], %[d]	\n\t"
		"cmp		%[tmp],$0x04		\n\t"
		"beq		"AP"singleloop2		\n\t"

	AP"dualloop2:					\n\t"
		"vldm		%[d], 	{d4}		\n\t"
		"vldm		%[s]!,	{d0}		\n\t"

		// Long version of d
		"vmovl.u8	q8, d4		\n\t"

		// Long version of s
		"vmovl.u8	q6, d0		\n\t"

		// q4/q5 = s-d
		"vsub.s16 	q4, q6, q8 	\n\t"

		// Multiply
		"vmul.s16	q4,  q4,q15	\n\t"

		// Shift down
		"vshr.s16	q4, #7		\n\t"

		// Add d
		"vqadd.s16	q4, q4, q8	\n\t"

		// Shrink to save
		"vqmovun.s16	d0,  q4		\n\t"

		"vstm		%[d]!,	{d0}	\n\t"

		"cmp 		%[d], %[e]		\n\t"
                "beq 		"AP"done		\n\t"

	AP"singleloop2:					\n\t"
		"vld1.32	d4[0],  [%[d]]		\n\t"
		"vld1.32	d0[0],  [%[s]]!		\n\t"

		// Long version of 'd'
		"vmovl.u8	q8, d4			\n\t"

		// Long version of 's'
		"vmovl.u8	q6, d0			\n\t"

		// d8 = s -d
		"vsub.s16	d8, d12, d16		\n\t"

		// Multiply
		"vmul.s16	d8, d8, d30		\n\t"

		// Shift down
		"vshr.s16	d8, #7			\n\t"

		// Add 'd'
		"vqadd.s16	d8, d8, d16		\n\t"

		// Shrink to save
		"vqmovun.s16	d0,  q4			\n\t"

		"vst1.32	d0[0], [%[d]]		\n\t"


	AP"done:					\n\t"

	// No output
	:
	// Input
	: [s] "r" (s), [d] "r" (d), [e] "r" (e), [c] "r" (c), [tmp] "r" (tmp)
	// Clobbered
	: "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "memory"
      );
#undef AP
#endif   
}

#define _op_blend_pas_c_dp_neon _op_blend_p_c_dp_neon
#define _op_blend_pan_c_dp_neon _op_blend_p_c_dp_neon
#define _op_blend_p_can_dp_neon _op_blend_p_c_dp_neon
#define _op_blend_pas_can_dp_neon _op_blend_p_c_dp_neon
#define _op_blend_p_caa_dp_neon _op_blend_p_c_dp_neon
#define _op_blend_pas_caa_dp_neon _op_blend_p_c_dp_neon

#define _op_blend_p_c_dpan_neon _op_blend_p_c_dp_neon
#define _op_blend_pas_c_dpan_neon _op_blend_pas_c_dp_neon
#define _op_blend_pan_c_dpan_neon _op_blend_pan_c_dp_neon
#define _op_blend_p_can_dpan_neon _op_blend_p_can_dp_neon
#define _op_blend_pas_can_dpan_neon _op_blend_pas_can_dp_neon
#define _op_blend_pan_can_dpan_neon _op_blend_pan_can_dp_neon
#define _op_blend_p_caa_dpan_neon _op_blend_p_caa_dp_neon
#define _op_blend_pas_caa_dpan_neon _op_blend_pas_caa_dp_neon
#define _op_blend_pan_caa_dpan_neon _op_blend_pan_caa_dp_neon


static void
init_blend_pixel_color_span_funcs_neon(void)
{
   op_blend_span_funcs[SP][SM_N][SC][DP][CPU_NEON] = _op_blend_p_c_dp_neon;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP][CPU_NEON] = _op_blend_pas_c_dp_neon;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP][CPU_NEON] = _op_blend_pan_c_dp_neon;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP][CPU_NEON] = _op_blend_p_can_dp_neon;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP][CPU_NEON] = _op_blend_pas_can_dp_neon;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP][CPU_NEON] = _op_blend_pan_can_dp_neon;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP][CPU_NEON] = _op_blend_p_caa_dp_neon;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP][CPU_NEON] = _op_blend_pas_caa_dp_neon;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP][CPU_NEON] = _op_blend_pan_caa_dp_neon;

   op_blend_span_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_p_c_dpan_neon;
   op_blend_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_pas_c_dpan_neon;
   op_blend_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_pan_c_dpan_neon;
   op_blend_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_p_can_dpan_neon;
   op_blend_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_pas_can_dpan_neon;
   op_blend_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_pan_can_dpan_neon;
   op_blend_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_p_caa_dpan_neon;
   op_blend_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_pas_caa_dpan_neon;
   op_blend_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_pan_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON
static void
_op_blend_pt_p_c_dp_neon(DATA32 s, DATA8 m EINA_UNUSED, DATA32 c, DATA32 *d) {
   s = MUL4_SYM(c, s);
   c = 256 - (s >> 24);
   *d = s + MUL_256(c, *d);
}

#define _op_blend_pt_pas_c_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pan_c_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_p_can_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pas_can_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pan_can_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_p_caa_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pas_caa_dp_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pan_caa_dp_neon _op_blend_pt_p_c_dp_neon

#define _op_blend_pt_p_c_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pas_c_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pan_c_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_p_can_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pas_can_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pan_can_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_p_caa_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pas_caa_dpan_neon _op_blend_pt_p_c_dp_neon
#define _op_blend_pt_pan_caa_dpan_neon _op_blend_pt_p_c_dp_neon

static void
init_blend_pixel_color_pt_funcs_neon(void)
{
   op_blend_pt_funcs[SP][SM_N][SC][DP][CPU_NEON] = _op_blend_pt_p_c_dp_neon;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP][CPU_NEON] = _op_blend_pt_pas_c_dp_neon;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP][CPU_NEON] = _op_blend_pt_pan_c_dp_neon;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP][CPU_NEON] = _op_blend_pt_p_can_dp_neon;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP][CPU_NEON] = _op_blend_pt_pas_can_dp_neon;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP][CPU_NEON] = _op_blend_pt_pan_can_dp_neon;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP][CPU_NEON] = _op_blend_pt_p_caa_dp_neon;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP][CPU_NEON] = _op_blend_pt_pas_caa_dp_neon;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP][CPU_NEON] = _op_blend_pt_pan_caa_dp_neon;

   op_blend_pt_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_pt_p_c_dpan_neon;
   op_blend_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_pt_pas_c_dpan_neon;
   op_blend_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_pt_pan_c_dpan_neon;
   op_blend_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_pt_p_can_dpan_neon;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_pt_pas_can_dpan_neon;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_pt_pan_can_dpan_neon;
   op_blend_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_pt_p_caa_dpan_neon;
   op_blend_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_pt_pas_caa_dpan_neon;
   op_blend_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_pt_pan_caa_dpan_neon;
}
#endif

/*-----*/

/* blend_rel pixel x color -> dst */

#ifdef BUILD_NEON

#define _op_blend_rel_p_c_dpan_neon _op_blend_p_c_dpan_neon
#define _op_blend_rel_pas_c_dpan_neon _op_blend_pas_c_dpan_neon
#define _op_blend_rel_pan_c_dpan_neon _op_blend_pan_c_dpan_neon
#define _op_blend_rel_p_can_dpan_neon _op_blend_p_can_dpan_neon
#define _op_blend_rel_pas_can_dpan_neon _op_blend_pas_can_dpan_neon
#define _op_blend_rel_pan_can_dpan_neon _op_blend_pan_can_dpan_neon
#define _op_blend_rel_p_caa_dpan_neon _op_blend_p_caa_dpan_neon
#define _op_blend_rel_pas_caa_dpan_neon _op_blend_pas_caa_dpan_neon
#define _op_blend_rel_pan_caa_dpan_neon _op_blend_pan_caa_dpan_neon

static void
init_blend_rel_pixel_color_span_funcs_neon(void)
{
   op_blend_rel_span_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_rel_p_c_dpan_neon;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_rel_pas_c_dpan_neon;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_rel_pan_c_dpan_neon;
   op_blend_rel_span_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_rel_p_can_dpan_neon;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_rel_pas_can_dpan_neon;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_rel_pan_can_dpan_neon;
   op_blend_rel_span_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_rel_p_caa_dpan_neon;
   op_blend_rel_span_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_rel_pas_caa_dpan_neon;
   op_blend_rel_span_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_rel_pan_caa_dpan_neon;
}
#endif

#ifdef BUILD_NEON

#define _op_blend_rel_pt_p_c_dpan_neon _op_blend_pt_p_c_dpan_neon
#define _op_blend_rel_pt_pas_c_dpan_neon _op_blend_pt_pas_c_dpan_neon
#define _op_blend_rel_pt_pan_c_dpan_neon _op_blend_pt_pan_c_dpan_neon
#define _op_blend_rel_pt_p_can_dpan_neon _op_blend_pt_p_can_dpan_neon
#define _op_blend_rel_pt_pas_can_dpan_neon _op_blend_pt_pas_can_dpan_neon
#define _op_blend_rel_pt_pan_can_dpan_neon _op_blend_pt_pan_can_dpan_neon
#define _op_blend_rel_pt_p_caa_dpan_neon _op_blend_pt_p_caa_dpan_neon
#define _op_blend_rel_pt_pas_caa_dpan_neon _op_blend_pt_pas_caa_dpan_neon
#define _op_blend_rel_pt_pan_caa_dpan_neon _op_blend_pt_pan_caa_dpan_neon

static void
init_blend_rel_pixel_color_pt_funcs_neon(void)
{
   op_blend_rel_pt_funcs[SP][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_rel_pt_p_c_dpan_neon;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_rel_pt_pas_c_dpan_neon;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC][DP_AN][CPU_NEON] = _op_blend_rel_pt_pan_c_dpan_neon;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_rel_pt_p_can_dpan_neon;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_rel_pt_pas_can_dpan_neon;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AN][DP_AN][CPU_NEON] = _op_blend_rel_pt_pan_can_dpan_neon;
   op_blend_rel_pt_funcs[SP][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_rel_pt_p_caa_dpan_neon;
   op_blend_rel_pt_funcs[SP_AS][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_rel_pt_pas_caa_dpan_neon;
   op_blend_rel_pt_funcs[SP_AN][SM_N][SC_AA][DP_AN][CPU_NEON] = _op_blend_rel_pt_pan_caa_dpan_neon;
}
#endif
