# demo-jump-between-functions
C语言不同函数内跳转  
jump between functions in C  
  
test.c:252 进入jump，然后从test.c:244 出来，没有个十年脑血栓想不到这操作（不是）  
现在支持tcc-v0.9.26-x86, gcc-v12.1.0-x86和gcc-v12.1.0-x86_64  
因为跳转了，所以"fun1()!!\n"这句不会输出呢
