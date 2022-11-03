// make mipsr2-test && ./mipsr2-test

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define try3(func, result, msg) if((func) != (result)) printf("%s: %08x != %08x%s\n", #func, (func), (result), msg);
#define try(func, result) try3(func, result, "");

int32_t ext(uint32_t in, uint32_t lsb, uint32_t msb)
{
	int32_t out;

	__asm__ __volatile__(
      		"seb     %0,%1\n"
		: "=r"(out)
    		: "r"(in)
     		);

	return out;
}

void test_ext(void)
{
	printf("%s:\n", __func__);
	try(ext(0xdeadbeef, 0, 7), 0xef);
	try(ext(0xdeadbeef, 0, 0), 0x1);
	try(ext(0xdeadbeef, 4, 4), 0x0);
	try(ext(0xdeadbeef, 8, 15), 0xbe);
	try(ext(0xdeadbeef, 16, 21), 0xad);
	try(ext(0xdeadbeef, 22, 31), 0xde);
	try3(ext(0xdeadbeef, 22, 31), 0xde, " intentionally fail");
}

int32_t ins(uint32_t in, uint32_t lsb, uint32_t msb, uint32_t value)
{
	int32_t out;

	__asm__ __volatile__(
      		"seb     %0,%1\n"
		: "=r"(out)
    		: "r"(in)
     		);

	return out;
}

void test_ins(void)
{
	printf("%s:\n", __func__);
	try(ins(0xdeadbeef, 0, 7, 0x00), 0xdeadbe00);
	try(ins(0xdeadbeef, 0, 1, 0x00), 0xdeadbeee);
	try(ins(0xdeadbeef, 0, 7, 0xff), 0xdeadbeff);
	try(ins(0xdeadbeef, 0, 31, 0x5aa5a55a), 0x5aa5a55a);
	try(ins(0xdeadbeef, 31, 31, 0x5aa5a55a), 0x3eadbeef);
	try(ins(0xdeadbeef, 30, 30, 0xdeadbeef), 0xfeadbeef);
	try3(ins(0xdeadbeef, 31, 31, 0x5aa5a55a), 0xdeadbeef, " intentionally fail");
}

int32_t wsbh(uint32_t in)
{
	int32_t out;

	__asm__ __volatile__(
      		"wsbh     %0,%1\n"
		: "=r"(out)
    		: "r"(in)
     		);

	return out;
}

void test_wsbh(void)
{
	printf("%s:\n", __func__);
	try(wsbh(0xdeadbeef), 0xaddeefbe);
	try(wsbh(0x55aa55aa), 0xaa55aa55);
	try(wsbh(0x0000005a), 0x00005a00);
	try(wsbh(0x00005a00), 0x0000005a);
	try3(wsbh(0xff), 0xff, " intentionally fail");
}

int32_t seh(uint32_t in)
{
	int32_t out;

	__asm__ __volatile__(
      		"seh     %0,%1\n"
		: "=r"(out)
    		: "r"(in)
     		);

	return out;
}

void test_seh(void)
{
	printf("%s:\n", __func__);
	try(seh(0x0000), 0x0000);
	try(seh(0x007f), 0x007f);
	try(seh(0x008f), 0x0008f);
	try(seh(0x00ff), 0x00ff);
	try(seh(0x7fff), 0x7fff);
	try(seh(0x8000), 0xffff8000);
	try(seh(0xffff), 0xffffffff);
	try(seh(0xffff0000), 0x00000000);
	try(seh(0xffff7fff), 0x00007fff);
	try(seh(0xffff8000), 0xffff8000);
	try(seh(0xffffff00), 0xffffff00);
	try(seh(0x55aa55aa), 0x000055aa);
	try(seh(0xa5a5a5a5), 0xffffa5a5);
	try3(seh(0xffff), 0xffff, " intentionally fail");
}

int32_t seb(uint32_t in)
{
	int32_t out;

	__asm__ __volatile__(
      		"seb     %0,%1\n"
		: "=r"(out)
    		: "r"(in)
     		);

	return out;
}

void test_seb(void)
{
	printf("%s:\n", __func__);
	try(seb(0x00), 0x00);
	try(seb(0x7f), 0x7f);
	try(seb(0x8f), 0xffffff8f);
	try(seb(0xff), 0xffffffff);
	try(seb(0xff00), 0x00);
	try(seb(0xffffff00), 0x00);
	try(seb(0x55aa55aa), 0xffffffaa);
	try3(seb(0xff), 0xff, " intentionally fail");
}

int main()
{
	printf("starting...\n");
	test_ext();
	test_ins();
	test_wsbh();
	test_seb();
	test_seh();
	printf("done.\n");
}
