#include <iostream>
#include <vector> 
#include <string>
#include <sstream>
#include <queue>
#include <limits> 


struct PCB
{
    int process_id;                             // ID of process
    int state;                                  // "new", "ready", "run","terminated" encoded as ints
    int program_counter;                        // index of next instruction to be executed, in logical memory
    int instruction_base;                       // starting address of instructions for this process
    int data_base;                              // address where the data for the instructions starts in logical memory
    int memory_limit;                           // total size of logical memory allocated to the process    
    int CPU_cycles_used;                        // number of cpu cycles process as consumed so far
    int register_value;                         // value of cpu register associated with process
    int max_memory_needed;                      // max logical memory required by process as defined in input file
    int main_memory_base;                       // starting address in main memory where process, PCB+logical_memory is loaded.  

    std::vector<std::vector<int>> instructions; // each vector is a instruction, each element in the vector is data associated with instruction, 1st element being the opcode
};

// ** FUNCTION PROTOTYPES ORDERED BY APPEARANCE BY CALL ** //
void show_PCB(PCB process);

void loadJobsToMemory(std::queue<PCB> &newJobQueue, std::queue<int> &readyQueue, std::vector<int> &mainMemory, int maxMemory);

void executeCPU(int startAddress, std::vector<int> &mainMemory);

void show_main_memory(std::vector<int> &mainMemory, int rows);

int main(int argc, char** argv) 
{
    // Step 1: Read and parse input file
    // TODO: Implement input parsing and populate newJobQueue

    // declare the variables to store the first two entires in a file
    int maxMemory;
    int num_processes;

    // declare the data structures to store the processes and main memory
    std::vector<int> mainMemory;
    std::queue<int> readyQueue;
    std::queue<PCB> newJobQueue;

    // read in data from file
    std::cin >> maxMemory;
    std::cin >> num_processes;

    mainMemory.resize(maxMemory, -1); // initialize main memory with -1 with size of maxMemory
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // this ignores any extra characters in the buffer namely the new line character, which would be picked up by the getline function in line 65

    /*
    * This loop will fun for the number of process in the file (we know this value as it's the 2nd number in the file) and we will use a string to grab the line
    * we then create a string stream from it which allows for us to extract data from the file. This wasn't needed for the sample files but in the larger files when the text
    * wraps we pick up the new line character and it causes issues. This allows us to ignore the new line character and just read in the data.
    */
    for (unsigned int i = 0; i < num_processes; i++) 
    {
        std::string line;
        std::getline(std::cin, line);  // grab the entire line

        std::istringstream ss(line);  // create a string stream to parse the line

        int process_id, max_memory_needed, number_of_instructions;  
        ss >> process_id >> max_memory_needed >> number_of_instructions;   // read in first 3 variables of process

        PCB process;
        process.process_id = process_id;
        process.max_memory_needed = max_memory_needed;
        process.state = 0; 
        process.program_counter = 0;
        process.memory_limit = process.max_memory_needed;
        process.CPU_cycles_used = 0;
        process.register_value = 0;
    

        // iterate the instructions of each process
        for (unsigned int j = 0; j < number_of_instructions; j++) 
        {
            int instruction_opcode;  // read in opcode based on that read in data of instruction
            ss >> instruction_opcode;

            std::vector<int> current_instruction;  // instruction-arr for cur-instruction
            current_instruction.push_back(instruction_opcode); // add opcode as first element for this instruction-arr

            if (instruction_opcode == 1) // 1: compute
            {
                int iterations, cycles;
                ss >> iterations >> cycles;
                current_instruction.push_back(iterations);
                current_instruction.push_back(cycles);

                //std::cout << "instruction: " << instruction_opcode << " iter: " << iterations << " cycles: " << cycles << std::endl;
            } 
            if (instruction_opcode == 2) // 2:  print 
            { 
                int cycles;
                ss >> cycles;
                current_instruction.push_back(cycles);

                //std::cout << "instruction: " << instruction_opcode << " cycles: " << cycles << std::endl;
            } 
            if (instruction_opcode == 3) // 3:  store
            {   
                int value, address;
                ss >> value >> address;
                current_instruction.push_back(value);  
                current_instruction.push_back(address);

                //std::cout << "instruction: " << instruction_opcode << " value: " << value << " address: " << address << std::endl;
            }
            if (instruction_opcode == 4) // 4:  load
            {
                int address;
                ss >> address;
                current_instruction.push_back(address);

                //std::cout << "instruction: " << instruction_opcode << " address: " << address << std::endl;
            }

            process.instructions.push_back(current_instruction);
        }

        // push the PCB to new-job-queue
        newJobQueue.push(process);  
    }
    std::cout << std::endl;


    
    // Step 2: Load jobs into main memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, maxMemory);

    // print main memory
    for (int i = 0; i < mainMemory.size(); i++)
    {
        std::cout << i << " : " << mainMemory[i] << "\n";
    }


    // // Step 4: Process execution
    while (!readyQueue.empty())
    {
        int PCB_start_address = readyQueue.front();
        readyQueue.pop();
        executeCPU(PCB_start_address, mainMemory);
    }

    return 0;
} // END OF MAIN


void show_PCB(PCB process) 
{
    std::cout << "PROCESS ["<<process.process_id<<"] " << "maxMemoryNeeded: " << process.max_memory_needed << std::endl;
    std::cout << "num-instructions: " << process.instructions.size() << std::endl;

    for (unsigned int i=0; i<process.instructions.size(); i++) 
    {
        std::vector<int> current_instruction = process.instructions[i];

        if (current_instruction[0] == 1) // compute: 2 parameters
        { 
            std::cout << "instruction: " << current_instruction[0] << " iter: " << current_instruction[1] << " cycles: " << current_instruction[2] << std::endl;
        }
        if (current_instruction[0] == 2) // print: 2 parameters
        {
            std::cout << "instruction: " << current_instruction[0] << " cycles: " << current_instruction[1] << std::endl;
        }
        if (current_instruction[0] == 3) // write: 2 parameters 
        {
            std::cout << "instruction: " << current_instruction[0] << " value: " << current_instruction[1] << " address: " << current_instruction[2] << std::endl;
        }
        if (current_instruction[0] == 4) // load: 2 parameters
        {
            std::cout << "instruction: " << current_instruction[0] << " address: " << current_instruction[1] << std::endl;
        }
        
    }// END FOR LOOP
} //  END show_PCB


void loadJobsToMemory(std::queue<PCB>& newJobQueue, std::queue<int>& readyQueue, std::vector<int>& mainMemory, int maxMemory) 
{

    int current_address = 0;

    while (!newJobQueue.empty()) 
    {
        PCB current_process = newJobQueue.front();  // access front element
        newJobQueue.pop();  

        current_process.main_memory_base = current_address;
        current_process.instruction_base = current_address + 10; 
        current_process.data_base = current_process.instruction_base + current_process.instructions.size();

        mainMemory[current_address] = current_process.process_id;
        mainMemory[current_address + 1] = 1;
        mainMemory[current_address + 2] = current_process.program_counter;
        mainMemory[current_address + 3] = current_process.instruction_base;
        mainMemory[current_address + 4] = current_process.data_base;
        mainMemory[current_address + 5] = current_process.memory_limit;
        mainMemory[current_address + 6] = current_process.CPU_cycles_used;
        mainMemory[current_address + 7] = current_process.register_value;
        mainMemory[current_address + 8] = current_process.max_memory_needed;
        mainMemory[current_address + 9] = current_process.main_memory_base;

        int num_instructions = current_process.instructions.size();

        int instruction_address = current_process.instruction_base;
        int data_address = current_process.data_base;

        for (unsigned int i = 0; i < num_instructions; i++) 
        {
            std::vector<int> current_instruction = current_process.instructions[i];
            int op_code = current_instruction[0];
           
            mainMemory[instruction_address++] = op_code; 

            if (op_code == 1) // compute
            {  
                mainMemory[data_address++] = current_instruction[1];
                mainMemory[data_address++] = current_instruction[2];
            }
            else if (op_code == 2) // print
            { 
                mainMemory[data_address++] = current_instruction[1];
            }
            else if (op_code == 3) // store
            { 
                mainMemory[data_address++] = current_instruction[1];
                mainMemory[data_address++] = current_instruction[2];
            }
            else if (op_code == 4) // load
            {  
                mainMemory[data_address++] = current_instruction[1];
            }
        }

        current_address = current_process.instruction_base + current_process.max_memory_needed;  // change to the next process
        readyQueue.push(current_process.main_memory_base);  // push the base-address of process

    } // END WHILE
} // END FUNCTION

void executeCPU(int startAddress, std::vector<int>&  mainMemory) 
{
    
    // pull metadata of PCB from main memory
    int process_id = mainMemory[startAddress];
    int state = mainMemory[startAddress + 1];
    int program_counter = mainMemory[startAddress + 2];  // create temporary variables that do no modify memory just yet!!
    int instruction_base = mainMemory[startAddress + 3];
    int data_base = mainMemory[startAddress + 4];
    int memory_limit = mainMemory[startAddress + 5];
    int CPU_cycles_used = mainMemory[startAddress + 6];
    int register_value = mainMemory[startAddress + 7];
    int max_memory_needed = mainMemory[startAddress + 8];
    int main_memory_base = mainMemory[startAddress + 9];

    // number of instructions or opcodes is distance between database and isntruction_base
    int num_instructions = data_base - instruction_base; 
    int data_size = memory_limit - num_instructions; // size of data-segment


    // index in the data segment of instructions
    int memory_index = 0; 

    // iterate number of instructions or opcodes
    for (unsigned int i=0; program_counter < num_instructions; i++) 
    {
        int current_op_code = mainMemory[instruction_base + i]; // get current opcode in memory, using address of thwere instructions start plus ith instruction
        std::vector<int> current_instruction_data;  // each element is the parameters for the current instruction/opcode

        // compute: has 2 parameters
        if (current_op_code == 1) 
        {
            if (memory_index + 1 >= data_size) // if not enough parameters to do a compute operation push -1 not possible
            {  
                current_instruction_data.push_back(-1);
                current_instruction_data.push_back(-1);
            } 
            else 
            {
                current_instruction_data.push_back(mainMemory[data_base + memory_index]);  // iterations
                current_instruction_data.push_back(mainMemory[data_base + memory_index + 1]);  // cycles
            }
            memory_index += 2;
        }

        // store: has 2 parameters
        else if (current_op_code == 3) 
        {
            if (memory_index + 1 >= data_size) 
            {
                current_instruction_data.push_back(-1);
                current_instruction_data.push_back(-1);
            } 
            else 
            {
                current_instruction_data.push_back(mainMemory[data_base + memory_index]);   // value
                current_instruction_data.push_back(mainMemory[data_base + memory_index + 1]);  // address
            }
            memory_index += 2;
        }

        // load: has 1 parameter
        else if (current_op_code == 4) 
        {
            if (memory_index >= data_size) 
            {
                current_instruction_data.push_back(-1);
            } 
            else 
            {
                current_instruction_data.push_back(mainMemory[data_base + memory_index]);  // address
            }
            memory_index += 1;
        }

        // print: has 1 parameter
        else if (current_op_code == 2) 
        {
            if (memory_index >= data_size) 
            {
                current_instruction_data.push_back(-1);
            } 
            else 
            {
                current_instruction_data.push_back(mainMemory[data_base + memory_index]);
            }
            memory_index += 1;
        }


        // process each instruction opcode and update the parameters

        if (current_op_code == 1) // COMPUTE
        {
            CPU_cycles_used += current_instruction_data[1];
            std::cout << "compute" << "\n";
        }
        else if (current_op_code == 2) // PRINT
        {
            CPU_cycles_used += current_instruction_data[0];
            std::cout << "print" << "\n";
        }
        else if (current_op_code == 3) // STORE
        {
            // check if we are inside data segment
            if (current_instruction_data[1] + instruction_base >= instruction_base && (current_instruction_data[1] + instruction_base) < max_memory_needed + instruction_base) 
            { 
                mainMemory[current_instruction_data[1] + instruction_base] = current_instruction_data[0];
                register_value = current_instruction_data[0];
                std::cout << "stored" << "\n";
            } 
            else 
            {
                register_value = current_instruction_data[0];
                std::cout << "store error!" << "\n";
            }
            CPU_cycles_used++;
        }
        // LOAD
        else if (current_op_code == 4) 
        {
            // check if we are inside data segment
            if ((current_instruction_data[0] + instruction_base) >= instruction_base && (current_instruction_data[0] + instruction_base) < (max_memory_needed + instruction_base)) 
            {
                register_value = mainMemory[current_instruction_data[0] + instruction_base];
                std::cout << "loaded" << "\n";
            } 
            else
            {
                std::cout << "load error!" << "\n";
            }
            CPU_cycles_used++;
        }
        // invalid opcode
        else 
        {
            std::cerr << "ERROR: Invalid opcode " << current_op_code << "\n";
        }

        program_counter++; // increment program counter
    } // END I FOR LOOP

    mainMemory[startAddress + 1] = 4;                    // terminate process
    mainMemory[startAddress + 2] = instruction_base - 1; // update program counter for this PCB, to be before instructionBase
    mainMemory[startAddress + 6] = CPU_cycles_used;
    mainMemory[startAddress + 7] = register_value;

    // Print PCB information.
    std::cout << "Process ID: " << process_id << "\n";
    std::cout << "State: TERMINATED\n";
    std::cout << "Program Counter: " << mainMemory[startAddress + 2] << "\n";
    std::cout << "Instruction Base: " << instruction_base << "\n";
    std::cout << "Data Base: " << data_base << "\n";
    std::cout << "Memory Limit: " << memory_limit << "\n";
    std::cout << "CPU Cycles Used: " << CPU_cycles_used << "\n";
    std::cout << "Register Value: " << register_value << "\n";
    std::cout << "Max Memory Needed: " << max_memory_needed << "\n";
    std::cout << "Main Memory Base: " << main_memory_base << "\n";
    std::cout << "Total CPU Cycles Consumed: " << CPU_cycles_used << "\n";
} // END FUNCTION

void show_main_memory(std::vector<int> &mainMemory, int rows) 
{
    for (unsigned int i = 0; i < rows; i++)
    {
        std::cout << i << ": " << mainMemory[i] << std::endl;
    }

    std::cout << std::endl;
} // END FUNCTION