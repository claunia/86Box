/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		Handling of ROM image files.
 *
 * NOTES:	- pc2386 BIOS is corrupt (JMP at F000:FFF0 points to RAM)
 *		- pc2386 video BIOS is underdumped (16k instead of 24k)
 *		- c386sx16 BIOS fails checksum
 *		- the loadfont() calls should be done elsewhere
 *
 * Version:	@(#)rom.c	1.0.6	2017/09/30
 *
 * Authors:	Sarah Walker, <http://pcem-emulator.co.uk/>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Fred N. van Kempen, <decwiz@yahoo.com>
 *		Copyright 2008-2017 Sarah Walker.
 *		Copyright 2016-2017 Miran Grca.
 *		Copyright 2017 Fred N. van Kempen.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>
#include "config.h"
#include "ibm.h"
#include "mem.h"
#include "rom.h"
#include "video/video.h"		/* for loadfont() */


int	romspresent[ROM_MAX];


FILE *
rom_fopen(wchar_t *fn, wchar_t *mode)
{
    wchar_t temp[1024];

    wcscpy(temp, exe_path);
    put_backslash_w(temp);
    wcscat(temp, fn);

    return(_wfopen(temp, mode));
}


int
rom_getfile(wchar_t *fn, wchar_t *s, int size)
{
    FILE *f;

    wcscpy(s, exe_path);
    put_backslash_w(s);
    wcscat(s, fn);

    f = _wfopen(s, L"rb");
    if (f != NULL) {
	(void)fclose(f);
	return(1);
    }

    return(0);
}


int
rom_present(wchar_t *fn)
{
    FILE *f;

    f = rom_fopen(fn, L"rb");
    if (f != NULL) {
	(void)fclose(f);
	return(1);
    }

    return(0);
}


uint8_t
rom_read(uint32_t addr, void *priv)
{
    rom_t *rom = (rom_t *)priv;

#ifdef ROM_TRACE
    if (rom->mapping.base==ROM_TRACE)
	pclog("ROM: read byte from BIOS at %06lX\n", addr);
#endif

    return(rom->rom[addr & rom->mask]);
}


uint16_t
rom_readw(uint32_t addr, void *priv)
{
    rom_t *rom = (rom_t *)priv;

#ifdef ROM_TRACE
    if (rom->mapping.base==ROM_TRACE)
	pclog("ROM: read word from BIOS at %06lX\n", addr);
#endif

    return(*(uint16_t *)&rom->rom[addr & rom->mask]);
}


uint32_t
rom_readl(uint32_t addr, void *priv)
{
    rom_t *rom = (rom_t *)priv;

#ifdef ROM_TRACE
    if (rom->mapping.base==ROM_TRACE)
	pclog("ROM: read long from BIOS at %06lX\n", addr);
#endif

    return(*(uint32_t *)&rom->rom[addr & rom->mask]);
}


/* Load a ROM BIOS from its chips, interleaved mode. */
int
rom_load_linear(wchar_t *fn, uint32_t addr, int sz, int off, uint8_t *ptr)
{
    FILE *f = rom_fopen(fn, L"rb");
        
    if (f == NULL) {
	pclog("ROM: image '%ws' not found\n", fn);
	return(0);
    }

    /* Make sure we only look at the base-256K offset. */
    if (addr >= 0x40000)
    {
	addr = 0;
    }
    else
    {
	addr &= 0x03ffff;
    }

    (void)fseek(f, off, SEEK_SET);
    (void)fread(ptr+addr, sz, 1, f);
    (void)fclose(f);

    return(1);
}


/* Load a ROM BIOS from its chips, interleaved mode. */
int
rom_load_interleaved(wchar_t *fnl, wchar_t *fnh, uint32_t addr, int sz, int off, uint8_t *ptr)
{
    FILE *fl = rom_fopen(fnl, L"rb");
    FILE *fh = rom_fopen(fnh, L"rb");
    int c;

    if (fl == NULL || fh == NULL) {
	if (fl == NULL) pclog("ROM: image '%ws' not found\n", fnl);
	  else (void)fclose(fl);
	if (fh == NULL) pclog("ROM: image '%ws' not found\n", fnh);
	  else (void)fclose(fh);

	return(0);
    }

    /* Make sure we only look at the base-256K offset. */
    if (addr >= 0x40000)
    {
	addr = 0;
    }
    else
    {
	addr &= 0x03ffff;
    }

    (void)fseek(fl, off, SEEK_SET);
    (void)fseek(fh, off, SEEK_SET);
    for (c=0; c<sz; c+=2) {
	ptr[addr+c] = fgetc(fl);
	ptr[addr+c+1] = fgetc(fh);
    }
    (void)fclose(fh);
    (void)fclose(fl);

    return(1);
}


int
rom_init(rom_t *rom, wchar_t *fn, uint32_t addr, int sz, int mask, int off, uint32_t flags)
{
    /* Allocate a buffer for the image. */
    rom->rom = malloc(sz);
    memset(rom->rom, 0xff, sz);

    /* Load the image file into the buffer. */
    if (! rom_load_linear(fn, addr, sz, off, rom->rom)) {
	/* Nope.. clean up. */
	free(rom->rom);
	rom->rom = NULL;
	return(-1);
    }

    rom->mask = mask;

    mem_mapping_add(&rom->mapping,
		    addr, sz,
		    rom_read, rom_readw, rom_readl,
		    mem_write_null, mem_write_nullw, mem_write_nulll,
		    rom->rom, flags, rom);

    return(0);
}


int
rom_init_interleaved(rom_t *rom, wchar_t *fnl, wchar_t *fnh, uint32_t addr, int sz, int mask, int off, uint32_t flags)
{
    /* Allocate a buffer for the image. */
    rom->rom = malloc(sz);
    memset(rom->rom, 0xff, sz);

    /* Load the image file into the buffer. */
    if (! rom_load_interleaved(fnl, fnh, addr, sz, off, rom->rom)) {
	/* Nope.. clean up. */
	free(rom->rom);
	rom->rom = NULL;
	return(-1);
    }

    rom->mask = mask;

    mem_mapping_add(&rom->mapping,
		    addr, sz,
		    rom_read, rom_readw, rom_readl,
		    mem_write_null, mem_write_nullw, mem_write_nulll,
		    rom->rom, flags, rom);

    return(0);
}


/* Load the ROM BIOS image(s) for the selected machine into memory. */
int
rom_load_bios(int rom_id)
{
    FILE *f;

    loadfont(L"roms/video/mda/mda.rom", 0);
    loadfont(L"roms/video/wyse700/wy700.rom", 3);

    /* If not done yet, allocate a 128KB buffer for the BIOS ROM. */
    if (rom == NULL)
	rom = (uint8_t *)malloc(131072);
    memset(rom, 0xff, 131072);

    /* Default to a 64K ROM BIOS image. */
    biosmask = 0xffff;

    /* Zap the BIOS ROM EXTENSION area. */
    memset(romext, 0xff, 0x8000);
    mem_mapping_disable(&romext_mapping);

    switch (rom_id) {
	case ROM_IBMPC:		/* IBM PC */
		if (! rom_load_linear(
			L"roms/machines/ibmpc/pc102782.bin",
			0x00e000, 8192, 0, rom)) break;

		/* Try to load the (full) BASIC ROM. */
		if (rom_load_linear(
			L"roms/machines/ibmpc/ibm-basic-1.10.rom",
			0x006000, 32768, 0, rom)) return(1);

		/* Nope. Try to load the first BASIC ROM image. */
		if (! rom_load_linear(
			L"roms/machines/ibmpc/basicc11.f6",
			0x006000, 8192, 0, rom)) return(1);	/* nope */
		if (! rom_load_linear(
			L"roms/machines/ibmpc/basicc11.f8",
			0x008000, 8192, 0, rom)) break;		/* nope */
		if (! rom_load_linear(
			L"roms/machines/ibmpc/basicc11.fa",
			0x00a000, 8192, 0, rom)) break;		/* nope */
		if (! rom_load_linear(
			L"roms/machines/ibmpc/basicc11.fc",
			0x00c000, 8192, 0, rom)) break;		/* nope */
		return(1);

	case ROM_IBMXT:		/* IBM PX-XT */
		if (rom_load_linear(
			L"roms/machines/ibmxt/xt.rom",
			0x000000, 65536, 0, rom)) return(1);

		if (! rom_load_linear(
			L"roms/machines/ibmxt/5000027.u19",
			0x000000, 32768, 0, rom)) break;
		if (rom_load_linear(
			L"roms/machines/ibmxt/1501512.u18",
			0x008000, 32768, 0, rom)) return(1);
		break;

	case ROM_IBMPCJR:	/* IBM PCjr */
		if (rom_load_linear(
			L"roms/machines/ibmpcjr/bios.rom",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_IBMAT:		/* IBM PC-AT */
		if (rom_load_interleaved(
			L"roms/machines/ibmat/62x0820.u27",
			L"roms/machines/ibmat/62x0821.u47",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_GENXT:		/* Generic PC-XT clone */
		if (rom_load_linear(
			L"roms/machines/genxt/pcxt.rom",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_PC1512:	/* Amstrad PC-1512 */
		if (! rom_load_interleaved(
			L"roms/machines/pc1512/40043.v1",
			L"roms/machines/pc1512/40044.v1",
			0x00c000, 16384, 0, rom)) break;
		loadfont(L"roms/machines/pc1512/40078.ic127", 2);
		return(1);

	case ROM_PC1640:	/* Amstrad PC-1640 */
		if (! rom_load_interleaved(
			L"roms/machines/pc1640/40044.v3",
			L"roms/machines/pc1640/40043.v3",
			0x00c000, 16384, 0, rom)) break;
		f = rom_fopen(L"roms/machines/pc1640/40100", L"rb");
		if (f == NULL) break;
		(void)fclose(f);
		return(1);

	case ROM_PC200:
		if (! rom_load_interleaved(
			L"roms/machines/pc200/pc20v2.1",
			L"roms/machines/pc200/pc20v2.0",
			0x00c000, 16384, 0, rom)) break;
		loadfont(L"roms/machines/pc200/40109.bin", 1);
		return(1);

	case ROM_TANDY:
		if (rom_load_linear(
			L"roms/machines/tandy/tandy1t1.020",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_TANDY1000HX:
		if (! rom_load_linear(
			L"roms/machines/tandy1000hx/v020000.u12",
			0x000000, 0x20000, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_TANDY1000SL2:
		if (rom_load_interleaved(
			L"roms/machines/tandy1000sl2/8079047.hu1",
			L"roms/machines/tandy1000sl2/8079048.hu2",
			0x000000, 65536, 0x30000/2, rom)) return(1);
		break;

	case ROM_PORTABLE:
		if (rom_load_linear(
			L"roms/machines/portable/Compaq Portable Plus 100666-001 Rev C u47.bin",
			0x00e000, 8192, 0, rom)) return(1);
		break;

#if NOT_USED
	case ROM_PORTABLEII:
		if (! rom_load_interleaved(
			L"roms/machines/portableii/106438-001.BIN",
			L"roms/machines/portableii/106437-001.BIN",
			0x000000, 32768, 0, rom)) break;
		biosmask = 0x7fff;
		return(1);

	case ROM_PORTABLEIII:
	case ROM_PORTABLEIII386:
		if (rom_load_interleaved(
			L"roms/machines/portableiii/109738-002.BIN",
			L"roms/machines/portableiii/109737-002.BIN",
			0x000000, 32768, 0, rom)) return(1);
		biosmask = 0x7fff;
		break;
#endif

	case ROM_DTKXT:
		if (rom_load_linear(
			L"roms/machines/dtk/DTK_ERSO_2.42_2764.bin",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_OLIM24:
		if (rom_load_interleaved(
			L"roms/machines/olivetti_m24/olivetti_m24_version_1.43_low.bin",
			L"roms/machines/olivetti_m24/olivetti_m24_version_1.43_high.bin",
			0x00c000, 16384, 0, rom)) return(1);
		break;

	case ROM_PC2086:
		if (! rom_load_interleaved(
			L"roms/machines/pc2086/40179.ic129",
			L"roms/machines/pc2086/40180.ic132",
			0x000000, 16384, 0, rom)) break;
		f = rom_fopen(L"roms/machines/pc2086/40186.ic171", L"rb");
		if (f == NULL) break;
		(void)fclose(f);
		biosmask = 0x3fff;
		return(1);

	case ROM_PC3086:
		if (! rom_load_linear(
			L"roms/machines/pc3086/fc00.bin",
			0x000000, 16384, 0, rom)) break;
		f = rom_fopen(L"roms/machines/pc3086/c000.bin", L"rb");
		if (f == NULL) break;
		(void)fclose(f);
		biosmask = 0x3fff;		
		return(1);

	case ROM_CMDPC30:
		if (! rom_load_interleaved(
			L"roms/machines/cmdpc30/commodore pc 30 iii even.bin",
			L"roms/machines/cmdpc30/commodore pc 30 iii odd.bin",
			0x000000, 16384, 0, rom)) break;
		biosmask = 0x7fff;
		return(1);

	case ROM_AMI386SX:
		if (rom_load_linear(
			L"roms/machines/ami386/ami386.bin",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_AMI386DX_OPTI495:	/* uses the OPTi 82C495 chipset */
		if (rom_load_linear(
			L"roms/machines/ami386dx/OPT495SX.AMI",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_MR386DX_OPTI495:	/* uses the OPTi 82C495 chipset */
		if (rom_load_linear(
			L"roms/machines/mr386dx/OPT495SX.MR",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_AWARD386SX_OPTI495:	/* uses the OPTi 82C495 chipset */
	case ROM_AWARD386DX_OPTI495:	/* uses the OPTi 82C495 chipset */
	case ROM_AWARD486_OPTI495:	/* uses the OPTi 82C495 chipset */
		if (rom_load_linear(
			L"roms/machines/award495/OPT495S.AWA",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_AMI286:
		if (rom_load_linear(
			L"roms/machines/ami286/amic206.bin",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_AWARD286:
		if (rom_load_linear(
			L"roms/machines/award286/award.bin",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_EUROPC:
		if (rom_load_linear(
			L"roms/machines/europc/50145",
			0x008000, 32768, 0, rom)) return(1);
		break;

	case ROM_MEGAPC:
	case ROM_MEGAPCDX:
		if (rom_load_interleaved(
			L"roms/machines/megapc/41651-bios lo.u18",
			L"roms/machines/megapc/211253-bios hi.u19",
			0x000000, 65536, 0x08000, rom)) return(1);
		break;

	case ROM_AMI486:
		if (rom_load_linear(
			L"roms/machines/ami486/ami486.BIN",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_WIN486:
		if (rom_load_linear(
			L"roms/machines/win486/ALI1429G.AMW",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_430VX:
		if (! rom_load_linear(
			L"roms/machines/430vx/55XWUQ0E.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_REVENGE:
		if (! rom_load_linear(
			L"roms/machines/revenge/1009AF2_.BIO",
			0x010000, 65536, 128, rom)) break;
		if (! rom_load_linear(
			L"roms/machines/revenge/1009AF2_.BI1",
			0x000000, 0x00c000, 128, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_ENDEAVOR:
		if (! rom_load_linear(
			L"roms/machines/endeavor/1006CB0_.BIO",
			0x010000, 65536, 128, rom)) break;
		if (! rom_load_linear(
			L"roms/machines/endeavor/1006CB0_.BI1",
			0x000000, 0x00d000, 128, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_IBMPS1_2011:
		if (! rom_load_linear(
			L"roms/machines/ibmps1es/f80000.bin",
			0x000000, 131072, 0x60000, rom)) break;
		biosmask = 0x1ffff;
		return(1);
 
	case ROM_IBMPS1_2121:
	case ROM_IBMPS1_2121_ISA:
		if (! rom_load_linear(
			L"roms/machines/ibmps1_2121/fc0000.bin",
			0x000000, 131072, 0x20000, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_IBMPS1_2133:
		if (! rom_load_linear(
			L"roms/machines/ibmps1_2133/PS1_2133_52G2974_ROM.bin",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_DESKPRO_386:
		if (rom_load_interleaved(
			L"roms/machines/deskpro386/109592-005.U11.bin",
			L"roms/machines/deskpro386/109591-005.U13.bin",
			0x000000, 32768, 0, rom)) break;
		biosmask = 0x7fff;
		return(1);

	case ROM_AMIXT:
		if (rom_load_linear(
			L"roms/machines/amixt/AMI_8088_BIOS_31JAN89.BIN",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_LTXT:
		if (rom_load_linear(
			L"roms/machines/ltxt/27C64.bin",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_LXT3:
		if (rom_load_linear(
			L"roms/machines/lxt3/27C64D.bin",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_SPC4200P:	/* Samsung SPC-4200P */
		if (rom_load_linear(
			L"roms/machines/spc4200p/U8.01",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_SUPER286TR:	/* Hyundai Super-286TR */
		if (rom_load_linear(
			L"roms/machines/super286tr/hyundai_award286.bin",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_DTK386:	/* uses NEAT chipset */
		if (rom_load_linear(
			L"roms/machines/dtk386/3cto001.bin",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_PXXT:
		if (rom_load_linear(
			L"roms/machines/pxxt/000p001.bin",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_JUKOPC:
		if (rom_load_linear(
			L"roms/machines/jukopc/000o001.bin",
			0x00e000, 8192, 0, rom)) return(1);
		break;

	case ROM_IBMPS2_M30_286:
		if (! rom_load_linear(
			L"roms/machines/ibmps2_m30_286/33f5381a.bin",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_DTK486:
		if (rom_load_linear(
			L"roms/machines/dtk486/4siw005.bin",
			0x000000, 65536, 0, rom)) return(1);
		break;

	case ROM_R418:
		if (! rom_load_linear(
			L"roms/machines/r418/r418i.bin",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

#if 0
	case ROM_586MC1:
		if (! rom_load_linear(
			L"roms/machines/586mc1/IS.34",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);
#endif

	case ROM_PLATO:
		if (! rom_load_linear(
			L"roms/machines/plato/1016AX1_.BIO",
			0x010000, 65536, 128, rom)) break;
		if (! rom_load_linear(
			L"roms/machines/plato/1016AX1_.BI1",
			0x000000, 0x00d000, 128, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_MB500N:
		if (! rom_load_linear(
			L"roms/machines/mb500n/031396S.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_AP53:
		if (! rom_load_linear(
			L"roms/machines/ap53/AP53R2C0.ROM",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_P55T2S:
		if (! rom_load_linear(
			L"roms/machines/p55t2s/S6Y08T.ROM",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_PRESIDENT:
		if (! rom_load_linear(
			L"roms/machines/president/BIOS.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_P54TP4XE:
		if (! rom_load_linear(
			L"roms/machines/p54tp4xe/T15I0302.AWD",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_ACERM3A:
		if (! rom_load_linear(
			L"roms/machines/acerm3a/r01-b3.bin",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_ACERV35N:
		if (! rom_load_linear(
			L"roms/machines/acerv35n/V35ND1S1.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_P55VA:
		if (! rom_load_linear(
			L"roms/machines/p55va/VA021297.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_P55T2P4:
		if (! rom_load_linear(
			L"roms/machines/p55t2p4/0207_J2.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_P55TVP4:
		if (! rom_load_linear(
			L"roms/machines/p55tvp4/TV5I0204.AWD",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_440FX:		/* working Tyan BIOS */
		if (! rom_load_linear(
			L"roms/machines/440fx/NTMAW501.BIN",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_S1668:		/* working Tyan BIOS */
		if (! rom_load_linear(
			L"roms/machines/tpatx/S1668P.ROM",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_THOR:
		if (! rom_load_linear(
			L"roms/machines/thor/1006CN0_.BIO",
			0x010000, 65536, 128, rom)) break;
		if (! rom_load_linear(
			L"roms/machines/thor/1006CN0_.BI1",
			0x000000, 65536, 128, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_MRTHOR:
		if (! rom_load_linear(
			L"roms/machines/mrthor/MR_ATX.BIO",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_ZAPPA:
		if (! rom_load_linear(
			L"roms/machines/zappa/1006BS0_.BIO",
			0x010000, 65536, 128, rom)) break;
		if (! rom_load_linear(
			L"roms/machines/zappa/1006BS0_.BI1",
			0x000000, 65536, 128, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_IBMPS2_M50:
		if (! rom_load_interleaved(
			L"roms/machines/ibmps2_m50/90x7423.zm14",
			L"roms/machines/ibmps2_m50/90x7426.zm16",
			0x000000, 65536, 0, rom)) break;
		if (! rom_load_interleaved(
			L"roms/machines/ibmps2_m50/90x7420.zm13",
			L"roms/machines/ibmps2_m50/90x7429.zm18",
			0x010000, 65536, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_IBMPS2_M55SX:
		if (! rom_load_interleaved(
			L"roms/machines/ibmps2_m55sx/33f8146.zm41",
			L"roms/machines/ibmps2_m55sx/33f8145.zm40",
			0x000000, 65536, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	case ROM_IBMPS2_M80:
	case ROM_IBMPS2_M80_486:
		if (! rom_load_interleaved(
			L"roms/machines/ibmps2_m80/15f6637.bin",
			L"roms/machines/ibmps2_m80/15f6639.bin",
			0x000000, 131072, 0, rom)) break;
		biosmask = 0x1ffff;
		return(1);

	default:
		pclog("ROM: don't know how to handle ROM set %d !\n", rom_id);
    }

    return(0);
}
