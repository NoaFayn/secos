/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <segmem.h>
#include <string.h>
#include <asm.h>

extern info_t *info;
extern uint32_t __kernel_start__;
extern uint32_t __kernel_end__;

struct multiboot_mmap_entry * g_ram;

void print_seg(seg_desc_t * seg)
{
  uint64_t limit = 0, base = 0;
  limit = seg->limit_2 << 16 | seg->limit_1;
  base = seg->base_3 << 24 | seg->base_2 << 16 | seg->base_1;
  debug("GDT:0x%llx:\n", seg->raw);
  debug("B:0x%x", base);
  debug(", G:%x", seg->g);
  debug(", DB:%x", seg->d);
  debug(", AVL:%x", seg->avl);
  debug(", L:0x%x", limit);
  debug(", P:%x", seg->p);
  debug(", DPL:%x", seg->dpl);
  debug(", S:%x", seg->s);
  debug(", T:0x%x", seg->type);
  debug("\n");
}

void create_gdt()
{
  gdt_reg_t gdt;
  gdt.addr = (offset_t) g_ram->addr;
  gdt.limit = sizeof(seg_desc_t) * 4 - 1;       // 4 segments

  seg_desc_t * segs = (seg_desc_t *) gdt.addr;

  // First segment is NULL
  seg_desc_t * seg = &segs[0];
  seg->raw = 0;

  // Setup common fields
  for (int i = 1; i < 4; i++)
  {
    seg_desc_t * p1 = &segs[i];
    p1->base_1 = 0;       // use all available memory
    p1->base_2 = 0;       // use all available memory
    p1->base_3 = 0;       // use all available memory
    p1->limit_1 = 0xffff; // use all available memory
    p1->limit_2 = 0xf;    // use all available memory
    p1->g = 1;            // limit is in units of as 4096-byte pages
    p1->d = 1;            // use 32-bit code segment
    p1->l = 0;            // do not use 64-bit segment
    p1->avl = 0;          // ??
    p1->p = 1;            // present (if not set, will consider this segment unusable)
    p1->dpl = 0;          // must be ring 0 to use this segment
    p1->s = 1;            // normal segment
    p1->type = 0;         // to be completed after
  }

  // Setup custom fields
  seg = &segs[1];
  seg->type = SEG_DESC_CODE_XR;
  seg = &segs[2];
  seg->type = SEG_DESC_DATA_RW;

  // Apply new GDT
  set_gdtr(gdt);

  // Change registers
  set_cs(gdt_krn_seg_sel(1));
  set_ss(gdt_krn_seg_sel(2));
  set_ds(gdt_krn_seg_sel(2));
  set_ss(gdt_krn_seg_sel(2));
  set_fs(gdt_krn_seg_sel(2));
  set_gs(gdt_krn_seg_sel(2));
  set_es(gdt_krn_seg_sel(2));
}

/**
 * Return mmap_entry available for the RAM.
 */
struct multiboot_mmap_entry * parse_mmap()
{
  debug("kernel mem [0x%x - 0x%x]\n", &__kernel_start__, &__kernel_end__);
  debug("mmap_addr=0x%x, mmap_length=%x\n", info->mbi->mmap_addr, info->mbi->mmap_length);
  struct multiboot_mmap_entry * data = (struct multiboot_mmap_entry *) info->mbi->mmap_addr;
  struct multiboot_mmap_entry * end = (struct multiboot_mmap_entry *) (info->mbi->mmap_addr + info->mbi->mmap_length);
  struct multiboot_mmap_entry * ram = NULL;
  while (data < end)
  {
    debug("ENTRY: ");
    debug("size=%x", data->size);
    debug(", addr=0x%llx", data->addr);
    debug(", len=%llx", data->len);
    debug(", type=%x", data->type);
    if (data->type == 1 && ram == NULL && data->len >= 0x100000)
    {
      debug(" (using as RAM)");
      ram = data;
    }
    debug("\n");
    data++;
  }
  g_ram = ram;
  return ram;
}

void print_gdt()
{
  gdt_reg_t gdt;
  get_gdtr(gdt);
  debug("gdt addr=%p\n", gdt.addr);
  debug("gdt limit=%u\n", gdt.limit);
  debug("gdt end=0x%x\n", gdt.addr + gdt.limit - 1);

  offset_t start = gdt.addr;
  offset_t end = start + gdt.limit - 1;
  for (offset_t curr = start; curr < end; curr += sizeof(seg_desc_t))
  {
    seg_desc_t * seg = (seg_desc_t *) curr;
    print_seg(seg);
  }

}

void set_seg_base(seg_desc_t * seg, int base)
{
  seg->base_1 = base;
  seg->base_2 = base >> 16;
  seg->base_3 = base >> 24;
}

void set_seg_limit(seg_desc_t * seg, int limit)
{
  limit--;
  seg->limit_1 = limit;
  seg->limit_2 = limit >> 16;
}

void dump_mem(int addr, int len)
{
  debug("Dump at 0x%x\n", addr);
  for (int i = 0; i < len; i++)
  {
    char * v = (char *) addr + i;
    debug("0x%x: %x", v, *v);
    if (i % 4 == 0)
      debug("\n");
    else
      debug("\t");
  }
  debug("\n");
}

void question2()
{
  while (1)
  {
    force_interrupts_on();
  }
}

void print_intseg(int_desc_t * seg)
{
  uint64_t offset = seg->offset_2 << 16 | seg->offset_1;
  debug("IDT:0x%llx:\n", seg->raw);
  debug("O:0x%x", offset);
  debug(", S:%x", seg->selector);
  debug(", IST:%x", seg->ist);
  debug(", T:%x", seg->type);
  debug(", DPL:%x", seg->dpl);
  debug(", P:%x", seg->p);
  debug("\n");
}

void bp_handler()
{
  debug("bp_handler\n");
}

void bp_trigger()
{
  asm volatile ("int3");
}

void question3()
{
  // 3.1
  // Localiser l'IDT et afficher son adresse de chargement
  idt_reg_t idt;
  get_idtr(idt);
  debug("IDT:\n");
  debug("idt addr=0x%x\n", idt.addr);
  debug("idt limit=%u\n", idt.limit);
  debug("idt end=0x%x\n", idt.addr + idt.limit - 1);

  offset_t start = idt.addr;
  offset_t end = start + idt.limit - 1;
  for (offset_t curr = start; curr < end; curr += sizeof(int_desc_t))
  {
    // int_desc_t * elt = (int_desc_t *) curr;
    // print_intseg(elt);

    // 3.4
    // Modifier le descripteur d'interruption pour appeler bp_handler()
  }


  bp_trigger();
}

void tp()
{
  parse_mmap();
  print_gdt();
  create_gdt();
  print_gdt();

  // question2();
  question3();
}
