/*
 * Copyright 2013 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/mmu.h>
#include <asm/cache.h>
#include <asm/immap_85xx.h>
#include <asm/io.h>
#include <miiphy.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <fsl_mdio.h>
#include <tsec.h>
#include <mmc.h>
#include <netdev.h>
#include <asm/fsl_ifc.h>
#include <hwconfig.h>
#include <i2c.h>
#include <asm/fsl_ddr_sdram.h>

#ifdef CONFIG_PCI
#include <pci.h>
#include <asm/fsl_pci.h>
#endif

#include "../common/qixis.h"
DECLARE_GLOBAL_DATA_PTR;


int board_early_init_f(void)
{
	struct fsl_ifc *ifc = (void *)CONFIG_SYS_IFC_ADDR;

	setbits_be32(&ifc->ifc_gcr, 1 << IFC_GCR_TBCTL_TRN_TIME_SHIFT);

	return 0;
}

void board_config_serdes_mux(void)
{
	ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	u32 pordevsr = in_be32(&gur->pordevsr);
	u32 srds_cfg = (pordevsr & MPC85xx_PORDEVSR_IO_SEL) >>
				MPC85xx_PORDEVSR_IO_SEL_SHIFT;

	switch (srds_cfg) {
	/* PEX(1) PEX(2) CPRI 2 CPRI 1 */
	case  1:
	case  2:
	case  3:
	case  4:
	case  5:
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
		QIXIS_WRITE_I2C(brdcfg[4], 0x03);
		break;

	/* PEX(1) PEX(2) SGMII1 CPRI 1 */
	case  6:
	case  7:
	case  8:
	case  9:
	case 10:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
		QIXIS_WRITE_I2C(brdcfg[4], 0x01);
		break;

	/* PEX(1) PEX(2) SGMII1 SGMII2 */
	case 11:
	case 32:
		QIXIS_WRITE_I2C(brdcfg[4], 0x00);
		break;

	/* PEX(1) SGMII2 CPRI 2 CPRI 1 */
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
		QIXIS_WRITE_I2C(brdcfg[4], 0x07);
		break;

	/* PEX(1) SGMII2 SGMII1 CPRI 1 */
	case 17:
	case 18:
	case 19:
	case 20:
	case 21:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
		QIXIS_WRITE_I2C(brdcfg[4], 0x05);
		break;

	/* SGMII1 SGMII2 CPRI 2 CPRI 1 */
	case 43:
	case 44:
	case 45:
	case 46:
	case 47:
		QIXIS_WRITE_I2C(brdcfg[4], 0x0F);
		break;


	default:
		break;
	}
}

int board_early_init_r(void)
{
#ifndef CONFIG_SYS_NO_FLASH
	const unsigned int flashbase = CONFIG_SYS_FLASH_BASE;
	const u8 flash_esel = find_tlb_idx((void *)flashbase, 1);

	/*
	 * Remap Boot flash region to caching-inhibited
	 * so that flash can be erased properly.
	 */

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash */
	disable_tlb(flash_esel);

	set_tlb(1, flashbase, CONFIG_SYS_FLASH_BASE_PHYS,
			MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
			0, flash_esel, BOOKE_PAGESZ_64M, 1);

	set_tlb(1, flashbase + 0x4000000,
			CONFIG_SYS_FLASH_BASE_PHYS + 0x4000000,
			MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
			0, flash_esel+1, BOOKE_PAGESZ_64M, 1);
#endif
	board_config_serdes_mux();
	return 0;
}

#ifdef CONFIG_PCI
void pci_init_board(void)
{
	fsl_pcie_init_board(0);
}
#endif /* ifdef CONFIG_PCI */

int checkboard(void)
{
	struct cpu_type *cpu;
	u8 sw;

	cpu = gd->arch.cpu;
	printf("Board: %sQDS\n", cpu->name);

	printf("Sys ID: 0x%02x, Sys Ver: 0x%02x, FPGA Ver: 0x%02x,\n",
	QIXIS_READ(id), QIXIS_READ(arch), QIXIS_READ(scver));

	sw = QIXIS_READ(brdcfg[0]);
	sw = (sw & QIXIS_LBMAP_MASK) >> QIXIS_LBMAP_SHIFT;

	printf("IFC chip select:");
	switch (sw) {
	case 0:
		printf("NOR\n");
		break;
	case 2:
		printf("Promjet\n");
		break;
	case 4:
		printf("NAND\n");
		break;
	default:
		printf("Invalid setting of SW%u\n", QIXIS_LBMAP_SWITCH);
		break;
	}

	return 0;
}

#ifdef CONFIG_TSEC_ENET
int board_eth_init(bd_t *bis)
{
	struct fsl_pq_mdio_info mdio_info;
	struct tsec_info_struct tsec_info[4];
	int num = 0;

#ifdef CONFIG_TSEC1
	SET_STD_TSEC_INFO(tsec_info[num], 1);
	num++;

#endif

#ifdef CONFIG_TSEC2
	SET_STD_TSEC_INFO(tsec_info[num], 2);
	num++;
#endif

	mdio_info.regs = (struct tsec_mii_mng *)CONFIG_SYS_MDIO_BASE_ADDR;
	mdio_info.name = DEFAULT_MII_NAME;

	fsl_pq_mdio_init(bis, &mdio_info);
	tsec_eth_init(bis, tsec_info, num);

	#ifdef CONFIG_PCI
	pci_eth_init(bis);
	#endif

	return 0;
}
#endif

#define USBMUX_SEL_MASK		0xc0
#define USBMUX_SEL_UART2	0xc0
#define USBMUX_SEL_USB		0x40
#define SPIMUX_SEL_UART3	0x80
#define GPS_MUX_SEL_GPS		0x40

#define TSEC_1588_CLKIN_MASK	0x03
#define CON_XCVR_REF_CLK	0x00

int misc_init_r(void)
{
	u8 val;
	ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	u32 porbmsr = in_be32(&gur->porbmsr);
	u32 romloc = (porbmsr >> MPC85xx_PORBMSR_ROMLOC_SHIFT) & 0xf;

	/*Configure 1588 clock-in source from RF Card*/
	val = QIXIS_READ_I2C(brdcfg[5]);
	QIXIS_WRITE_I2C(brdcfg[5],
		(val & ~(TSEC_1588_CLKIN_MASK)) | CON_XCVR_REF_CLK);

	if (hwconfig("uart2") && hwconfig("usb1")) {
		printf("UART2 and USB cannot work together on the board\n");
		printf("Remove one from hwconfig and reset\n");
	} else {
		if (hwconfig("uart2")) {
			val = QIXIS_READ_I2C(brdcfg[5]);
			QIXIS_WRITE_I2C(brdcfg[5],
				(val & ~(USBMUX_SEL_MASK)) | USBMUX_SEL_UART2);
			clrbits_be32(&gur->pmuxcr3,
						MPC85xx_PMUXCR3_USB_SEL_MASK);
			setbits_be32(&gur->pmuxcr3, MPC85xx_PMUXCR3_UART2_SEL);
		} else {
			/* By default USB should be selected.
			* Programming FPGA to select USB. */
			val = QIXIS_READ_I2C(brdcfg[5]);
			QIXIS_WRITE_I2C(brdcfg[5],
				(val & ~(USBMUX_SEL_MASK)) | USBMUX_SEL_USB);
		}

	}

	if (hwconfig("sim")) {
		if (romloc == PORBMSR_ROMLOC_NAND_2K ||
			romloc == PORBMSR_ROMLOC_NOR ||
			romloc == PORBMSR_ROMLOC_SPI) {

			val = QIXIS_READ_I2C(brdcfg[3]);
			QIXIS_WRITE_I2C(brdcfg[3], val|0x10);
			clrbits_be32(&gur->pmuxcr,
				MPC85xx_PMUXCR0_SIM_SEL_MASK);
			setbits_be32(&gur->pmuxcr, MPC85xx_PMUXCR0_SIM_SEL);
		}
	}

	if (hwconfig("uart3")) {
		if (romloc == PORBMSR_ROMLOC_NAND_2K ||
			romloc == PORBMSR_ROMLOC_NOR ||
			romloc == PORBMSR_ROMLOC_SDHC) {

			/* UART3 and SPI1 (Flashes) are muxed together */
			val = QIXIS_READ_I2C(brdcfg[3]);
			QIXIS_WRITE_I2C(brdcfg[3], (val | SPIMUX_SEL_UART3));
			clrbits_be32(&gur->pmuxcr3,
						MPC85xx_PMUXCR3_UART3_SEL_MASK);
			setbits_be32(&gur->pmuxcr3, MPC85xx_PMUXCR3_UART3_SEL);

			/* MUX to select UART3 connection to J24 header
			 * or to GPS */
			val = QIXIS_READ_I2C(brdcfg[6]);
			if (hwconfig("gps"))
				QIXIS_WRITE_I2C(brdcfg[6],
						(val | GPS_MUX_SEL_GPS));
			else
				QIXIS_WRITE_I2C(brdcfg[6],
						(val & ~(GPS_MUX_SEL_GPS)));
		}
	}
	return 0;
}

void fdt_del_node_compat(void *blob, const char *compatible)
{
	int err;
	int off = fdt_node_offset_by_compatible(blob, -1, compatible);
	if (off < 0) {
		printf("WARNING: could not find compatible node %s: %s.\n",
			compatible, fdt_strerror(off));
		return;
	}
	err = fdt_del_node(blob, off);
	if (err < 0) {
		printf("WARNING: could not remove %s: %s.\n",
			compatible, fdt_strerror(err));
	}
}

#if defined(CONFIG_OF_BOARD_SETUP)
void ft_board_setup(void *blob, bd_t *bd)
{
	phys_addr_t base;
	phys_size_t size;

	ft_cpu_setup(blob, bd);

	base = getenv_bootm_low();
	size = getenv_bootm_size();

	#if defined(CONFIG_PCI)
	FT_FSL_PCI_SETUP;
	#endif

	fdt_fixup_memory(blob, (u64)base, (u64)size);

	ccsr_gur_t *gur = (void *)(CONFIG_SYS_MPC85xx_GUTS_ADDR);
	u32 porbmsr = in_be32(&gur->porbmsr);
	u32 romloc = (porbmsr >> MPC85xx_PORBMSR_ROMLOC_SHIFT) & 0xf;

	if (!(hwconfig("uart2") && hwconfig("usb1"))) {
		/* If uart2 is there in hwconfig remove usb node from
		 *  device tree */

		if (hwconfig("uart2")) {
			/* remove dts usb node */
			fdt_del_node_compat(blob, "fsl-usb2-dr");
		} else {
			fdt_fixup_dr_usb(blob, bd);
			fdt_del_node_and_alias(blob, "serial2");
		}
	}

	if (hwconfig("uart3")) {
		if (romloc == PORBMSR_ROMLOC_NAND_2K ||
			romloc == PORBMSR_ROMLOC_NOR ||
			romloc == PORBMSR_ROMLOC_SDHC)
			/* Delete SPI node from the device tree */
				fdt_del_node_and_alias(blob, "spi1");
	} else
		fdt_del_node_and_alias(blob, "serial3");

	if (hwconfig("sim")) {
		if (romloc == PORBMSR_ROMLOC_NAND_2K ||
			romloc == PORBMSR_ROMLOC_NOR ||
			romloc == PORBMSR_ROMLOC_SPI) {

			/* remove dts sdhc node */
			fdt_del_node_compat(blob, "fsl,esdhc");
		} else if (romloc == PORBMSR_ROMLOC_SDHC) {

			/* remove dts sim node */
			fdt_del_node_compat(blob, "fsl,sim-v1.0");
			printf("SIM & SDHC can't work together on the board");
			printf("\nRemove sim from hwconfig and reset\n");
		}
	}
}
#endif
