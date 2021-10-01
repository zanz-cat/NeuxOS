
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            x86.h
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#ifndef	__ARCH_X86_H__
#define	__ARCH_X86_H__

#include <stdint.h>

/* 存储段描述符/系统段描述符 */
struct descriptor		/* 共 8 个字节 */
{
	uint16_t limit_low;		/* Limit */
	uint16_t base_low;		/* Base */
	uint8_t	base_mid;		/* Base */
	uint8_t	attr1;			/* P(1) DPL(2) DT(1) TYPE(4) */
	uint8_t	limit_high_attr2;	/* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
	uint8_t	base_high;		/* Base */
};

/* 门描述符 */
struct gate
{
	uint16_t offset_low;	/* Offset Low */
	uint16_t selector;	/* Selector */
	uint8_t	dcount;		/* 该字段只在调用门描述符中有效。如果在利用
				   调用门调用子程序时引起特权级的转换和堆栈
				   的改变，需要将外层堆栈中的参数复制到内层
				   堆栈。该双字计数字段就是用于说明这种情况
				   发生时，要复制的双字参数的数量。*/
	uint8_t	attr;		/* P(1) DPL(2) DT(1) TYPE(4) */
	uint16_t	offset_high;	/* Offset High */
};

/* 任务状态段 */
struct tss {
	uint32_t prev;
	uint32_t esp0;
	uint32_t ss0;
	uint32_t esp1;
	uint32_t ss1;
	uint32_t esp2;
	uint32_t ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint32_t es;
	uint32_t cs;
	uint32_t ss;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	uint32_t ldt;
	uint16_t attrs;
	uint16_t io;
};


/* GDT */
/* 描述符索引 */
#define	INDEX_DUMMY		0	// ┓
#define	INDEX_FLAT_C		1	// ┣ LOADER 里面已经确定了的.
#define	INDEX_FLAT_RW		2	// ┃
#define	INDEX_VIDEO		3	// ┛
/* 选择子 */
#define	SELECTOR_DUMMY		   0		// ┓
#define	SELECTOR_FLAT_C		0x08		// ┣ LOADER 里面已经确定了的.
#define	SELECTOR_FLAT_RW	0x10		// ┃
#define	SELECTOR_VIDEO		(0x18+3)	// ┛<-- RPL=3

#define	SELECTOR_KERNEL_CS 0x08
#define	SELECTOR_KERNEL_DS 0x10
#define SELECTOR_USER_CS 0x1b
#define SELECTOR_USER_DS 0x23

/* 描述符类型值说明 */
#define	DA_32			0x4000	/* 32 位段				*/
#define	DA_LIMIT_4K		0x8000	/* 段界限粒度为 4K 字节			*/
#define	DA_DPL0			0x00	/* DPL = 0				*/
#define	DA_DPL1			0x20	/* DPL = 1				*/
#define	DA_DPL2			0x40	/* DPL = 2				*/
#define	DA_DPL3			0x60	/* DPL = 3				*/
/* 存储段描述符类型值说明 */
#define	DA_DR			0x90	/* 存在的只读数据段类型值		*/
#define	DA_DRW			0x92	/* 存在的可读写数据段属性值		*/
#define	DA_DRWA			0x93	/* 存在的已访问可读写数据段类型值	*/
#define	DA_C			0x98	/* 存在的只执行代码段属性值		*/
#define	DA_CR			0x9A	/* 存在的可执行可读代码段属性值		*/
#define	DA_CCO			0x9C	/* 存在的只执行一致代码段属性值		*/
#define	DA_CCOR			0x9E	/* 存在的可执行可读一致代码段属性值	*/
/* 系统段描述符类型值说明 */
#define	DA_LDT			0x82	/* 局部描述符表段类型值			*/
#define	DA_TaskGate		0x85	/* 任务门类型值				*/
#define	DA_386TSS		0x89	/* 可用 386 任务状态段类型值		*/
#define	DA_386CGate		0x8C	/* 386 调用门类型值			*/
#define	DA_386IGate		0x8E	/* 386 中断门类型值			*/
#define	DA_386TGate		0x8F	/* 386 陷阱门类型值			*/

#define MEM_TYPE_AVL 1U
#define MEM_TYPE_RES 2U

struct ARDS {
    uint32_t base_addr_low;
	uint32_t base_addr_high;
	uint32_t len_low;
	uint32_t len_high;
	uint32_t type;
} __attribute__((packed));

#define PAGE_SHIFT 12
#define PAGE_SIZE (1U << PAGE_SHIFT)
#define PAGE_MASK (~(PAGE_SIZE - 1))

#define PGDIR_SHIFT 22
#define PGDIR_MASK (~((1U << PGDIR_SHIFT) - 1))

#endif /* __ARCH_X86_H__ */
