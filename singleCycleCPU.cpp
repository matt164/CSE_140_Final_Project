//Single Cycle CPU
//Authors: Matthew Mengell and Katharina Ng

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

using namespace std;

int PC = 0;
int next_PC = PC + 4;
long jump_target = 0;
long branch_target = 0;
int total_clock_cycles = 1;
int alu_zero = 0;

int JUMP = 0;
int REGDST = 0;
int ALUSRC = 0;
int MEMTOREG = 0;
int REGWRITE = 0;
int MEMREAD = 0;
int MEMWRITE = 0;
int BRANCH = 0;
int INSTTYPE = 0;
int ALUOPCODE = 0;

long opcode;
long rs, rt;
long rd, shamt, funct;
long imm;

int register_file[32] = {0};
bool modified[32] = {false};
int d_mem[32] = {0};

void fetch(string nextInst);
void decode(string nextInst);
void controlUnit(string nextInst);
void execute(int alu_op, long rs, long rt, long rd, long imm);
void mem();
void writeback(long value);
long binaryToDecimalLong(string bin, int begin, int end);
long signedBinaryToDecimal(string bin, int begin, int end);
string registerDecode(long regNum);

int main(int argc, char **argv)
{	
	cout << endl;
	cout << "Enter the program file name to run: " << endl;
	string file;
	cin >> file;
	ifstream instList;
	instList.open(file.c_str());
	register_file[9] = 32;
	register_file[10] = 5;
	register_file[16] = 112;
	d_mem[112] = 5;
	d_mem[116] = 16;
	
	cout << endl;
	string nextInst;
	int fileLine = 0;
	while (getline(instList, nextInst)) {
		if (fileLine == PC/4) {
			fetch(nextInst);
		}
		fileLine++;
	} 

	//register_file[13] = 12;
	//register_file[11] = 3;
	//fetch("00000010001011010110100000100101");
	//fetch("00000001001010100110100000100010");
	//fetch("00000001101010111000100000101010");  
	//fetch("00010010001000000000000000000011"); 
	//fetch("10101110000011010000000000000000"); 

	cout << "program terminated:" << endl;
	cout << "total execution time is " << total_clock_cycles-1 << " cycles" << endl << endl;;               
}

void fetch(string nextInst)
{		
	cout << "total_clock_cycles " << total_clock_cycles << " :" << endl;
	next_PC = PC + 4;
	jump_target = 0;
	branch_target = 0;
	alu_zero = 0;
	JUMP = 0;
	REGDST = 0;
	ALUSRC = 0;
	MEMTOREG = 0;
	REGWRITE = 0;
	MEMREAD = 0;
	MEMWRITE = 0;
	BRANCH = 0;
	INSTTYPE = 0;
	ALUOPCODE = 0;

	opcode = 0;
	rs = 0;
	rt = 0;
	rd = 0;
	shamt = 0;
	funct = 0;
	imm = 0;

	decode(nextInst);

	if(jump_target != 0)
		PC = jump_target;
	else if(branch_target != 0)
		PC = branch_target;
	else
		PC = next_PC;
	cout << "pc is modified to 0x" << std::hex << PC << endl << endl;
}

void decode(string nextInst)
{
	opcode = binaryToDecimalLong(nextInst, 0, 5);
	rs = binaryToDecimalLong(nextInst, 6, 10);
	//cout << std::dec << opcode << endl;
	//cout << std::dec << rs << endl;

	if(opcode == 0)
	{
		rt = binaryToDecimalLong(nextInst, 11, 15);
		rd = binaryToDecimalLong(nextInst, 16, 20);
		shamt = binaryToDecimalLong(nextInst, 21, 25);
		funct = binaryToDecimalLong(nextInst, 26, 31);
		//cout << std::dec << funct << endl;
	}
	else if(opcode <= 3)
	{
		jump_target = binaryToDecimalLong(nextInst, 6, 31);
		jump_target = jump_target * 4 + PC + 4;
	}
	else
	{
		rt = binaryToDecimalLong(nextInst, 11, 15);
		imm = signedBinaryToDecimal(nextInst, 16, 31);
		//cout << std::dec << imm << endl;
	}

	controlUnit(nextInst);
	execute(ALUOPCODE, rs, rt, rd, imm);
}

void controlUnit(string nextInst)
{
	long opcode, funct;
	opcode = binaryToDecimalLong(nextInst, 0, 5);

	if(opcode == 0)
	{
		funct = binaryToDecimalLong(nextInst, 26, 31);
		REGDST = 1;
		REGWRITE = 1;
		INSTTYPE = 2;
	}
	else if(opcode == 35)
	{
		ALUSRC = 1;
		MEMTOREG = 1;
		REGWRITE = 1;
		MEMREAD = 1;
	}
	else if(opcode == 43)
	{
		ALUSRC = 1;
		MEMWRITE = 1;
	}
	else if(opcode == 4)
	{
		BRANCH = 1;
		INSTTYPE = 1;
	}
	else if(opcode == 2)
	{
		JUMP = 1;	
	}

	if(INSTTYPE == 0)
	{
		ALUOPCODE = 2;
	}
	else if(INSTTYPE == 1)
	{
		ALUOPCODE = 6;
	}
	else if(INSTTYPE == 2)
	{
		if(funct == 32)
			ALUOPCODE = 2;
		if(funct == 34)
			ALUOPCODE = 6;
		if(funct == 36)
			ALUOPCODE = 0;
		if(funct == 37)
			ALUOPCODE = 1;
		if(funct == 42)
			ALUOPCODE = 7;
		if(funct == 39)
			ALUOPCODE = 12;
	}
}

void execute(int alu_op, long rs, long rt, long rd, long imm) {
    long value = 0;
	if (alu_op == 0) {   //AND
        value = register_file[rs] & register_file[rt];
		writeback(value);
    }
    else if (alu_op == 1) {  //OR
        value = register_file[rs] | register_file[rt];
		writeback(value);
    }
    else if (alu_op == 2) {  //add
        if (REGWRITE == 1 && MEMREAD == 1 && MEMWRITE == 0) { //lw
			mem();
        }
        else if (REGWRITE == 0 && MEMREAD == 0 && MEMWRITE == 1) {    //sw
			mem();
        }
        else {  //add
            value = register_file[rs] + register_file[rt];
			writeback(value);
        }
    }
    else if (alu_op == 6) {  //subtract
        if (BRANCH == 1) {  //beq
            if (register_file[rs] - register_file[rt] == 0) {
                alu_zero = 1;
				branch_target = next_PC + imm * 4;
				writeback(register_file[rs] - register_file[rt] != 0);
            }
            else {
                alu_zero = 0;
            }
        }
        else {  //subtract
            value = register_file[rs] - register_file[rt];
			writeback(value);
        }
    }
    else if (alu_op == 7) {  //set-on-less-than
        value = register_file[rs] < register_file[rt];
		//cout << value << endl;
		writeback(value);
    }
    else if (alu_op == 12) {  //NOR
        value = !(register_file[rs] | register_file[rt]);
		writeback(value);
    }
	else if (JUMP == 1) {
		writeback(value);
	}
}

void mem() {
    if (REGWRITE == 1 && MEMREAD == 1 && MEMWRITE == 0) { //lw
        long memAddress = register_file[rs] + imm;
		long value = d_mem[memAddress];
		writeback(value);
    }
    else if (REGWRITE == 0 && MEMREAD == 0 && MEMWRITE == 1) {    //sw
        long memAddress = register_file[rs] + imm;
		long value = register_file[rt];
		writeback(value);
    }
}

void writeback(long value) {
    if (REGWRITE == 1 && MEMREAD == 1 && MEMWRITE == 0) { //lw
		if (register_file[rt] != value || modified[rt] == false) {
            register_file[rt] = value;
			modified[rt] = true;
        	cout << registerDecode(rt) << " is modified to 0x" << std::hex << value << endl;
		}
    }
	else if (funct == 34) {	//sub
		if (register_file[rd] != value || modified[rd] == false) {
            register_file[rd] = value;
			modified[rd] = true;
			cout << registerDecode(rd) << " is modified to 0x" << std::hex << value << endl;
		}
	}
	else if (funct == 42) {	//slt
	//cout << register_file[rd] << "  " << value << endl;
		if (register_file[rd] != value || modified[rd] == false) {
            register_file[rd] = value;
			modified[rd] = true;
			cout << registerDecode(rd) << " is modified to 0x" << std::hex << value << endl;
		}
	}
	else if (opcode == 4) {	//beq
		if (register_file[rs] != value || modified[rs] == false) {
            register_file[rs] = value;
			modified[rs] = true;
			cout << registerDecode(rs) << " is modified to 0x" << std::hex << value << endl;
		}
	}
	else if (REGWRITE == 0 && MEMREAD == 0 && MEMWRITE == 1) {	//sw
		cout << "memory 0x" << std::hex << register_file[rs] + imm << " is modified to 0x" << std::hex << value << endl;
	}
	else if (funct == 32 || funct == 37 || funct == 36 || funct == 39) {	//add & or & and & nor
		if (register_file[rd] != value || modified[rd] == false) {
            register_file[rd] = value;
			modified[rd] = true;
			cout << registerDecode(rd) << " is modified to 0x" << std::hex << value << endl;
		}
	}

    total_clock_cycles++;
}

long binaryToDecimalLong(string bin, int begin, int end)     
{                                                         
	int pos = 0;
	long dec = 0;
	for(int i = end; i >= begin; i--)
	{
		if(bin[i] == '1')
			dec += pow(2,pos);
		pos++;
	}
	return dec;
}

long signedBinaryToDecimal(string bin, int begin, int end)           
{
	int pos = 0;
    long dec = 0;
	for(int i = end; i > begin; i--)                     
	{
		if(bin[i] == '1')
			dec += pow(2,pos);
		pos++;
	}
	if(bin[begin] == '1')                             
		dec -= pow(2,pos);
	return dec;
}

string registerDecode(long regNum)
{
	if(regNum == 0)
		return "$zero";   //uses the integer value to determine which register name to return
	else if(regNum == 1)
		return "$at";
	else if(regNum == 2)
		return "$v0";
	else if(regNum == 3)
		return "$v1";
	else if(regNum == 4)
		return "$a0";
	else if(regNum == 5)
		return "$a1";
	else if(regNum == 6)
		return "$a2";
	else if(regNum == 7)
		return "$a3";
	else if(regNum == 8)
		return "$t0";
	else if(regNum == 9)
		return "$t1";
	else if(regNum == 10)
		return "$t2";
	else if(regNum == 11)
		return "$t3";
	else if(regNum == 12)
		return "$t4";
	else if(regNum == 13)
		return "$t5";
	else if(regNum == 14)
		return "$t6";
	else if(regNum == 15)
		return "$t7";
	else if(regNum == 16)
		return "$s0";
	else if(regNum == 17)
		return "$s1";
	else if(regNum == 18)
		return "$s2";
	else if(regNum == 19)
		return "$s3";
	else if(regNum == 20)
		return "$s4";
	else if(regNum == 21)
		return "$s5";
	else if(regNum == 22)
		return "$s6";
	else if(regNum == 23)
		return "$s7";
	else if(regNum == 24)
		return "$t8";
	else if(regNum == 25)
		return "$t9";
	else if(regNum == 26)
		return "$k0";
	else if(regNum == 27)
		return "$k1";
	else if(regNum == 28)
		return "$gp";
	else if(regNum == 29)
		return "$sp";
	else if(regNum == 30)
		return "$fp";
	else if(regNum == 31)
		return "$ra";
	else
		return "unknown";
}