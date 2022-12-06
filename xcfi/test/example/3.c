#include <stdio.h>
#include <string.h>
#include <stdint.h>

void func(int data)
{
}

int main(void)
{
	void (*fptr)(int);
	uint64_t target_addr = (uint64_t)(&func);
	uint64_t pac_addr;
	fptr = func;
	memcpy(&pac_addr, &fptr, sizeof(fptr));
	printf("pac_addr = %lx, origin_addr = %lx\n", pac_addr, target_addr);

	if (target_addr == pac_addr) {
		return -1;
	}

	return 0;
}
