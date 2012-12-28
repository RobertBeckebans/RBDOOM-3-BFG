#include "precompiled.h"
#pragma hdrstop

#include "Unzip.h"

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
/* for more info about .ZIP format, see
      ftp://ftp.cdrom.com/pub/infozip/doc/appnote-970311-iz.zip
   PkWare has also a specification at :
      ftp://ftp.pkware.com/probdesc.zip */


#if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES) && \
                      !defined(CASESENSITIVITYDEFAULT_NO)
#define CASESENSITIVITYDEFAULT_NO
#endif


#ifndef UNZ_BUFSIZE
#define UNZ_BUFSIZE (65536)
#endif

#ifndef UNZ_MAXFILENAMEINZIP
#define UNZ_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
# define ALLOC(size) (Mem_Alloc(size, TAG_IDFILE))
#endif
#ifndef TRYFREE
# define TRYFREE(p) {if (p) Mem_Free(p);}
#endif

#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)


idCVar zip_numSeeks( "zip_numSeeks", "0", CVAR_INTEGER, "" );
idCVar zip_skippedSeeks( "zip_skippedSeeks", "0", CVAR_INTEGER, "" );
idCVar zip_seeksForward( "zip_seeksForward", "0", CVAR_INTEGER, "" );
idCVar zip_seeksBackward( "zip_seeksBackward", "0", CVAR_INTEGER, "" );
idCVar zip_avgSeekDistance( "zip_avgSeekDistance", "0", CVAR_INTEGER, "" );

/* ===========================================================================
     Read a byte from a gz_stream; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been sucessfully opened for reading.
*/

/*
static int unzlocal_getByte(FILE *fin,int *pi)
{
    unsigned char c;
	int err = fread(&c, 1, 1, fin);
    if (err==1)
    {
        *pi = (int)c;
        return UNZ_OK;
    }
    else
    {
        if (ferror(fin))
            return UNZ_ERRNO;
        else
            return UNZ_EOF;
    }
}
*/


/* ===========================================================================
   Reads a long in LSB order from the given gz_stream. Sets
*/
static int unzlocal_getShort( idFile* fin, uLong* pX )
{
	byte s[2];
	if( fin->Read( s, 2 ) != 2 )
	{
		*pX = 0;
		return UNZ_EOF;
	}
	*pX = ( s[1] << 8 ) | s[0];
	return UNZ_OK;
}

static int unzlocal_getLong( idFile* fin, uLong* pX )
{
	byte s[4];
	if( fin->Read( s, 4 ) != 4 )
	{
		*pX = 0;
		return UNZ_EOF;
	}
	*pX = ( s[3] << 24 ) | ( s[2] << 16 ) | ( s[1] << 8 ) | s[0];
	return UNZ_OK;
}


/* My own strcmpi / strcasecmp */
static int strcmpcasenosensitive_internal( const char* fileName1, const char* fileName2 )
{
	for( ;; )
	{
		char c1 = *( fileName1++ );
		char c2 = *( fileName2++ );
		if( ( c1 >= 'a' ) && ( c1 <= 'z' ) )
			c1 -= 0x20;
		if( ( c2 >= 'a' ) && ( c2 <= 'z' ) )
			c2 -= 0x20;
		if( c1 == '\0' )
			return ( ( c2 == '\0' ) ? 0 : -1 );
		if( c2 == '\0' )
			return 1;
		if( c1 < c2 )
			return -1;
		if( c1 > c2 )
			return 1;
	}
}


#ifdef  CASESENSITIVITYDEFAULT_NO
#define CASESENSITIVITYDEFAULTVALUE 2
#else
#define CASESENSITIVITYDEFAULTVALUE 1
#endif

#ifndef STRCMPCASENOSENTIVEFUNCTION
#define STRCMPCASENOSENTIVEFUNCTION strcmpcasenosensitive_internal
#endif

#define BUFREADCOMMENT (0x400)

/*
  Locate the Central directory of a zipfile (at the end, just before
    the global comment)
*/
static uLong unzlocal_SearchCentralDir( idFile* fin )
{
	unsigned char* buf;
	uLong uSizeFile;
	uLong uBackRead;
	uLong uMaxBack = 0xffff; /* maximum size of global comment */
	uLong uPosFound = 0;
	
	if( fin->Seek( 0, FS_SEEK_END ) != 0 )
		return 0;
		
	uSizeFile = fin->Tell();
	
	if( uMaxBack > uSizeFile )
		uMaxBack = uSizeFile;
		
	buf = ( unsigned char* )ALLOC( BUFREADCOMMENT + 4 );
	if( buf == NULL )
		return 0;
		
	uBackRead = 4;
	while( uBackRead < uMaxBack )
	{
		uLong uReadSize, uReadPos ;
		int i;
		if( uBackRead + BUFREADCOMMENT > uMaxBack )
			uBackRead = uMaxBack;
		else
			uBackRead += BUFREADCOMMENT;
		uReadPos = uSizeFile - uBackRead ;
		
		uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ?
					( BUFREADCOMMENT + 4 ) : ( uSizeFile - uReadPos );
					
		if( fin->Seek( uReadPos, FS_SEEK_SET ) != 0 )
			break;
			
		if( fin->Read( buf, uReadSize ) != ( int )uReadSize )
			break;
			
		for( i = ( int )uReadSize - 3; ( i-- ) > 0; )
			if( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) &&
					( ( *( buf + i + 2 ) ) == 0x05 ) && ( ( *( buf + i + 3 ) ) == 0x06 ) )
			{
				uPosFound = uReadPos + i;
				break;
			}
			
		if( uPosFound != 0 )
			break;
	}
	TRYFREE( buf );
	return uPosFound;
}

extern unzFile unzReOpen( const char* path, unzFile file )
{
	unz_s* s;
	idFile_Cached* fin;
	
	fin = fileSystem->OpenExplicitPakFile( path );
	if( fin == NULL )
		return NULL;
		
	s = ( unz_s* )ALLOC( sizeof( unz_s ) );
	memcpy( s, ( unz_s* )file, sizeof( unz_s ) );
	
	s->file = fin;
	s->pfile_in_zip_read = NULL;
	
	return ( unzFile )s;
}

/*
   Translate date/time from Dos format to tm_unz (readable more easilty)
*/
static void unzlocal_DosDateToTmuDate( uLong ulDosDate, tm_unz* ptm )
{
	uLong uDate;
	uDate = ( uLong )( ulDosDate >> 16 );
	ptm->tm_mday = ( uInt )( uDate & 0x1f ) ;
	ptm->tm_mon = ( uInt )( ( ( ( uDate ) & 0x1E0 ) / 0x20 ) - 1 ) ;
	ptm->tm_year = ( uInt )( ( ( uDate & 0x0FE00 ) / 0x0200 ) + 1980 ) ;
	
	ptm->tm_hour = ( uInt )( ( ulDosDate & 0xF800 ) / 0x800 );
	ptm->tm_min = ( uInt )( ( ulDosDate & 0x7E0 ) / 0x20 ) ;
	ptm->tm_sec = ( uInt )( 2 * ( ulDosDate & 0x1f ) ) ;
}


/*
  Get Info about the current file in the zipfile, with internal only info
*/
static int unzlocal_GetCurrentFileInfoInternal( unzFile file,
		unz_file_info* pfile_info,
		unz_file_info_internal
		*pfile_info_internal,
		char* szFileName,
		uLong fileNameBufferSize,
		void* extraField,
		uLong extraFieldBufferSize,
		char* szComment,
		uLong commentBufferSize )
{
	unz_s* s;
	unz_file_info file_info;
	unz_file_info_internal file_info_internal;
	int err = UNZ_OK;
	uLong uMagic;
	long lSeek = 0;
	
	if( file == NULL )
		return UNZ_PARAMERROR;
	s = ( unz_s* )file;
	
	int tellpos = s->file->Tell() - s->pos_in_central_dir + s->byte_before_the_zipfile;
	if( tellpos != 0 )
	{
		if( s->file->Seek( s->pos_in_central_dir + s->byte_before_the_zipfile, FS_SEEK_SET ) != 0 )
		{
			err = UNZ_ERRNO;
		}
		if( tellpos < 0 )
		{
			zip_seeksForward.SetInteger( zip_seeksForward.GetInteger() + 1 );
		}
		else
		{
			zip_seeksBackward.SetInteger( zip_seeksBackward.GetInteger() + 1 );
		}
		
		static long zip_totalSeekSize = 0;
		if( zip_numSeeks.GetInteger() == 0 )
		{
			zip_totalSeekSize = 0;
		}
		zip_totalSeekSize += abs( tellpos );
		
		zip_numSeeks.SetInteger( zip_numSeeks.GetInteger() + 1 );
		zip_avgSeekDistance.SetInteger( zip_totalSeekSize / zip_numSeeks.GetInteger() );
	}
	else
	{
		zip_skippedSeeks.SetInteger( zip_skippedSeeks.GetInteger() + 1 );
	}
	
	
	/* we check the magic */
	if( err == UNZ_OK )
	{
		if( unzlocal_getLong( s->file, &uMagic ) != UNZ_OK )
			err = UNZ_ERRNO;
		else if( uMagic != 0x02014b50 )
			err = UNZ_BADZIPFILE;
	}
	
	if( unzlocal_getShort( s->file, &file_info.version ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.version_needed ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.flag ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.compression_method ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getLong( s->file, &file_info.dosDate ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	unzlocal_DosDateToTmuDate( file_info.dosDate, &file_info.tmu_date );
	
	if( unzlocal_getLong( s->file, &file_info.crc ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getLong( s->file, &file_info.compressed_size ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getLong( s->file, &file_info.uncompressed_size ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.size_filename ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.size_file_extra ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.size_file_comment ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.disk_num_start ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &file_info.internal_fa ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getLong( s->file, &file_info.external_fa ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getLong( s->file, &file_info_internal.offset_curfile ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	lSeek += file_info.size_filename;
	if( ( err == UNZ_OK ) && ( szFileName != NULL ) )
	{
		uLong uSizeRead ;
		if( file_info.size_filename < fileNameBufferSize )
		{
			*( szFileName + file_info.size_filename ) = '\0';
			uSizeRead = file_info.size_filename;
		}
		else
			uSizeRead = fileNameBufferSize;
			
		if( ( file_info.size_filename > 0 ) && ( fileNameBufferSize > 0 ) )
			if( s->file->Read( szFileName, uSizeRead ) != ( int )uSizeRead )
				err = UNZ_ERRNO;
		lSeek -= uSizeRead;
	}
	
	
	if( ( err == UNZ_OK ) && ( extraField != NULL ) )
	{
		uLong uSizeRead ;
		if( file_info.size_file_extra < extraFieldBufferSize )
			uSizeRead = file_info.size_file_extra;
		else
			uSizeRead = extraFieldBufferSize;
			
		if( lSeek != 0 )
		{
			if( s->file->Seek( lSeek, FS_SEEK_CUR ) == 0 )
				lSeek = 0;
			else
				err = UNZ_ERRNO;
		}
		if( ( file_info.size_file_extra > 0 ) && ( extraFieldBufferSize > 0 ) )
			if( s->file->Read( extraField, uSizeRead ) != ( int )uSizeRead )
				err = UNZ_ERRNO;
		lSeek += file_info.size_file_extra - uSizeRead;
	}
	else
		lSeek += file_info.size_file_extra;
		
		
	if( ( err == UNZ_OK ) && ( szComment != NULL ) )
	{
		uLong uSizeRead ;
		if( file_info.size_file_comment < commentBufferSize )
		{
			*( szComment + file_info.size_file_comment ) = '\0';
			uSizeRead = file_info.size_file_comment;
		}
		else
			uSizeRead = commentBufferSize;
			
		if( lSeek != 0 )
		{
			if( s->file->Seek( lSeek, FS_SEEK_CUR ) == 0 )
				lSeek = 0;
			else
				err = UNZ_ERRNO;
		}
		if( ( file_info.size_file_comment > 0 ) && ( commentBufferSize > 0 ) )
			if( s->file->Read( szComment, uSizeRead ) != ( int )uSizeRead )
				err = UNZ_ERRNO;
		lSeek += file_info.size_file_comment - uSizeRead;
	}
	else
		lSeek += file_info.size_file_comment;
		
	if( ( err == UNZ_OK ) && ( pfile_info != NULL ) )
		*pfile_info = file_info;
		
	if( ( err == UNZ_OK ) && ( pfile_info_internal != NULL ) )
		*pfile_info_internal = file_info_internal;
		
	return err;
}

/*
  Get the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int unzGetCurrentFileInfoPosition( unzFile file, unsigned long* pos )
{
	unz_s* s;
	
	if( file == NULL )
		return UNZ_PARAMERROR;
	s = ( unz_s* )file;
	
	*pos = s->pos_in_central_dir;
	return UNZ_OK;
}

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int unzSetCurrentFileInfoPosition( unzFile file, unsigned long pos )
{
	unz_s* s;
	int err;
	
	if( file == NULL )
		return UNZ_PARAMERROR;
	s = ( unz_s* )file;
	
	s->pos_in_central_dir = pos;
	err = unzlocal_GetCurrentFileInfoInternal( file, &s->cur_file_info,
			&s->cur_file_info_internal,
			NULL, 0, NULL, 0, NULL, 0 );
	s->current_file_ok = ( err == UNZ_OK );
	return UNZ_OK;
}

/*
  Read the static header of the current zipfile
  Check the coherency of the static header and info in the end of central
        directory about this file
  store in *piSizeVar the size of extra info in static header
        (filename and size of extra field data)
*/
static int unzlocal_CheckCurrentFileCoherencyHeader( unz_s* s, uInt* piSizeVar,
		uLong* poffset_local_extrafield,
		uInt* psize_local_extrafield )
{
	uLong uMagic, uData, uFlags;
	uLong size_filename;
	uLong size_extra_field;
	int err = UNZ_OK;
	
	*piSizeVar = 0;
	*poffset_local_extrafield = 0;
	*psize_local_extrafield = 0;
	
	if( s->file->Seek( s->cur_file_info_internal.offset_curfile + s->byte_before_the_zipfile, FS_SEEK_SET ) != 0 )
		return UNZ_ERRNO;
		
		
	if( err == UNZ_OK )
	{
		if( unzlocal_getLong( s->file, &uMagic ) != UNZ_OK )
			err = UNZ_ERRNO;
		else if( uMagic != 0x04034b50 )
			err = UNZ_BADZIPFILE;
	}
	
	if( unzlocal_getShort( s->file, &uData ) != UNZ_OK )
		err = UNZ_ERRNO;
	/*
		else if ((err==UNZ_OK) && (uData!=s->cur_file_info.wVersion))
			err=UNZ_BADZIPFILE;
	*/
	if( unzlocal_getShort( s->file, &uFlags ) != UNZ_OK )
		err = UNZ_ERRNO;
		
	if( unzlocal_getShort( s->file, &uData ) != UNZ_OK )
		err = UNZ_ERRNO;
	else if( ( err == UNZ_OK ) && ( uData != s->cur_file_info.compression_method ) )
		err = UNZ_BADZIPFILE;
		
	if( ( err == UNZ_OK ) && ( s->cur_file_info.compression_method != 0 ) &&
			( s->cur_file_info.compression_method != Z_DEFLATED ) )
		err = UNZ_BADZIPFILE;
		
	if( unzlocal_getLong( s->file, &uData ) != UNZ_OK ) /* date/time */
		err = UNZ_ERRNO;
		
	if( unzlocal_getLong( s->file, &uData ) != UNZ_OK ) /* crc */
		err = UNZ_ERRNO;
	else if( ( err == UNZ_OK ) && ( uData != s->cur_file_info.crc ) &&
			 ( ( uFlags & 8 ) == 0 ) )
		err = UNZ_BADZIPFILE;
		
	if( unzlocal_getLong( s->file, &uData ) != UNZ_OK ) /* size compr */
		err = UNZ_ERRNO;
	else if( ( err == UNZ_OK ) && ( uData != s->cur_file_info.compressed_size ) &&
			 ( ( uFlags & 8 ) == 0 ) )
		err = UNZ_BADZIPFILE;
		
	if( unzlocal_getLong( s->file, &uData ) != UNZ_OK ) /* size uncompr */
		err = UNZ_ERRNO;
	else if( ( err == UNZ_OK ) && ( uData != s->cur_file_info.uncompressed_size ) &&
			 ( ( uFlags & 8 ) == 0 ) )
		err = UNZ_BADZIPFILE;
		
		
	if( unzlocal_getShort( s->file, &size_filename ) != UNZ_OK )
		err = UNZ_ERRNO;
	else if( ( err == UNZ_OK ) && ( size_filename != s->cur_file_info.size_filename ) )
		err = UNZ_BADZIPFILE;
		
	*piSizeVar += ( uInt )size_filename;
	
	if( unzlocal_getShort( s->file, &size_extra_field ) != UNZ_OK )
		err = UNZ_ERRNO;
	*poffset_local_extrafield = s->cur_file_info_internal.offset_curfile +
								SIZEZIPLOCALHEADER + size_filename;
	*psize_local_extrafield = ( uInt )size_extra_field;
	
	*piSizeVar += ( uInt )size_extra_field;
	
	return err;
}
