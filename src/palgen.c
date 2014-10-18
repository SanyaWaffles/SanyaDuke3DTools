/* Copyright (C) 2012 SanyaWaffles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef		signed char		int8_t;
typedef		signed short	int16_t;
typedef		signed int		int32_t;
typedef	  unsigned char		uint8_t;
typedef	  unsigned short	uint16_t;
typedef	  unsigned int		uint32_t;

#ifndef __cplusplus
typedef enum {false, true} bool;
#endif

#define		PATH_DELIMITER "/"

#ifdef _WIN32		// If we're on Win32/Win64

#include <direct.h>
#define GetCurrentDir _getcwd

#else				// If we're on *nix/Apple Mac OS X

#include <unistd.h>
#define GetCurrentDir getcwd

#endif

#define TRANS_BYTES 65536
#define	PALETTEBYTES 768
#define	MAXPALOOKUPS 256

const char* PALETTE_FILENAMES[5] = {"main.act", "uwater.act",
						"slime.act", "title.act", "boss1.act"};

uint8_t		main_palette[PALETTEBYTES];
uint8_t		water_palette[PALETTEBYTES];
uint8_t		night_palette[PALETTEBYTES];
uint8_t		title_palette[PALETTEBYTES];
uint8_t		boss1_palette[PALETTEBYTES];
uint8_t		shadetables[MAXPALOOKUPS][PALETTEBYTES /3];
uint8_t		spr_tables[MAXPALOOKUPS][PALETTEBYTES / 3];
uint8_t		transdata[TRANS_BYTES];

uint32_t		shadetablenum;
uint32_t		spritepals;

char		cwd[FILENAME_MAX];
char		collectiondir[FILENAME_MAX];
char		outputdir[FILENAME_MAX];

static bool readPaletteDat(void);

static bool readLookupTable(void);

static bool dumpPalLookupScript(void);

static bool dumpACTPalettes(void);

// static void readPalLookupScript(void);

static bool readPaletteDat(void)
{
	uint32_t i, j;
	char palpath[FILENAME_MAX];
	FILE* pFile;
	
	sprintf(palpath, "%s%s%s%s%s", cwd, PATH_DELIMITER,
		collectiondir, PATH_DELIMITER, "PALETTE.DAT");
		
	// printf("%s\n", palpath);
	
	pFile = fopen(palpath, "rb");
	
	if (pFile == NULL)
	{
		printf("ERROR: PALETTE.DAT not found in \"%s\"\n", collectiondir);
		return false;
	}
	
	for (i = 0; i < PALETTEBYTES; i++)
	{
		main_palette[i] = fgetc(pFile);
	}
	
	shadetablenum = fgetc(pFile);
	
	if (shadetablenum > MAXPALOOKUPS)
	{
		printf("ERROR: SHADETABLENUM > 256!\n");
		return false;
	}
	
	for (i = 0; i <= shadetablenum; i++)
	{
		for (j = 0; j < (PALETTEBYTES / 3); j++)
		{
			shadetables[i][j] = fgetc(pFile);
		}
	}
	
	fclose(pFile);
	
	return true;
}

static bool readLookupTable(void)
{
	uint32_t i, j, id;
	char lookpath[FILENAME_MAX];
	FILE* luFile;
	
	sprintf(lookpath, "%s%s%s%s%s", cwd, PATH_DELIMITER,
		collectiondir, PATH_DELIMITER, "LOOKUP.DAT");
		
	// printf("%s\n", lookpath);
		
	luFile = fopen(lookpath, "rb");
	
	if (luFile == NULL)
	{
		printf("ERROR: LOOKUP.DAT not found\n");
		return false;
	}
	
	fread(&spritepals, sizeof(uint8_t), 1, luFile);
	
	if (spritepals > MAXPALOOKUPS && spritepals < 1)
	{
		printf("ERROR: SHADETABLENUM > 256!\n");
		return false;
	}
	
	for (i = 1; i <= spritepals; i++)
	{
		id = fgetc(luFile);
		printf("id parsed: %i\n", id);
		for (j = 0; j < (PALETTEBYTES / 3); j++)
		{
			spr_tables[id-1][j] = fgetc(luFile);
		}
	}
	
	for (i = 0; i < PALETTEBYTES; i++)
	{
		water_palette[i] = fgetc(luFile);
	}
	
	for (i = 0; i < PALETTEBYTES; i++)
	{
		night_palette[i] = fgetc(luFile);
	}
	
	for (i = 0; i < PALETTEBYTES; i++)
	{
		title_palette[i] = fgetc(luFile);
	}
	
	for (i = 0; i < PALETTEBYTES; i++)
	{
		boss1_palette[i] = fgetc(luFile);
	}
	
	fclose(luFile);
	
	return true;
}

static bool dumpPalLookupScript(void)
{
	uint32_t i, j, ors, ore, dss, dse;		
	uint8_t		difftables[spritepals][PALETTEBYTES / 3];
	bool printcomment;
	char scrpath[FILENAME_MAX];
	
	FILE* scrFile;
	
	sprintf(scrpath, "%s%s%s%s%s", cwd, PATH_DELIMITER, outputdir,
		PATH_DELIMITER, "pal_scr.txt");
		
	scrFile = fopen(scrpath, "wt");
	
	if (scrFile == NULL)
	{
		printf("ERROR: Cannot create pal_scr.txt\n");
		
		return false;
	}
	
	fprintf(scrFile, "EDUKE32_PALETTE_GENSCR\n");
	
	fprintf(scrFile, "[PALETTE.DAT]\n");
	
	fprintf(scrFile, "mainact = %s\n", PALETTE_FILENAMES[0]);
	
	fprintf(scrFile, "%s = %i\n\n", "shades", shadetablenum);
	
	fprintf(scrFile, "%s = %i\n\n", "updatetrans", 1);
	
	fprintf(scrFile, "[LOOKUP.DAT]\n");
	
	fprintf(scrFile, "%s = %i\n", "sprpals", spritepals);
	
	
	for (i = 0; i <= spritepals; i++)
	{
		fprintf(scrFile, "\n%s = %i\n\n", "palswap", i+1);
		if(!printcomment)
		{
			fprintf(scrFile, "; palgen can accept ranges in the form xxx:xxx=yyy:yyy\n");
			fprintf(scrFile, "; rather than individual ranges.\n\n");
			printcomment = true;
		}
		for (j = 0; j < (PALETTEBYTES / 3) ; j++)
		{
			if (spr_tables[i][j] != j)
			{
				fprintf(scrFile, "%i -> %i\n", j, spr_tables[i][j]);
			}
		}
	}
	
	
	fprintf(scrFile, "\nuwtract = %s\n", PALETTE_FILENAMES[1]);
	
	fprintf(scrFile, "nvs_act = %s\n", PALETTE_FILENAMES[2]);
	
	fprintf(scrFile, "ttl_act = %s\n", PALETTE_FILENAMES[3]);
	
	fprintf(scrFile, "bossact = %s\n\n", PALETTE_FILENAMES[4]);
	
	fclose(scrFile);
	
	return true;
}

static bool dumpACTPalettes(void)
{
	uint32_t i, j, temppal, succeed;
	char actpath[FILENAME_MAX];
	uint8_t *	currentPal;
	
	FILE* actFile;
	
	succeed = 0;
	
	for (i = 0; i < 5; i++)
	{
		sprintf(actpath, "%s%s%s%s%s", cwd, PATH_DELIMITER, outputdir,
			PATH_DELIMITER, PALETTE_FILENAMES[i]);
			
		actFile = fopen(actpath, "wb");
		
		if (actFile != NULL)
		{
		
			switch (i)
			{
				case 0:
					currentPal = main_palette;
					break;
				case 1:
					currentPal = water_palette;
					break;
				case 2:
					currentPal = night_palette;
					break;
				case 3:
					currentPal = title_palette;
					break;
				case 4:
					currentPal = boss1_palette;
					break;
				default:
					break;
				
			}
			
			if (currentPal != NULL)
			{
				for (j = 0; j < PALETTEBYTES; j++)
				{
					temppal = (uint8_t)currentPal[j] * 4;
					putc(temppal, actFile);
				}
			}
			else
			{
				printf("ERROR: invalid palette index %i\n", i);
			}
			
			fclose(actFile);
			
			succeed++;
		
		}
		else
		{
			printf("ERROR: Cannot create %s\n", PALETTE_FILENAMES[i]);
			continue;
		}
		
	}
	
	if (succeed == 5)
	{
		return true;
	}
	
	return false;
	
}

/* static void readPalLookupScript(void)
{
	uint8_t 
} */

int main(int argc, char* argv[])
{
	GetCurrentDir(cwd, sizeof(cwd));
	
	printf("\n"
		"palgen by Kraig Culp\n"
		"uses transpal code by Ken Silverman and JonoF and eDuke32\n\n");
		
	if (argc != 4 || !strcmp("--o", argv[1]) || !strcmp("--i", argv[1]))
	{
		printf("syntax: palgen -o|-i [palette.dat & lookup.dat dir] [pal_scr.txt dir]\n"
			"generate script ex: palgen -o ./grp ./palgen\n"
			"generate *.dat ex: palgen -i ./grp ./palgen\n"
			"-o = generate script || -i = generate .dat files\n\n");
			
		return EXIT_FAILURE;
	}
	
	sprintf(collectiondir, "%s", argv[2]);
		
	sprintf(outputdir, "%s", argv[3]);
	
	// printf("cwd: %s\n\n%s and %s\n", cwd, collectiondir, outputdir);
	
	if (!readPaletteDat())
	{
		printf("ERROR: readPaletteDat() failed with return false\n\n" );
		return EXIT_FAILURE;
	}
	
	if (!readLookupTable())
	{
		printf("ERROR: readLookupTable() failed with return false\n\n");
		return EXIT_FAILURE;
	}
	
	if (!dumpPalLookupScript())
	{
		printf("ERROR: dumpPalLookupScript() failed with return false\n\n");
		return EXIT_FAILURE;
	}
	
	if (!dumpACTPalettes())
	{
		printf("ERROR: dumpACTPalettes() failed with return false\n\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}