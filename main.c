#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include "instructions.h"

struct inst_t {
    unsigned int opcode;
    unsigned int rs;
    unsigned int rd;
    unsigned int rt;
    unsigned int shamt;
    unsigned int funct;
    unsigned int imm;
    unsigned int jimm;
};

int instruction_reg;
unsigned int Mem[MAX_MEM];
unsigned int regfile[MAX_REG];
unsigned int PC;

void initialize();
void loadprog();
int fetch(FILE *fd);
int decode (struct inst_t *inst);
int execute (struct inst_t* inst);
//int load(struct inst_t* inst);

int main(int argc, char* argv[]) {
    FILE *fd;
    struct inst_t instruction; 
    int ret;

    char *filename;
    if(argc == 2) {
        filename = argv[1];
    } else {
        filename = "simple.bin";
    }

    if (fd == NULL){
        printf("Exit\n");
        exit(1);
    }
    else {
        fd = fopen(filename, "rb");
    }
        
    loadprog(fd); 
    fclose(fd);
    initialize();
    
    for(int i=0; i<10; i++){
        ret = fetch(fd);
            if(ret <= 0) 
            break;
        ret = decode(&instruction);
            if(ret <= 0) 
            break; 
        ret = execute(&instruction); 
            if(ret <= 0) break;
        // ret = load(&instruction);
        //     if(ret <= 0) break;
        } 
    return 0;
}

void initialize() { 
    bzero(regfile, sizeof(regfile)); 
    PC = 0;
    regfile[sp]= (unsigned int *) 0x1000000;
    regfile[ra] = 0xFFFFFFFF;
}

void loadprog(FILE *fd){
    size_t ret = 0;
    int mips_val, i =0;
    int mem_val;

    do {
        mips_val = 0;
        ret = fread(&mips_val, 1, 4, fd);
        mem_val = ntohl(mips_val);
        Mem[i] = mem_val;
        //printf("(%d) Load Mem[%X] pa: 0x%x val: 0x%X\n", (int)ret, i, &Mem[i], Mem[i]);
        i++;
    } while (ret==4);
}

int fetch (FILE *fd) {

    if (PC == 0xFFFFFFFF) 
    return 0;

    memset(&instruction_reg, 0, sizeof(instruction_reg));
    instruction_reg = Mem[PC/4];
    printf("Fet: [0x%08X]: %08X\n",PC, instruction_reg);

}

int decode (struct inst_t *inst) {
    //basic instruction formats :opcode, rs, rd, rt, imm, jump imm, shamt, funct
    int ret = 1;
    unsigned int opcode;
    unsigned int rs;
    unsigned int rd;
    unsigned int rt;
    unsigned int imm;
    unsigned int jimm;
    unsigned int shamt;
    unsigned int funct;

    opcode = (instruction_reg & 0xFC000000);
    opcode = opcode >> 26;
    
    rs = (instruction_reg & 0x03E00000);
    rs = rs >> 21;

    rt = (instruction_reg & 0x001F0000);
    rt = rt >> 16;

    rd = (instruction_reg & 0x0000F800);
    rd = rd >> 11;

    shamt = (instruction_reg & 0x000007C0);
    shamt = shamt >> 5;

    funct = (instruction_reg & 0x0000003F);

    imm = (instruction_reg & 0x0000FFFF);

    jimm = (instruction_reg & 0x03FFFFFF);

    bzero (inst, sizeof(*inst));

    inst -> opcode = opcode;
    inst -> rs = rs;
    inst -> rd = rd;
    inst -> rt = rt;
    inst -> shamt = shamt;
    inst -> funct = funct;
    inst -> imm = imm;
    inst -> jimm = jimm;
    
    printf("Dec:\t opcode: %x, rs: %x, rt: %x, rd: %x, shamt: %x, funct: %x, imm %x, jump imm:%x\n", opcode, rs, rt, rd, shamt, funct, imm, jimm);
}

int execute (struct inst_t* inst) { //except lbu, lhu, sb, sc, sh, ll

    unsigned int opcode, rs, rd, rt, shamt, funct, imm, jimm;

    int ret = 1;
    int signexitimm; 
    int zeroexitimm;
    int *branchaddr;
    int *jumpaddr;
    
    unsigned int temp;
    unsigned int temp1;
    unsigned int temp2;
   
    opcode = inst->opcode;
	rs = inst->rs;
	rd = inst->rd;
	rt = inst->rt;
	shamt = inst->shamt;
    funct = inst->funct;
	imm = inst->imm;
	jimm = inst->jimm;

    if (imm >> 15){
        signexitimm = imm| 0xFFFF0000;
    } else {
        zeroexitimm = imm;
    }

    branchaddr = imm << 2;

    temp = PC + 4;
    temp = temp & 0xF0000000;
    temp1 = jimm << 2;
    temp = temp | temp1;
    jumpaddr = (unsigned int *) temp;

    if (opcode == 0) {
        if (funct == add){ //0x20
            regfile[rd] = regfile[rs] + regfile[rt];
            printf("ADD\t R%d (0x%x) = R%d + R%d\n", rd, regfile[rd], rs, rt);
            PC = PC + 4;
        }else if (funct == addu) { //0x21
            regfile[rd] = regfile[rs] + regfile[rt];
            printf("ADDU\t R%d (0x%x) = R%d + R%d\n", rd, regfile[rd], rs, rt);
            PC = PC + 4;
        } else if (funct == and) { //0x24
            regfile[rd] = regfile[rs] & regfile[rt];
            printf("AND\t R%d (0x%x) = R%d & R%d\n", rd, regfile[rd], rs, rt);
            PC = PC + 4;
        }else if (funct == jr){ //0x8
            temp = regfile[rs];
            PC = temp;
            printf("JR\t PC = R%d (0x%x)\n", rs, regfile[rs]);
        } else if (funct == nor) { //0x27
            regfile[rd] = ~(regfile[rs] | regfile[rt]);
            printf("NOR\t R%d (0x%x) = ~ (R%d | R%d)\n", rd, regfile[rd], rs, rt);
            PC = PC + 4;
        }else if (funct == or){ //0x25
            regfile[rd] = regfile[rs] | regfile[rt];
            printf("OR\t R%d (0x%x) = R%d | R%d\n", rd, regfile[rd], rs, rt);
            PC = PC + 4;
        }else if (funct == slt) { //0x2a
            regfile[rd] = (regfile[rs] < regfile[rt])? 1 : 0;
            printf("SLT\t R%d = R%d (0x%x) < R%d (0x%x) \n", rd, rs, regfile[rs], rt, regfile[rt]);
            PC = PC + 4;
        }else if (funct == sltu) { //0x2b
            regfile[rd] = (regfile[rs] < regfile[rt])? 1 : 0;
            printf("SLTU\t R%d = R%d (0x%x) < R%d (0x%x) \n", rd, rs, regfile[rs], rt, regfile[rt]);
            PC = PC + 4;
        }else if (funct == sll ) { //0x0
            regfile[rd] = regfile[rt] << shamt;
            printf("SLL\t R%d (0x%x) = R%d (0x%x) << %d \n", rd, regfile[rd], rt, regfile[rt], shamt);
            PC = PC + 4;
        } else if (funct == srl) { //0x2
            regfile[rd] = regfile[rt] >> shamt;
            printf("SRL\t R%d (0x%x) = R%d (0x%x) >> %d \n", rd, regfile[rd], rt, regfile[rt], shamt);
            PC = PC + 4;
        } else if (funct == sub){ //0x22
            regfile[rd] = regfile[rs] - regfile[rt];
            printf("SUB\t R%d (0x%x) = R%d (0x%x) -  R%d (0x%x) \n", rd, regfile[rd], rs, regfile[rs], rt, regfile[rt]);
            PC = PC + 4;
        } else if (funct == subu){ //0x23
            regfile[rd] = regfile[rs] - regfile[rt];
            printf("SUBU\t R%d (0x%x) = R%d (0x%x) -  R%d (0x%x) \n", rd, regfile[rd], rs, regfile[rs], rt, regfile[rt]);
            PC = PC + 4;
        }

    } else if (opcode == addi){ //0x8
        regfile[rt] = (int)regfile[rs] + signexitimm;
        printf("ADDI\t R%d (0x%x) = R%d + (%d)\n", rd, regfile[rd], rs, signexitimm);
        PC = PC + 4;
    }else if (opcode == addiu) { //0x9
        regfile[rt] = (unsigned int)regfile[rs] + signexitimm;
        printf("ADDIU\t R%d (0x%x) = R%d + (%d)\n", rd, regfile[rd], rs, signexitimm);
        PC = PC + 4;
    }else if (opcode == andi) { //0xc
        regfile[rt] = regfile[rs] & signexitimm;
        printf("ANDI\t R%d (0x%x) = R%d + (%d)\n", rd, regfile[rd], rs, signexitimm);
        PC = PC + 4;
    }else if (opcode == beq) { //0x4
        if (regfile[rs] == regfile[rt]){
            temp = PC + 4 + branchaddr;
            PC = temp;
        }
        printf("BEQ\t Branch Address 0x%x if R%d (0x%x) == R%d 0x%x)\n", branchaddr, rs, regfile[rs], rt, regfile[rt]);
    }else if (opcode == bne) { //0x5
        if (regfile[rs] != regfile[rt]){
            temp = PC + 4 + branchaddr;
            PC = temp;
        }
        printf("BNE\t Branch Address 0x%x if R%d (0x%x) == R%d 0x%x)\n", branchaddr, rs, regfile[rs], rt, regfile[rt]);
    }else if (opcode == j) { //0x2
        PC = jumpaddr;
        printf("J\t PC = Jump Address 0x%x\n", jumpaddr);
    }else if (opcode == jal) { //0x3
        regfile[ra] = PC + 8;
        PC = jumpaddr;
        printf("JAL\t R%d = PC + 8; PC = Jump Address 0x%x\n", regfile[ra], jumpaddr);
    }else if (opcode == lui) { //0xf
        regfile[rt] = imm << 16;
        printf("LUI\t R%d (0x%x) = Imm (%d) << 16\n", rt, regfile[rt], rs, imm);
        PC = PC + 4; 
    }else if (opcode == lw) { //0x23
        temp = &Mem[regfile[rs] + signexitimm];
        regfile[rt] = temp;
        printf("LW\t R%d (0x%x) = M[R%d + (%d)]\n", rt, regfile[rt], regfile[rd], signexitimm);
        PC = PC + 4;
    }else if (opcode == ori){ //0xd
        temp = regfile[rs] | zeroexitimm;
        regfile[rt] = temp;
        printf("ORI\t R%d (0x%x)= R%d | (%d)\n", rt, regfile[rt], regfile[rs], zeroexitimm);
        PC = PC + 4;
    }else if (opcode == slti){ //0xa
        regfile[rt] = ((regfile[rs] < signexitimm) ? 1 : 0);
        printf("SLTI\t R%d (0x%x) = ((R%d < (%d) ? 1 : 0)\n", rt, regfile[rt], rs, signexitimm);
        PC = PC +4;
    }else if (opcode == sltiu){ //0xb
        regfile[rt] = ((regfile[rs] < signexitimm) ? 1 : 0);
        printf("SLTIU\t R%d (0x%x) = ((R%d < (%d) ? 1 : 0)\n", rt, regfile[rt], rs, signexitimm);
        PC = PC +4;
    }else if (opcode == sw){ //0x2b
        temp = regfile[rt];
        temp1 = (regfile[rs] + signexitimm) << 2;
        temp1 = &Mem[temp1];
        temp1 = temp;
        printf("SW\t M[R%d (0x%x) + %d] = R%d (0x%x)\n", rd, regfile[rd], signexitimm, rt, regfile[rt]);//keknya salah
        PC = PC + 4;
    }
}   

