
typedef	unsigned int		u32;
typedef	unsigned short		u16;
typedef	unsigned char		u8;

/* GDT 和 IDT 中描述符的个数 */
#define	GDT_SIZE	128

/* 函数类型 */
#define	PUBLIC		/* PUBLIC is the opposite of PRIVATE */
#define	PRIVATE	static	/* PRIVATE x limits the scope of x */

/* 存储段描述符/系统段描述符 */
typedef struct s_descriptor		/* 共 8 个字节 */
{
	u16	limit_low;		/* Limit */
	u16	base_low;		/* Base */
	u8	base_mid;		/* Base */
	u8	attr1;			/* P(1) DPL(2) DT(1) TYPE(4) */
	u8	limit_high_attr2;	/* G(1) D(1) 0(1) AVL(1) LimitHigh(4) */
	u8	base_high;		/* Base */
}DESCRIPTOR;

//PUBLIC void* memcpy(void* pDst, void* pSrc, int iSize);

PUBLIC u8  gdt_ptr[6];
PUBLIC DESCRIPTOR  gdt[GDT_SIZE];

PUBLIC void cstart(){
    // memcpy(&gdt, (void*)(*((u32*)(&gdt_ptr[2]))), *((u16*)(&gdt_ptr[0]))+1);
    u16* p_gdt_limit = (u16*)(&gdt_ptr[0]);
    u32* p_gdt_base = (u32*)(&gdt_ptr[2]);
    *p_gdt_limit = GDT_SIZE * sizeof(DESCRIPTOR) - 1;
    *p_gdt_base = (u32)&gdt;
}
