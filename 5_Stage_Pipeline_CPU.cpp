//5 Stage Pipeline MIPS CPU
//Authors: Matthew Mengell and Katharina Ng

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <list>

using namespace std;

int PC = 0;            

int stall = 0;             //global variable to prevent reading more instructions while stalling

int inst_finish = 0;       //global variable to remove an instruction once it finishes writeback

int stall_next = 0;        //set when we detect a data hazard to stall the next instruction in fetch

int flush_inst;             //set when a control hazard is detected to flush the 3 following instructions

int total_clock_cycles = 0;

int prev_dest = -1;            //stores the last destination register for detecting data hazards
int prev_dest_2 = -1;          //stores the second to last destination register

int register_file[32] = {0};         //array for registers and memory
int d_mem[32] = {0};

struct instruction                //struct to store the data for an instruction between steps acts as the stage buffer
{
	string inst;
	long op_code;
	long rs;
	long rt;
	long rd;
	long shamt;
	long funct;
	long branch_target;
	long jump_target;
	long imm;
	long ALU_value;                    //return from the execute stage
	int alu_zero = 0;                  //set by the beq instruction
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
	int stage = 0;            //used to determine the current stage of the cpu to execute on an instruction
	int stall = 0;            //set to the number of times this specific instruction needs to be stalled
	int start_PC;             //records the pc from when the instruction starts for use in calculating the branch target address, it essentially is = PC + 4
};

void pipeline_control(instruction &inst);
void fetch(instruction &inst);
void decode(instruction &instr);
void control_unit(instruction &instr);
void execute(instruction &instr);
void mem(instruction &instr);
void writeback(instruction &instr);
long binaryToDecimalLong(string bin, int begin, int end);
long signedBinaryToDecimal(string bin, int begin, int end);
string registerDecode(long regNum);

int main(int argc, char **argv)
{
	cout << endl;
	cout << "Enter the program file name to run: " << endl;
	string file;
	cin >> file;
	ifstream inst_list;
	inst_list.open("sample_binary.txt");
	register_file[9] = 32;
	register_file[10] = 5;
	register_file[16] = 112;
	d_mem[28] = 5;
	d_mem[29] = 16;

	cout << endl;
	instruction *next_inst;
	list< instruction > pipeline;  //keeping the different instruction structs in a list that allows storing them in a FIFO ordering

	int file_length;
	inst_list.seekg(0,inst_list.end);  //finds the length of the file and divides to find the number of instructions in the file
	file_length = inst_list.tellg();
	file_length = file_length/34;
	inst_list.seekg(0,inst_list.beg);

	do
	{
	    total_clock_cycles++;
	    cout << "\ntotal_clock_cycles: " << total_clock_cycles << endl;
	    
	    prev_dest_2 = prev_dest;              //for each time step updates the prev to the second prev           
	    prev_dest = -1;                       //sets prev to -1 as a default in case the next instruction doesn't update a register
		
		if(stall == 0 && PC/4 < file_length)             //As long as we aren't currently stalling and we haven't read the last instruction, read in the next instruction pointed to by PC
		{
			inst_list.seekg(34*PC/4,inst_list.beg);         //points to the instruction indicated by the PC value 34 is the length of a line in the txt document
			next_inst = new instruction;
			getline(inst_list,next_inst->inst);                        //read in the next instruction and store it at the end of the pipeline
			pipeline.push_back(*next_inst);
			PC = PC + 4;
			cout << "PC is modified to " << std::hex << PC << endl;
		}
		if(stall > 0)                                   //if the global stall is > 0 meaning we skipped reading an instruction decrement it.
			stall--;
        list<instruction>::iterator it = pipeline.begin();      //point the iterator to the beginning of the pipeline
		for(int i = 0; i < pipeline.size(); i++)          //for all of the instructions in the pipeline perform the appropriate stage
		{
			if(flush_inst == 1)              //if there is a control hazard in the last iteration of the for loop flush_inst will be set
            {
				pipeline.erase(it,pipeline.end());          //it will then remove all instructions from it to the end of the pipeline
				break; 
            }
			else                                //otherwise it calls the control function and increments it
			{
				pipeline_control(*it);
				++it;
			}
		}
		flush_inst = 0;                    //makes sure to set flush_inst back to 0 after exiting
		if(inst_finish == 1)               //once an instruction completes writeback set remove to 1 and remove it from the front of the pipeline
		{
			pipeline.pop_front();
			inst_finish = 0;
		}
	}while(pipeline.size() != 0);          //loops while the pipeline isn't empty
}

void pipeline_control(instruction &inst)       //control function that calls the appropriate function for an instruction
{
	if(inst.stall == 0)                  //if the local stall variable is not set for this instruction it increments the stage and then calls the corresponding function 
	{
	    inst.stage++;
		if(inst.stage == 1)
			fetch(inst);
		if(inst.stage == 2)
			decode(inst);
		if(inst.stage == 3)
			execute(inst);
		if(inst.stage == 4)
			mem(inst);
		if(inst.stage == 5)
			writeback(inst);
	}
	else                        //otherwise decrements the local stall variable and moves on 
    {
		inst.stall--;
    }
}

void fetch(instruction &inst)      //fetch function
{
	if(stall_next == 1)            //if the previous instruction detected a data hazard this instruction will also be stalled the same number of time steps
	{
		inst.stall = stall;
		stall_next = 0;
	}
	inst.start_PC = PC;            //sets the start pc for branch target address calculation
}

void decode(instruction &instr)       //decode function
{ 
	instr.op_code = binaryToDecimalLong(instr.inst, 0, 5);        //decodes the opcode to identify the type

	if(instr.op_code == 0)  //R type
	{
		instr.rs = binaryToDecimalLong(instr.inst, 6, 10);
		instr.rt = binaryToDecimalLong(instr.inst, 11, 15);
		instr.rd = binaryToDecimalLong(instr.inst, 16, 20);
		instr.shamt = binaryToDecimalLong(instr.inst, 21, 25);
		instr.funct = binaryToDecimalLong(instr.inst, 26, 31);
	}
	else if(instr.op_code <= 3)  //J type
	{
		instr.jump_target = binaryToDecimalLong(instr.inst, 6, 31);
		instr.jump_target = instr.jump_target * 4;
	}
	else   //I type
	{
		instr.rs = binaryToDecimalLong(instr.inst, 6, 10);
		instr.rt = binaryToDecimalLong(instr.inst, 11, 15);
		instr.imm = signedBinaryToDecimal(instr.inst, 16, 31);
	}
	if(instr.rs == prev_dest_2 || instr.rt == prev_dest_2)    //if either rs or rt is equal to the destination of the second to last instruction
	{
		stall = 1;                                                //we detected a data hazard and will stall one cycle
		instr.stall = stall;
		stall_next = 1;                                           //we will also stall the next instruction in fetch
		cout << "Data hazard detected (NOP 1)" << endl;
	}
	if(instr.rs == prev_dest || instr.rt == prev_dest)        //if rs or rt are equal to the destination of the last instruction we stall 2 cycles
	{
		stall = 2;
		instr.stall = stall;
		stall_next = 1;
		cout << "Data hazard detected (NOP 2)" << endl;
	}

	if(instr.op_code == 0)  //if it is an r type instruction
	{
		prev_dest_2 = prev_dest;       //set the last destination to rd
		prev_dest = instr.rd;
	}
	if(instr.op_code == 35)     //if it is load word
    {
        prev_dest_2 = prev_dest;      //set the last destination to rt 
        prev_dest = instr.rt;
    }

	control_unit(instr);             //generates the control signals for the instruction
}
void control_unit(instruction &instr) //control unit sets the various control signals based on the types of instructions
{
	if(instr.op_code == 0)
	{
		instr.REGDST = 1;
		instr.REGWRITE = 1;
		instr.INSTTYPE = 2;
	}
	else if(instr.op_code == 35)
	{
		instr.ALUSRC = 1;
		instr.MEMTOREG = 1;
		instr.REGWRITE = 1;
		instr.MEMREAD = 1;
	}
	else if(instr.op_code == 43)
	{
		instr.ALUSRC = 1;
		instr.MEMWRITE = 1;
	}
	else if(instr.op_code == 4)
	{
		instr.BRANCH = 1;
		instr.INSTTYPE = 1;
	}
	else if(instr.op_code == 2)
	{
		instr.JUMP = 1;
	}

	if(instr.INSTTYPE == 0)
	{
		instr.ALUOPCODE = 2;
	}
	else if(instr.INSTTYPE == 1)
	{
		instr.ALUOPCODE = 6;
	}
	else if(instr.INSTTYPE == 2)
	{
		if(instr.funct == 32)
			instr.ALUOPCODE = 2;
		if(instr.funct == 34)
			instr.ALUOPCODE = 6;
		if(instr.funct == 36)
			instr.ALUOPCODE = 0;
		if(instr.funct == 37)
			instr.ALUOPCODE = 1;
		if(instr.funct == 42)
			instr.ALUOPCODE = 7;
		if(instr.funct == 39)
			instr.ALUOPCODE = 12;
	}
}

void execute(instruction &instr)         //execute function
{
	if(instr.ALUOPCODE == 0)      //AND
		instr.ALU_value = register_file[instr.rs] & register_file[instr.rt];
	if(instr.ALUOPCODE == 1)      //OR
		instr.ALU_value = register_file[instr.rs] | register_file[instr.rt];
	if(instr.ALUOPCODE == 2)      //ADD and LW and SW
    {
        if(instr.MEMREAD == 0 && instr.MEMWRITE == 0 && instr.REGWRITE == 0) //specifically for AND
			instr.ALU_value = register_file[instr.rs] + register_file[instr.rt];
    }
	if(instr.ALUOPCODE == 6)     //SUB and BEQ
	{
		if(instr.BRANCH == 1)  //if BEQ
		{
			if(register_file[instr.rs] - register_file[instr.rt] == 0)  //and rs = rt 
			{
				instr.alu_zero = 1;           //set the bit to tell the cpu to branch
				instr.branch_target = instr.imm * 4 + instr.start_PC;          //and calculate the branch target address
			}
        }
		else     //otherwise just subtract
            instr.ALU_value = register_file[instr.rs] - register_file[instr.rt];
	}
	if(instr.ALUOPCODE == 7)     //SLT
		instr.ALU_value = register_file[instr.rs] < register_file[instr.rt];
	if(instr.ALUOPCODE == 12)    //NOR
		instr.ALU_value = !(register_file[instr.rs] | register_file[instr.rt]);
}

void mem(instruction &instr)    //mem function
{
	if(instr.REGWRITE == 1 && instr.MEMREAD == 1 && instr.MEMWRITE == 0)    //LW
	{

		long mem_address = (register_file[instr.rs] + instr.imm)/4;   //calculates the memory address and divides by 4 as we are dealing with an array of length 32
		instr.ALU_value = d_mem[mem_address];                         //stores the value from memory to write back to rt
	}
	else if(instr.REGWRITE == 0 && instr.MEMREAD == 0 && instr.MEMWRITE == 1)      //SW
	{
		long mem_address = register_file[instr.rs] + instr.imm;          //calculates the memory address to store to
		d_mem[mem_address] = register_file[instr.rt];                    //stores the value from rt into memory
		cout << "memory 0x" << std::hex << mem_address << " is modified to 0x" << std::hex << d_mem[mem_address] << endl;
	}
	if(instr.alu_zero == 1)         //if we are branching
	{
		PC = instr.branch_target;    //set PC to the branch target address and flush the next 3 instructions 
		flush_inst = 1;
		cout << "Control hazard detected (flush 3 instructions)" << endl;
		cout << "PC is modified to " << std::hex << PC << endl;
	}
	if(instr.JUMP == 1)               //if we are jumping
	{
		PC = instr.jump_target;         //set the PC to the jump target address and flush the next 3 instructions
		flush_inst = 1;
		cout << "Control hazard detected (flush 3 instructions)" << endl;
		cout << "PC is modified to " << std::hex << PC << endl;
	}
}

void writeback(instruction &instr)     //writeback function
{
	if(instr.REGWRITE == 1 && instr.MEMREAD == 1 && instr.MEMWRITE == 0)       //LW
	{
		register_file[instr.rt] = instr.ALU_value;      //write the value from mem to rt
		cout << registerDecode(instr.rt) << " is modified to 0x" << std::hex << instr.ALU_value << endl;
	}
	else if(instr.funct == 32 || instr.funct == 34 || instr.funct == 36 || instr.funct == 37 || instr.funct == 39 || instr.funct == 42 )  //R types
	{
		register_file[instr.rd] = instr.ALU_value;        //writes the value from the ALU to rd
		cout << registerDecode(instr.rd) << " is modified to 0x" << std::hex << instr.ALU_value << endl;
	}
	inst_finish = 1;        //sets finish so that the instruction will be removed from the pipeline as it is done
}

long binaryToDecimalLong(string bin, int begin, int end)       //unsigned binary conversion function returns a long
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

long signedBinaryToDecimal(string bin, int begin, int end)      //signed binary conversion function returns a long
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
