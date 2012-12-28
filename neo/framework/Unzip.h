
/* unzip.h -- IO for uncompress .zip files using zlib
   Version 0.15 beta, Mar 19th, 1998,

   Copyright (C) 1998 Gilles Vollant

   This unzip package allow extract file from .ZIP file, compatible with PKZip 2.04g
     WinZip, InfoZip tools and compatible.
   Encryption and multi volume ZipFile (span) are not supported.
   Old compressions used by old PKZip 1.x are not supported

   THIS IS AN ALPHA VERSION. AT THIS STAGE OF DEVELOPPEMENT, SOMES API OR STRUCTURE
   CAN CHANGE IN FUTURE VERSION !!
   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution

   Condition of use and distribution are the same than zlib :

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*/
#ifndef __UNZIP_H__
#define __UNZIP_H__

#include <zlib.h>
#include <minizip/unzip.h>

#if defined(STRICTUNZIP) || defined(STRICTZIPUNZIP)
/* like the STRICT of WIN32, we define a pointer that cannot be converted
    from (void*) without cast */
typedef struct TagunzFile__
{
	int unused;
} unzFile__;
typedef unzFile__* unzFile;
#else
typedef void* unzFile;
#endif

/* unz_file_info_interntal contain internal info about a file in zipfile*/
typedef struct unz_file_info_internal_s
{
	unsigned long offset_curfile;/* relative offset of static header 4 unsigned chars */
} unz_file_info_internal;

/* file_in_zip_read_info_s contain internal information about a file in zipfile,
    when reading and decompress it */
typedef struct
{
	char*  read_buffer;         /* internal buffer for compressed data */
	z_stream stream;            /* zLib stream structure for inflate */
	
	unsigned long pos_in_zipfile;       /* position in unsigned char on the zipfile, for fseek*/
	unsigned long stream_initialised;   /* flag set if stream structure is initialised*/
	
	unsigned long offset_local_extrafield;/* offset of the static extra field */
	unsigned int  size_local_extrafield;/* size of the static extra field */
	unsigned long pos_local_extrafield;   /* position in the static extra field in read*/
	
	unsigned long crc32;                /* crc32 of all data uncompressed */
	unsigned long crc32_wait;           /* crc32 we must obtain after decompress all */
	unsigned long rest_read_compressed; /* number of unsigned char to be decompressed */
	unsigned long rest_read_uncompressed;/*number of unsigned char to be obtained after decomp*/
	idFile* file;                  /* io structore of the zipfile */
	unsigned long compression_method;   /* compression method (0==store) */
	unsigned long byte_before_the_zipfile;/* unsigned char before the zipfile, (>0 for sfx)*/
} file_in_zip_read_info_s;


/* unz_s contain internal information about the zipfile
*/
typedef struct
{
	idFile_Cached* file;                  /* io structore of the zipfile */
	unz_global_info gi;       /* public global information */
	unsigned long byte_before_the_zipfile;/* unsigned char before the zipfile, (>0 for sfx)*/
	unsigned long num_file;             /* number of the current file in the zipfile*/
	unsigned long pos_in_central_dir;   /* pos of the current file in the central dir*/
	unsigned long current_file_ok;      /* flag about the usability of the current file*/
	unsigned long central_pos;          /* position of the beginning of the central dir*/
	
	unsigned long size_central_dir;     /* size of the central directory  */
	unsigned long offset_central_dir;   /* offset of start of central directory with
								   respect to the starting disk number */

	unz_file_info cur_file_info; /* public info about the current file in zip*/
	unz_file_info_internal cur_file_info_internal; /* private info about it*/
	file_in_zip_read_info_s* pfile_in_zip_read; /* structure about the current
	                                    file if we are decompressing it */
} unz_s;

#define UNZ_OK                                  (0)
#define UNZ_END_OF_LIST_OF_FILE (-100)
#define UNZ_ERRNO               (Z_ERRNO)
#define UNZ_EOF                 (0)
#define UNZ_PARAMERROR                  (-102)
#define UNZ_BADZIPFILE                  (-103)
#define UNZ_INTERNALERROR               (-104)
#define UNZ_CRCERROR                    (-105)

#define UNZ_CASESENSITIVE		1
#define UNZ_NOTCASESENSITIVE	2
#define UNZ_OSDEFAULTCASE		0

extern unzFile unzReOpen( const char* path, unzFile file );

/*
  Open a Zip file. path contain the full pathname (by example,
     on a Windows NT computer "c:\\zlib\\zlib111.zip" or on an Unix computer
	 "zlib/zlib111.zip".
	 If the zipfile cannot be opened (file don't exist or in not valid), the
	   return value is NULL.
     Else, the return value is a unzFile Handle, usable with other function
	   of this unzip package.
*/

/***************************************************************************/
/* Unzip package allow you browse the directory of the zipfile */

extern int unzGetCurrentFileInfoPosition( unzFile file, unsigned long* pos );

/*
  Get the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/

extern int unzSetCurrentFileInfoPosition( unzFile file, unsigned long pos );

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/

#endif /* __UNZIP_H__ */
