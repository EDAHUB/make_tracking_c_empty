// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2017 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0
//======================================================================

// For std::unique_ptr
#include <memory>

// Include common routines
#include <verilated.h>

// Include model header, generated from Verilating "top.v"
#include "Vtop.h"

// Marcel - includ docker header file
#include "Vtop_top.h"
#include "Vtop_sub.h"

// Legacy function required only so linking works on Cygwin and MSVC++
double sc_time_stamp() { return 0; }

int main(int argc, char** argv, char** env) {
    // This is a more complicated example, please also see the simpler examples/make_hello_c.

    // Prevent unused variable warnings
    if (false && argc && argv && env) {}

    // Create logs/ directory in case we have traces to put under it
    Verilated::mkdir("logs");

    // Construct a VerilatedContext to hold simulation time, etc.
    // Multiple modules (made later below with Vtop) may share the same
    // context to share time, or modules may have different contexts if
    // they should be independent from each other.

    // Using unique_ptr is similar to
    // "VerilatedContext* contextp = new VerilatedContext" then deleting at end.
    const std::unique_ptr<VerilatedContext> contextp{new VerilatedContext};

    // Set debug level, 0 is off, 9 is highest presently used
    // May be overridden by commandArgs argument parsing
    contextp->debug(0);

    // Randomization reset policy
    // May be overridden by commandArgs argument parsing
    contextp->randReset(2);

    // Verilator must compute traced signals
    contextp->traceEverOn(true);

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    contextp->commandArgs(argc, argv);

    // Construct the Verilated model, from Vtop.h generated from Verilating "top.v".
    // Using unique_ptr is similar to "Vtop* top = new Vtop" then deleting at end.
    // "TOP" will be the hierarchical name of the module.
    const std::unique_ptr<Vtop> top{new Vtop{contextp.get(), "TOP"}};

    // Set Vtop's input signals
    top->reset_l = !0;
    top->clk = 0;
    top->in_small = 1;
    top->in_quad = 0x1234;
    top->in_wide[0] = 0x11111111;
    top->in_wide[1] = 0x22222222;
    top->in_wide[2] = 0x3;

    // Marcel : decide time increasing for each step
    int time_incr_for_sub = 1;

    // Simulate until $finish
    while (!contextp->gotFinish()) {
        contextp->timeInc(time_incr_for_sub);  // 1 timeprecision period passes...

        if (contextp->time() > 1 && contextp->time() < 10) {
            top->reset_l = !1;  // Assert reset
        } else {
            top->reset_l = !0;  // Deassert reset
        }

        top->in_small += 0x2;

	top->top->sub->out_small += 1;

        top->eval();

	// ==================================
	// Marcel : collect sub docker data
	// ==================================
	unsigned char clk_docker = top->top->sub->clk;
	unsigned char reset_l_docker = top->top->sub->reset_l;
	unsigned int  in_small_docker = top->top->sub->in_small;
	//printf ("[Collect sub docker data] TIME_INC=%d, clk =%d, reset=%d, in_small=%x\n",time_incr_for_sub, clk_docker,reset_l_docker,in_small_docker);
	
	
        top->clk = !top->clk;

        top->eval();

        // Read outputs
        VL_PRINTF("[%" VL_PRI64 "d] clk=%x rstl=%x ismall=%x -> osmall=%x \n",
                  contextp->time(), top->clk, top->reset_l, top->in_small, top->out_small);
        if (contextp->time() > 50) { break; }

    }

    // Final model cleanup
    top->final();

    // Coverage analysis (calling write only after the test is known to pass)
#if VM_COVERAGE
    Verilated::mkdir("logs");
    contextp->coveragep()->write("logs/coverage.dat");
#endif

    // Return good completion status
    // Don't use exit() or destructor won't get called
    return 0;
}
