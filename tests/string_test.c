/*
** Copyright (C) 2003-2008 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "sfconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include	<sndfile.h>

#include	"utils.h"

#define	BUFFER_LEN			(1 << 10)
#define LOG_BUFFER_SIZE		1024

static void	string_start_test (const char *filename, int typemajor) ;
static void	string_start_end_test (const char *filename, int typemajor) ;
static void	string_rdwr_test (const char *filename, int typemajor) ;

static int libsndfile_str_count (const char * cptr) ;

int
main (int argc, char *argv [])
{	int		do_all = 0 ;
	int		test_count = 0 ;

	if (argc != 2)
	{	printf ("Usage : %s <test>\n", argv [0]) ;
		printf ("    Where <test> is one of the following:\n") ;
		printf ("           wav  - test adding strings to WAV files\n") ;
		printf ("           aiff - test adding strings to AIFF files\n") ;
		printf ("           flac - test adding strings to FLAC files\n") ;
		printf ("           ogg  - test adding strings to OGG files\n") ;
		printf ("           all  - perform all tests\n") ;
		exit (1) ;
		} ;

	do_all =! strcmp (argv [1], "all") ;

	if (do_all || ! strcmp (argv [1], "wav"))
	{	string_start_end_test ("strings.wav", SF_FORMAT_WAV) ;
		string_rdwr_test ("rdwr.wav", SF_FORMAT_WAV) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "aiff"))
	{	string_start_end_test ("strings.aiff", SF_FORMAT_AIFF) ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "flac"))
	{	if (HAVE_EXTERNAL_LIBS)
			string_start_test ("strings.flac", SF_FORMAT_FLAC) ;
		else
			puts ("    No FLAC tests because FLAC support was not compiled in.") ;
		test_count++ ;
		} ;

	if (do_all || ! strcmp (argv [1], "ogg"))
	{	if (HAVE_EXTERNAL_LIBS)
			string_start_test ("vorbis.oga", SF_FORMAT_OGG) ;
		else
			puts ("    No Ogg/Vorbis tests because Ogg/Vorbis support was not compiled in.") ;
		test_count++ ;
		} ;

	if (test_count == 0)
	{	printf ("Mono : ************************************\n") ;
		printf ("Mono : *  No '%s' test defined.\n", argv [1]) ;
		printf ("Mono : ************************************\n") ;
		return 1 ;
		} ;

	return 0 ;
} /* main */


/*============================================================================================
**	Here are the test functions.
*/

static const char
	software	[]	= "software (libsndfile-X.Y.Z)",
	artist		[]	= "The Artist",
	copyright	[]	= "Copyright (c) 2001 The Artist",
	comment		[]	= "Comment goes here!!!",
	date		[]	= "2001/01/27",
	album		[]	= "The Album",
	license		[]	= "The license",
	title		[] = "This is the title" ;

static	short	data_out [BUFFER_LEN] ;

static void
string_start_end_test (const char *filename, int typemajor)
{	const char	*cptr ;
	SNDFILE		*file ;
	SF_INFO		sfinfo ;
	int			frames, errors = 0 ;

	typemajor = typemajor ;

	print_test_name ("string_start_end_test", filename) ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.channels		= 1 ;
	sfinfo.frames		= 0 ;
	sfinfo.format		= typemajor | SF_FORMAT_PCM_16 ;

	frames = BUFFER_LEN / sfinfo.channels ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, SF_TRUE, __LINE__) ;

	/* Write stuff at start of file. */
	sf_set_string (file, SF_STR_TITLE, filename) ;
	sf_set_string (file, SF_STR_SOFTWARE, software) ;
	sf_set_string (file, SF_STR_ARTIST, artist) ;

	/* Write data to file. */
	test_write_short_or_die (file, 0, data_out, BUFFER_LEN, __LINE__) ;
	test_seek_or_die (file, 0, SEEK_SET, 0, sfinfo.channels, __LINE__) ;

	/* Write more stuff at end of file. */
	sf_set_string (file, SF_STR_COPYRIGHT, copyright) ;
	sf_set_string (file, SF_STR_COMMENT, comment) ;
	sf_set_string (file, SF_STR_DATE, date) ;
	sf_set_string (file, SF_STR_ALBUM, album) ;
	sf_set_string (file, SF_STR_LICENSE, license) ;

	sf_close (file) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, SF_TRUE, __LINE__) ;

	check_log_buffer_or_die (file, __LINE__) ;

	if (sfinfo.frames != BUFFER_LEN)
	{	printf ("***** Bad frame count %d (should be %d)\n\n", (int) sfinfo.frames, BUFFER_LEN) ;
		errors ++ ;
		} ;

	cptr = sf_get_string (file, SF_STR_TITLE) ;
	if (cptr == NULL || strcmp (filename, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad filename  : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_COPYRIGHT) ;
	if (cptr == NULL || strcmp (copyright, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad copyright : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_SOFTWARE) ;
	if (cptr == NULL || strstr (cptr, software) != cptr)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad software  : %s\n", cptr) ;
		} ;

	if (libsndfile_str_count (cptr) != 1)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad software  : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_ARTIST) ;
	if (cptr == NULL || strcmp (artist, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad artist    : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_COMMENT) ;
	if (cptr == NULL || strcmp (comment, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad comment   : %s\n", cptr) ;
		} ;

	if (typemajor != SF_FORMAT_AIFF)
	{	cptr = sf_get_string (file, SF_STR_DATE) ;
		if (cptr == NULL || strcmp (date, cptr) != 0)
		{	if (errors++ == 0)
				puts ("\n") ;
			printf ("    Bad date      : %s\n", cptr) ;
			} ;
		} ;

	if (typemajor != SF_FORMAT_WAV && typemajor != SF_FORMAT_AIFF)
	{	cptr = sf_get_string (file, SF_STR_ALBUM) ;
		if (cptr == NULL || strcmp (album, cptr) != 0)
		{	if (errors++ == 0)
				puts ("\n") ;
			printf ("    Bad album   : %s\n", cptr) ;
			} ;
		} ;

	if (typemajor != SF_FORMAT_WAV && typemajor != SF_FORMAT_AIFF)
	{	cptr = sf_get_string (file, SF_STR_LICENSE) ;
		if (cptr == NULL || strcmp (license, cptr) != 0)
		{	if (errors++ == 0)
				puts ("\n") ;
			printf ("    Bad license : %s\n", cptr) ;
			} ;
		} ;

	if (errors > 0)
	{	printf ("\n*** Error count : %d ***\n\n", errors) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	sf_close (file) ;
	unlink (filename) ;

	puts ("ok") ;
} /* string_start_end_test */

static void
string_start_test (const char *filename, int typemajor)
{	const char	*cptr ;
	SNDFILE		*file ;
	SF_INFO		sfinfo ;
	int			frames, errors = 0 ;

	print_test_name ("string_start_test", filename) ;

	sfinfo.samplerate	= 44100 ;
	sfinfo.channels		= 1 ;
	sfinfo.frames		= 0 ;

	switch (typemajor)
	{	case SF_FORMAT_OGG :
			sfinfo.format = typemajor | SF_FORMAT_VORBIS ;
			break ;

		default :
			sfinfo.format = typemajor | SF_FORMAT_PCM_16 ;
			break ;
		} ;

	frames = BUFFER_LEN / sfinfo.channels ;

	file = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, SF_TRUE, __LINE__) ;

	/* Write stuff at start of file. */
	sf_set_string (file, SF_STR_TITLE, filename) ;
	sf_set_string (file, SF_STR_SOFTWARE, software) ;
	sf_set_string (file, SF_STR_ARTIST, artist) ;
	sf_set_string (file, SF_STR_COPYRIGHT, copyright) ;
	sf_set_string (file, SF_STR_COMMENT, comment) ;
	sf_set_string (file, SF_STR_DATE, date) ;
	sf_set_string (file, SF_STR_ALBUM, album) ;
	sf_set_string (file, SF_STR_LICENSE, license) ;

	/* Write data to file. */
	test_write_short_or_die (file, 0, data_out, BUFFER_LEN, __LINE__) ;

	sf_close (file) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, SF_TRUE, __LINE__) ;

	check_log_buffer_or_die (file, __LINE__) ;

	if (sfinfo.frames != BUFFER_LEN)
	{	printf ("***** Bad frame count %d (should be %d)\n\n", (int) sfinfo.frames, BUFFER_LEN) ;
		errors ++ ;
		} ;

	cptr = sf_get_string (file, SF_STR_TITLE) ;
	if (cptr == NULL || strcmp (filename, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad filename  : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_COPYRIGHT) ;
	if (cptr == NULL || strcmp (copyright, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad copyright : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_SOFTWARE) ;
	if (cptr == NULL || strstr (cptr, software) != cptr)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad software  : %s\n", cptr) ;
		} ;

	if (libsndfile_str_count (cptr) != 1)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad software  : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_ARTIST) ;
	if (cptr == NULL || strcmp (artist, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad artist    : %s\n", cptr) ;
		} ;

	cptr = sf_get_string (file, SF_STR_COMMENT) ;
	if (cptr == NULL || strcmp (comment, cptr) != 0)
	{	if (errors++ == 0)
			puts ("\n") ;
		printf ("    Bad comment   : %s\n", cptr) ;
		} ;

	if (typemajor != SF_FORMAT_AIFF)
	{	cptr = sf_get_string (file, SF_STR_DATE) ;
		if (cptr == NULL || strcmp (date, cptr) != 0)
		{	if (errors++ == 0)
				puts ("\n") ;
			printf ("    Bad date      : %s\n", cptr) ;
			} ;
		} ;

	if (typemajor != SF_FORMAT_WAV && typemajor != SF_FORMAT_AIFF)
	{	cptr = sf_get_string (file, SF_STR_ALBUM) ;
		if (cptr == NULL || strcmp (album, cptr) != 0)
		{	if (errors++ == 0)
				puts ("\n") ;
			printf ("    Bad album     : %s\n", cptr) ;
			} ;
		} ;

	if (typemajor != SF_FORMAT_WAV && typemajor != SF_FORMAT_AIFF)
	{	cptr = sf_get_string (file, SF_STR_LICENSE) ;
		if (cptr == NULL || strcmp (license, cptr) != 0)
		{	if (errors++ == 0)
				puts ("\n") ;
			printf ("    Bad license   : %s\n", cptr) ;
			} ;
		} ;

	if (errors > 0)
	{	printf ("\n*** Error count : %d ***\n\n", errors) ;
		dump_log_buffer (file) ;
		exit (1) ;
		} ;

	sf_close (file) ;
	unlink (filename) ;

	puts ("ok") ;
} /* string_start_test */

static void
string_rdwr_test (const char *filename, int typemajor)
{	SNDFILE *file ;
	SF_INFO sfinfo ;
	sf_count_t frames ;
	const char * str ;

	print_test_name (__func__, filename) ;
	create_short_sndfile (filename, typemajor | SF_FORMAT_PCM_16, 2) ;

	file = test_open_file_or_die (filename, SFM_RDWR, &sfinfo, SF_FALSE, __LINE__) ;
	frames = sfinfo.frames ;
	sf_set_string (file, SF_STR_TITLE, title) ;
	sf_close (file) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, SF_FALSE, __LINE__) ;
	exit_if_true (frames != sfinfo.frames, "\n\nLine %d : Frame count %lld should be %lld.\n", __LINE__, sfinfo.frames, frames) ;
	str = sf_get_string (file, SF_STR_TITLE) ;
	exit_if_true (str == NULL, "\n\nLine %d : SF_STR_TITLE string is NULL.\n", __LINE__) ;
	exit_if_true (strcmp (str, title) != 0, "\n\nLine %d : SF_STR_TITLE doesn't match what was written.\n", __LINE__) ;
	sf_close (file) ;

	file = test_open_file_or_die (filename, SFM_RDWR, &sfinfo, SF_FALSE, __LINE__) ;
	frames = sfinfo.frames ;
	sf_set_string (file, SF_STR_TITLE, title) ;
	sf_close (file) ;

	file = test_open_file_or_die (filename, SFM_RDWR, &sfinfo, SF_FALSE, __LINE__) ;
	sf_set_string (file, SF_STR_ARTIST, artist) ;
	sf_close (file) ;

	file = test_open_file_or_die (filename, SFM_READ, &sfinfo, SF_FALSE, __LINE__) ;
	exit_if_true (frames != sfinfo.frames, "\n\nLine %d : Frame count %lld should be %lld.\n", __LINE__, sfinfo.frames, frames) ;

	str = sf_get_string (file, SF_STR_ARTIST) ;
	exit_if_true (str == NULL, "\n\nLine %d : SF_STR_ARTIST string is NULL.\n", __LINE__) ;
	exit_if_true (strcmp (str, artist) != 0, "\n\nLine %d : SF_STR_ARTIST doesn't match what was written.\n", __LINE__) ;

	str = sf_get_string (file, SF_STR_TITLE) ;
	exit_if_true (str == NULL, "\n\nLine %d : SF_STR_TITLE string is NULL.\n", __LINE__) ;
	exit_if_true (strcmp (str, title) != 0, "\n\nLine %d : SF_STR_TITLE doesn't match what was written.\n", __LINE__) ;

	sf_close (file) ;

	unlink (filename) ;

	puts ("ok") ;
} /* string_rdwr_test */

static int
libsndfile_str_count (const char * str)
{	const char * cptr ;

	if (str != NULL)
	{	if ((cptr = strstr (str, "libsndfile")) == NULL)
			return 0 ;

		if ((cptr = strstr (cptr + 1, "libsndfile")) == NULL)
			return 1 ;
		} ;

	return 2 ;
} /* libsndfile_str_count */


