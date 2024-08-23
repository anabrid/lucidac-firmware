#include "imxrt.h"

/** \file
 * Memory Processing Unit macros from framework-arduinoteensy/cores/teensy4/startup.c
 **/


#define NOEXEC		SCB_MPU_RASR_XN
#define READONLY	SCB_MPU_RASR_AP(7)
#define READWRITE	SCB_MPU_RASR_AP(3)
#define NOACCESS	SCB_MPU_RASR_AP(0)
#define MEM_CACHE_WT	SCB_MPU_RASR_TEX(0) | SCB_MPU_RASR_C
#define MEM_CACHE_WB	SCB_MPU_RASR_TEX(0) | SCB_MPU_RASR_C | SCB_MPU_RASR_B
#define MEM_CACHE_WBWA	SCB_MPU_RASR_TEX(1) | SCB_MPU_RASR_C | SCB_MPU_RASR_B
#define MEM_NOCACHE	SCB_MPU_RASR_TEX(1)
#define DEV_NOCACHE	SCB_MPU_RASR_TEX(2)
#define SIZE_32B	(SCB_MPU_RASR_SIZE(4) | SCB_MPU_RASR_ENABLE)
#define SIZE_64B	(SCB_MPU_RASR_SIZE(5) | SCB_MPU_RASR_ENABLE)
#define SIZE_128B	(SCB_MPU_RASR_SIZE(6) | SCB_MPU_RASR_ENABLE)
#define SIZE_256B	(SCB_MPU_RASR_SIZE(7) | SCB_MPU_RASR_ENABLE)
#define SIZE_512B	(SCB_MPU_RASR_SIZE(8) | SCB_MPU_RASR_ENABLE)
#define SIZE_1K		(SCB_MPU_RASR_SIZE(9) | SCB_MPU_RASR_ENABLE)
#define SIZE_2K		(SCB_MPU_RASR_SIZE(10) | SCB_MPU_RASR_ENABLE)
#define SIZE_4K		(SCB_MPU_RASR_SIZE(11) | SCB_MPU_RASR_ENABLE)
#define SIZE_8K		(SCB_MPU_RASR_SIZE(12) | SCB_MPU_RASR_ENABLE)
#define SIZE_16K	(SCB_MPU_RASR_SIZE(13) | SCB_MPU_RASR_ENABLE)
#define SIZE_32K	(SCB_MPU_RASR_SIZE(14) | SCB_MPU_RASR_ENABLE)
#define SIZE_64K	(SCB_MPU_RASR_SIZE(15) | SCB_MPU_RASR_ENABLE)
#define SIZE_128K	(SCB_MPU_RASR_SIZE(16) | SCB_MPU_RASR_ENABLE)
#define SIZE_256K	(SCB_MPU_RASR_SIZE(17) | SCB_MPU_RASR_ENABLE)
#define SIZE_512K	(SCB_MPU_RASR_SIZE(18) | SCB_MPU_RASR_ENABLE)
#define SIZE_1M		(SCB_MPU_RASR_SIZE(19) | SCB_MPU_RASR_ENABLE)
#define SIZE_2M		(SCB_MPU_RASR_SIZE(20) | SCB_MPU_RASR_ENABLE)
#define SIZE_4M		(SCB_MPU_RASR_SIZE(21) | SCB_MPU_RASR_ENABLE)
#define SIZE_8M		(SCB_MPU_RASR_SIZE(22) | SCB_MPU_RASR_ENABLE)
#define SIZE_16M	(SCB_MPU_RASR_SIZE(23) | SCB_MPU_RASR_ENABLE)
#define SIZE_32M	(SCB_MPU_RASR_SIZE(24) | SCB_MPU_RASR_ENABLE)
#define SIZE_64M	(SCB_MPU_RASR_SIZE(25) | SCB_MPU_RASR_ENABLE)
#define SIZE_128M	(SCB_MPU_RASR_SIZE(26) | SCB_MPU_RASR_ENABLE)
#define SIZE_256M	(SCB_MPU_RASR_SIZE(27) | SCB_MPU_RASR_ENABLE)
#define SIZE_512M	(SCB_MPU_RASR_SIZE(28) | SCB_MPU_RASR_ENABLE)
#define SIZE_1G		(SCB_MPU_RASR_SIZE(29) | SCB_MPU_RASR_ENABLE)
#define SIZE_2G		(SCB_MPU_RASR_SIZE(30) | SCB_MPU_RASR_ENABLE)
#define SIZE_4G		(SCB_MPU_RASR_SIZE(31) | SCB_MPU_RASR_ENABLE)
#define REGION(n)	(SCB_MPU_RBAR_REGION(n) | SCB_MPU_RBAR_VALID) 
