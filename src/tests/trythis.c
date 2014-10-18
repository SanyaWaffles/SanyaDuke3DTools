#include <stdio.h>

#define		PATH_DELIMITER "/"

#ifdef _WIN32		// If we're on Win32/Win64

#include <direct.h>
#define GetCurrentDir _getcwd

#else				// If we're on *nix/Apple Mac OS X

#include <unistd.h>
#define GetCurrentDir getcwd

#endif

int main (int argc, char* argv[])
{
	char str[FILENAME_MAX];
	
	GetCurrentDir(str, sizeof(str));

	printf("The cwd is:\n\t\t%s%s\n", str, PATH_DELIMITER);
	fflush(stdout);

	return 0;
}