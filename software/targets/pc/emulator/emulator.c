#include "emulator.h"

#include <stdio.h>
#include <stdint.h>

//Move these defines later
#define REG_TYPE_GENERAL 0
#define REG_TYPE_SPECIAL 1
#define REG_TYPE_ARITH 2
#define REG_TYPE_OTHER 3

#define REG_A 0
#define REG_B 1
#define REG_C 2
#define REG_SP 3
#define REG_RL 4
#define REG_RH 5
#define REG_I 6
#define REG_Z 7
#define REG_AA 8
#define REG_AB 9
#define REG_AC 10
#define REG_PL 12
#define REG_PH 13
#define REG_F 14
#define REG_NULL 15

uint16_t registers[16];
uint16_t addr_space[16777216];

uint8_t reg_decode_table[12] = {	REG_A, REG_B, REG_C, REG_SP,
                                	REG_RL, REG_RH, REG_I, REG_Z,
                                	REG_AA, REG_AB, REG_AC, REG_Z};

//Instruction arg format:
//uint32_t, with lower 24 bits being the payload
//Bit 0 is set if this is an address
//Bit 1 is set if this is a register

int main () {
	int read_char = 0x20; //Space as default, bc why not
	while(read_char != EOF) {
		uint32_t ins = 0x00000000;
		int i = 0;
		while ((read_char = fgetc (stdin)) != EOF) {
			if (read_char == '\n') {
				break;
			}
			if (read_char == '1' || read_char == '0') {
				if (read_char == '1') {
					ins |= (0x80000000 >> i);
				}
				if (read_char != ' ') {
					i++;
				}
			}
		}
		printf ("%x\n", ins);
		//printf ("%s\n", disassemble(((uint32_t)opcode) << 24));
	}
	/*//Instruction table
	int i;
	for (i = 0; i < 256; i++) {
		printf ("%s ", disassemble (((uint32_t)i) << 24));
		if (i % 16 == 15) {
			printf ("\n");
		}
	}*/
	return 0;
}

uint8_t get_full_reg_id (uint8_t reg_id, uint8_t reg_type) {
        uint8_t reg_idx = (reg_type << 2) || reg_id;
        return reg_decode_table[reg_idx];
}

uint32_t encode_dual_register (uint8_t reg_1, uint8_t reg_2) {
	return 0x10000000 | (uint32_t)reg_1 | (uint32_t)(reg_2 << 4);
}

uint32_t encode_large_register (uint8_t reg_id) {
	return 0x20000000 | (uint32_t)reg_id;
}

uint32_t encode_register (uint8_t reg_id) {
	return 0x40000000 | (uint32_t)reg_id;
}

uint32_t encode_addr (uint32_t addr) {
	return 0x80000000 | (addr & 0x00FFFFFF);
}

void do_move (uint32_t src, uint32_t dest) {
	if (src & 0x20000000 || dest & 0x20000000) {
		//LR move
		if (src & 0x20000000) {
			uint32_t reg_1 = (dest & 0x000000F0) >> 4;
			uint32_t reg_2 = (dest & 0x0000000F);
			if (src & 0x00000001) {
				//PC
				registers[reg_1] = registers[REG_PH];
				registers[reg_2] = registers[REG_PL];
			} else {
				//RT
				registers[reg_1] = registers[REG_RH];
				registers[reg_2] = registers[REG_RL];
			}
		} else {
			uint32_t reg_1 = (src & 0x000000F0) >> 4;
			uint32_t reg_2 = (src & 0x0000000F);
			if (dest & 0x00000001) {
				//PC
				registers[REG_PH] = registers[reg_1];
				registers[REG_PL] = registers[reg_2];
			} else {
				//RT
				registers[REG_RH] = registers[reg_1];
				registers[REG_RL] = registers[reg_2];
			}
		}
	} else {
		//16-bit move
		uint16_t src_val;
		if (src & 0x40000000) {
			//src is a register
			src_val = registers[src & 0x0000000F];
		} else if (src & 0x80000000) {
			//src is an address
			src_val = addr_space[src & 0x00FFFFFF];
		} else {
			//src is a literal value
			src_val = src;
		}
		if (dest & 0x40000000) {
			//dest is a register
			registers[dest & 0x0000000F] = src_val;
		} else if (dest & 0x80000000) {
			//dest is an address
			addr_space[src & 0x00FFFFFF] = src_val;
		}
	}
}

char* ins_mov (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	uint8_t args = (instruction & 0x00FF0000) >> 16;
	uint32_t src, dest;
	if (!(opcode & 0x20)) {
		//MOVs with registers in args
		uint8_t a1_rel = opcode & 0x08;
		uint8_t a2_rel = opcode & 0x04;
		uint8_t r1_spec = args & 0x80;
		uint8_t r2_spec = args & 0x40;
		if (!(opcode & 0x10)) {
			//'Regular' MOVs
			uint8_t rh = get_full_reg_id (args & 0x03, REG_TYPE_GENERAL);
			uint8_t r2 = get_full_reg_id ((args & 0x0C) >> 2, r2_spec);
			if (!(opcode & 0x01)) {
				uint8_t r1 = get_full_reg_id ((args & 0x30) >> 4, r1_spec);
				src = a1_rel ? encode_addr (registers[r1]) : encode_register(r1);
				dest = a2_rel ? encode_addr (((registers[rh] & 0x00FF) << 16) | registers[r2]) : encode_register(r2);
			} else {
				src = a1_rel ? encode_addr (instruction & 0xFFFF) : instruction & 0xFFFF;
				dest = a2_rel ? encode_addr (((registers[rh] & 0x00FF) << 16) | registers[r2]) : encode_register(r2);
			}
		} else {
			//'Irregular' MOVs
			if (!(opcode & 0x01)) {
				//Large register MOV
				uint8_t rh = get_full_reg_id (args & 0x03, REG_TYPE_GENERAL);
				uint8_t r2 = get_full_reg_id ((args & 0x0C) >> 2, r2_spec);
				uint8_t lrid = (args & 0x10) >> 4;
				src = encode_large_register (lrid);
				dest = encode_dual_register (rh, r2);
			} else {
				//Split literal hi-byte MOV
				uint8_t r1 = get_full_reg_id ((args & 0x30) >> 4, r1_spec);
				uint8_t r2 = get_full_reg_id ((args & 0x0C) >> 2, r2_spec);
				uint32_t high_1 = (instruction & 0x0000FF00) << 8;
				uint32_t high_2 = (instruction & 0x000000FF) << 16;
				src = a1_rel ? encode_addr (r1) : encode_addr (high_1 | registers[r1]);
				dest = a2_rel ? encode_addr (r2) : encode_addr (high_2 | registers[r2]);
			}
		}
	} else {
		//MOVs with register in opcode
		uint8_t r1_spec = opcode & 0x04 ? 1 : 0;
		uint8_t r1 = get_full_reg_id ((opcode & 0x18) >> 3, r1_spec);
		src = encode_register (r1);
		if (opcode & 0x01) {
			//24L MOV
			dest = instruction & 0x00FFFFFF;
		} else {
			//8L MOV
			dest = (instruction & 0x00FF0000) >> 16;
		}
	}
	if (opcode & 0x02) {
		uint32_t temp = src;
		src = dest;
		dest = temp;
	}
	do_move (src, dest);
}

char* ins_psh (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	if (!(opcode & 0x02)) {
		//Register PSH
		return "PSH1, 2";
	} else {
		//Literal PSH
		if (opcode & 0x01) {
			//16L PSH
			return "PSH3";
		} else {
			//8L PSH
			return "PSH4";
		}
	}
	return "PSH0"; //Invalid PSH
}

char* ins_pop (uint32_t instruction) {
	return "POP1, 2"; //Cannot distinguish without args
}

char* ins_jcs (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	if (!(opcode & 0x01)) {
		return "JCS1, 2";
	} else {
		return "JCS3";
	}
	return "JCS0";
}

char* ins_jcc (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	if (!(opcode & 0x01)) {
		return "JCC1, 2";
	} else {
		return "JCC3";
	}
	return "JCC0";
}

char* ins_jcn (uint32_t instruction) {
	return "JCN";
}

char* ins_jsr (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	switch (opcode & 0x18) {
		case 0x00:
			return "JSR1";
			break;
		case 0x08:
			return "JSR5";
			break;
		case 0x10:
			return "JSR2";
			break;
		case 0x18:
			if (!(opcode & 0x02)) {
				return "JSR3";
			} else {
				return "JSR4";
			}
			break;
	}
	return "JMP0";
}

char* ins_jmp (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	switch (opcode & 0x18) {
		case 0x00:
			return "JMP1";
			break;
		case 0x08:
			return "JMP5";
			break;
		case 0x10:
			return "JMP2";
			break;
		case 0x18:
			if (!(opcode & 0x02)) {
				return "JMP3";
			} else {
				return "JMP4";
			}
			break;
	}
	return "JMP0";
}

char* ins_sbc (uint32_t instruction) {
	return "SBC";
}

char* ins_sub (uint32_t instruction) {
	return "SUB";
}

char* ins_adc (uint32_t instruction) {
	return "ADC";
}

char* ins_add (uint32_t instruction) {
	return "ADD";
}

char* ins_ror (uint32_t instruction) {
	return "ROR";
}

char* ins_bsr (uint32_t instruction) {
	return "BSR";
}

char* ins_rol (uint32_t instruction) {
	return "ROL";
}

char* ins_bsl (uint32_t instruction) {
	return "BSL";
}

char* ins_xor (uint32_t instruction) {
	return "XOR";
}

char* ins_ior (uint32_t instruction) {
	return "IOR";
}

char* ins_and (uint32_t instruction) {
	return "AND";
}

char* ins_nop (uint32_t instruction) {
	return "NOP";
}

char* ins_sef (uint32_t instruction) {
	return "SEF";
}

char* ins_clf (uint32_t instruction) {
	return "CLF";
}

char* ins_rti (uint32_t instruction) {
	return "RTI";
}

char* ins_itr (uint32_t instruction) {
	return "ITR";
}

char* disassemble (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	uint8_t inst_type = opcode & 0xC0;
	if (inst_type == 0x40) {
		//Memory-type instruction
		if ((opcode & 0xF8) != 0x58) {
			//MOV instruction
			return ins_mov(instruction);
		} else {
			//Stack instruction
			if (opcode & 0x04) {
				//Pop
				return ins_pop(instruction);
			} else {
				//Push
				return ins_psh(instruction);
			}
		}
	} else if (inst_type == 0x80) {
		//Branch-type instruction
		if (opcode & 0x20) {
			//JCS, JCC, or JCN
			if (!((opcode & 0x05) == 0x05)) {
				if (opcode & 0x10) {
					//JCS
					return ins_jcs(instruction);
				} else {
					//JCC
					return ins_jcc(instruction);
				}
			} else {
				//JCN
				return "JCN"; //TODO
			}
		} else {
			//JMP or JSR
			if (opcode & 0x04) {
				//JSR
				return ins_jsr(instruction);
			} else {
				//JMP
				return ins_jmp(instruction);
			}
		}
	} else if (inst_type == 0xC0) {
		//Arithmetic-type instruction
		if (opcode & 0x20) {
			//Add/adc/sub/sbc
			if (opcode & 0x10) {
				if (opcode & 0x08) {
					//SBC
					return ins_sbc(instruction);
				} else {
					//SUB
					return ins_sub(instruction);
				}
			} else {
				if (opcode & 0x08) {
					//ADC
					return ins_adc(instruction);
				} else {
					//ADD
					return ins_add(instruction);
				}
			}
		} else {
			if (opcode & 0x04) {
				if (opcode & 0x10) {
					if (opcode & 0x08) {
						//ROR
						return ins_ror(instruction);
					} else {
						//BSR
						return ins_bsr(instruction);
					}
				} else {
					if (opcode & 0x08) {
						//ROL
						return ins_rol(instruction);
					} else {
						//BSL
						return ins_bsl(instruction);
					}
				}
			} else {
				if (opcode & 0x08) {
					if (opcode & 0x10) {
						//XOR
						return ins_xor(instruction);
					} else {
						//IOR
						return ins_ior(instruction);
					}
				} else {
					//AND
					return ins_and(instruction);
				}
			}
		}
	} else if (inst_type == 0x00) {
		//Misc-type instruction
		if (opcode == 0x00) {
			//NOP
			return ins_nop(instruction);
		} else if ((opcode & 0x30) == 0x10) {
			//SEF
			return ins_sef(instruction);
		} else if ((opcode & 0x30) == 0x30) {
			//CLF
			return ins_clf(instruction);
		} else if ((opcode & 0x30) == 0x20) {
			if (opcode & 0x02) {
				//RTI
				return ins_rti(instruction);
			} else {
				//ITR
				return ins_itr(instruction);
			}
		}
	}
	return "UND";
}
