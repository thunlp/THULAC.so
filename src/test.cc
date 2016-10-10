#include "thulac_lib.h"
#include <cstdio>

int main() {
	init("./models", "./kpb.dict", 1024*1024*16, 0, 0);
	seg("我们是一只小猪\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("我们是一只小猪\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("我们是一只小猪\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("我们是一只小猪\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("我们是一只小猪\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("狐狸猫猫猪大咖咖\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("我们是一只小猪\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	seg("不理画画啦啦啦\n恒生电子是猪头\n哇哈哈是上证指数");
	std::printf("%s\n", getResult()); 
	deinit();
}
