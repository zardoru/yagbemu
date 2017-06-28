
#include "types.h"
#include "Z80.h"

namespace GBEmu {

	// defines operation x with pr as the processor
	// return value: machine time used
#define OP(x) byte x (Z80*pr)

	// 8 bit carry/halfcarry check
	byte proc_add(Z80* pr, byte l, byte r) {
		word rs = (word)l + (word)r;

		if (rs & 0x0F00)
			pr->setflag(Z80::cf);
		else
			pr->resetflag(Z80::cf);

		if (((l & 0xF) + (r & 0xF)) & 0x10)
			pr->setflag(Z80::hcf);
		else
			pr->resetflag(Z80::hcf);

		return rs & 0xFF;
	}

	// 16 bit carry/half carry check
	word proc_add(Z80 *pr, word l, word r)
	{
		int32_t rs = (int32_t)l + (int32_t)r;

		if (rs > 0xFFFF)
			pr->setflag(Z80::cf);
		else
			pr->resetflag(Z80::cf);

		if (((l & 0x0F00) + (r & 0x0F00)) & 0x1000)
			pr->setflag(Z80::hcf);
		else
			pr->resetflag(Z80::hcf);

		return rs & 0xFFFF;
	}

	// 8 bit Half Carry check, 16 bit Carry check
	word proc_add(Z80 *pr, byte l, word r)
	{
		int32_t rs = (int32_t)l + (int32_t)r;

		if (rs > 0xFFFF)
			pr->setflag(Z80::cf);
		else
			pr->resetflag(Z80::cf);

		if (((l & 0xF) + (r & 0xF)) & 0x10)
			pr->setflag(Z80::hcf);
		else
			pr->resetflag(Z80::hcf);

		return rs & 0xFFFF;
	}

	OP(NOP) {
		return 4;
	}

#define forallregs(op) op(A)\
	op(B)\
	op(C)\
	op(D)\
	op(E)\
	op(H)\
	op(L)

#define forallregsexceptA(op) op(B)\
	op(C)\
	op(D)\
	op(E)\
	op(H)\
	op(L)

#define fetchtoreg(reg) OP(LD_ ##reg ##_n) { pr-> ##reg = pr->fetchb(); return 8; }

	forallregs(fetchtoreg)

		// define LD dst,src
#define ldop(src,dst) OP(LD_ ##dst ##_ ##src) { pr->##dst = pr->##src; return 4; }
#define lds(dst) ldop(A,dst)\
	ldop(B, dst)\
	ldop(C, dst)\
	ldop(D, dst)\
	ldop(E, dst)\
	ldop(H, dst)\
	ldop(L, dst)

		forallregs(lds)

		// load into (HL) operations
#define hlld(src) OP(LD_HL_##src) {pr->setvalueatHL(pr->##src); return 8; }
		forallregs(hlld)

		// load from (HL) operations
#define ldhl(dst) OP(LD_ ##dst ##_HL) {pr->##dst = pr->getvaluepointedbyHL(); return 8;}
		forallregs(ldhl)

	// 16 regs as pointers load
	OP(LD_DE_A) {
		pr->mmu.writeb(pr->getDE(), pr->a);
		return 8;
	}

	OP(LD_BC_A) {
		pr->mmu.writeb(pr->getBC(), pr->a);
		return 8;
	}

	OP(LD_HL_n) {
		pr->setvalueatHL(pr->fetchb());
		return 8;
	}

	OP(LD_nn_A) {
		pr->mmu.writeb(pr->fetchw(), pr->a);
		return 16;
	}

	OP(LD_A_BC) {
		pr->a = pr->mmu.readb(pr->getBC()); return 8;
	}

	OP(LD_A_DE)	{
		pr->a = pr->mmu.readb(pr->getDE()); return 8;
	}

	OP(LD_A_nn) {
		pr->a = pr->mmu.readb(pr->fetchw());
		return 16;
	}

	// 0xFF00 + reg/byte write/read ops
	OP(LD_A_PC) {
		pr->a = pr->mmu.readb(packWord(0xFF, pr->c));
		return 8;
	}

	OP(LD_PC_A) {
		pr->mmu.writeb(packWord(0xFF, pr->c), pr->a);
		return 8;
	}

	OP(LD_Pn_A) {
		pr->mmu.writeb(packWord(0xFF, pr->fetchb()), pr->a);
		return 12;
	}

	OP(LD_A_Pn) {
		pr->a = pr->mmu.readb(packWord(0xFF, pr->fetchb()));
		return 12;
	}

template<byte Z80::*ptr>
OP(SWAP) {
	byte val = (*pr).*ptr;
	byte nibbleL = (val & 0x0F) << 4;
	byte nibbleR = (val & 0xF0) >> 4;
	(*pr).*ptr = nibbleR | nibbleL;
	return 8;
}

	// ALU
#define addR(src,dst) OP(ADD_ ##dst ##_ ##src) { \
		pr->##dst = proc_add(pr, pr->##src, pr->##dst);\
		pr->zfset (pr->##dst);\
		pr->resetflag(Z80::opf); return 4;\
			}
#define addR_A(src) addR(src,A)

	forallregs(addR_A)

		OP(ADD_A_HL) {
		pr->a = proc_add(pr, pr->a, pr->getvaluepointedbyHL());
		pr->zfset(pr->a);
			pr->resetflag(Z80::opf);
		return 8;
	}

	OP(ADD_A_n) {
		pr->a = proc_add(pr, pr->a, pr->fetchb());
		pr->zfset(pr->a);
		pr->resetflag(Z80::opf);
		return 8;
	}

#define adcR(src) OP(ADC_A_##src) { \
		pr->a = proc_add(pr, pr->##src + (pr->isflagset(Z80::cf) ? 1 : 0), pr->a);\
		pr->zfset (pr->a);\
		pr->resetflag(Z80::opf); return 4;\
			}

	forallregs(adcR)

	OP(ADC_A_HL) {
		pr->a = proc_add(pr, pr->a + (pr->isflagset(Z80::cf) ? 1 : 0), pr->getvaluepointedbyHL());
		pr->zfset(pr->a);
		pr->resetflag(Z80::opf);
		return 8;
	}

	OP(ADC_A_n) {
		pr->a = proc_add(pr, (pr->isflagset(Z80::cf) ? 1 : 0), pr->a);
		pr->a = proc_add(pr, pr->a, pr->fetchb());
		pr->zfset(pr->a);
		pr->resetflag(Z80::opf);
		return 8;
	}

	// sub by add using 2s complement
#define sub(src) OP(SUB_##src) { \
		byte oa = pr->a;\
		pr->a = proc_add(pr, pr->a, (byte)(~pr->##src+1)); \
		if ((signed char)pr->##src > (signed char)oa) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);\
		pr->setflag(Z80::opf);\
		pr->zfset (pr->a); return 4;\
			}

	forallregs(sub)

	OP(SUB_HL) {
		pr->setflag(Z80::opf);
		pr->a = ~proc_add(pr, pr->a, (byte)(~pr->getvaluepointedbyHL() + 1)) + 1;
		if ((signed char)pr->getvaluepointedbyHL() > (signed char)pr->a) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		pr->zfset(pr->a); return 8;
	}

	OP(SUB_n) {
		byte inp = pr->fetchb();
		pr->setflag(Z80::opf);
		pr->a = ~proc_add(pr, pr->a, (byte)(~inp + 1)) + 1;
		if (inp > pr->a) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf); \
			pr->zfset(pr->a); return 8;
	}

#define and(src) OP(AND_##src) {\
		pr->a &= pr->##src;\
		pr->zfset(pr->a);\
		pr->setflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);\
	return 4; }

	forallregs(and)

	OP(AND_HL) {
		pr->a &= pr->getvaluepointedbyHL();
		pr->zfset(pr->a);
		pr->setflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);
		return 8;
	}

	OP(AND_n) {
		pr->a &= pr->fetchb();
		pr->zfset(pr->a);
		pr->setflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);
		return 8;
	}

#define or(src) OP(OR_##src) {\
		pr->a |= pr->##src;\
		pr->zfset(pr->a);\
		pr->resetflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);\
		return 4;	}

	forallregs(or)

	OP(OR_HL) {
		pr->a |= pr->getvaluepointedbyHL();
		pr->zfset(pr->a);
		pr->resetflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);
		return 8;
	}

	OP(OR_n) {
		pr->a |= pr->fetchb();
		pr->zfset(pr->a);
		pr->resetflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);
		return 8;
	}


#define xor(src) OP(XOR_##src) {\
		pr->a ^= pr->##src;\
		pr->zfset(pr->a);\
		pr->resetflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);\
		return 4;	}

	forallregs(xor)

	OP(XOR_HL) {
		pr->a ^= pr->getvaluepointedbyHL();
		pr->zfset(pr->a);
		pr->resetflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);
		return 8;
	}

	OP(XOR_n) {
		pr->a ^= pr->fetchb();
		pr->zfset(pr->a);
		pr->resetflag(Z80::hcf); pr->resetflag(Z80::opf); pr->resetflag(Z80::cf);
		return 8;
	}

	// cp doesn't know whether the input is signed or unsigned, positive or negative, so just assume everything's unsigned all the time.
	// Same with SUB n.
#define cp(src) OP(CP_##src) {\
			word tmp = (~proc_add(pr, pr->a, (byte)(~pr->##src+1))+1) & 0xFF;\
			pr->zfset(tmp);\
			if (pr->##src > pr->a) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);\
			return 4;	}

	forallregs(cp)

	OP(CP_HL) {
		word tmp = (~proc_add(pr, pr->a, (byte)(~pr->getvaluepointedbyHL() + 1)) + 1) & 0xFF;
		pr->zfset(tmp);
		if (pr->getvaluepointedbyHL() > pr->a) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		return 8;
	}

	OP(CP_n) {
		byte inp = pr->fetchb();
		word tmp = (~proc_add(pr, pr->a, (byte)(~inp + 1)) + 1) & 0xFF;
		pr->zfset(tmp);
		if (inp > pr->a) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		return 8;
	}

#define inc(src) OP(INC_##src) {\
			byte carry = pr->isflagset(Z80::cf);\
			pr->##src = proc_add(pr, 1, pr->##src);\
			pr->zfset(pr->##src);\
			pr->resetflag(Z80::opf);\
			if (carry) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);\
			return 4;	}

	forallregs(inc);

	OP(INC_PHL) {
		pr->setHL(pr->getvaluepointedbyHL() + 1);
		pr->zfset(pr->getHL());
		pr->resetflag(Z80::opf);
		return 12;
	}

#define dec(src) OP(DEC_##src) {\
			byte carry = pr->isflagset(Z80::cf);\
			pr->##src = proc_add(pr, (byte)~1+1, pr->##src);\
			pr->zfset(pr->##src);\
			pr->resetflag(Z80::opf);\
			if (carry) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);\
			return 4;		}

	forallregs(dec);

	OP(DEC_PHL) {
		pr->setHL(pr->getHL() - 1);
		pr->zfset(pr->getHL());
		pr->resetflag(Z80::opf);
		return 12;
	}

#define forall16bitregs(op) op(AF)\
	op(BC)\
	op(DE)\
	op(HL)\
	op(SP)

#define wordinto16reg(dst) OP(LD_ ##dst ##_nn) { \
		pr->set ##dst (pr->fetchw()); \
		return 12; \
			}

	forall16bitregs(wordinto16reg)


#define add16(src,dst) OP(ADD_ ##dst ##_ ##src) {\
			pr->set##dst (proc_add(pr, pr->get##src(), pr->get##dst()));\
			pr->resetflag(Z80::opf);\
				return 8;		}

#define add16HL(src) add16(src,HL)
#define add16SP(src) add16(src,SP)

		forall16bitregs(add16HL)
		forall16bitregs(add16SP)

		// no flags affected yesss
#define inc16(src) OP(INC_##src) {\
		pr->set##src(pr->get##src() + 1);\
			return 8;	}

		forall16bitregs(inc16);

#define dec16(src) OP(DEC_##src) {\
		pr->set##src(pr->get##src() - 1);\
			return 4; }

	forall16bitregs(dec16);

#define push16(src) OP(PUSH_##src) { \
		pr->sp -= 2; pr->mmu.writew(pr->sp, pr->get##src()); return 16;\
				}

	forall16bitregs(push16)

#define pop16(dst) OP(POP_##dst) { \
		pr->set##dst (pr->mmu.readw(pr->sp)); pr->sp += 2; return 12;\
			}

		forall16bitregs(pop16)


	OP(ADD_SP_n) {
		pr->sp = proc_add(pr, pr->fetchb(), pr->sp);
		pr->resetflag(Z80::zf);
		pr->resetflag(Z80::opf);
		return 16;
	}

	OP(LD_HL_SPn) {
		pr->resetflag(Z80::zf);
		pr->resetflag(Z80::opf);

		word v = proc_add(pr, pr->fetchb(), pr->sp);
		pr->setHL(v);
		return 12;
	}

	OP(LD_nn_SP) {
		pr->mmu.writew(pr->fetchw(), pr->sp);
		return 20;
	}

	// LDD and LDI
	OP(LDD_A_HL) {
		LD_A_HL(pr);
		DEC_HL(pr); return 8;
	}

	OP(LDD_HL_A) {
		LD_HL_A(pr);
		DEC_HL(pr); return 8;
	}

	OP(LDI_A_HL) {
		LD_A_HL(pr);
		INC_HL(pr); return 8;
	}

	OP(LDI_HL_A) {
		LD_HL_A(pr);
		INC_HL(pr); return 8;
	}

	OP(CPL) {
		pr->a = ~pr->a;
		pr->setflag(Z80::opf);
		pr->setflag(Z80::hcf); return 4;
	}

	OP(CCF) {
		if (pr->isflagset(Z80::cf))
			pr->resetflag(Z80::cf);
		else
			pr->setflag(Z80::cf);

		pr->resetflag(Z80::opf);
		pr->resetflag(Z80::hcf);
		return 4;
	}

	OP(SCF) {
		pr->setflag(Z80::cf);
		pr->resetflag(Z80::opf);
		pr->resetflag(Z80::hcf); return 4;
	}

	OP(HALT) {
		pr->halted = true;
		return 4;
		// disabled interrupts = skip next instruction. we'll go with the gbc behaviour.
		// do nothing else.
		// if (!pr->interrupts)
	}

	OP(STOP) {
		pr->stopped = true;
		pr->fetchb(); return 4;
	}

	OP(EI) {
		pr->interrupts = true; return 4;
	}

	OP(DI) {
		pr->interrupts = false; return 4;
	}

	OP(LD_SP_HL)
	{
		pr->setSP(pr->getHL()); return 8;
	}

	OP(opTableB) {
		byte opc2 = pr->fetchb();
		word op = 0xCB00 | opc2;
		return pr->runopcode(op);
	}

	OP(CALL) {
		word jmpaddr = pr->fetchw();
		word pushaddr = pr->pc; // call + return address
		pr->sp -= 2; pr->mmu.writew(pr->sp, pushaddr);
		pr->pc = jmpaddr; // holy cow, subroutines
		return 12;
	}

	OP(CALLNZ) {
		if (!pr->isflagset(Z80::zf))
			CALL(pr);
		else pr->fetchw(); return 12;
	}

	OP(CALLZ) {
		if (pr->isflagset(Z80::zf))
			CALL(pr);
		else pr->fetchw(); return 12;
	}

	OP(CALLNC) {
		if (!pr->isflagset(Z80::cf))
			CALL(pr);
		else pr->fetchw(); return 12;
	}

	OP(CALLC) {
		if (pr->isflagset(Z80::cf))
			CALL(pr);
		else pr->fetchw(); return 12;
	}

	void RSTgen(GBEmu::Z80 *pr, word jmpaddr) {
		pr->mmu.writew(pr->sp, pr->pc - 1); pr->sp -= 2;
		pr->pc = jmpaddr;
	}

	OP(RST00) {
		RSTgen(pr, 0x00); return 32;
	}

	OP(RST08) {
		RSTgen(pr, 0x08); return 32;
	}

	OP(RST10) {
		RSTgen(pr, 0x10); return 32;
	}

	OP(RST18) {
		RSTgen(pr, 0x18); return 32;
	}

	OP(RST20) {
		RSTgen(pr, 0x20); return 32;
	}

	OP(RST28) {
		RSTgen(pr, 0x28); return 32;
	}

	OP(RST30) {
		RSTgen(pr, 0x30); return 32;
	}

	OP(RST38) {
		RSTgen(pr, 0x38); return 32;
	}

	OP(RET) {
		word jmpaddr = pr->mmu.readw(pr->sp);
		pr->sp += 2;
		pr->pc = jmpaddr; // WOW we're BACK to the previous routine. So cool!
		return 8;
	}

	OP(RETI) {
		RET(pr);
		EI(pr); return 8;
	}

	OP(RETNZ) {
		if (!pr->isflagset(Z80::zf))
			RET(pr);
		else pr->fetchw(); return 8;
	}

	OP(RETZ) {
		if (pr->isflagset(Z80::zf))
			RET(pr); else pr->fetchw(); return 8;
	}

	OP(RETNC) {
		if (!pr->isflagset(Z80::cf))
			RET(pr); else pr->fetchw(); return 8;
	}

	OP(RETC) {
		if (pr->isflagset(Z80::cf))
			RET(pr); else pr->fetchw(); return 8;
	}

	OP(JP) {
		word jmpaddr = pr->fetchw();
		pr->pc = jmpaddr; return 12;
	}

	OP(JPNZ) {
		if (!pr->isflagset(Z80::zf))
			JP(pr);
		else pr->fetchw(); return 12;
	}
	OP(JPZ) {
		if (pr->isflagset(Z80::zf))
			JP(pr);
		else pr->fetchw(); return 12;
	}
	OP(JPNC) {
		if (!pr->isflagset(Z80::cf))
			JP(pr);
		else pr->fetchw(); return 12;
	}
	OP(JPC) {
		if (pr->isflagset(Z80::cf))
			JP(pr);
		else pr->fetchw(); return 12;
	}

	OP(JP_HL) {
		word jmpaddr = pr->getvaluepointedbyHL();
		pr->pc = jmpaddr; return 4;
	}

	OP(JR) {
		signed char n = pr->fetchb();
		pr->pc += n; return 8;
	}

	OP(JRZ) {
		if (pr->isflagset(Z80::zf))
			JR(pr);
		else pr->fetchb(); return 8;
	}

	OP(JRNZ) {
		if (!pr->isflagset(Z80::zf))
			JR(pr);
		else pr->fetchb(); return 8;
	}

	OP(JRC) {
		if (pr->isflagset(Z80::cf))
			JR(pr);
		else pr->fetchb(); return 8;
	}

	OP(JRNC) {
		if (!pr->isflagset(Z80::cf))
			JR(pr);
		else pr->fetchb(); return 8;
	}


	// rotate v left.
	void _RLC(byte& v, Z80* pr)
	{
		byte newcarry = (v & 0x80) >> 7;
		v <<= 1;
		v |= newcarry;
		if (newcarry) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		pr->zfset(v);
		pr->resetflag(Z80::hcf);
	}

	// rotate v left through carry. return: new carry
	void _RL(byte& v, Z80* pr)
	{
		byte newcarry = (v & 0x80) >> 7;
		byte carry = pr->isflagset(Z80::cf);
		v <<= 1;
		v |= carry;
		if (newcarry) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		pr->zfset(v);
		pr->resetflag(Z80::hcf);
	}

	// rotate v right.
	void _RRC(byte& v, Z80* pr)
	{
		byte carry = (v & 0x01) << 7;
		v >>= 1;
		v |= carry;
		if (carry) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		pr->zfset(v);
		pr->resetflag(Z80::hcf);
	}

	// rotate right through carry. return: new carry
	void _RR(byte& v, Z80* pr)
	{
		byte newcarry = (v & 0x01);
		byte carry = pr->isflagset(Z80::cf);
		v >>= 1;
		v |= carry;
		if (newcarry) pr->setflag(Z80::cf); else pr->resetflag(Z80::cf);
		pr->zfset(v);
		pr->resetflag(Z80::hcf);
	}

#define rotops_reg(reg) OP(xRLC##reg) {_RLC(pr->##reg, pr); return 8;} \
	OP(xRRC##reg) {_RRC(pr->##reg, pr); return 8;} \
	OP(xRL##reg) {_RL(pr->##reg, pr); return 8;} \
	OP(xRR##reg) {_RR(pr->##reg, pr); return 8;}

	forallregs(rotops_reg)

#undef rotops_reg
#define rotops_reg(reg) OP(RLC##reg) {_RLC(pr->##reg, pr); return 4;} \
	OP(RRC##reg) {_RRC(pr->##reg, pr); return 4;} \
	OP(RL##reg) {_RL(pr->##reg, pr); return 4;} \
	OP(RR##reg) {_RR(pr->##reg, pr); return 4;}

	rotops_reg(A)

	OP(xRLCHL){
		byte w = pr->getvaluepointedbyHL();
		_RLC(w, pr);
		pr->mmu.writeb(pr->getHL(), w); return 16;
	}

	OP(RRCHL){
		byte w = pr->getvaluepointedbyHL();
		_RRC(w, pr);
		pr->mmu.writeb(pr->getHL(), w); return 16;
	}

	OP(RLHL){
		byte w = pr->getvaluepointedbyHL();
		_RL(w, pr);
		pr->mmu.writeb(pr->getHL(), w); return 16;
	}

	OP(RRHL){
		byte w = pr->getvaluepointedbyHL();
		_RR(w, pr);
		pr->mmu.writeb(pr->getHL(), w); return 16;
	}

OP(ILLOP) {
	throw std::runtime_error("illegal cpu operation");
}

/*
	When rotating through carry, the nth bit of the shifted n bits will be equal to the carry.
	Similarily, the carry is set to the nth bit of the shifted n bits.
*/

void testBit(byte v, byte bit, Z80* pr)
{
	byte isbitset = v & (1 << bit);
	if (!isbitset) pr->setflag(Z80::zf); // set zf if 0
	else pr->resetflag(Z80::zf);
	pr->resetflag(Z80::opf);
	pr->setflag(Z80::hcf);
}

#define bitop(n,dst) OP(BIT_ ##n ##_ ##dst) {testBit(pr->##dst, n, pr); return 8;}
#define bop(dst) bitop(0,dst) bitop(1,dst) bitop(2,dst) bitop(3,dst) bitop(4,dst) bitop(5,dst) bitop(6,dst) bitop(7,dst)
forallregs(bop)

template<int N>
OP(BIT_N_HL) {
	testBit(pr->getvaluepointedbyHL(), N, pr); return 16;
}

OP(DAA) {
	auto bcd = pr->A;
	unsigned char b1 = bcd >> 4; // high nibble
	unsigned char b2 = bcd & 0xF; // low nibble

	unsigned char adjustedHigh = (b1 + b2 / 10) % 10;
	unsigned char adjustedLow = b2 % 10;

	pr->A = (adjustedHigh << 4) | adjustedLow;
	return 4;
}

Z80::z80op ops[256] =
{
	// 0x00
	{ NOP, "nop" },
	{ LD_BC_nn, "LD BC, w" },
	{ LD_BC_A, "LD (BC), A" },
	{ INC_BC, "INC BC" },
	{ INC_B, "INC B" },
	{ DEC_B, "DEC B" },
	{ LD_B_n, "LD B, n" },
	{ RLCA, "RLCA" },
	{ LD_nn_SP, "LD (nn), SP" },
	{ ADD_HL_BC, "ADD HL, BC" },
	{ LD_A_BC, "LD A, (BC)" },
	{ DEC_BC, "DEC BC" },
	{ INC_C, "INC C" },
	{ DEC_C, "DEC C" },
	{ LD_C_n, "LD C, n" },
	{ RRCA, "RRCA" },

	// 0x10
	{ STOP, "STOP" },
	{ LD_DE_nn, "LD DE, w" },
	{ LD_DE_A, "LD (DE), A" },
	{ INC_DE, "INC DE" },
	{ INC_D, "INC D" },
	{ DEC_D, "DEC D" },
	{ LD_D_n, "LD D, n" },
	{ RLA, "RLA" },
	{ JR, "JR n" },
	{ ADD_HL_DE, "ADD HL, DE" },
	{ LD_A_DE, "LD A, (DE)" },
	{ DEC_DE, "DEC DE" },
	{ INC_E, "INC E" },
	{ DEC_E, "DEC E" },
	{ LD_E_n, "LD E, n" },
	{ RRA, "RRA" },

	// 0x20
	{ JRNZ, "JR NZ n" },
	{ LD_HL_nn, "LD HL, w" },
	{ LDI_HL_A, "LDI (HL), A" },
	{ INC_HL, "INC HL" },
	{ INC_H, "INC H" },
	{ DEC_H, "DEC H" },
	{ LD_H_n, "LD H, n" },
	{ DAA, "DAA" },
	{ JRZ, "JR Z n" },
	{ ADD_HL_HL, "ADD HL, HL" },
	{ LDI_A_HL, "LDI A, (HL)" },
	{ DEC_HL, "DEC_HL" },
	{ INC_L, "INC L" },
	{ DEC_L, "DEC L" },
	{ LD_L_n, "LD L, n" },
	{ CPL, "CPL" },

	// 0x30
	{ JRNC, "JRNC" },
	{ LD_SP_nn, "LD SP, w" },
	{ LDD_HL_A, "LDD (HL), A" },
	{ INC_SP, "INC SP" },
	{ INC_PHL, "INC (HL)" },
	{ DEC_PHL, "DEC (HL)" },
	{ LD_HL_n, "LD (HL), n" },
	{ SCF, "SCF" },
	{ JRC, "JRC" },
	{ ADD_HL_HL, "ADD HL, HL" },
	{ LDD_A_HL, "LDD A, (HL)" },
	{ DEC_SP, "DEC SP" },
	{ INC_A, "INC A" },
	{ DEC_A, "DEC A" },
	{ LD_A_n, "LD A, n" },
	{ CCF, "CCF" },

	// 0x40
	{ LD_B_B, "LD B, B" },
	{ LD_B_C, "LD B, C" },
	{ LD_B_D, "LD B, D" },
	{ LD_B_E, "LD B, E" },
	{ LD_B_H, "LD B, H" },
	{ LD_B_L, "LD B, L" },
	{ LD_B_HL, "LD B, (HL)" },
	{ LD_B_A, "LD B, A" },
	{ LD_C_B, "LD C, B" },
	{ LD_C_C, "LD C, C" },
	{ LD_C_D, "LD C, D" },
	{ LD_C_E, "LD C, E" },
	{ LD_C_H, "LD C, H" },
	{ LD_C_L, "LD C, L" },
	{ LD_C_HL, "LD C, (HL)" },
	{ LD_C_A, "LD C, A" },

	//0x50
	{ LD_D_B, "LD D, B" },
	{ LD_D_C, "LD D, C" },
	{ LD_D_D, "LD D, D" },
	{ LD_D_E, "LD D, E" },
	{ LD_D_H, "LD D, H" },
	{ LD_D_L, "LD D, L" },
	{ LD_D_HL, "LD D, (HL)" },
	{ LD_D_A, "LD D, A" },
	{ LD_E_B, "LD E, B" },
	{ LD_E_C, "LD E, C" },
	{ LD_E_D, "LD E, D" },
	{ LD_E_E, "LD E, E" },
	{ LD_E_H, "LD E, H" },
	{ LD_E_L, "LD E, L" },
	{ LD_E_HL, "LD E, (HL)" },
	{ LD_E_A, "LD E, A" },

	// 0x60
	{ LD_H_B, "LD H, B" },
	{ LD_H_C, "LD H, C" },
	{ LD_H_D, "LD H, D" },
	{ LD_H_E, "LD H, E" },
	{ LD_H_H, "LD H, H" },
	{ LD_H_L, "LD H, L" },
	{ LD_H_HL, "LD H, (HL)" },
	{ LD_H_A, "LD H, A" },
	{ LD_L_B, "LD L, B" },
	{ LD_L_C, "LD L, C" },
	{ LD_L_D, "LD L, D" },
	{ LD_L_E, "LD L, E" },
	{ LD_L_H, "LD L, H" },
	{ LD_L_L, "LD L, L" },
	{ LD_L_HL, "LD L, (HL)" },
	{ LD_L_A, "LD L, A" },

	//0x70
	{ LD_HL_B, "LD (HL), B" },
	{ LD_HL_C, "LD (HL), C" },
	{ LD_HL_D, "LD (HL), D" },
	{ LD_HL_E, "LD (HL), E" },
	{ LD_HL_H, "LD (HL), H" },
	{ LD_HL_L, "LD (HL), L" },
	{ HALT, "HALT" },
	{ LD_HL_A, "LD (HL), A" },
	{ LD_A_B, "LD A, B" },
	{ LD_A_C, "LD A, C" },
	{ LD_A_D, "LD A, D" },
	{ LD_A_E, "LD A, E" },
	{ LD_A_H, "LD A, H" },
	{ LD_A_L, "LD A, L" },
	{ LD_A_HL, "LD A, (HL)" },
	{ LD_A_A, "LD A, A" },

	//0x80
	{ ADD_A_B, "ADD A, B" },
	{ ADD_A_C, "ADD A, C" },
	{ ADD_A_D, "ADD A, D" },
	{ ADD_A_E, "ADD A, E" },
	{ ADD_A_H, "ADD A, H" },
	{ ADD_A_L, "ADD A, L" },
	{ ADD_A_HL, "ADD A, (HL)" },
	{ ADD_A_A, "ADD A, A" },
	{ ADC_A_B, "ADC A, B" },
	{ ADC_A_C, "ADC A, C" },
	{ ADC_A_D, "ADC A, D" },
	{ ADC_A_E, "ADC A, E" },
	{ ADC_A_H, "ADC A, H" },
	{ ADC_A_L, "ADC A, L" },
	{ ADC_A_HL, "ADC A, (HL)" },
	{ ADC_A_A, "ADC A, A" },

	//0x90
	{ SUB_B, "SUB B" },
	{ SUB_C, "SUB C" },
	{ SUB_D, "SUB D" },
	{ SUB_E, "SUB E" },
	{ SUB_H, "SUB H" },
	{ SUB_L, "SUB L" },
	{ SUB_HL, "SUB (HL)" },
	{ SUB_A, "SUB A" },
	{ ILLOP, "nop" }, // SBC stuff
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	//0xA0
	{ AND_B, "AND B" },
	{ AND_C, "AND C" },
	{ AND_D, "AND D" },
	{ AND_E, "AND E" },
	{ AND_H, "AND H" },
	{ AND_L, "AND L" },
	{ AND_HL, "AND (HL)" },
	{ AND_A, "AND A" },
	{ XOR_B, "XOR B" },
	{ XOR_C, "XOR C" },
	{ XOR_D, "XOR D" },
	{ XOR_E, "XOR E" },
	{ XOR_H, "XOR H" },
	{ XOR_L, "XOR L" },
	{ XOR_HL, "XOR (HL)" },
	{ XOR_A, "XOR A" },

	//0xB0
	{ OR_B, "OR B" },
	{ OR_C, "OR C" },
	{ OR_D, "OR D" },
	{ OR_E, "OR E" },
	{ OR_H, "OR H" },
	{ OR_L, "OR L" },
	{ OR_HL, "OR (HL)" },
	{ OR_A, "OR A" },
	{ CP_B, "CP B" },
	{ CP_C, "CP C" },
	{ CP_D, "CP D" },
	{ CP_E, "CP E" },
	{ CP_H, "CP H" },
	{ CP_L, "CP L" },
	{ CP_HL, "CP (HL)" },
	{ CP_A, "CP A" },

	//0xC0
	{ RETNZ, "RET NZ" },
	{ POP_BC, "POP BC" },
	{ JPNZ, "JP NZ" },
	{ JP, "JP" },
	{ CALLNZ, "CALL NZ w" },
	{ PUSH_BC, "PUSH BC" },
	{ ADD_A_n, "ADD A, n" },
	{ RST00, "RST 00" },
	{ RETZ, "RET Z" },
	{ RET, "RET" },
	{ JPZ, "JP Z" },
	{ opTableB, "ec" },
	{ CALLZ, "CALLZ w" },
	{ CALL, "CALL w" },
	{ ADC_A_n, "ADC A, n" }, // ADC :S
	{ RST08, "RST 08" },

	//0xD0
	{ RETNC, "RET NC" },
	{ POP_DE, "POP DE" },
	{ JPNC, "JP NC" },
	{ ILLOP, "nop" },
	{ CALLNC, "CALL NC" },
	{ PUSH_DE, "PUSH DE" },
	{ SUB_n, "SUB n" },
	{ RST10, "RST 10" },
	{ RETC, "RET C" },
	{ RETI, "RETI" },
	{ JPC, "JP C" },
	{ ILLOP, "nop" },
	{ CALLC, "CALL C" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" }, // SBC A, d8
	{ RST18, "RST 18" },

	//0xE0
	{ LD_Pn_A, "LD ($FF00+n), A" },
	{ POP_HL, "POP HL" },
	{ LD_PC_A, "LD (FF00+C), A" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ PUSH_HL, "PUSH HL" },
	{ AND_n, "AND n" },
	{ RST20, "RST 20" },
	{ ADD_SP_n, "ADD SP, n" },
	{ ILLOP, "nop" },
	{ LD_nn_A, "LD (w), A" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ XOR_n, "XOR n" },
	{ RST28, "RST 28" },

	//0xF0
	{ LD_A_Pn, "LD A, ($FF00+n)" },
	{ POP_AF, "POP AF" },
	{ LD_A_PC, "LD A, (C)" },
	{ DI, "DI" },
	{ ILLOP, "nop" },
	{ PUSH_AF, "PUSH AF" },
	{ OR_n, "OR n" },
	{ RST30, "RST 30" },
	{ LD_HL_SPn, "LD HL, SP+n" },
	{ LD_SP_HL, "LD SP, HL" },
	{ LD_A_nn, "LD A, (w)" },
	{ EI, "EI" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ CP_n, "CP n" },
	{ RST38, "RST 38" },
};

Z80::z80op optable2[256] =
{
	// 00
	{ xRLCB, "RLCB" },
	{ xRLCC, "RLCC" },
	{ xRLCD, "RLCD" },
	{ xRLCE, "RLCE" },
	{ xRLCH, "RLCH" },
	{ xRLCL, "RLCL" },
	{ xRLCHL, "RLC (HL)" },
	{ xRLCA, "RLC A" },
	{ xRRCB, "RRCB" },
	{ xRRCC, "RRCC" },
	{ xRRCD, "RRCD" },
	{ xRRCE, "RRCE" },
	{ xRRCH, "RRCH" },
	{ xRRCL, "RRCL" },
	{ RRCHL, "RRC (HL)" },
	{ xRRCA, "RRC A" },

	// 10
	{ xRLB, "RLB" },
	{ xRLC, "RLC" },
	{ xRLD, "RLD" },
	{ xRLE, "RLE" },
	{ xRLH, "RLH" },
	{ xRLL, "RLL" },
	{ RLHL, "RL (HL)" },
	{ xRLA, "RL A" },
	{ xRRB, "RRB" },
	{ xRRC, "RRC" },
	{ xRRD, "RRD" },
	{ xRRE, "RRE" },
	{ xRRH, "RRH" },
	{ xRRL, "RRL" },
	{ RRHL, "RR (HL)" },
	{ xRRA, "RR A" },

	// 20
	{ ILLOP, "nop" }, // RL
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" }, // RR
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// 30
	{ SWAP<&Z80::b>, "SWAP B" }, // SLA
	{ SWAP<&Z80::c>, "SWAP C" },
	{ SWAP<&Z80::e>, "SWAP D" },
	{ SWAP<&Z80::e>, "SWAP E" },
	{ SWAP<&Z80::h>, "SWAP H" },
	{ SWAP<&Z80::l>, "SWAP L" },
	{ ILLOP, "nop" },
	{ SWAP<&Z80::a>, "SWAP A" },
	{ ILLOP, "nop" }, // SRB
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// 40
	{ BIT_0_B, "BIT 0,B" },
	{ BIT_0_C, "BIT 0,C" },
	{ BIT_0_D, "BIT 0,D" },
	{ BIT_0_E, "BIT 0,E" },
	{ BIT_0_H, "BIT 0,H" },
	{ BIT_0_L, "BIT 0,L" },
	{ BIT_N_HL<0>, "BIT 0,(HL)" },
	{ BIT_0_A, "BIT 0,A" },
	{ BIT_1_B, "BIT 1,B" },
	{ BIT_1_C, "BIT 1,C" },
	{ BIT_1_D, "BIT 1,D" },
	{ BIT_1_E, "BIT 1,E" },
	{ BIT_1_H, "BIT 1,H" },
	{ BIT_1_L, "BIT 1,L" },
	{ BIT_N_HL<1>, "BIT 1,(HL)" },
	{ BIT_1_A, "BIT 1,A" },


	// 50
	{ BIT_2_B, "BIT 2,B" },
	{ BIT_2_C, "BIT 2,C" },
	{ BIT_2_D, "BIT 2,D" },
	{ BIT_2_E, "BIT 2,E" },
	{ BIT_2_H, "BIT 2,H" },
	{ BIT_2_L, "BIT 2,L" },
	{ BIT_N_HL<2>, "BIT 2,(HL)" },
	{ BIT_2_A, "BIT 2,A" },
	{ BIT_3_B, "BIT 3,B" },
	{ BIT_3_C, "BIT 3,C" },
	{ BIT_3_D, "BIT 3,D" },
	{ BIT_3_E, "BIT 3,E" },
	{ BIT_3_H, "BIT 3,H" },
	{ BIT_3_L, "BIT 3,L" },
	{ BIT_N_HL<3>, "BIT 3,(HL)" },
	{ BIT_3_A, "BIT 3,A" },

	// 60
	{ BIT_4_B, "BIT 4,B" },
	{ BIT_4_C, "BIT 4,C" },
	{ BIT_4_D, "BIT 4,D" },
	{ BIT_4_E, "BIT 4,E" },
	{ BIT_4_H, "BIT 4,H" },
	{ BIT_4_L, "BIT 4,L" },
	{ BIT_N_HL<4>, "BIT 4,(HL)" },
	{ BIT_4_A, "BIT 4,A" },
	{ BIT_5_B, "BIT 5,B" },
	{ BIT_5_C, "BIT 5,C" },
	{ BIT_5_D, "BIT 5,D" },
	{ BIT_5_E, "BIT 5,E" },
	{ BIT_5_H, "BIT 5,H" },
	{ BIT_5_L, "BIT 5,L" },
	{ BIT_N_HL<5>, "BIT 5,(HL)" },
	{ BIT_5_A, "BIT 5,A" },

	// 70
	{ BIT_6_B, "BIT 6,B" },
	{ BIT_6_C, "BIT 6,C" },
	{ BIT_6_D, "BIT 6,D" },
	{ BIT_6_E, "BIT 6,E" },
	{ BIT_6_H, "BIT 6,H" },
	{ BIT_6_L, "BIT 6,L" },
	{ BIT_N_HL<6>, "BIT 6,(HL)" },
	{ BIT_6_A, "BIT 6,A" },
	{ BIT_7_B, "BIT 7,B" },
	{ BIT_7_C, "BIT 7,C" },
	{ BIT_7_D, "BIT 7,D" },
	{ BIT_7_E, "BIT 7,E" },
	{ BIT_7_H, "BIT 7,H" },
	{ BIT_7_L, "BIT 7,L" },
	{ BIT_N_HL<7>, "BIT 7,(HL)" },
	{ BIT_7_A, "BIT 7,A" },

	// 80
	{ ILLOP, "nop" }, // RES
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// 90
	{ ILLOP, "nop" }, // RES
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// A0
	{ ILLOP, "nop" }, // RES
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// B0
	{ ILLOP, "nop" }, // RES
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// C0
	{ ILLOP, "nop" }, // SET
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// D0
	{ ILLOP, "nop" }, // SET
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// E0
	{ ILLOP, "nop" }, // SET
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },

	// F0
	{ ILLOP, "nop" }, // SET
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
	{ ILLOP, "nop" },
};

}