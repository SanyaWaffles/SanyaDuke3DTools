README for Build PNG tools

build-png-tools-v###.zip

ALL THE FILES IN THIS ZIP:
changelog.txt				->		change log for each major version
ideas.txt					->		ideas for future versions
lic-gpl3.txt				->		License for my code
readme.txt					->		This file
PALETTE.DAT					->		test palette for reference (modified a bit, still uses similar color ranges though)
source\art2png.c			->		Source C file for art2png
source\png2art.c			->		Source C file for png2art
source\trythis.c			->		Experiment for working directories, not needed to be compiled.
palettes\duke3d_normal.act	->		Photoshop Raw Color Table for making PNGs with
palettes\duke3d_alt.act		->		Photoshop Raw Color Table for making PNGs with (slightly different, less saturated)

DESCRIPTION:

Observing the Duke 3D community, I wanted to help people edit Duke 3D's tiles in a way that would be quick yet result in classic ART tiles.

See, eDuke32 is great, don't get me wrong. You can create PNGs and use those as tiles using a bunch of tilefromtexture commands.

But if you're going to replace many graphics, like I do, it's better to go vanilla. One problem - no tools exist aside EditART and BastART, which are showing their age.

Sure, there's art2tga - which does it's job - but tga2art always doesn't work, due to it's admittedly messy way it handles tiles.

What I did was I improved these tools by rewriting them to handle PNGs using FreeImage, which works better IMO.

No point beating around the bush, my code isn't that well commented and at times is quite ugly.

There is no excuse, but if I had to pull one out of my behind it's the fact I mainly used ReBuild TGA-ART tools code as a base/reference, which wasn't well commented. Oh well.

Right now it only supports Duke 3D's tiles - which means increments of 256 per tilexxx.art. You can change this for Blood support or other build games.

I have since removed the pre-compiled binaries and freeimage.dll. I have compiled these on Windows and Mac using gcc and it works.

This requires FreeImage.dll to be stored in the same directory the executables are (Win32/Win64) or the FreeImage library to be linked to it (Linux/Unix/MacOS) somehow.

Two things to note about PNGs:
+Assumes PNG palette index #255 is transparent even if not marked so in Photoshop or Paint Shop Pro. If the image isn't indexed properly, the ART files will mess up. I should fix this somehow.
+Thus, make sure to use an act file for making PNGs that "knows" this, so what's transparent to you is transparent to the png2art.

Here is the tool description and syntax:

[ART2PNG]

This extracts the RAW art tiles to indexed PNGs. These PNGs can then be edited.

Syntax:

art2png numofartfiles palettefile inputdir outputdir

numofartfiles	-	total number of art files to process (usually 19 for DN3D Atomic)
palettefile		-	the file (only tested int current working directory) holding Duke 3D's PALETTE.DAT
inputdir		-	the directory where the art files are stored.
outputdir		-	the directory where the pngs will be stored, as well as animation data ini files.

for the directories, make sure they are created before populating or reading from them. mkdir can create directories from the command line on Windows

example syntax:

art2png 19 ./PALETTE.DAT ./tilesin ./pngout

[PNG2ART]

This populates RAW art tiles from indexed PNGs or full-color PNGs. PALETTE.DAT is used to aid conversion to 8-bit ART.

Valid PNGs include:

+8-bit Indexed PNGs
+24-bit full-colour PNGs
+24-bit full colour PNGs with an alpha Channel (aka 32-bit PNGs)

non-PNG formats are not supported and probably never will be.

8-bit indexed PNG images are not verified on a per-color basis, rather their pixels are copied into the art tile buffer.

Syntax:

png2art numofartfiles palettefile inputdir outputdir

numofartfiles	-	total number of art files to process (usually 19 for DN3D atomic)
inputdir		-	the directory where the pngs are stored, as well as animation data ini files.
outputdir		-	the directory where the art files will be created/overwritten.

for the directories, again, make sure they are created before populating/reading from them.

example syntax:

png2art 19 ./PALETTE.DAT ./pngin ./tilesout

Both assume that all files/pngs are going to need extracting/replaced/etc. It's recommended as this is alpha software to do a backup of any work.

These programs are released under the GPL license v3.

I am allowing people to beta-test this, offer constructive feedback. I am open to expanding these.

Look at changelog.txt and ideas.txt for more information.

CREDITS:

SanyaWaffles - Main coding
Mathieu Olivier - Original TGA-based tools, which inspired me to write these
Ken Silverman - Build Engine, documentation for ART format
eDuke32 team - for solving the riddle on how to convert PNGs to ART files
FreeImage - the mere 2MB (on Windows) library which made this all possible. Can be found here: http://freeimage.sourceforge.net/index.html
