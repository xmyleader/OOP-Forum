## 有关 WSL 使用的一些深层bug以及solution

#### 1. 行尾序列的差异在跨系统时的bug
在终端的wsl中，使用了2种输入方式，得到不同的结果
1. **重定向输入** 
   `./main < in.txt`
2. **键盘输入** 
   `./main` 随后在终端进行键盘输入

这两种输入有很大差别！
这是因为WSL里的程序按 Linux 方式读文件。
如果你的输入文件 `in.txt` 是在 Windows 编辑器里保存的，往往是 `CRLF` 行尾，Linux 下读入时那个 `\r` 会留在字符串里。
`\r` 在 Linux 里的作用是：把光标立刻跳回到当前行的最开头，但不换行，这将在很多场景下覆盖你的原有内容！

**不同系统行尾序列说明**
>1. Windows 系统
行尾序列：CRLF
符号表示：\r\n

>2. Linux / WSL /macOS 系统
行尾序列：LF
符号表示：\n

#### Solution
以后输入文件尽量用 Linux 换行保存
只需在 VS Code 中右下角把 `CRLF` 改成 `LF` 就解决啦

---

#### To be continued~~