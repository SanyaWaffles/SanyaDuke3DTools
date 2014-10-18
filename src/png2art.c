/* Copyright (C) 2011 SanyaWaffles
 * Based on code by Mathieu Olivier
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeImage.h>

//
// Types and Constants
//

typedef		signed char		int8_t;
typedef		signed short	int16_t;
typedef		signed int		int32_t;
typedef	  unsigned char		uint8_t;
typedef	  unsigned short	uint16_t;
typedef	  unsigned int		uint32_t;

// Tile Struct
typedef struct {
	uint16_t sizex;
	uint16_t sizey;
	uint32_t animdata;
	uint32_t offset;
} tile_t;

// Line Type Struct
typedef enum {
	LINE_TYPE_UNKNOWN,
	LINE_TYPE_SECTION,
	LINE_TYPE_KEY_VALUE,
	LINE_TYPE_EOF
} linetype_e;


#define		PATH_DELIMITER "/"

#ifdef _WIN32		// If we're on Win32/Win64

#include <direct.h>
#define GetCurrentDir _getcwd

#else				// If we're on *nix/Apple Mac OS X

#include <unistd.h>
#define GetCurrentDir getcwd

#endif

#ifndef __cplusplus
typedef enum {false, true} bool;
#endif

#define MAX_NUMBER_OF_TILES 9216	// Maximum number of tiles
#define MAX_KEY_SIZE 128			// Key size for parsing ini file keys
#define MAX_VALUE_SIZE 128			// Value size for parsing ini file values
#define PALETTE_SIZE (256 * 3)		// Palette size (768)

// Global Variables

static uint32_t numtiles = 0;					// Number of tiles
static uint32_t currfilenum = 0;				// Current .ART index
static uint32_t tilestartnum = 0;				// current file starting number
static uint8_t artfilenum = 0;					// Not sure. Redundant?
static uint8_t maxartfiles = 0;					// Maximum art tile
static uint32_t tileendnum = 255;				// Current file ending number
static tile_t TilesList[MAX_NUMBER_OF_TILES];	// list of tiles

// Animation types for Adata###.ini parser
static const char* animtypes[4] = {"none", "oscillation", "forward", "backward"};

// Holds palette. Currently unused
static uint8_t palette[PALETTE_SIZE];

static RGBQUAD rgbpal[256];

// Stores input/output directory strings
static char palfilestr[FILENAME_MAX];
static char inputdir[FILENAME_MAX];
static char outputdir[FILENAME_MAX];

// Method prototypes
static bool createArtFile(const char* afname);

static linetype_e extractAnimDataLine(FILE * animdatafile, char* key, char* value);

static void getAnimData(void);

static bool LoadPalette(char *pfname);

static uint16_t GetLittleEndianUInt16(const uint8_t* buffer);

static void SetLittleEndianUInt16(uint16_t integer, uint8_t* buffer);

static void SetLittleEndianUInt32(uint32_t integer, uint8_t* buffer);

static bool parsePNGFile(uint32_t pngi, FILE* afile);

//
// IMPLEMENTATIONS
//


// CreateArtFile()
// Creates an art file. Pulls in PNGs
static bool createArtFile(const char* afname)
{
	// Variables
	FILE* artfile;
	uint8_t buffer[16 + MAX_NUMBER_OF_TILES * (2 + 2 + 4)];
	uint32_t i;
	uint32_t j;
	char png[FILENAME_MAX];

	artfile = fopen(afname, "wb");
	if (artfile == NULL)
	{
		printf("error: cannot create %d\n", artfilenum);
		return false;
	}

	SetLittleEndianUInt32(1, &buffer[0]);
	SetLittleEndianUInt32(tilestartnum + numtiles, &buffer[4]);
	SetLittleEndianUInt32(tilestartnum, &buffer[8]);
	SetLittleEndianUInt32(tilestartnum + numtiles - 1, &buffer[12]);

	fwrite(buffer, 1, 16 + numtiles * (2 + 2 + 4), artfile);

	for (i = 0; i < numtiles; i++)
	{
		parsePNGFile(tilestartnum + i, artfile);
	}

	getAnimData();

	if (fseek(artfile, 16, SEEK_SET) != 0)
	{
		printf("error: can't go to beginning of art file to write it's header\n");
		fclose(artfile);
		return false;
	}

	for (i = 0; i < numtiles; i++)
		SetLittleEndianUInt16(TilesList[tilestartnum + i].sizex, &buffer[i * 2]);
	fwrite(buffer, 1, numtiles * 2, artfile);

	for (i = 0; i < numtiles; i++)
		SetLittleEndianUInt16(TilesList[tilestartnum + i].sizey, &buffer[i * 2]);
	fwrite(buffer, 1, numtiles * 2, artfile);

	for (i = 0; i < numtiles; i++)
		SetLittleEndianUInt32(TilesList[tilestartnum + i].animdata, &buffer[i * 4]);
	fwrite(buffer, 1, numtiles * 4, artfile);

	fclose(artfile);

	return true;
}

// extractAnimDataLine()
// Extracts a line of data for reading
static linetype_e extractAnimDataLine(FILE* animdatafile, char* key, char* value)
{
	char line[128];
	char* charptr;
	uint32_t ki, vi;

	while (fgets(line, sizeof(line), animdatafile) != NULL)
	{
		charptr = line;
		while (*charptr == ' ')
			charptr++;

		switch(*charptr)
		{
		case '[':
			charptr++;
			while (*charptr == ' ')
				charptr++;

			ki = 0;
			while (*charptr != ']' && *charptr != '\0')
				key[ki++] = *charptr++;
			key[ki] = '\0';
			value[0] = '\0';

			if (*charptr == '\0')
				return LINE_TYPE_UNKNOWN;

			while (ki > 0 && key[ki - 1] == ' ')
				key[--ki] = '\0';

			return LINE_TYPE_SECTION;

		case '\n':
		case ';':
			break;

		default:
			ki = 0;
			while (*charptr != '=' && *charptr != '\0')
				key[ki++] = *charptr++;
			key[ki] = '\0';
			value[0] = '\0';

			if (*charptr == '\0')
				return LINE_TYPE_UNKNOWN;

			while (ki > 0 && key[ki - 1] == ' ')
				key[--ki] = '\0';

			charptr++;
			while (*charptr == ' ')
				charptr++;

			vi = 0;
			while (*charptr != '\n' && *charptr != '\0')
				value[vi++] = *charptr++;
			value[vi] = '\0';

			while(vi > 0 && value[vi - 1] == ' ')
				value[--vi] = '\0';

			return LINE_TYPE_KEY_VALUE;
		}
	}
	
	return LINE_TYPE_EOF;

}

// getAnimData()
static void getAnimData(void)
{
	FILE* adfile;
	linetype_e ltype;
	uint32_t currtilei, tilei1, tilei2;
	int32_t integer;
	char key[MAX_KEY_SIZE], value[MAX_VALUE_SIZE];
	char curradatafile[FILENAME_MAX];
	char* charptr;
	bool validsel;

	sprintf(curradatafile, "%s%sadata%03u.ini", inputdir, PATH_DELIMITER, artfilenum);
	adfile = fopen(curradatafile, "rt");
	if (adfile == NULL)
	{
		printf("warning: %s not found. No animation data added\n", curradatafile);
		return;
	}
	printf("parsing...\n");

	currtilei = tilestartnum;
	ltype = extractAnimDataLine(adfile, key, value);
	while (ltype != LINE_TYPE_EOF)
	{
		switch (ltype)
		{
		case LINE_TYPE_KEY_VALUE:
			if (strcmp(key, "AnimationType") == 0)
			{
				for (integer = 0; integer < 4; integer++)
					if (strcmp(animtypes[integer], value) == 0)
						break;

				if (integer == 4)
				{
					printf("Error: invalid animation type (%s)\n", value);
					break;
				}
				TilesList[currtilei].animdata &= 0xFFFFFF3F;
				TilesList[currtilei].animdata |= (integer & 0x03) << 6;
			}
			else if (strcmp(key, "AnimationSpeed") == 0)
			{
				integer = atoi(value);
				TilesList[currtilei].animdata &= 0xF0FFFFFF;
				TilesList[currtilei].animdata |= (integer & 0x0F) << 24;
			}
			else if (strcmp(key, "XCenterOffset") == 0)
			{
				integer = atoi(value);
				TilesList[currtilei].animdata &= 0xFFFF00FF;
				TilesList[currtilei].animdata |= (uint8_t)((int8_t)integer) << 8;
			}
			else if (strcmp(key, "YCenterOffset") == 0)
			{
				integer = atoi(value);
				TilesList[currtilei].animdata &= 0xFF00FFFF;
				TilesList[currtilei].animdata |= (uint8_t)((int8_t)integer) << 16;
			}
			else if (strcmp(key, "OtherFlags") == 0)
			{
				integer = atoi(value);
				TilesList[currtilei].animdata &= 0x0FFFFFFF;
				TilesList[currtilei].animdata |= (uint8_t)integer << 28;
			}
			else
				printf("error: unknown key %s\n", key);
			break;


		case LINE_TYPE_SECTION:
			do
			{
				validsel = false;

				charptr = strstr(key, "->");
				if (charptr != NULL)
				{
					if (sscanf(key, "tile%u.png -> tile%u.png", &tilei1, &tilei2) != 2)
						printf("Error: invalid selection name (%s)\n", key);
					else if (tilei1 < tilestartnum || tilei1 >= tilestartnum + numtiles)
						printf("error: invalid tile number (%u)\n", tilei1);
					else if (tilei2 < tilestartnum || tilei2 >= tilestartnum + numtiles)
						printf("Error: invalid tile number (%u)\n", tilei2);
					else if (tilei1 > tilei2)
						printf("Error: first tile number greater than second one (%u > %u)\n", tilei1, tilei2);
					else
					{
						currtilei = tilei1;
						TilesList[currtilei].animdata &= 0xFFFFFFC0;
						TilesList[currtilei].animdata |= (tilei2 - tilei1) & 0x3F;
						validsel = true;
					}
				}

				else
				{
					if (sscanf(key, "tile%u.png", &tilei1) != 1)
						printf("error: invalid section name (%s)\n", key);
					else if (tilei1 < tilestartnum || tilei1 >= tilestartnum + numtiles)
						printf("error: invalid tilenumber (%u)\n", tilei1);
					else
					{
						currtilei = tilei1;
						validsel = true;
					}
				}

				if (!validsel)
				{
					ltype = extractAnimDataLine(adfile, key, value);
					while (ltype != LINE_TYPE_SECTION)
					{
						if (ltype != LINE_TYPE_EOF)
						{
							fclose(adfile);
							return;
						}
						ltype = extractAnimDataLine(adfile, key, value);
					}
				}

			} while(!validsel) ;
			break;
		default:
			assert(ltype == LINE_TYPE_UNKNOWN);
			printf("    Warning: syntax error while parsing line %s\n", key);
			break;
			}
		ltype = extractAnimDataLine(adfile, key, value);
	}
	fclose(adfile);
}

static bool LoadPalette(char *pfname)
{
	// variables
	FILE* pfile;

	uint32_t i;
	uint8_t color;

	pfile = fopen(pfname, "rb");
	if (pfile == NULL)
	{
		printf("warning: cannot open palette.dat\n");
		return false;
	}

	// read palette
	if (fread(palette, 1, PALETTE_SIZE, pfile) != PALETTE_SIZE)
	{
		printf("warning: cannot read the whole palette from palette.dat file\n");
		fclose(pfile);
		return false;
	}

	for (i = 0; i < PALETTE_SIZE; i += 3)
	{
		rgbpal[i/3].rgbRed = palette[i] * 4;
		rgbpal[i/3].rgbGreen = palette[i + 1] * 4;
		rgbpal[i/3].rgbBlue = palette[i + 2] * 4;
	}
	
	if (rgbpal[255].rgbRed != 0 && rgbpal[255].rgbGreen != 255 && rgbpal[255].rgbBlue != 255)
	{
		rgbpal[255].rgbRed = 0;
		rgbpal[255].rgbGreen = 247;
		rgbpal[255].rgbBlue = 247;
	}
	
}

// GetLittleEndianUInt16()
// returns a bitshifted LittleEndian 16-bit int
static uint16_t GetLittleEndianUInt16(const uint8_t* buffer)
{
	return (uint16_t)(buffer[0] | (buffer[1] << 8));
}

// Main method
int main(int argc, char* argv[])
{
	char cwd[FILENAME_MAX];
	char path[FILENAME_MAX];	// Temp path
	int8_t tempnum;

	FreeImage_Initialise(0);	// We have to initialize FreeImage library before anything else.
	
	GetCurrentDir(cwd, sizeof(cwd));

	printf("\n"
		"png2art by Kraig Culp\n"
		"based on tga2art by Matthieu Oliver\n\n");

	if (argc != 5)
	{
		printf("syntax: png2art ## palette indir outdir\n"
			"ex: png2art 19 palette.dat pngs newart\n\n");
		FreeImage_DeInitialise();
		return EXIT_FAILURE;
	}

	tilestartnum = 0;
	tileendnum = 255;

	tempnum = atoi(argv[1]);
	
	// check if it's in range
	/* if (tempnum < 0)
		tempnum = 0;
	else if (tempnum > 255)
		tempnum = 255; */
	
	maxartfiles = tempnum;
	sprintf(palfilestr, "%s%s%s", cwd, PATH_DELIMITER, argv[2]);
	sprintf(inputdir, "%s%s%s", cwd, PATH_DELIMITER, argv[3]);
	sprintf(outputdir, "%s%s%s", cwd, PATH_DELIMITER, argv[4]);
	
	if (!LoadPalette(palfilestr))
	{
		FreeImage_DeInitialise();
		return EXIT_FAILURE;
	}

	numtiles = tileendnum - tilestartnum + 1;
	if (tilestartnum > tileendnum)
	{
		printf("error: the number of the first tile is greater than that of the last one!\n");
		FreeImage_DeInitialise();
		return EXIT_FAILURE;
	}

	for (artfilenum = 0; artfilenum <= maxartfiles; artfilenum++)
	{
		tilestartnum = (artfilenum * 256);
		tileendnum = tilestartnum + 256 - 1;

		numtiles = tileendnum - tilestartnum + 1;

		sprintf(path, "%s%sTILES%03u.art", outputdir, PATH_DELIMITER, artfilenum);
		if (!createArtFile(path))
		{
			FreeImage_DeInitialise();
			return EXIT_FAILURE;
		}
	}

	FreeImage_DeInitialise();
	return EXIT_SUCCESS;
}

static void SetLittleEndianUInt16(uint16_t integer, uint8_t* buffer)
{
	buffer[0] = (uint8_t)(integer & 255);
	buffer[1] = (uint8_t)(integer >> 8);
}

static void SetLittleEndianUInt32(uint32_t integer, uint8_t* buffer)
{
	buffer[0] = (uint8_t)(integer & 255);
	buffer[1] = (uint8_t)((integer >> 8) & 255);
	buffer[2] = (uint8_t)((integer >> 16) & 255);
	buffer[3] = (uint8_t)(integer >> 24);
}

// parsePNGFile()
// Takes in a PNG file and plugs the indexes into
// the proper art format.
static bool parsePNGFile(uint32_t pngi, FILE* afile)
{
	FILE* pngfile;
	char pngfilename[FILENAME_MAX];
	uint8_t* buffer;
	uint32_t xsize, ysize;
	int32_t xi, yi;
	FIBITMAP *pngas;
	FIBITMAP *pngaspal;
	FIBITMAP *pngasbg;
	FIBITMAP *pngas24;
	FIBITMAP *pngastemp;
	uint32_t m;
	uint32_t n;
	uint32_t _px;
	uint32_t* _pbx;
	
	pngi;

	TilesList[pngi].animdata = 0;
	TilesList[pngi].offset = ftell(afile);
	TilesList[pngi].sizex = 0;
	TilesList[pngi].sizey = 0;

	sprintf(pngfilename, "%s%stile%04u.png", inputdir, PATH_DELIMITER, pngi);
	
	pngastemp = FreeImage_Load(FIF_PNG, pngfilename, 0);

	if (pngastemp == NULL)
		return false;
		
	pngas = FreeImage_AllocateEx(FreeImage_GetWidth(pngastemp), 
		FreeImage_GetHeight(pngastemp), 8, &rgbpal[255], 0, rgbpal, 0, 0, 0);

	FreeImage_FlipVertical(pngastemp);
	
	if (FreeImage_GetBPP(pngastemp) != 8)
	{
		// printf("ssssnaaaaoooo\n");
		
		if (FreeImage_GetBPP(pngastemp) == 32)
		{
			pngasbg = FreeImage_Composite(pngastemp, FALSE, &rgbpal[255], NULL);
			// pngas24 = FreeImage_ConvertTo24Bits(pngasbg);
			
			
			pngaspal = FreeImage_ColorQuantizeEx(pngasbg, FIQ_NNQUANT, 256, 256, rgbpal);
			FreeImage_SetTransparentIndex(pngaspal, 255);
		}
		else
		{
			pngaspal = FreeImage_ColorQuantizeEx(pngastemp, FIQ_NNQUANT, 256, 256, rgbpal);
			FreeImage_SetTransparentIndex(pngaspal, 255);
		}
		
		if (pngaspal == NULL)
		{
			printf("error: tile%04u.png is an invalid 8/24/32bit image\n", pngi);
			return false;
		}	
			
		pngas = pngaspal;
		
	}
	else
	{
		pngas = pngastemp;
	}
	
	xsize = FreeImage_GetWidth(pngas);
	ysize = FreeImage_GetHeight(pngas);

	TilesList[pngi].sizex = xsize;
	TilesList[pngi].sizey = ysize;

	buffer = malloc(xsize * ysize);
	
	if (buffer == NULL)
	{
		printf("error: not enough memory to read image\n");
		return false;
	}

	// This is where the magic happens
	for (xi = 0; xi  < TilesList[pngi].sizex; xi++)
	{
		for (yi = 0; yi < TilesList[pngi].sizey; yi++)
		{
			FreeImage_GetPixelIndex(pngas, xi, yi, &buffer[xi * TilesList[pngi].sizey + yi]);
			fwrite(&buffer[xi * TilesList[pngi].sizey + yi], 1, 1, afile);
		}
	}
	
	free(buffer);	// Gotta free memory explicitly

	return true;
}