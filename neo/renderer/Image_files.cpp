/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2024 Robert Beckebans
Copyright (C) 2022 Stephen Pridham

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "precompiled.h"
#pragma hdrstop

#undef strncmp

#define STBI_NO_STDIO  // images are passed as buffers
#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../libs/stb/stb_image_write.h"

#define TINYEXR_IMPLEMENTATION
#include "../libs/tinyexr/tinyexr.h"

#include "../libs/mesa/format_r11g11b10f.h"

#include "RenderCommon.h"

/*

This file only has a single entry point:

void R_LoadImage( const char *name, byte **pic, int *width, int *height, bool makePowerOf2 );

*/

/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */
void jpg_Error( const char* fmt, ... )
{
	va_list		argptr;
	char		msg[2048];

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	common->FatalError( "%s", msg );
}

void jpg_Printf( const char* fmt, ... )
{
	va_list		argptr;
	char		msg[2048];

	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	common->Printf( "%s", msg );
}



/*
================
R_WriteTGA
================
*/
void R_WriteTGA( const char* filename, const byte* data, int width, int height, bool flipVertical, const char* basePath )
{
	byte*	buffer;
	int		i;
	int		bufferSize = width * height * 4 + 18;
	int     imgStart = 18;

	idTempArray<byte> buf( bufferSize );
	buffer = ( byte* )buf.Ptr();
	memset( buffer, 0, 18 );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 32;	// pixel size
	if( !flipVertical )
	{
		buffer[17] = ( 1 << 5 );	// flip bit, for normal top to bottom raster order
	}

	// swap rgb to bgr
	for( i = imgStart ; i < bufferSize ; i += 4 )
	{
		buffer[i] = data[i - imgStart + 2];		// blue
		buffer[i + 1] = data[i - imgStart + 1];		// green
		buffer[i + 2] = data[i - imgStart + 0];		// red
		buffer[i + 3] = data[i - imgStart + 3];		// alpha
	}

	fileSystem->WriteFile( filename, buffer, bufferSize, basePath );
}

void LoadTGA( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp );

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

typedef struct _TargaHeader
{
	unsigned char 	id_length, colormap_type, image_type;
	unsigned short	colormap_index, colormap_length;
	unsigned char	colormap_size;
	unsigned short	x_origin, y_origin, width, height;
	unsigned char	pixel_size, attributes;
} TargaHeader;


/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
void LoadTGA( const char* name, byte** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	int		columns, rows, numPixels, fileSize, numBytes;
	byte*	pixbuf;
	int		row, column;
	byte*	buf_p;
	byte*	buffer;
	TargaHeader	targa_header;
	byte*		targa_rgba;

	if( !pic )
	{
		fileSystem->ReadFile( name, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	//
	// load the file
	//
	fileSize = fileSystem->ReadFile( name, ( void** )&buffer, timestamp );
	if( !buffer )
	{
		return;
	}

	buf_p = buffer;

	targa_header.id_length = *buf_p++;
	targa_header.colormap_type = *buf_p++;
	targa_header.image_type = *buf_p++;

	targa_header.colormap_index = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.colormap_length = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.colormap_size = *buf_p++;
	targa_header.x_origin = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.y_origin = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.width = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.height = LittleShort( *( short* )buf_p );
	buf_p += 2;
	targa_header.pixel_size = *buf_p++;
	targa_header.attributes = *buf_p++;

	if( targa_header.image_type != 2 && targa_header.image_type != 10 && targa_header.image_type != 3 )
	{
		common->Error( "LoadTGA( %s ): Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported\n", name );
	}

	if( targa_header.colormap_type != 0 )
	{
		common->Error( "LoadTGA( %s ): colormaps not supported\n", name );
	}

	if( ( targa_header.pixel_size != 32 && targa_header.pixel_size != 24 ) && targa_header.image_type != 3 )
	{
		common->Error( "LoadTGA( %s ): Only 32 or 24 bit images supported (no colormaps)\n", name );
	}

	if( targa_header.image_type == 2 || targa_header.image_type == 3 )
	{
		numBytes = targa_header.width * targa_header.height * ( targa_header.pixel_size >> 3 );
		if( numBytes > fileSize - 18 - targa_header.id_length )
		{
			common->Error( "LoadTGA( %s ): incomplete file\n", name );
		}
	}

	columns = targa_header.width;
	rows = targa_header.height;
	numPixels = columns * rows;

	if( width )
	{
		*width = columns;
	}
	if( height )
	{
		*height = rows;
	}

	targa_rgba = ( byte* )R_StaticAlloc( numPixels * 4, TAG_IMAGE );
	*pic = targa_rgba;

	if( targa_header.id_length != 0 )
	{
		buf_p += targa_header.id_length;  // skip TARGA image comment
	}

	if( targa_header.image_type == 2 || targa_header.image_type == 3 )
	{
		// Uncompressed RGB or gray scale image
		for( row = rows - 1; row >= 0; row-- )
		{
			pixbuf = targa_rgba + row * columns * 4;
			for( column = 0; column < columns; column++ )
			{
				unsigned char red, green, blue, alphabyte;
				switch( targa_header.pixel_size )
				{

					case 8:
						blue = *buf_p++;
						green = blue;
						red = blue;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;

					case 24:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = 255;
						break;
					case 32:
						blue = *buf_p++;
						green = *buf_p++;
						red = *buf_p++;
						alphabyte = *buf_p++;
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						break;
					default:
						common->Error( "LoadTGA( %s ): illegal pixel_size '%d'\n", name, targa_header.pixel_size );
						break;
				}
			}
		}
	}
	else if( targa_header.image_type == 10 )      // Runlength encoded RGB images
	{
		unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;

		red = 0;
		green = 0;
		blue = 0;
		alphabyte = 0xff;

		for( row = rows - 1; row >= 0; row-- )
		{
			pixbuf = targa_rgba + row * columns * 4;
			for( column = 0; column < columns; )
			{
				packetHeader = *buf_p++;
				packetSize = 1 + ( packetHeader & 0x7f );
				if( packetHeader & 0x80 )           // run-length packet
				{
					switch( targa_header.pixel_size )
					{
						case 24:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = 255;
							break;
						case 32:
							blue = *buf_p++;
							green = *buf_p++;
							red = *buf_p++;
							alphabyte = *buf_p++;
							break;
						default:
							common->Error( "LoadTGA( %s ): illegal pixel_size '%d'\n", name, targa_header.pixel_size );
							break;
					}

					for( j = 0; j < packetSize; j++ )
					{
						*pixbuf++ = red;
						*pixbuf++ = green;
						*pixbuf++ = blue;
						*pixbuf++ = alphabyte;
						column++;
						if( column == columns )    // run spans across rows
						{
							column = 0;
							if( row > 0 )
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
				else                              // non run-length packet
				{
					for( j = 0; j < packetSize; j++ )
					{
						switch( targa_header.pixel_size )
						{
							case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							default:
								common->Error( "LoadTGA( %s ): illegal pixel_size '%d'\n", name, targa_header.pixel_size );
								break;
						}
						column++;
						if( column == columns )    // pixel packet run spans across rows
						{
							column = 0;
							if( row > 0 )
							{
								row--;
							}
							else
							{
								goto breakOut;
							}
							pixbuf = targa_rgba + row * columns * 4;
						}
					}
				}
			}
breakOut:
			;
		}
	}

	if( ( targa_header.attributes & ( 1 << 5 ) ) )  			// image flp bit
	{
		if( width != NULL && height != NULL )
		{
			R_VerticalFlip( *pic, *width, *height );
		}
	}

	fileSystem->FreeFile( buffer );
}

/*
=========================================================

JPEG/PNG LOADING

Interfaces with STB_image
=========================================================
*/

/*
=============
LoadSTB_RGBA8
=============
*/
void LoadSTB_RGBA8( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	// load the file
	const byte* fbuffer = NULL;
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	int32 numChannels;

	byte* rgba = stbi_load_from_memory( ( stbi_uc const* ) fbuffer, fileSize, width, height, &numChannels, 4 );

	Mem_Free( ( void* )fbuffer );

	//if( numChannels != 4 )
	//{
	//	common->Error( "LoadSTB_RGBA8( %s ): HDR has not 4 channels\n", filename );
	//}

	if( !rgba )
	{
		common->Warning( "stb_image was unable to load JPG %s : %s\n",
						 filename, stbi_failure_reason() );
		return;
	}

	// *pic must be allocated with R_StaticAlloc(), but stb_image allocates with malloc()
	// (and as there is no R_StaticRealloc(), #define STBI_MALLOC etc won't help)
	// so the decoded data must be copied once
	if( rgba )
	{
		int32 pixelCount = *width * *height;
		byte* out = ( byte* )R_StaticAlloc( pixelCount * 4, TAG_IMAGE );

		*pic = out;

		memcpy( *pic, rgba, pixelCount * 4 );

		stbi_image_free( rgba );
	}
}

// RB begin
/*
=========================================================

PNG LOADING

=========================================================
*/

/*
==================
WriteScreenshotForSTBIW

Callback to each stbi_write_* function
==================
*/
static void WriteScreenshotForSTBIW( void* context, void* data, int size )
{
	idFile* f = ( idFile* )context;
	f->Write( data, size );
}

/*
================
R_WritePNG
================
*/
void R_WritePNG( const char* filename, const byte* data, int bytesPerPixel, int width, int height, const char* basePath )
{
	if( bytesPerPixel != 4  && bytesPerPixel != 3 )
	{
		common->Error( "R_WritePNG( %s ): bytesPerPixel = %i not supported", filename, bytesPerPixel );
	}

	idFileLocal file( fileSystem->OpenFileWrite( filename, basePath ) );
	if( file == NULL )
	{
		common->Printf( "R_WritePNG: Failed to open %s\n", filename );
		return;
	}

	//stbi_write_png_compression_level = idMath::ClampInt( 0, 9, r_screenshotPngCompression.GetInteger() );
	stbi_write_png_to_func( WriteScreenshotForSTBIW, file, width, height, bytesPerPixel, data, bytesPerPixel * width );
}

/*
=========================================================

EXR LOADING

Interfaces with tinyexr
=========================================================
*/

/*
=======================
LoadEXR
=======================
*/
static void LoadEXR( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	// load the file
	const byte* fbuffer = NULL;
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	float* rgba;
	const char* err;

	{
		int ret = LoadEXRFromMemory( &rgba, width, height, fbuffer, fileSize, &err );
		if( ret != 0 )
		{
			common->Error( "LoadEXR( %s ): %s\n", filename, err );
			FreeEXRErrorMessage( err );
			return;
		}
	}

#if 0
	// dump file as .hdr for testing - this works
	{
		idStrStatic< MAX_OSPATH > hdrFileName = "test";
		//hdrFileName.AppendPath( filename );
		hdrFileName.SetFileExtension( ".hdr" );

		int ret = stbi_write_hdr( hdrFileName.c_str(), *width, *height, 4, rgba );

		if( ret == 0 )
		{
			return; // fail
		}
	}
#endif

	if( rgba )
	{
		int32 pixelCount = *width * *height;
		byte* out = ( byte* )R_StaticAlloc( pixelCount * 4, TAG_IMAGE );

		*pic = out;

		// convert to packed R11G11B10F as uint32 for each pixel

		const float* src = rgba;
		byte* dst = out;
		for( int i = 0; i < pixelCount; i++ )
		{
			// read 3 floats and ignore the alpha channel
			float p[3];

			p[0] = src[0];
			p[1] = src[1];
			p[2] = src[2];

			// convert
			uint32_t value = float3_to_r11g11b10f( p );
			*( uint32_t* )dst = value;

			src += 4;
			dst += 4;
		}

		free( rgba );
	}

	// RB: EXR needs to be flipped to match the .tga behavior
	//R_VerticalFlip( *pic, *width, *height );

	Mem_Free( ( void* )fbuffer );
}

/*
================
R_WriteEXR
================
*/
void R_WriteEXR( const char* filename, const void* rgba16f, int channelsPerPixel, int width, int height, const char* basePath )
{
#if 0
	// miniexr.cpp - v0.2 - public domain - 2013 Aras Pranckevicius / Unity Technologies
	//
	// Writes OpenEXR RGB files out of half-precision RGBA or RGB data.
	//
	// Only tested on Windows (VS2008) and Mac (clang 3.3), little endian.
	// Testing status: "works for me".
	//
	// History:
	// 0.2 Source data can be RGB or RGBA now.
	// 0.1 Initial release.

	const unsigned ww = width - 1;
	const unsigned hh = height - 1;
	const unsigned char kHeader[] =
	{
		0x76, 0x2f, 0x31, 0x01, // magic
		2, 0, 0, 0, // version, scanline
		// channels
		'c', 'h', 'a', 'n', 'n', 'e', 'l', 's', 0,
		'c', 'h', 'l', 'i', 's', 't', 0,
		55, 0, 0, 0,
		'B', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // B, half
		'G', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // G, half
		'R', 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, // R, half
		0,
		// compression
		'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'i', 'o', 'n', 0,
		'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'i', 'o', 'n', 0,
		1, 0, 0, 0,
		0, // no compression
		// dataWindow
		'd', 'a', 't', 'a', 'W', 'i', 'n', 'd', 'o', 'w', 0,
		'b', 'o', 'x', '2', 'i', 0,
		16, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		uint8( ww & 0xFF ), uint8( ( ww >> 8 ) & 0xFF ), uint8( ( ww >> 16 ) & 0xFF ), uint8( ( ww >> 24 ) & 0xFF ),
		uint8( hh & 0xFF ), uint8( ( hh >> 8 ) & 0xFF ), uint8( ( hh >> 16 ) & 0xFF ), uint8( ( hh >> 24 ) & 0xFF ),
		// displayWindow
		'd', 'i', 's', 'p', 'l', 'a', 'y', 'W', 'i', 'n', 'd', 'o', 'w', 0,
		'b', 'o', 'x', '2', 'i', 0,
		16, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		uint8( ww & 0xFF ), uint8( ( ww >> 8 ) & 0xFF ), uint8( ( ww >> 16 ) & 0xFF ), uint8( ( ww >> 24 ) & 0xFF ),
		uint8( hh & 0xFF ), uint8( ( hh >> 8 ) & 0xFF ), uint8( ( hh >> 16 ) & 0xFF ), uint8( ( hh >> 24 ) & 0xFF ),
		// lineOrder
		'l', 'i', 'n', 'e', 'O', 'r', 'd', 'e', 'r', 0,
		'l', 'i', 'n', 'e', 'O', 'r', 'd', 'e', 'r', 0,
		1, 0, 0, 0,
		0, // increasing Y
		// pixelAspectRatio
		'p', 'i', 'x', 'e', 'l', 'A', 's', 'p', 'e', 'c', 't', 'R', 'a', 't', 'i', 'o', 0,
		'f', 'l', 'o', 'a', 't', 0,
		4, 0, 0, 0,
		0, 0, 0x80, 0x3f, // 1.0f
		// screenWindowCenter
		's', 'c', 'r', 'e', 'e', 'n', 'W', 'i', 'n', 'd', 'o', 'w', 'C', 'e', 'n', 't', 'e', 'r', 0,
		'v', '2', 'f', 0,
		8, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		// screenWindowWidth
		's', 'c', 'r', 'e', 'e', 'n', 'W', 'i', 'n', 'd', 'o', 'w', 'W', 'i', 'd', 't', 'h', 0,
		'f', 'l', 'o', 'a', 't', 0,
		4, 0, 0, 0,
		0, 0, 0x80, 0x3f, // 1.0f
		// end of header
		0,
	};
	const int kHeaderSize = sizeof( kHeader );

	const int kScanlineTableSize = 8 * height;
	const unsigned pixelRowSize = width * 3 * 2;
	const unsigned fullRowSize = pixelRowSize + 8;

	unsigned bufSize = kHeaderSize + kScanlineTableSize + height * fullRowSize;
	unsigned char* buf = ( unsigned char* )Mem_Alloc( bufSize, TAG_TEMP );
	if( !buf )
	{
		return;
	}

	// copy in header
	memcpy( buf, kHeader, kHeaderSize );

	// line offset table
	unsigned ofs = kHeaderSize + kScanlineTableSize;
	unsigned char* ptr = buf + kHeaderSize;
	for( int y = 0; y < height; ++y )
	{
		*ptr++ = ofs & 0xFF;
		*ptr++ = ( ofs >> 8 ) & 0xFF;
		*ptr++ = ( ofs >> 16 ) & 0xFF;
		*ptr++ = ( ofs >> 24 ) & 0xFF;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
		ofs += fullRowSize;
	}

	// scanline data
	const unsigned char* src = ( const unsigned char* )rgba16f;
	const int stride = channelsPerPixel * 2;
	for( int y = 0; y < height; ++y )
	{
		// coordinate
		*ptr++ = y & 0xFF;
		*ptr++ = ( y >> 8 ) & 0xFF;
		*ptr++ = ( y >> 16 ) & 0xFF;
		*ptr++ = ( y >> 24 ) & 0xFF;
		// data size
		*ptr++ = pixelRowSize & 0xFF;
		*ptr++ = ( pixelRowSize >> 8 ) & 0xFF;
		*ptr++ = ( pixelRowSize >> 16 ) & 0xFF;
		*ptr++ = ( pixelRowSize >> 24 ) & 0xFF;
		// B, G, R
		const unsigned char* chsrc;
		chsrc = src + 4;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 2;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}
		chsrc = src + 0;
		for( int x = 0; x < width; ++x )
		{
			*ptr++ = chsrc[0];
			*ptr++ = chsrc[1];
			chsrc += stride;
		}

		src += width * stride;
	}

	assert( ptr - buf == bufSize );

	fileSystem->WriteFile( filename, buf, bufSize, basePath );

	Mem_Free( buf );

#else

	// TinyEXR version with compression to save disc size

	if( channelsPerPixel != 3 )
	{
		common->Error( "R_WriteEXR( %s ): channelsPerPixel = %i not supported", filename, channelsPerPixel );
	}

	EXRHeader header;
	InitEXRHeader( &header );

	EXRImage image;
	InitEXRImage( &image );

	image.num_channels = 3;

	std::vector<halfFloat_t> images[3];
	images[0].resize( width * height );
	images[1].resize( width * height );
	images[2].resize( width * height );

	halfFloat_t* rgb = ( halfFloat_t* ) rgba16f;

	for( int i = 0; i < width * height; i++ )
	{
		images[0][i] = ( rgb[3 * i + 0] );
		images[1][i] = ( rgb[3 * i + 1] );
		images[2][i] = ( rgb[3 * i + 2] );
	}

	halfFloat_t* image_ptr[3];
	image_ptr[0] = &( images[2].at( 0 ) ); // B
	image_ptr[1] = &( images[1].at( 0 ) ); // G
	image_ptr[2] = &( images[0].at( 0 ) ); // R

	image.images = ( unsigned char** )image_ptr;
	image.width = width;
	image.height = height;

	header.num_channels = 3;
	header.channels = ( EXRChannelInfo* )malloc( sizeof( EXRChannelInfo ) * header.num_channels );

	// Must be BGR(A) order, since most of EXR viewers expect this channel order.
	strncpy( header.channels[0].name, "B", 255 );
	header.channels[0].name[strlen( "B" )] = '\0';
	strncpy( header.channels[1].name, "G", 255 );
	header.channels[1].name[strlen( "G" )] = '\0';
	strncpy( header.channels[2].name, "R", 255 );
	header.channels[2].name[strlen( "R" )] = '\0';

	header.pixel_types = ( int* )malloc( sizeof( int ) * header.num_channels );
	header.requested_pixel_types = ( int* )malloc( sizeof( int ) * header.num_channels );
	for( int i = 0; i < header.num_channels; i++ )
	{
		header.pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT; // pixel type of input image
		header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_HALF; // pixel type of output image to be stored in .EXR
	}

	header.compression_type = TINYEXR_COMPRESSIONTYPE_ZIP;

	byte* buffer = NULL;
	const char* err;
	size_t size = SaveEXRImageToMemory( &image, &header, &buffer, &err );
	if( size == 0 )
	{
		common->Error( "R_WriteEXR( %s ): Save EXR err: %s\n", filename, err );
		FreeEXRErrorMessage( err );
		goto cleanup;
	}

	fileSystem->WriteFile( filename, buffer, size, basePath );

cleanup:
	free( header.channels );
	free( header.pixel_types );
	free( header.requested_pixel_types );

#endif
}
// RB end


/*
=========================================================

HDR LOADING

Interfaces with stb_image
=========================================================
*/


/*
=======================
LoadHDR

RB: load floating point data from memory and convert it into packed R11G11B10F data
=======================
*/
static void LoadHDR( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp )
{
	if( !pic )
	{
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}

	*pic = NULL;

	// load the file
	const byte* fbuffer = NULL;
	int fileSize = fileSystem->ReadFile( filename, ( void** )&fbuffer, timestamp );
	if( !fbuffer )
	{
		return;
	}

	int32 numChannels;

	float* rgb = stbi_loadf_from_memory( ( stbi_uc const* ) fbuffer, fileSize, width, height, &numChannels, 0 );

	if( numChannels != 3 )
	{
		common->Error( "LoadHDR( %s ): HDR has not 3 channels\n", filename );
	}

	if( rgb )
	{
		int32 pixelCount = *width * *height;
		byte* out = ( byte* )R_StaticAlloc( pixelCount * 4, TAG_IMAGE );

		*pic = out;

		// convert to packed R11G11B10F as uint32 for each pixel

		const float* src = rgb;
		byte* dst = out;
		for( int i = 0; i < pixelCount; i++ )
		{
			// read 3 floats and ignore the alpha channel
			float p[3];

			p[0] = src[0];
			p[1] = src[1];
			p[2] = src[2];

			// convert
			uint32_t value = float3_to_r11g11b10f( p );
			*( uint32_t* )dst = value;

			src += 3;
			dst += 4;
		}

		free( rgb );
	}

	Mem_Free( ( void* )fbuffer );
}

//===================================================================


typedef struct
{
	const char*	ext;
	void	( *ImageLoader )( const char* filename, unsigned char** pic, int* width, int* height, ID_TIME_T* timestamp );
} imageExtToLoader_t;

static imageExtToLoader_t imageLoaders[] =
{
	{"png", LoadSTB_RGBA8}, // STB_image also handles PNG
	{"tga", LoadTGA},
	{"jpg", LoadSTB_RGBA8},
	{"exr", LoadEXR},
	{"hdr", LoadHDR},
};

static const int numImageLoaders = sizeof( imageLoaders ) / sizeof( imageLoaders[0] );

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic will be NULL if the load failed.

Anything that is going to make this into a texture would use
makePowerOf2 = true, but something loading an image as a lookup
table of some sort would leave it in identity form.

It is important to do this at image load time instead of texture load
time for bump maps.

Timestamp may be NULL if the value is going to be ignored

If pic is NULL, the image won't actually be loaded, it will just find the
timestamp.
=================
*/
void R_LoadImage( const char* cname, byte** pic, int* width, int* height, ID_TIME_T* timestamp, bool makePowerOf2, textureUsage_t* usage )
{
	idStr name = cname;

	if( pic )
	{
		*pic = NULL;
	}
	if( timestamp )
	{
		*timestamp = FILE_NOT_FOUND_TIMESTAMP;
	}
	if( width )
	{
		*width = 0;
	}
	if( height )
	{
		*height = 0;
	}

	name.DefaultFileExtension( ".tga" );

	if( name.Length() < 5 )
	{
		return;
	}

	name.ToLower();
	idStr ext;
	name.ExtractFileExtension( ext );
	idStr origName = name;

	// RB begin

	// PBR HACK - look for the same file name that provides a _rmao[d] suffix and prefer it
	// if it is available, otherwise
	bool pbrImageLookup = false;
	if( usage && *usage == TD_SPECULAR )
	{
		name.StripFileExtension();

		if( name.StripTrailingOnce( "_s" ) )
		{
			name += "_rmao";

			ext = "png";
			name.DefaultFileExtension( ".png" );

			pbrImageLookup = true;
		}
		else
		{
			name = origName;
		}
	}

retry:

	// try
	if( !ext.IsEmpty() )
	{
		// try only the image with the specified extension: default .tga
		int i;
		for( i = 0; i < numImageLoaders; i++ )
		{
			if( !ext.Icmp( imageLoaders[i].ext ) )
			{
				imageLoaders[i].ImageLoader( name.c_str(), pic, width, height, timestamp );
				break;
			}
		}

		if( i < numImageLoaders )
		{
			if( ( pic && *pic == NULL ) || ( timestamp && *timestamp == FILE_NOT_FOUND_TIMESTAMP ) )
			{
				// image with the specified extension was not found so try all extensions
				for( i = 0; i < numImageLoaders; i++ )
				{
					name.SetFileExtension( imageLoaders[i].ext );
					imageLoaders[i].ImageLoader( name.c_str(), pic, width, height, timestamp );

					if( pic && *pic != NULL )
					{
						//idLib::Warning( "image %s failed to load, using %s instead", origName.c_str(), name.c_str());
						break;
					}

					if( !pic && timestamp && *timestamp != FILE_NOT_FOUND_TIMESTAMP )
					{
						// we are only interested in the timestamp and we got one
						break;
					}
				}
			}
		}

		if( pbrImageLookup )
		{
			if( ( pic && *pic == NULL ) || ( !pic && timestamp && *timestamp == FILE_NOT_FOUND_TIMESTAMP ) )
			{
				name = origName;
				name.ExtractFileExtension( ext );

				pbrImageLookup = false;
				goto retry;
			}

			if( ( pic && *pic != NULL ) || ( !pic && timestamp && *timestamp != FILE_NOT_FOUND_TIMESTAMP ) )
			{
				idLib::Printf( "PBR hack: using '%s' instead of '%s'\n", name.c_str(), origName.c_str() );
				*usage = TD_SPECULAR_PBR_RMAO;
			}
		}
	}
	// RB end

	if( ( width && *width < 1 ) || ( height && *height < 1 ) )
	{
		if( pic && *pic )
		{
			R_StaticFree( *pic );
			*pic = 0;
		}
	}

	//
	// convert to exact power of 2 sizes
	//
	/*
	if ( pic && *pic && makePowerOf2 ) {
		int		w, h;
		int		scaled_width, scaled_height;
		byte	*resampledBuffer;

		w = *width;
		h = *height;

		for (scaled_width = 1 ; scaled_width < w ; scaled_width<<=1)
			;
		for (scaled_height = 1 ; scaled_height < h ; scaled_height<<=1)
			;

		if ( scaled_width != w || scaled_height != h ) {
			resampledBuffer = R_ResampleTexture( *pic, w, h, scaled_width, scaled_height );
			R_StaticFree( *pic );
			*pic = resampledBuffer;
			*width = scaled_width;
			*height = scaled_height;
		}
	}
	*/
}

/*
=======================
R_LoadCubeImages

Loads six files with proper extensions
=======================
*/
bool R_LoadCubeImages( const char* imgName, cubeFiles_t extensions, byte* pics[6], int* outSize, ID_TIME_T* timestamp, int cubeMapSize )
{
	int		i, j;
	const char*	quakeSides[6] =  { "_ft.tga", "_bk.tga", "_lf.tga", "_rt.tga",
								   "_up.tga", "_dn.tga"
								 };
	const char*	cameraSides[6] =  { "_forward.tga", "_back.tga", "_left.tga", "_right.tga",
									"_up.tga", "_down.tga"
								  };
	const char*	axisSides[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga",
								  "_pz.tga", "_nz.tga"
								};
	const char**	sides;
	char	fullName[MAX_IMAGE_NAME];
	int		width, height, size = 0;

	if( extensions == CF_CAMERA )
	{
		sides = cameraSides;
	}
	else if( extensions == CF_QUAKE1 )
	{
		sides = quakeSides;
	}
	else
	{
		sides = axisSides;
	}

	// FIXME: precompressed cube map files
	if( pics )
	{
		memset( pics, 0, 6 * sizeof( pics[0] ) );
	}
	if( timestamp )
	{
		*timestamp = 0;
	}

	if( extensions == CF_SINGLE && cubeMapSize != 0 )
	{
		ID_TIME_T thisTime;
		byte* thisPic[1];
		thisPic[0] = nullptr;

		if( pics )
		{
			R_LoadImageProgram( imgName, thisPic, &width, &height, &thisTime );
		}
		else
		{
			// load just the timestamps
			R_LoadImageProgram( imgName, nullptr, &width, &height, &thisTime );
		}


		if( thisTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			return false;
		}

		if( timestamp )
		{
			if( thisTime > *timestamp )
			{
				*timestamp = thisTime;
			}
		}

		if( pics )
		{
			*outSize = cubeMapSize;

			for( int i = 0; i < 6; i++ )
			{
				pics[i] = R_GenerateCubeMapSideFromSingleImage( thisPic[0], width, height, cubeMapSize, i );
				switch( i )
				{
					case 0:	// forward
						R_RotatePic( pics[i], cubeMapSize );
						break;
					case 1:	// back
						R_RotatePic( pics[i], cubeMapSize );
						R_HorizontalFlip( pics[i], cubeMapSize, cubeMapSize );
						R_VerticalFlip( pics[i], cubeMapSize, cubeMapSize );
						break;
					case 2:	// left
						R_VerticalFlip( pics[i], cubeMapSize, cubeMapSize );
						break;
					case 3:	// right
						R_HorizontalFlip( pics[i], cubeMapSize, cubeMapSize );
						break;
					case 4:	// up
						R_RotatePic( pics[i], cubeMapSize );
						break;
					case 5: // down
						R_RotatePic( pics[i], cubeMapSize );
						break;
				}
			}

			R_StaticFree( thisPic[0] );
		}

		return true;
	}

	if( extensions == CF_PANORAMA )
	{
		ID_TIME_T thisTime;
		byte* thisPic[1];
		thisPic[0] = nullptr;

		if( pics )
		{
			R_LoadImageProgram( imgName, thisPic, &width, &height, &thisTime );
		}
		else
		{
			// load just the timestamps
			R_LoadImageProgram( imgName, nullptr, &width, &height, &thisTime );
		}


		if( thisTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			return false;
		}

		if( timestamp )
		{
			if( thisTime > *timestamp )
			{
				*timestamp = thisTime;
			}
		}

		if( pics )
		{
			cubeMapSize = 1024;
			*outSize = cubeMapSize;

			for( int i = 0; i < 6; i++ )
			{
				pics[i] = R_GenerateCubeMapSideFromPanoramaImage( thisPic[0], width, height, cubeMapSize, i );
			}

			R_StaticFree( thisPic[0] );
		}

		return true;
	}

	for( i = 0 ; i < 6 ; i++ )
	{
		idStr::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );

		ID_TIME_T thisTime;
		if( !pics )
		{
			// just checking timestamps
			R_LoadImageProgram( fullName, NULL, &width, &height, &thisTime );
		}
		else
		{
			R_LoadImageProgram( fullName, &pics[i], &width, &height, &thisTime );
		}

		if( thisTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			break;
		}
		if( i == 0 )
		{
			size = width;
		}
		if( width != size || height != size )
		{
			common->Warning( "Mismatched sizes on cube map '%s'", imgName );
			break;
		}
		if( timestamp )
		{
			if( thisTime > *timestamp )
			{
				*timestamp = thisTime;
			}
		}
		if( pics && extensions == CF_CAMERA )
		{
			// convert from "camera" images to native cube map images
			switch( i )
			{
				case 0:	// forward
					R_RotatePic( pics[i], width );
					break;
				case 1:	// back
					R_RotatePic( pics[i], width );
					R_HorizontalFlip( pics[i], width, height );
					R_VerticalFlip( pics[i], width, height );
					break;
				case 2:	// left
					R_VerticalFlip( pics[i], width, height );
					break;
				case 3:	// right
					R_HorizontalFlip( pics[i], width, height );
					break;
				case 4:	// up
					R_RotatePic( pics[i], width );
					break;
				case 5: // down
					R_RotatePic( pics[i], width );
					break;
			}
		}
	}

	if( i != 6 )
	{
		// we had an error, so free everything
		if( pics )
		{
			for( j = 0 ; j < i ; j++ )
			{
				R_StaticFree( pics[j] );
			}
		}

		if( timestamp )
		{
			*timestamp = 0;
		}
		return false;
	}

	if( outSize )
	{
		*outSize = size;
	}
	return true;
}
