## 1. 线程函数原型为`void (*ThdFunc)(void *)`
## 2. 用户线程中调用`void YieldThd()`完成切换到下一个线程运行，本线程停止运行。被切换运行的线程，如果是已经运行的，需要从上次它自己调用`YieldThd`之后代码运行。
## 3. `CreateThd(ThdFunc func, void * arg)` 创建用户线程
## 4. `StartThds();`开始调度线程
## 5. 演示代码如下：

```
void thd2(void * arg)
{
	for (int i = 4; i < 12; i++)
	{
		printf("thd2: arg=%d , i = %d\n", (int)arg, i);
		YieldThd();
	}	
}

void thd1(void * arg)
{
	if (CreateThd(thd2, (void *)2) == INVALID_THD)
	{
		printf("cannot create\n");
	}
	for (int i = 0; i < 12; i++)
	{
		printf("thd1: arg=%d , i = %d\n", (int)arg, i);
		YieldThd();
	}	
}

int main()
{
	if (CreateThd(thd1, (void *)1) == INVALID_THD)
	{
		printf("cannot create\n");
	}
	
	StartThds();
	return 0;
}
```

## 6. 测试
将5中的代码thd1,thd2的打印注释，然后将循环修改为无限循环，看看是否程序报错

## 7. 提示
1）可能要用__declspec(naked)函数

2）要注意context切换，对于要保存的寄存器，可参考调试的寄存器窗口中给出寄存器，尽量多的保存和恢复。

3）要清晰函数调用的过程

4）肯定要用汇编嵌入

5）如果测试第6点出错，则认真分析调用和切换过程。

6）如果c库的函数在汇编中调用有问题，请用反汇编字节分析一下，提示c库有动态和静态链接两种方式
