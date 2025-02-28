#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <queue>
#include <limits>
#include <fstream>

// ------------------------------------------------------------------
// *** CHANGES FOR PROJECT 2 ***
// 1) A global CPU clock
// 2) Reading context_switch_time, CPU_allocated_time from the input
// 3) Round-robin logic: timeouts, I/O interrupts
// 4) Logging transitions exactly as required
// ------------------------------------------------------------------

int CPU_clock = 0; 
int context_switch_time = 2; 
int CPU_allocated_time = 5; 
int totalCPUtimeAllProcesses = 0; // Sum of (termination_time – first_run_time) for each process

std::ofstream fout("out.txt"); // For writing to "out.txt"

// ------------------------------------------------------------------
// Basic PCB structure
// ------------------------------------------------------------------
struct PCB
{
    int process_id;
    int state;             // 0=NEW, 1=READY, 2=RUNNING, 3=IOWAIT, 4=TERMINATED
    int program_counter;
    int instruction_base;  
    int data_base;
    int memory_limit;
    int CPU_cycles_used;
    int register_value;
    int max_memory_needed;
    int main_memory_base;

    // For storing instructions read from input (not always required, but used for reference)
    std::vector<std::vector<int>> instructions;
};

// ------------------------------------------------------------------
// Function Prototypes
// ------------------------------------------------------------------
void loadJobsToMemory(std::queue<PCB> &newJobQueue,std::queue<int> &readyQueue,std::vector<int> &mainMemory,int maxMemory);

int executeCPU(int startAddress, std::vector<int> &mainMemory);

void show_main_memory(const std::vector<int> &mainMemory);

// ------------------------------------------------------------------
// main
// ------------------------------------------------------------------
int main(int argc, char **argv)
{
    // 1) Read input: max_memory, context_switch_time, CPU_allocated_time, num_processes
    int max_memory, num_processes;
    std::cin >> max_memory;
    std::cin >> context_switch_time;
    std::cin >> CPU_allocated_time;
    std::cin >> num_processes;


    // 2) Prepare data structures
    std::vector<int> mainMemory(max_memory, -1); 
    std::queue<int> readyQueue;          
    std::queue<PCB> newJobQueue;

    // 3) Read each process block from stdin
    for (int i = 0; i < num_processes; i++)
    {
        std::string line;
        std::getline(std::cin, line);

        // Sometimes the line might be empty if there's an extra newline, so skip it
        if (line.empty())
        {
            std::getline(std::cin, line);
        }
        std::istringstream ss(line);

        PCB p;
        int number_of_instructions;
        ss >> p.process_id >> p.max_memory_needed >> number_of_instructions;
        p.state           = 0; // NEW
        p.program_counter = 0;
        p.memory_limit    = p.max_memory_needed;
        p.CPU_cycles_used = 0;
        p.register_value  = 0;
        p.main_memory_base = 0; // Will be set in loadJobsToMemory

        // Read instructions from line
        for (int j = 0; j < number_of_instructions; j++)
        {
            int opcode;
            ss >> opcode;
            std::vector<int> inst;
            inst.push_back(opcode);

            if (opcode == 1) // compute
            {
                int iterations, cycles;
                ss >> iterations >> cycles;
                inst.push_back(iterations);
                inst.push_back(cycles);
            }
            else if (opcode == 2) // print
            {
                int cycles;
                ss >> cycles;
                inst.push_back(cycles);
            }
            else if (opcode == 3) // store
            {
                int val, addr;
                ss >> val >> addr;
                inst.push_back(val);
                inst.push_back(addr);
            }
            else if (opcode == 4) // load
            {
                int addr;
                ss >> addr;
                inst.push_back(addr);
            }
            p.instructions.push_back(inst);
        }
        newJobQueue.push(p);
    }

    // 4) Load all jobs to main memory
    loadJobsToMemory(newJobQueue, readyQueue, mainMemory, max_memory);

    // Optional: show main memory to confirm
    show_main_memory(mainMemory);

    // 5) Additional data structures for round-robin + IO queue
    std::queue<int> IOWaitingQueue;

    // For tracking the first time each PID enters running
    std::vector<int> startRunningTime(num_processes+1, -1);

    // 6) Round-robin scheduling loop
    while (!readyQueue.empty() || !IOWaitingQueue.empty())
    {
        // If the ready queue is empty but we still have processes in IO, CPU idle
        if (readyQueue.empty() && !IOWaitingQueue.empty())
        {
            CPU_clock += context_switch_time;

            // Move everything from IO to ready (in a naive approach)
            while (!IOWaitingQueue.empty())
            {
                int pcbAddr = IOWaitingQueue.front();
                IOWaitingQueue.pop();

                mainMemory[pcbAddr + 1] = 1; // ready
                int pid = mainMemory[pcbAddr];

                std::cout << "Process " << pid << " completed I/O and is moved to ReadyQueue\n";
                fout << "Process " << pid << " completed I/O and is moved to ReadyQueue\n";
                readyQueue.push(pcbAddr);
            }
            continue;
        }

        // Otherwise, pop front of readyQueue
        int pcbStart = readyQueue.front();
        readyQueue.pop();

        // Context switch overhead
        CPU_clock += context_switch_time;

        // Mark running
        mainMemory[pcbStart + 1] = 2; // RUNNING
        int pid = mainMemory[pcbStart]; // the process ID

        std::cout << "Process " << pid << " has moved to Running." << std::endl;
        fout << "Process " << pid << " has moved to Running." << std::endl;


        // Record time if first time running
        if (startRunningTime[pid] < 0)
        {
            startRunningTime[pid] = CPU_clock;
        }

        // Execute
        int rc = executeCPU(pcbStart, mainMemory);
        // rc: 0=terminated, 1=timeout, 2=IO interrupt

        if (rc == 0)
        {
            // Terminated
            mainMemory[pcbStart + 1] = 4; // state=TERMINATED
            int endTime = CPU_clock;
            int begin   = startRunningTime[pid];

            std::cout << "Process " << pid 
                      << " terminated. Entered running at: " << begin
                      << ". Terminated at: " << endTime
                      << ". Total Execution Time: " 
                      << (endTime - begin) << std::endl;

            fout << "Process " << pid 
                 << " terminated. Entered running at: " << begin
                 << ". Terminated at: " << endTime
                 << ". Total Execution Time: " 
                 << (endTime - begin) << std::endl;

            totalCPUtimeAllProcesses += (endTime - begin);
        }
        else if (rc == 1)
        {
            // Timeout
            std::cout << "Process " << pid << " has a TimeOUT interrupt and is moved to the ReadyQueue.\n";
            fout << "Process " << pid << " has a TimeOUT interrupt and is moved to the ReadyQueue.\n";

            // Mark ready
            mainMemory[pcbStart + 1] = 1;
            readyQueue.push(pcbStart);
        }
        else if (rc == 2)
        {
            // I/O
            std::cout << "Process " << pid << " issued an IOInterrupt and is moved to IOWaitingQueue.\n";
            fout << "Process " << pid << " issued an IOInterrupt and is moved to IOWaitingQueue.\n";

            mainMemory[pcbStart + 1] = 3; // IOWAIT
            IOWaitingQueue.push(pcbStart);
        }

    }

    // After all processes are done, we might add a final context switch time if required
    CPU_clock += context_switch_time;

    std::cout << "All processes complete. Total CPU time used: " << totalCPUtimeAllProcesses << std::endl;
    fout << "All processes complete. Total CPU time used: " << totalCPUtimeAllProcesses << std::endl;

    return 0;
}

// ------------------------------------------------------------------
// executeCPU
//   runs instructions up to CPU_allocated_time or until
//   a) process terminates
//   b) process hits I/O
//   c) times out
// Returns code: 
//   0 => terminated
//   1 => time-out
//   2 => IO interrupt
// ------------------------------------------------------------------
int executeCPU(int startAddress, std::vector<int> &mainMemory)
{
    // Pull PCB fields from mainMemory
    int pid              = mainMemory[startAddress];
    int state            = mainMemory[startAddress + 1];
    int program_counter  = mainMemory[startAddress + 2];
    int instruction_base = mainMemory[startAddress + 3];
    int data_base        = mainMemory[startAddress + 4];
    int memory_limit     = mainMemory[startAddress + 5];
    int cpu_used         = mainMemory[startAddress + 6];
    int reg_value        = mainMemory[startAddress + 7];
    int max_mem_needed   = mainMemory[startAddress + 8];
    // mainMemory[startAddress + 9] is just the base

    // # instructions
    int num_instructions = data_base - instruction_base;

    // Track how many CPU ticks used in this time slice
    int slice_used = 0;

    while (program_counter < num_instructions && slice_used < CPU_allocated_time)
    {
        int opcode = mainMemory[instruction_base + program_counter];

        // Retrieve parameters from data segment; minimal approach:
        int param1 = -1, param2 = -1;

        if (opcode == 1) // compute => 2 params
        {
            // param layout: data_base + (program_counter*2) and +1
            param1 = mainMemory[data_base + (program_counter * 2)];
            param2 = mainMemory[data_base + (program_counter * 2) + 1];
        }
        else if (opcode == 2) // print => 1 param
        {
            param1 = mainMemory[data_base + (program_counter)];
        }
        else if (opcode == 3) // store => 2 params
        {
            param1 = mainMemory[data_base + (program_counter * 2)];
            param2 = mainMemory[data_base + (program_counter * 2) + 1];
        }
        else if (opcode == 4) // load => 1 param
        {
            param1 = mainMemory[data_base + (program_counter)];
        }

        // Execute
        if (opcode == 1) // compute
        {
            // param2 is CPU cycles
            CPU_clock += param2;
            slice_used += param2;
            cpu_used   += param2;

            std::cout << "compute\n";
            fout << "compute\n";
        }
        else if (opcode == 2) // print => IO
        {
            CPU_clock   += param1;
            slice_used  += param1;
            cpu_used    += param1;

            std::cout << "print\n";
            fout << "print\n";

            // Save updated PCB
            mainMemory[startAddress + 2] = program_counter + 1; // next instruction
            mainMemory[startAddress + 6] = cpu_used;
            mainMemory[startAddress + 7] = reg_value;

            return 2; // indicates IO
        }
        else if (opcode == 3) // store
        {
            // store costs 1 CPU cycle
            CPU_clock++;
            slice_used++;
            cpu_used++;

            int storeAddress = instruction_base + param2;
            if (storeAddress >= instruction_base && storeAddress < (instruction_base + max_mem_needed))
            {
                mainMemory[storeAddress] = param1;
                reg_value = param1;
                std::cout << "stored\n";
                fout << "stored\n";
            }
            else
            {
                std::cout << "Process " << pid  << " store error! address " << storeAddress << "\n";
                fout << "Process " << pid  << " store error! address " << storeAddress << "\n";
            }
        }
        else if (opcode == 4) // load
        {
            // load costs 1 CPU cycle
            CPU_clock++;
            slice_used++;
            cpu_used++;

            int loadAddr = instruction_base + param1;
            if (loadAddr >= instruction_base && loadAddr < (instruction_base + max_mem_needed))
            {
                reg_value = mainMemory[loadAddr];
                std::cout << "loaded\n";
                std::cout << "loaded\n";
            }
            else
            {
                std::cout << "Process " << pid << " load error! address " << loadAddr << "\n";
                fout << "Process " << pid << " load error! address " << loadAddr << "\n";
            }
        }
        else
        {
            std::cout << "ERROR: invalid opcode " << opcode << " in process " << pid << "\n";
            fout << "ERROR: invalid opcode " << opcode << " in process " << pid << "\n";
        }

        program_counter++;

        // Timeout check
        if (slice_used >= CPU_allocated_time)
        {
            // Save updated PCB 
            mainMemory[startAddress + 2] = program_counter;
            mainMemory[startAddress + 6] = cpu_used;
            mainMemory[startAddress + 7] = reg_value;
            return 1; // time-out
        }
    }

    // If we exit loop, we check if the process is done
    if (program_counter >= num_instructions)
    {
        // Mark terminated
        mainMemory[startAddress + 1] = 4;  // TERMINATED
        mainMemory[startAddress + 2] = program_counter;
        mainMemory[startAddress + 6] = cpu_used;
        mainMemory[startAddress + 7] = reg_value;
        return 0; // done
    }

    // Otherwise we just end because we used up instructions? 
    // Typically means done, but you might interpret differently.
    return 0;
}

// ------------------------------------------------------------------
// loadJobsToMemory
//   moves from newJobQueue => mainMemory => readyQueue
// ------------------------------------------------------------------
void loadJobsToMemory(std::queue<PCB> &newJobQueue,std::queue<int> &readyQueue,std::vector<int> &mainMemory,int maxMemory)
{
    int current_address = 0;

    while (!newJobQueue.empty())
    {
        PCB p = newJobQueue.front();
        newJobQueue.pop();

        p.main_memory_base = current_address;
       
        p.instruction_base = current_address + 10; // PCB has 10 cells

        // data_base after instructions
        p.data_base = p.instruction_base + p.instructions.size();

        // Write PCB to main memory
        mainMemory[current_address + 0] = p.process_id; // ID
        mainMemory[current_address + 1] = 1;            // state => READY by default
        mainMemory[current_address + 2] = 0;            
        mainMemory[current_address + 3] = p.instruction_base; 
        mainMemory[current_address + 4] = p.data_base;
        mainMemory[current_address + 5] = p.memory_limit;
        mainMemory[current_address + 6] = p.CPU_cycles_used;
        mainMemory[current_address + 7] = p.register_value;
        mainMemory[current_address + 8] = p.max_memory_needed;
        mainMemory[current_address + 9] = p.main_memory_base;

        // Then instructions + parameters
        int instr_addr = p.instruction_base;
        int data_addr  = p.data_base;

        for (size_t i = 0; i < p.instructions.size(); i++)
        {
            const auto &inst = p.instructions[i];
            int opcode = inst[0];
            mainMemory[instr_addr++] = opcode;

            if (opcode == 1) // compute => {1, iterations, cycles}
            {
                mainMemory[data_addr++] = inst[1];
                mainMemory[data_addr++] = inst[2];
            }
            else if (opcode == 2) // print => {2, cycles}
            {
                mainMemory[data_addr++] = inst[1];
            }
            else if (opcode == 3) // store => {3, value, address}
            {
                mainMemory[data_addr++] = inst[1];
                mainMemory[data_addr++] = inst[2];
            }
            else if (opcode == 4) // load => {4, address}
            {
                mainMemory[data_addr++] = inst[1];
            }
        }

        // Next process: skip over the chunk of memory used by this process
        // We used 10 for PCB + p.max_memory_needed
        // That’s the minimal approach to avoid overlap.
        current_address += (10 + p.max_memory_needed);

        // The process is now READY. We push its base to the readyQueue
        readyQueue.push(p.main_memory_base);
    }
}

// ------------------------------------------------------------------
// show_main_memory
// ------------------------------------------------------------------
void show_main_memory(const std::vector<int> &mainMemory)
{
    for (int i = 0;i < mainMemory.size(); i++)
    {
        std::cout << i << " : " << mainMemory[i] << std::endl;
        fout << i << " : " << mainMemory[i] << std::endl;
    }
    std::cout << std::endl;
    fout << std::endl;
}
