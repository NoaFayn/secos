/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t   *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

#define i32 multiboot_uint32_t

/**
 * Prints the multiboot_mmap_entry structure.
 *
 * @param addr Address where the structure is located
 **/
void print_mmap(i32 addr)
{
	struct multiboot_mmap_entry * data = (struct multiboot_mmap_entry *) addr;
	// NOTE(noa): Why is the type not showing 1 properly?
	debug("multiboot_mmap_entry[%p]\n\tsize=%d\n\taddr=%p\n\tlen=%d\n\ttype=%d\n---\n\n", addr, data->size, data->addr, data->len, data->type);
}

/**
 * Prints the int as binary.
 *
 * @param bin Binary number to print
 * @param nb Number of bits to print
 **/
void print_bits(i32 bin)
{
	char c[34];
	for (i32 i = 0; i < 32; i++)
	{
		if (bin&(0x1<<i))	c[31-i] = '1';
		else				c[31-i] = '0';
	}
	c[32] = '\n';
	c[33] = 0;
	debug(c);
}

void tp() {
   debug("kernel mem [0x%x - 0x%x]\n", &__kernel_start__, &__kernel_end__);
   debug("MBI flags 0x%x\n", info->mbi->flags);
	//print_bits(info->mbi->flags);
	// Check that flag[6] is set to access mmap_* values
	if (info->mbi->flags&(0x1<<6))
		debug("flag[6] set!\n");
	else
	{
		debug("flag[6] missing\n");
		return;
	}
	debug("mmap_addr=%p\n", info->mbi->mmap_addr);
	debug("mmap_length=%d\n", info->mbi->mmap_length);

	// TODO(noa): Use the defined values from the .h file to check for each case
	print_bits(data->type);
	if (data->type == 1) debug("Value is 1\n");
}
