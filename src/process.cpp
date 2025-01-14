#include "process.h"

// Process class methods
Process::Process(ProcessDetails details, uint64_t current_time)
{
    int i;
    pid = details.pid;
    start_time = details.start_time;
    num_bursts = details.num_bursts;
    current_burst = 0;
    burst_times = new uint32_t[num_bursts];
    for (i = 0; i < num_bursts; i++)
    {
        burst_times[i] = details.burst_times[i];
    }
    priority = details.priority;
    state = (start_time == 0) ? State::Ready : State::NotStarted;
    if (state == State::Ready)
    {
        launch_time = current_time;
    }
    is_interrupted = false;
    core = -1;
    turn_time = 0;
    wait_time = 0;
    cpu_time = 0;
    remain_time = 0;
    for (i = 0; i < num_bursts; i+=2)
    {
        remain_time += burst_times[i];
    }
    current_wait_start_time = 0;
}

Process::~Process()
{
    delete[] burst_times;
}

uint16_t Process::getPid() const
{
    return pid;
}

uint32_t Process::getStartTime() const
{
    return start_time;
}

uint8_t Process::getPriority() const
{
    return priority;
}

uint64_t Process::getBurstStartTime() const
{
    return burst_start_time;
}

Process::State Process::getState() const
{
    return state;
}

bool Process::isInterrupted() const
{
    return is_interrupted;
}

int8_t Process::getCpuCore() const
{
    return core;
}

double Process::getTurnaroundTime() const
{
    return (double)turn_time / 1000.0;
}

double Process::getWaitTime() const
{
    return (double)wait_time / 1000.0;
}

double Process::getCpuTime() const
{
    return (double)cpu_time / 1000.0;
}

double Process::getRemainingTime() const
{
    return (double)remain_time / 1000.0;
}

uint16_t Process::getCurrentBurst() const
{
    return current_burst;
}

uint32_t Process::getCurrentBurstTime() const
{
    return burst_times[current_burst];
}

uint8_t Process::getNumberOfBursts() const
{
    return num_bursts;
}

uint64_t Process::getCurrentWaitStartTime() const 
{
    return current_wait_start_time;
}

void Process::setBurstStartTime(uint64_t current_time)
{
    burst_start_time = current_time;
}

void Process::setState(State new_state, uint64_t current_time)
{
    if (state == State::NotStarted && new_state == State::Ready)
    {
        launch_time = current_time;
    }
    state = new_state;
}

void Process::setCpuCore(int8_t core_num)
{
    core = core_num;
}

void Process::interrupt()
{
    is_interrupted = true;
}

void Process::interruptHandled()
{
    is_interrupted = false;
}

void Process::moveToNextBurst()
{
    current_burst += 1;
}

void Process::setCurrentWaitStartTime(uint64_t current_time)
{
    current_wait_start_time = current_time;
}

void Process::setStartTime(uint64_t current_time)
{
    start_time = current_time;
    
}



//  Where am I supposed to call this? End of the loop iteration in CRP?
//  How do I do most of these
void Process::updateProcess(uint64_t current_time)
{
    // use `current_time` to update turnaround time, wait time, burst times, 
    // cpu time, and remaining time
    //  Turn Time
    if (state != Process::State::Terminated) {
        turn_time = (current_time - start_time);
    } 

    //  Wait Time
    uint64_t current_wait_period = current_time - current_wait_start_time;
    wait_time += current_wait_period;

    //  CPU time 
    for (int j = 0; j < current_burst; j+=2) {
        cpu_time += burst_times[j];
    }

    //remain time
    int start;
    if (current_burst % 2 == 0) {
        start = current_burst;
    } else {
        start = current_burst + 1;
    }

    for (int i = start; i < num_bursts; i+=2) {
        remain_time += burst_times[i];
    }

}

void Process::updateBurstTime(int burst_idx, uint32_t new_time)
{
    burst_times[burst_idx] = new_time;
}


// Comparator methods: used in std::list sort() method
// No comparator needed for FCFS or RR (ready queue never sorted)

// SJF - comparator for sorting read queue based on shortest remaining CPU time
bool SjfComparator::operator ()(const Process *p1, const Process *p2)
{
    if ((p1->getRemainingTime()) > (p2->getRemainingTime())) {
        return true;
    } else {
        return false;
    }


    return false; // change this!
}

// PP - comparator for sorting read queue based on priority
bool PpComparator::operator ()(const Process *p1, const Process *p2)
{
    if ((p1->getPriority()) > (p2->getPriority())) {
        return true;
    } else {
        return false;
    }
    return false; // change this!
}
