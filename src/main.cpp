#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include "configreader.h"
#include "process.h"

// Shared data for all cores
typedef struct SchedulerData {
    std::mutex mutex;
    std::condition_variable condition;
    ScheduleAlgorithm algorithm;
    uint32_t context_switch;
    uint32_t time_slice;
    std::list<Process*> ready_queue;
    bool all_terminated;
} SchedulerData;

void coreRunProcesses(uint8_t core_id, SchedulerData *data);
int printProcessOutput(std::vector<Process*>& processes, std::mutex& mutex);
void clearOutput(int num_lines);
uint64_t currentTime();
std::string processStateToString(Process::State state);

int main(int argc, char **argv)
{
    // Ensure user entered a command line parameter for configuration file name
    if (argc < 2)
    {
        std::cerr << "Error: must specify configuration file" << std::endl;
        exit(EXIT_FAILURE);
    }

    // Declare variables used throughout main
    int i;
    SchedulerData *shared_data;
    std::vector<Process*> processes;

    // Read configuration file for scheduling simulation
    SchedulerConfig *config = readConfigFile(argv[1]);

    // Store configuration parameters in shared data object
    uint8_t num_cores = config->cores;
    shared_data = new SchedulerData();
    shared_data->algorithm = config->algorithm;
    shared_data->context_switch = config->context_switch;
    shared_data->time_slice = config->time_slice;
    shared_data->all_terminated = false;

    // Create processes
    uint64_t start = currentTime();
    for (i = 0; i < config->num_processes; i++)
    {
        Process *p = new Process(config->processes[i], start);
        processes.push_back(p);
        // If process should be launched immediately, add to ready queue
        if (p->getState() == Process::State::Ready)
        {
            shared_data->ready_queue.push_back(p);
        }
    }

    // Free configuration data from memory
    deleteConfig(config);

    // Launch 1 scheduling thread per cpu core
    std::thread *schedule_threads = new std::thread[num_cores];
    for (i = 0; i < num_cores; i++)
    {
        schedule_threads[i] = std::thread(coreRunProcesses, i, shared_data);
    }

    // Main thread work goes here
    int num_lines = 0;
    while (!(shared_data->all_terminated))
    {
        // Clear output from previous iteration
        clearOutput(num_lines);

        // Do the following:
        //   - Get current time
        //   - *Check if any processes need to move from NotStarted to Ready (based on elapsed time), and if so put that process in the ready queue
        //  How can i tell if processes need to move baesd on elapsed time?
        //  How do I track elapsed time?

        //   - *Check if any processes have finished their I/O burst, and if so put that process back in the ready queue
        //  How can I tell if a process is done with its IO burst?

        //   - *Check if any running process need to be interrupted (RR time slice expires or newly ready process has higher priority)
        //  Isn't this handled in coreRunProcesses?

        //   - *Sort the ready queue (if needed - based on scheduling algorithm)
        //  Think I use the comparators here but idk how
        //if (shared_data->algorithm == ScheduleAlgorithm::SJF) {

        //} else if (shared_data->algorithm == ScheduleAlgorithm::PP) {

        //}

        //   - Determine if all processes are in the terminated state
        //  This is handled by the while condition

        //   - * = accesses shared data (ready queue), so be sure to use proper synchronization

        // output process status table
        num_lines = printProcessOutput(processes, shared_data->mutex);

        // sleep 50 ms
        usleep(50000);
    }


    // wait for threads to finish
    for (i = 0; i < num_cores; i++)
    {
        schedule_threads[i].join();
    }


    // print final statistics
    //  - CPU utilization
    double cpu_time = 0;
    double total_time = static_cast<double>(currentTime());

    for (int i = 0; i < sizeof(processes); i++)
    {
        cpu_time = cpu_time + processes[i]->getTurnaroundTime();
    }

    std::cout << "CPU Utilization: " << (cpu_time/total_time);

    //  - Throughput
    //     - Average for first 50% of processes finished
    double first_processes_time = 0;

    for (int i = 0; i < (sizeof(processes)/2); i++)
    {
        first_processes_time = first_processes_time + processes[i]->getTurnaroundTime();
    }
    std::cout << "Average of First 50% of Processes Finished: " << ((processes.size()/2)/first_processes_time);

    //     - Average for second 50% of processes finished
    double second_processes_time = 0;

    for (int i = (sizeof(processes)/2); i < sizeof(processes); i++)
    {
        second_processes_time = second_processes_time + processes[i]->getTurnaroundTime();
    }
    std::cout << "Average of Second 50% of Processes Finished: " << ((processes.size() - processes.size()/2)/second_processes_time);

    //     - Overall average of processes finished
    std::cout << "Overall Average of Processes Finished: " << (processes.size()/(first_processes_time + second_processes_time));

    //  - Average turnaround time
    double total_turn_time = 0;

    for (int i = 0; i < sizeof(processes); i++)
    {
        total_turn_time = total_turn_time + processes[i]->getTurnaroundTime();
    }
    std::cout << "Average Turnaround Time: " << (total_turn_time/processes.size());

    //  - Average waiting time
    double total_wait_time = 0;
    
    for (int i = 0; i < processes.size(); i++)
    {
        total_wait_time = total_wait_time + processes[i]->getWaitTime();
    }
    std::cout << "Average Waiting Time: " << (total_wait_time/processes.size());


    // Clean up before quitting program
    processes.clear();

    return 0;
}

void coreRunProcesses(uint8_t core_id, SchedulerData *shared_data)
{
    // Work to be done by each core idependent of the other cores
    // Repeat until all processes in terminated state:

    //  Is this current while loop set up properly? Do I run until the ready queue is empty or until all_terminated?
    while (!(shared_data->all_terminated)) {
        //  Is my pointer setup correct? 
        Process* current = shared_data->ready_queue.front();
        shared_data->ready_queue.pop_front();
        current->setState(Process::State::Running, currentTime());

        //  Do I need this setBurstStartTime line -- It doesn't work, currentTime con
        current->setBurstStartTime(currentTime());

        uint64_t timeElapsed = currentTime() - current->getBurstStartTime();
        uint16_t currentBurst = current->getCurrentBurst();
        uint32_t currentBurstTime = current->getCurrentBurstTime();
        uint16_t processBurstNum = current->getNumberOfBursts();

        while (timeElapsed < currentBurstTime) {
            //  This seems wrong, interrupt is set to true by default
            if (current->isInterrupted() == true) {
                timeElapsed = currentTime() - current->getBurstStartTime();
                current->updateBurstTime(currentBurst, currentBurstTime - timeElapsed);
                current->setState(Process::State::Ready, currentTime());
                usleep(shared_data->context_switch);
                shared_data->ready_queue.push_back(current);
                break;
            } else {
                timeElapsed = currentTime() - current->getBurstStartTime();
            }
        }

        //  If it gets to this point, the burst is done

        //  Update burst time to reflect no time left on that burst
        current->updateBurstTime(currentBurst, currentBurstTime - timeElapsed);

        //  Are these two IO/Terminated switches done correctly?

        //  Done with current burst but more bursts remain == on to an IO burst
        if (processBurstNum > currentBurst + 1) {
            current->setBurstStartTime(currentTime());
            current->moveToNextBurst();
            //Context Switch wait time
            usleep(shared_data->context_switch);
            current->setState(Process::State::IO, currentTime()); 
        }

        //  Done with current burst and no more bursts remain == terminate
        if (processBurstNum == currentBurst + 1) {
            //Context Switch wait time
            usleep(shared_data->context_switch);
            current->setState(Process::State::Terminated, currentTime());
        }
    }

    //   - *Get process at front of ready queue - DONE
    //   - Simulate the processes running until one of the following:
    //     - CPU burst time has elapsed  -- Done I think
    //     - Interrupted (RR time slice has elapsed or process preempted by higher priority process) --- How does coreRunProcesses handle an interrupt?

    //  - Place the process back in the appropriate queue
    //     - I/O queue if CPU burst finished (and process not finished) -- no actual queue, simply set state to IO -- Done I think
    //     - Terminated if CPU burst finished and no more bursts remain -- no actual queue, simply set state to Terminated -- Done I think
    //     - *Ready queue if interrupted (be sure to modify the CPU burst time to now reflect the remaining time) -- Done I think
    //  - Wait context switching time -- DONE
    //  - * = accesses shared data (ready queue), so be sure to use proper synchronization
}

int printProcessOutput(std::vector<Process*>& processes, std::mutex& mutex)
{
    int i;
    int num_lines = 2;
    std::lock_guard<std::mutex> lock(mutex);
    printf("|   PID | Priority |      State | Core | Turn Time | Wait Time | CPU Time | Remain Time |\n");
    printf("+-------+----------+------------+------+-----------+-----------+----------+-------------+\n");
    for (i = 0; i < processes.size(); i++)
    {
        if (processes[i]->getState() != Process::State::NotStarted)
        {
            uint16_t pid = processes[i]->getPid();
            uint8_t priority = processes[i]->getPriority();
            std::string process_state = processStateToString(processes[i]->getState());
            int8_t core = processes[i]->getCpuCore();
            std::string cpu_core = (core >= 0) ? std::to_string(core) : "--";
            double turn_time = processes[i]->getTurnaroundTime();
            double wait_time = processes[i]->getWaitTime();
            double cpu_time = processes[i]->getCpuTime();
            double remain_time = processes[i]->getRemainingTime();
            printf("| %5u | %8u | %10s | %4s | %9.1lf | %9.1lf | %8.1lf | %11.1lf |\n", 
                   pid, priority, process_state.c_str(), cpu_core.c_str(), turn_time, 
                   wait_time, cpu_time, remain_time);
            num_lines++;
        }
    }
    return num_lines;
}

void clearOutput(int num_lines)
{
    int i;
    for (i = 0; i < num_lines; i++)
    {
        fputs("\033[A\033[2K", stdout);
    }
    rewind(stdout);
    fflush(stdout);
}

uint64_t currentTime()
{
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch()).count();
    return ms;
}

std::string processStateToString(Process::State state)
{
    std::string str;
    switch (state)
    {
        case Process::State::NotStarted:
            str = "not started";
            break;
        case Process::State::Ready:
            str = "ready";
            break;
        case Process::State::Running:
            str = "running";
            break;
        case Process::State::IO:
            str = "i/o";
            break;
        case Process::State::Terminated:
            str = "terminated";
            break;
        default:
            str = "unknown";
            break;
    }
    return str;
}
