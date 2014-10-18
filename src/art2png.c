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

typedef struct {
	uint16_t sizex;
	uint16_t sizey;
	uint32_t animdata;
	uint32_t offset;
} tile_t;

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

#define MAX_NUMBER_OF_TILES 9216

#define PALETTE_SIZE (256 * 3)

#define VERSION "0.1.1"

const char* animtypes[4] = {"none", "oscillation", "forward", "backward"};

//
// Global Variables
//

FILE* artfile = NULL;
FILE* artfiles[20];
FILE* palfile = NULL;

uint32_t numtiles = 0;
uint32_t tilestartnum;
uint32_t filenum = 0;
tile_t TilesList[MAX_NUMBER_OF_TILES];

// Color palette
uint8_t palette[PALETTE_SIZE];

const char* artfilename;
RGBQUAD rgbpal[256];

//
// Function
//

// PROTOTYPES
// Dump animation data into "adataXXX.ini"
static bool DumpAnimationData(uint16_t an, char* od);

// extract images from the ART file
static bool ExtractImages(const char* od);

// Get a uint16_t from a little-endian ordered bufffer
static uint16_t GetLittleEndianUInt16(const uint8_t* buffer);

// Get a uint32_t from a little-endian ordered buffer
static uint32_t GetLittleEndianUInt32(const uint8_t* buffer);

// create the pictures list from the art header
static bool GetPicturesList(void);

// load the color palette from the palette.dat or palette.act file
static bool LoadPalette(char *pfname);

// Set a uint16_t into a little-endian ordered buffer
static void SetLittleEndianUInt16(uint16_t number, uint8_t* buffer);

// Extract the picture at tileslist[tileindex] and save it as picname
static bool SpawnPNG(uint32_t ti, const char* picname, const char* outdir);

// Implementations
static bool DumpAnimationData(uint16_t an, char* od)
{
	// Variables
	FILE* animDataFile;
	uint32_t i;
	char str[FILENAME_MAX];

	sprintf(str, "%s%sadata%03u.ini", od, PATH_DELIMITER, an);

	animDataFile = fopen(str, "wt");
	if (animDataFile == NULL)
	{
		printf("Error: cannot create animdata data file\n");
		return false;
	}

	printf("Creating animation data ini file...");
	fflush(stdout);

	fprintf(animDataFile,
		"; this file contains animation data from \"%s\"\n"
		"; extracted by art2png version " VERSION "\n"
		"\n",
		artfilename
		);

	// For each tile
	for (i = 0; i < numtiles; i++)
	{
		// if it has animation data...
		if (TilesList[i].animdata != 0)
		{
			// if the tile has animation data
			// print them first...
			if (((TilesList[i].animdata >> 0) & 0x3F) != 0 ||
				((TilesList[i].animdata >> 6) & 0x03) != 0 ||
				((TilesList[i].animdata >> 24) & 0x0F) != 0)
			{
				fprintf(animDataFile, "[tile%04u.png -> tile%04u.png]\n",
					i + tilestartnum, i + tilestartnum + (TilesList[i].animdata & 0x3F));
				fprintf(animDataFile, "    AnimationType=%s\n",
					animtypes[(TilesList[i].animdata >> 6) & 0x03]);
				fprintf(animDataFile, "    AnimationSpeed=%u\n",
					(TilesList[i].animdata >> 24) & 0x0F);
				fprintf(animDataFile, "\n");
			}

			fprintf(animDataFile, "[tile%04u.png]\n", i + tilestartnum);

			fprintf(animDataFile, "    XCenterOffset=%d\n",
				(int8_t)((TilesList[i].animdata >> 8) & 0xFF));
			fprintf(animDataFile, "    YCenterOffset=%d\n",
				(int8_t)((TilesList[i].animdata >> 16) & 0xFF));
			fprintf(animDataFile, "    OtherFlags=%u\n",
				TilesList[i].animdata >> 28);
			fprintf(animDataFile, "\n");
		}
	}

	fclose(animDataFile);
	printf(" done\n\n");
	return true;
}

// ExtractImages - extract pictures from the ART file

static bool ExtractImages(const char* od)
{
	uint32_t i;
	char imagefilename[13];

	// a little counter
	printf("Extracting images:        0");
	fflush(stdout);

	for (i = 0; i < numtiles; i++)
	{
		// updating counter
		printf("\b\b\b\b%4u", i);
		fflush(stdout);

		sprintf(imagefilename, "tile%04u.png", i + tilestartnum);
		SpawnPNG(i, imagefilename, od);
	}

	printf("\b\b\b\bdone\n\n");
	return true;
}

static bool GetPicturesList(void)
{
	// Veriables
	uint8_t buffer[MAX_NUMBER_OF_TILES * 4];
	uint32_t ver, tileendnum;
	uint32_t i;
	size_t crtoffset;

	if (fread(buffer, 1, 16, artfile) != 16)
	{
		printf("Error: invalid ART file: not enough header data\n");
		return false;
	}
	ver	= GetLittleEndianUInt32(&buffer[0]);
	numtiles = GetLittleEndianUInt32(&buffer[4]);
	tilestartnum = GetLittleEndianUInt32(&buffer[8]);
	tileendnum = GetLittleEndianUInt32(&buffer[12]);

	numtiles = tileendnum - tilestartnum + 1;

	if (ver != 1)
	{
		printf("error: invalid ART file: invalid version number(%u)\n", ver);
		return false;
	}

	printf("%u tiles declared in the ART header\n", numtiles);

	// Extract sizes
	fread(buffer, 1, numtiles * 2, artfile);
	for (i = 0; i < numtiles; i++)
		TilesList[i].sizex = GetLittleEndianUInt16(&buffer[i * 2]);
	fread(buffer, 1, numtiles * 2, artfile);
	for (i = 0; i < numtiles; i++)
		TilesList[i].sizey = GetLittleEndianUInt16(&buffer[i * 2]);

	fread(buffer, 1, numtiles * 4, artfile);
	for (i = 0; i < numtiles; i++)
		TilesList[i].animdata = GetLittleEndianUInt32(&buffer[i * 4]);

	crtoffset = 16 + numtiles * (2 + 2 + 4);
	for (i = 0; i < numtiles; i++)
	{
		TilesList[i].offset = crtoffset;
		crtoffset += TilesList[i].sizex * TilesList[i].sizey;
	}

	return true;
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
	
}

int main (int argc, char* argv[])
{
	char* numarg;
	char* palfilestr;
	char* dirinstr;
	char* diroutstr;
	char cdout[FILENAME_MAX];
	char currfile[FILENAME_MAX];
	char path[FILENAME_MAX];
	char cwd[FILENAME_MAX];
	char palfile[FILENAME_MAX];
	char dirout[FILENAME_MAX];
	char dirin[FILENAME_MAX];
	uint32_t artn;
	uint32_t extpos = 8;
	uint32_t artcount;

	// header
	printf("\n"
			"Art2PNG version " VERSION " by SanyaWaffles\n"
			"Based on Art2TGA by Mathieu Olivier\n"
			"===================================\n\n"
			);

	if (argc != 5)
	{
		printf("Syntax: art2png <num> <palette> <folder in> <folder out>\n"
				"	Extract pictures from art files in a folder to another folder as pngs\n"
				"	eg: art2png 19 palette.dat folderin folderout\n");
		FreeImage_DeInitialise();
		return EXIT_FAILURE;
	}

	FreeImage_Initialise(0);

	GetCurrentDir(cwd, sizeof(cwd));

	numarg = argv[1];
	palfilestr = argv[2];
	dirinstr = argv[3];
	diroutstr = argv[4];

	sprintf(palfile, "%s%s%s", cwd, PATH_DELIMITER, palfilestr);
	sprintf(dirin, "%s%s%s", cwd, PATH_DELIMITER, dirinstr);
	sprintf(dirout, "%s%s%s", cwd, PATH_DELIMITER, diroutstr);

	artcount = atoi(numarg);

	if (!LoadPalette(palfile))
	{
		FreeImage_DeInitialise();
		return EXIT_FAILURE;
	}

	for (artn = 0; artn <= artcount; artn++)
	{
		sprintf(currfile, "%s%sTILES%03u.ART", dirin, PATH_DELIMITER, artn);
		artfile = fopen(currfile, "rb");
		if (artfile == NULL)
		{
			FreeImage_DeInitialise();
			return EXIT_FAILURE;
		}

		if (!GetPicturesList() || !ExtractImages(dirout) || !DumpAnimationData(artn, dirout))
		{
			fclose(artfile);
			FreeImage_DeInitialise();
			return EXIT_FAILURE;
		}

		fclose(artfile);
	}

	FreeImage_DeInitialise();
	return EXIT_SUCCESS;

}

static bool SpawnPNG(uint32_t ti, const char* picname, const char* outdir)
{
	FILE* imgfile;
	char tmp[FILENAME_MAX];
	uint32_t xindex, yindex;
	uint8_t* ibuff;
	FIBITMAP* pngas;

	const uint32_t picsize = TilesList[ti].sizex * TilesList[ti].sizey;

	if (picsize == 0)
		return true;

	fseek(artfile, TilesList[ti].offset, SEEK_SET);
	
	ibuff = malloc(picsize);

	if (ibuff == NULL)
	{
		printf("error: cannot alloc enough memory to load %s\n", picname);
		return false;
	}
	if (fread(ibuff, 1, picsize, artfile) != picsize)
	{
		printf("error: cannot read enough data in ART file to load %s\n", picname);
		free(ibuff);
		return false;
	}

	pngas = FreeImage_AllocateEx(TilesList[ti].sizex, TilesList[ti].sizey, 8, &rgbpal[255], 0, rgbpal, 0, 0, 0);
	FreeImage_SetTransparentIndex(pngas, 255);

	for (yindex = TilesList[ti].sizey; yindex > 0 ; yindex--)
	{
		for (xindex = 0; xindex <= TilesList[ti].sizex; xindex++)
		{
			FreeImage_SetPixelIndex(pngas, xindex, yindex-1, &ibuff[(yindex-1) + (xindex) * (TilesList[ti].sizey)]);
		}
	}

	FreeImage_FlipVertical(pngas);

	sprintf(tmp, "%s%stile%04u.png", outdir, PATH_DELIMITER, ti + tilestartnum);

	free(ibuff);

	FreeImage_Save(FIF_PNG, pngas, tmp, 0);

	FreeImage_Unload(pngas);

	return true;
}

static uint16_t GetLittleEndianUInt16 (const uint8_t* Buffer)
{
   return (uint16_t)(Buffer[0] | (Buffer[1] << 8));
}

static uint32_t GetLittleEndianUInt32 (const uint8_t* Buffer)
{
   return Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
}