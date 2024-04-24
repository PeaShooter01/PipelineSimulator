基于MIPS架构的五段式流水线模拟器，支持指令乱序执行，流水线重定向等，对于分支指令采用预测分支失败的策略，支持对指令运行过程中的stall进行统计。
目前仅支持load、store、add、beqz指令，对于访存指令简化为读取或写入一个int型数据到内存。
编译命令：g++ main.cpp -o MIPSsim -std=c++11
使用说明：
1. 编译后运行./MIPSsim <指令文件> <内存数据文件> (redirection)，其中redirecttion可选，表示开启流水线重定向。
2. 指令文件样例为input.txt，内存数据文件样例为memory.txt。
3. 程序运行后输入n或next单步执行一个时钟周期，输入r或regs打印寄存器，输入c或components打印各个功能部件的运行状态，输入“b 行号”执行到某一行，输入s或statistics打印程序运行过程中的性能统计信息，输入exit退出程序。
