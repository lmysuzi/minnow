My name: 李明扬

My Student number : 502024330026

This lab took me about 2 hours to do. I did attend the lab session.


#### 1. Program Structure and Design:

- 本实验第一部分的主要结构是建立一个TCPsocket类的socket，并用其与特定的网址以HTTP的方式建立连接。将报文用字符串的形式通过之前建立的socket发送到服务器端，等待响应后读出socket中返回的网站内容，直到到达eof。

- 本实验第二部分的主要结构是完善ByteStream类，以字节流的形式实现其读写的过程。通过维护一个字符串以及其容量，在读写的时候分别对其进行操作以完成。


#### 2. Implementation Challenges:

- 本次实验最大的挑战在于手册的阅读，需要理清库函数中类的继承关系，掌握类方法的用法。并且需要理解linux系统所提供的关于网络编程的API，了解它们是怎样被封装到所提供的类之中的。

- 另一个挑战是对于ByteStream容量的理解，需要正确理解容量的含义才能写出正确的代码。

#### 3. Remaining Bugs:

- 暂时尚未找到bug。

测试结果如下图![avatar](pictures/result.png)

