//*****************************************************************************
// strncpy.c : string function
// 2002/02/04 by Gaku : this is rough sketch
//*****************************************************************************

#include <stddef.h>

//=============================================================================
// copy no more SZ bytes S to D
//=============================================================================
char* GO_strncpy (char *d, char *s, size_t sz)
{
	char *tmp = d;

	while ('¥0' != *s) {
		if (0 == sz)
			break;
		sz--;
		*d++ = *s++;
	}

	while (sz--)
		*d++ = '¥0';

	return tmp;
}
