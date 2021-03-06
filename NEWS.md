libFirm 1.22.0 (2015-07-31)
---------------------------
* Improved PIC support, linux/elf is now supported
* Additional local optimization rules
* Inline assembly support for sparc/amd64
* Improved load/store optimization, featuring compound value optimizations
* Improved amd64 (aka x86_64) backend (but still experimental)
* Improved arm backend (but still experimental)
* Improved inliner (can inline compound types)
* Compiletime performance improvements
* Cleanups and API simplifications
* Switch to C99 and corresponding code cleanup and simplification
* Bugfixes

libFirm 1.21.0 (2012-11-16)
---------------------------
* Improvements of x86 backend (esp. x87 floatingpoint code)
* Improvements to sparc backend (better use of delay slots)
* Improved local optimization rules (esp. conversions)
* Make compiler more deterministic
* Bugfixes

libFirm 1.20.0 (2011-12-07)
---------------------------

* Further improvemens to sparc backend (SPEC2000 works with softfloat)
* Tuning of x86 backend
* Software floatingpoint lowerer
* Fixed firm profiling
* New pass management allowing to specify pre-/postconditions for passes
* Remove dependency on liblpp, add support for gurobi ILP solver
* Experimental dwarf debugging support
* Code cleanups, refactoring
* Restructured API documentation
* Bugfixes (we did alot of csmith testing)

libFirm 1.19.1 (2011-05-17)
---------------------------

* Fix some set_XXX functions not being exported in the shared library

libFirm 1.19.0 (2011-03-15)
---------------------------

* Includes "SSA-Based Register Allocation with PBQP"
* Improved Sparc backend
* New (optimistic) fixpoint based value-range propagation/bit analysis
* Code cleanup and refactoring
* Bugfixes

libFirm 1.18.1 (2010-05-05)
---------------------------

* Fix bug where stackframe was not always setup for -fno-omit-frame-pointer
* bugfixes in Asm handling

libFirm 1.18.0 (2010-04-15)
---------------------------

* Includes "Preference Guided Register Assignment" algorithm
* Experimental Value Range Propagation
* Loop Inversion and experimental Loop Unrolling code
* Simplified construction interface. Most node constructors don't need graph/block arguments anymore.
* Reworked type interface. Type names are optional now. Support for additional linkage types that among others support C++ 'linkonce' semantics now.
* Small changes in constructors and node getters/setters (mostly adding 'const' to some getters)
* code cleanup, smaller improvements in API specification
* bugfixes

libFirm 1.17.0 (2009-05-15)
---------------------------

* bugfixes
* advanced load/store optimization which hoists loads out of loops
* Internal restruturing: Alot of node structures are automatically generated
   from a specification file now.
* Add support for multiple calling conventions
* New experimental support for reading and writing programgraphs to disk
* Support and optimizations for trampolines
* fix PIC support

libFirm 1.16.0 (2009-01-28)
---------------------------

* bugfixes
* support for builtin nodes

libFirm 1.15.0 (2008-12-01)
---------------------------
* bugfixes

libFirm 1.14.0 (2008-11-22)
---------------------------

* Implementation of Clicks Combined Analysis/Optimizations
* New switch lowering code
* support for global asm statements
* improved asm support
* PIC support for Mac OS X
* New register pressure minimizing scheduler
* Improvements to spill algorithm
* fix endless loop problems
* further improve inlining heuristics
* improve peephole optimizations for x86
* bugfixes

libFirm 1.13.0 (2008-07-31)
---------------------------

* VanDrunen's GVN-PRE fixed
* operator strength reduce fixed and improved
* fixed 64bit code generation for some rare compare cases
* better tailrecursion optimization: handles x * func() and x + func()
* improved inliner: better heuristics for inlining, can now inline recursive calls
* improved spiller
* lowering of CopyB nodes
* better memory disambiguator
* float->64bit conversion fixed for x87
* removed old verbosity level based debugging: all modules use the new debug facility
* Improved Confirm based optimization and conditional evaluation (using Confirm nodes)
* BugFixes: tail recursion, load/store optimization, lowering of structure return, conditional
  evaluation, removal of unused methods
* reduced numer of indirections for backend operation
* ia32 Backend: supports more CPU architectures
* ARM Backend: fixed frame access
* support for special segments (like constructors, destructors)

libFirm 1.12.1 (2008-02-18)
---------------------------

* bugfixes for new style initializers with bitfield types
* make lowerer look at constant initializers too

libFirm 1.12.0 (2008-02-14)
---------------------------

* dependency on libcore and libobstack dropped
* there's an alternative easier to use way to construct compound initializers
* bugfixes
* improved support for exceptions
* speed improvements
* optimization of known libc functions

libFirm 1.11.0 (2008-11-05)
---------------------------

* Lots of bugfixes
* Compilation speed improved
* Completely improved and rewritten handling of x86 address mode
* Optimized Mul -> Lea,Shift,Add transformation
* 64bit operations fixed and improved
* More local optimizations
* New backend peephole optimizations
* Explicit status flag modeling (only for x86 for now)
* Improvements of Load/Store optimization and alias analysis
* All C benchmarks from Spec CINT2000 work now (with our edg frontend)
