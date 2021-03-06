/*
    psftools: Manipulate console fonts in the .PSF format
    Copyright (C) 2005, 2008, 2020  John Elliott

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include <stdio.h>
#include "cnvshell.h"
#include "psflib.h"

/* Convert an Amstrad PCW font generated by PSF2AMS to PSF */

static int v1 = 0, v2 = 0;
static PSF_MAPPING *codepage = NULL;


char *cnv_progname = "AMS2PSF";

static char helpbuf[2048];

char *cnv_set_option(int ddash, char *variable, char *value)
{
	if (!stricmp(variable, "psf1"))   { v1 = 1; return NULL; }
	if (!stricmp(variable, "psf2"))   { v2 = 1; return NULL; }
	if (!stricmp(variable, "codepage") || !stricmp(variable, "setcodepage"))  
	{ 
		codepage = psf_find_mapping(value); 
		if (codepage == NULL) return "Code page name not recognised.";
		return NULL;
	}
	if (strlen(variable) > 2000) variable[2000] = 0;
  
	sprintf(helpbuf, "Unknown option: %s", variable);
	return helpbuf;
}

char *cnv_help(void)
    {
    sprintf(helpbuf, "Syntax: %s { options } amstrad_file psf_file\n\n"
		     "Options:\n"
	             "    --codepage=x   Create a Unicode directory from the specified code page\n"
                     "    --psf1         Output in PSF1 format\n"
                     "    --psf2         Output in PSF2 format (default) \n"
		    ,cnv_progname);
    return helpbuf;
    }

/* PSFCOM magic */
unsigned char psfcom8_magic[] = "\rFont converted with PSF2AMS\r\n\032";
unsigned char psfcom16_magic[] = "\rFont Converted with PSF2AMS\r\n\032";
unsigned char setfont_magic[] =  "\r\033MCharacter set\r\n\032       ";
unsigned char letafont_magic[] = { 0x21, 0x0E, 0x01, 	/* LD HL, 010Eh */
				   0x11, 0x90, 0xE2, 	/* LD DE, 0E290h */
				   0x01, 0x64, 0x08,	/* LD BC, 0864h */
				   0xED, 0xB0,		/* LDIR */
				   0xC3, 0x90, 0xE2 };	/* JP 0E290h */

/* Offsets in PSFCOM: 
 * 0000-000E  Initial code
 * 000F-002D  Signature
 * 002E-002F  Length of font, bytes (2k or 4k)
 * 0030-0031  Address of font */

static unsigned char filebuf[0xC800];

char *cnv_execute(FILE *fpin, FILE *fpout)
{	
	int rv, n;
	PSF_FILE psf;
	psf_dword ch;
	psf_dword y;
	int file_len = 0;
	int found = 0;
	int prefix = 0;	/* Blanks to add at beginning (scrchar.joy) */
	int maxchar = 512;
	int cell_h = 8;
	unsigned char cksum;

	unsigned offset;
	/* The first 8 bytes of the standard Amstrad font */
	static unsigned char font_magic[] = 
		{ 0x00, 0x00, 0x66, 0xDB, 0xDB, 0xDB, 0x66, 0x00 };
	/* And of the +3 narrow font */
	static unsigned char font3_magic[] = 
		{ 0x00, 0x00, 0x50, 0xA8, 0xA8, 0xA8, 0x50, 0x00 };

	file_len = fread(filebuf, 1, sizeof(filebuf), fpin);

	if (file_len >= 128)
	{
		cksum = 0;
		for (n = 0; n < 127; n++)
		{
			cksum += filebuf[n];
		}
		if (cksum == filebuf[127] && !memcmp(filebuf, "SCR",3))
		{
			found = 1;
		}
	}
	if (found)	/* SCRCHAR.JOY detected */
	{
		fprintf(stderr, "LocoScript screen font:\n"
				"     %-30.30s\n"
				"     %-30.30s\n"
				"     %-30.30s\n",
				filebuf + 5, filebuf + 35, filebuf + 65);
		maxchar = 480;
		offset = 0x80;
		prefix = 32;
	}
	/* Font could be: 
	 * - Generated by AMS2PSF */
	else if (!memcmp(filebuf + 0x0F, psfcom8_magic, sizeof(psfcom8_magic)-1))
	{
		found = 1;
		offset = filebuf[0x30] + 256 * (filebuf[0x31] - 1);
		maxchar = (filebuf[0x2e] + 256 * filebuf[0x2F])	/ 8;
		fprintf(stderr, "AMS2PSF 8x8 font detected at 0x%04x, %d characters\n", offset, maxchar);
	}
	else if (!memcmp(filebuf + 0x0F, psfcom16_magic, sizeof(psfcom16_magic)-1))
	{
		found = 1;
		cell_h = 16;
		offset = filebuf[0x30] + 256 * (filebuf[0x31] - 1);
		maxchar = (filebuf[0x2e] + 256 * filebuf[0x2F])	/ 16;
		fprintf(stderr, "AMS2PSF 8x16 font detected at 0x%04x, %d characters\n", offset, maxchar);
	}
	/* - Generated by SETFONT */
	else if (!memcmp(filebuf + 0x66, setfont_magic, sizeof(setfont_magic)-1))
	{
		found = 1;
		offset =  filebuf[4] + 256 * (filebuf[5] - 1);
		maxchar = (filebuf[1] + 256 * filebuf[2]) / 8;
		fprintf(stderr, "SETFONT font detected at 0x%04x, %d characters\n", offset, maxchar);
	}
	/* - Generated by LETAFONT */
	else if (!memcmp(filebuf, letafont_magic, sizeof(letafont_magic)))
	{
		found = 1;
		offset = 0x2C;
		maxchar = 0x100;
		fprintf(stderr, "LETAFONT font detected at 0x%04x, %d characters\n", offset, maxchar);
		
	}
/* - A raw font */
	else if (file_len == 2048 || file_len == 4096)
	{
		found = 1;
		offset = 0;
		maxchar = file_len / 8;
		fprintf(stderr, "Presuming raw font with %d characters\n", maxchar);
	}
	else if (file_len < 0x800)
	{
		return "File is too short to contain an Amstrad CP/M font";
	}
	else	/* Probe for EMS file containing font */
	{
		for (offset = 0; offset < file_len - 0x800; offset++)
		{
			if (!memcmp(filebuf + offset, font_magic, 8)) 
			{
				found = 1;
				break;
			}
		}
		if (!found) 
		{
			return "Cannot determine input file type - no font signature found.";
		}
		/* If there is no 'small' +3 font, limit to 256 chars */
		if (memcmp(filebuf + offset + 0x800, font3_magic, 8))
		{
			maxchar = 256;
		}
		else
		{
			maxchar = 512;
		}
		fprintf(stderr, "Amstrad font signature detected at 0x%04x, %d characters\n", offset, maxchar);
	}

	psf_file_new(&psf);
	rv = psf_file_create(&psf, 8, cell_h, maxchar + prefix, 0);
	if (!rv)
	{
		for (ch = 0; ch < maxchar; ch++)
		{
			for (y = 0; y < cell_h; y++)
			{
				psf.psf_data[psf.psf_charlen * (ch+prefix) + y] 
					= filebuf[offset + ch * cell_h + y];
			}
		}

		if (codepage)
		{
			psf_unicode_addall(&psf, codepage, 0, (maxchar + prefix) - 1);
		}


		if (v1) psf_force_v1(&psf);
		if (v2) psf_force_v2(&psf);
		rv = psf_file_write(&psf, fpout);
	}
	psf_file_delete(&psf);	
	if (rv) return psf_error_string(rv);
	return NULL;
}
