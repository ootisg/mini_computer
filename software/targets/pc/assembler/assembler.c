#include "assembler.h"

#include <stdio.h>

int main () {
	while(1) {
		uint8_t opcode = 0x00;
		char buf[33];
		char* success = fgets (buf, 33, stdin);
		if (!success) {
			return 0;
		}
		int i;
		for (i = 0; i < 8; i++) {
			opcode = opcode << 1;
			if (buf[i] == '1') {
				opcode |= 0x01;
			} else {
				opcode |= 0x00;
			}
		}
		printf ("%s\n", disassemble(((uint32_t)opcode) << 24));
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

char* ins_mov (uint32_t instruction) {
	uint8_t opcode = instruction >> 24;
	uint8_t args = (instruction & 0x00FF0000) >> 16;
	if (!(opcode & 0x20)) {
		//MOVs with registers in args
		if (!(opcode & 0x10)) {
			//'Regular' MOVs
			if (!(opcode & 0x01)) {
				if (!(opcode & 0x02)) {
					return "MOV1";
				} else {
					return "MOV2";
				}
			} else {
				if (!(opcode & 0x02)) {
					return "MOV3";
				} else {
					return "MOV4";
				}
			}
		} else {
			//'Irregular' MOVs
			if (!(opcode & 0x01)) {
				//Large register MOVs
				if (!(opcode & 0x02)) {
					return "MOV5";
				} else {
					return "MOV6";
				}
			} else {
				//Split literal hi-byte MOV
				return "MOV7";
			}
		}
	} else {
		//MOVs with register in opcode
		if (opcode & 0x01) {
			//24L MOV
			if (!(opcode & 0x02)) {
				return "MOV8";
			} else {
				return "MOV9";
			}
		} else {
			//8L MOV
			return "MOV10";
		}
	}
	return "MOV0"; //Invalid MOV instruction
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
