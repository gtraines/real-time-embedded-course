/*
 * $QNXLicenseC: 
 * Copyright 2009, QNX Software Systems.  
 *  
 * Licensed under the Apache License, Version 2.0 (the "License"). You  
 * may not reproduce, modify or distribute this software except in  
 * compliance with the License. You may obtain a copy of the License  
 * at: http://www.apache.org/licenses/LICENSE-2.0  
 *  
 * Unless required by applicable law or agreed to in writing, software  
 * distributed under the License is distributed on an "AS IS" basis,  
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied. 
 * 
 * This file may contain contributions from others, either as  
 * contributors under the License or as licensors under other terms.   
 * Please review this entire file for other proprietary rights or license  
 * notices, as well as the QNX Development Suite License Guide at  
 * http://licensing.qnx.com/license-guide/ for other information. 
 * $ 
 */



#ifndef _AT91SAM9M10_H_
#define _AT91SAM9M10_H_

//#define IS_DDR()                                (1 == (PWR_PAD_CTRL & 3))

#define DDR2_SDRAM_BASE                         0x70000000	
#define PERIPHERAL_BASE                         0xf0000000	// Internal Peripherals
#define SPI0_BASE	0xfffa4000
#define SPI_CR		0x00000000
#define SPI_MR		0x00000004
#define SPI_RDR		0x00000008
#define SPI_TDR		0x0000000c
#define SPI_SR		0x00000010
#define SPI_CSR0	0x00000030
#define SPI_MR_PS	0x00000002
#define SPI_SR_TDRE	0x00000002
#define SPI_SR_TXEMPTY	0x00000200
#define SPI_CSR_CSAAT	0x00000008
#define SPIEN_BIT	0
#define SYSTEM_BASE                             (PERIPHERAL_BASE + 0x0fffc000)	// SYSC
#define MEM_CONTROL_BASE                        0xffffe400	// System Controller Mapping - SDRAMC
#define DMA_CONTROL_BASE              (SYSTEM_BASE + 0x00002c00)	// DMAC
#define DDR2_SDRAMC_BASE              (SYSTEM_BASE + 0x00002600)	// SDRAMC
#define SMC_BASE                      (SYSTEM_BASE + 0x00002800)        // SMC
#define MATRIX_BASE                   (SYSTEM_BASE + 0x00002a00)	// Bus Matrix User Interface
#define PIOA_BASE                     (SYSTEM_BASE + 0x00003200)	// PIOA
#define PIOB_BASE                     (SYSTEM_BASE + 0x00003400)	// PIOB
#define PIOC_BASE                     (SYSTEM_BASE + 0x00003600)	// PIOD
#define PIOD_BASE                     (SYSTEM_BASE + 0x00003800)	// PIOD
#define PMC_BASE                      (SYSTEM_BASE + 0x00003c00)		// PMC
#define WDTC_BASE                     (SYSTEM_BASE + 0x00003d40)	// WDTC

#define PIO_OFFSET			0x00003400

/* Watchdog and Clock definitions */
#define WDDIS_BIT 15
#define ALL_SYSCLK_EN		0x00000344
#define ALL_PHCLK_EN		0xfffffffc		
#define MATRIX_MAST_REMAP	0x00000fff
#define CKGR_PLLAR_192MHZ	0x20c73f03
#define MCKRDY_BIT		0x00000008
#define PMC_MCKR_133MHZ		0x00001302
#define ENABLE_UPLL		0x00000901

/* DDR2SDRAMC Registers */
#define DDRSDRC_MR                    0x00000000	/* SDRAMC Mode Register */
#define DDRSDRC_RTR                   0x00000004	/* SDRAMC Refresh Timer Register */
#define DDRSDRC_CR                    0x00000008	/* SDRAMC Configuration Register */
#define DDRSDRC_T0PR                  0x0000000c	/* SDRAMC Timinig0 Register */
#define DDRSDRC_T1PR                  0x00000010	/* SDRAMC Timinig1 Register */
#define DDRSDRC_T2PR                  0x00000014	/* SDRAMC Timinig1 Register */
#define DDRSDRC_MD                    0x00000020	/* SDRAMC Memory Device Register */

/* SDRAMC Mode Register Commands */
#define SDRAMC_NORMAL                 0x00000000
#define SDRAMC_NOP                    0x00000001
#define SDRAMC_ALL_BANKS_PERCHARGE    0x00000002
#define SDRAMC_LMR                    0x00000003
#define SDRAMC_AUTO_REFRESH           0x00000004		
#define SDRAMC_ELMR                   0x00000005

/* Matrix Registers */
#define MATRIX_EBI_CSA                0x00000128	/* EBI Chip Select Assignment Register */
#define MATRIX_MRCR	              0x00000100	/* EBI Chip Select Assignment Register */

/* PIO Registers */
#define PIO_BSR                       0x00000074	/* Select B Peripheral*/
#define PIO_ASR                       0x00000070	/* Select A Peripheral*/
#define PIO_PUDR                      0x00000060	/* Pullup Disable*/
#define PIO_PUER                      0x00000064	/* Pullup Enable*/
#define PIO_MDDR                      0x00000054	/* Disable Multi-drive */
#define PIO_IDR                       0x00000044	/* Interrupt Disable */
#define PIO_PDSR                      0x0000003c	/* Pin Data Status */
#define PIO_CODR                      0x00000034	/* Set LED */
#define PIO_SODR                      0x00000030	/* Set LED */
#define PIO_ODR                       0x00000014	/* Set Output Disable */
#define PIO_OER                       0x00000010	/* Set Output Enable */
#define PIO_PDR                       0x00000004	/* PIO Disable */ 
#define PIO_PER                       0x00000000	/* PIO Enable */

/* Watchdog Timer Registers */
#define WDTC_MR                       0x00000004	/* Mode Register */

/* PMC Registers */
#define PMC_SCER	0x00000000
#define PMC_PCER	0x00000010
#define CKGR_UCKR	0x0000001C	/* UTMI Clock Register */
#define CKGR_PLLAR	0x00000028
#define PMC_MCKR	0x00000030
#define PMC_USB		0x00000038
#define PMC_SR		0x00000068

/* USB clock Definitions */
#define PLLCOUNT	(0x3 << 20)	/* PLL count for USB Clock */
#define UPLLEN		(0x1 << 16)	/* UTMI PLL is enabled */
#define BIASEN		(0x1<< 24)	/* UTMI BIAS is enabled */
#define LOCKU		0x00000040	/* UPLL lock bit */

/* DBGU Device regiser */
#define SER0_OFFSET		0x00002e00
#define DBGU_OFFSET		0x0
#define DISABLE_TXRX		0x0000000c
#define DBGU_BRGR_VAL		0x00000048

/* DBGU Pins */
#define DBGU_DRXD_BIT	12
#define DBGU_DTXD_BIT	13

/* SPI Definitions */
#define	SPI_FLASH_PAGESIZE		528	/*Page Size for AT45DB321D Data Flash */
#define SELECT_SPI_PINS                 0x0000000f

/* Static Memory Controller (SMC) */
#define AT91_SMC_SETUP(CS_NUM)         (0x00 + CS_NUM*0x10)
#define AT91_SMC_NWESETUP_(x)          ((x) << 0)
#define AT91_SMC_NCS_WRSETUP_(x)       ((x) << 8)
#define AT91_SMC_NRDSETUP_(x)          ((x) << 16)
#define AT91_SMC_NCS_RDSETUP_(x)       ((x) << 24)

#define AT91_SMC_PULSE(CS_NUM)         (0x04 + CS_NUM*0x10)
#define AT91_SMC_NWEPULSE_(x)          ((x) << 0)
#define AT91_SMC_NCS_WRPULSE_(x)       ((x) << 8)
#define AT91_SMC_NRDPULSE_(x)          ((x) << 16)
#define AT91_SMC_NCS_RDPULSE_(x)       ((x) << 24)

#define AT91_SMC_CYCLE(CS_NUM)         (0x08 + CS_NUM*0x10)
#define AT91_SMC_NWECYCLE_(x)          ((x) << 0)
#define AT91_SMC_NRDCYCLE_(x)          ((x) << 16)

#define AT91_SMC_MODE(CS_NUM)          (0x0c + CS_NUM*0x10)
#define AT91_SMC_READMODE              (1 <<  0)                       /* Read Mode */
#define AT91_SMC_WRITEMODE             (1 <<  1)                       /* Write Mode */
#define AT91_SMC_EXNWMODE_DISABLE      (0 << 4)
#define AT91_SMC_DBW_16                (1 << 12)
#define AT91_SMC_TDF_(x)               ((x) << 16)

/*
 * Constants for hardware specific CLE/ALE/NCE function
 *
 * These are bits which can be or'ed to set/clear multiple
 * bits in one go.
 */
/* Select the chip by setting nCE to low */
#define NAND_NCE                0x01
/* Select the command latch by setting CLE to high */
#define NAND_CLE                0x02
/* Select the address latch by setting ALE to high */
#define NAND_ALE                0x04

#define NAND_CTRL_CLE           (NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE           (NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE        0x80
                
/* Nand device specific data structures */
#define NANDCMD_SPAREREAD               0x50
#define NANDCMD_READ                    0x00
#define NANDCMD_READCONFIRM     0x30
#define NANDCMD_PROGRAM                 0x80
#define NANDCMD_PROGRAMCONFIRM  0x10
#define NANDCMD_ERASE                   0x60
#define NANDCMD_ERASECONFIRM    0xD0
#define NANDCMD_IDREAD                  0x90
#define NANDCMD_STATUSREAD              0x70
#define NANDCMD_RESET                   0xFF
#define NANDCMD_NONE                    -1

/* These timeouts are very generous. */
#define MAX_RESET_USEC  600     // 600us
#define MAX_READ_USEC   50      //  50us
#define MAX_POST_USEC   2000    //   2ms
#define MAX_ERASE_USEC  10000   //  10ms

#endif  //  #ifndef _AT91SAM9M10_H_
