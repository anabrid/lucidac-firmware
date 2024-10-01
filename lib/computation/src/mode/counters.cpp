#include "mode/counters.h"

FLASHMEM void mode::PerformanceCounter::reset() {
    total_op_time_us = 0;
    total_ic_time_us = 0;
    total_number_of_runs = 0;
}

#define FROMTO(A,B) cur_mode == mode::Mode::A && new_mode == mode::Mode::B

FLASHMEM void mode::PerformanceCounter::to(mode::Mode new_mode) {
    if(FROMTO(IC,IC))     {}
    if(FROMTO(IC,OP))     { total_ic_time_us   += ic;  op   = 0; }
    if(FROMTO(IC, HALT))  { total_ic_time_us   += ic;  halt = 0; }
    if(FROMTO(OP,IC))     { total_op_time_us   += op;    ic = 0;   }
    if(FROMTO(OP,OP))     {}
    if(FROMTO(OP, HALT))  { total_op_time_us   += op;  halt = 0; }
    if(FROMTO(HALT, IC))  { total_halt_time_us += halt;  ic = 0; }
    if(FROMTO(HALT, OP))  { total_halt_time_us += halt;  op = 0; }
    if(FROMTO(HALT,HALT)) {}
}

FLASHMEM void mode::PerformanceCounter::add(mode::Mode some_mode, uint32_t us) {
    switch(some_mode) {
        case mode::Mode::IC:   total_ic_time_us   += us; return;
        case mode::Mode::OP:   total_op_time_us   += us; return;
        case mode::Mode::HALT: total_halt_time_us += us; return; 
    }
}

FLASHMEM void mode::PerformanceCounter::increase_run() {
    total_number_of_runs++;
}

FLASHMEM void mode::PerformanceCounter::to_json(JsonObject target) {
    target["total_ic_time_us"] = total_ic_time_us;
    target["total_op_time_us"] = total_op_time_us;
    target["total_halt_time_us"] = total_halt_time_us;
    target["total_number_of_runs"] = total_number_of_runs;
}