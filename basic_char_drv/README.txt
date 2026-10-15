内核版本：4.1.15
代码内容：普通字符设备框架；
实现内容：(1)参考内核源码编写的工业级字符设备框架，主要是对于各个函数的错误处理以及驱动的多实例化做了改进；
	(2)适合学习字符框架时进行参考；代码也包含了本人对于多实例化的一些理解，比如init函数中做的工作、probe函数中做的工作；
	(3)对应代码编写的文章网址为：https://blog.csdn.net/Faith_is_eternal/article/details/164617518?fromshare=blogdetail&sharetype=blogdetail&sharerId=164617518&sharerefer=PC&sharesource=Faith_is_eternal&sharefrom=from_link；