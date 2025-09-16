/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2025 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel
Copyright (C) 2016-2017 Dustin Land
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

#include "../framework/Common_local.h"
#include "RenderCommon.h"

/*
================
BitsForFormat
================
*/
int BitsForFormat( textureFormat_t format )
{
	switch( format )
	{
		case FMT_NONE:
			return 0;
		case FMT_RGBA8:
			return 32;
		case FMT_XRGB8:
			return 32;
		case FMT_RGB565:
			return 16;
		case FMT_L8A8:
			return 16;
		case FMT_ALPHA:
			return 8;
		case FMT_LUM8:
			return 8;
		case FMT_INT8:
			return 8;
		case FMT_DXT1:
			return 4;
		case FMT_DXT5:
			return 8;
		// RB: added ETC compression
		case FMT_ETC1_RGB8_OES:
			return 4;
		case FMT_SHADOW_ARRAY:
			return ( 32 * 6 );
		case FMT_RG16F:
			return 32;
		case FMT_RGBA16F:
			return 64;
		case FMT_RGBA16S:
			return 64;
		case FMT_RGBA32F:
			return 128;
		case FMT_R32F:
			return 32;
		case FMT_R11G11B10F:
			return 32;
		case FMT_BC6H:
			return 8;
		case FMT_BC7:
			return 8;
		// RB end
		case FMT_DEPTH:
			return 32;
		case FMT_DEPTH_STENCIL:
			return 32;
		case FMT_X16:
			return 16;
		case FMT_Y16_X16:
			return 32;
		case FMT_R8F:
			return 4;
		default:
			assert( 0 );
			return 0;
	}
}

int BlockSizeForFormat( const textureFormat_t& format )
{
	switch( format )
	{
		case FMT_NONE:
			return 0;
		case FMT_DXT1:
			return 8;
		case FMT_DXT5:
			return 16;
		case FMT_BC6H:
			return 16;
		case FMT_BC7:
			return 16;
		default:
			return 1;
	}
}

/*
=========================
GetRowBytes
Returns the row bytes for the given image.
=========================
*/
int GetRowPitch( const textureFormat_t& format, int width )
{
	bool bc = ( format == FMT_DXT1 || format == FMT_DXT5 || format == FMT_BC6H || format == FMT_BC7 );

	if( bc )
	{
		int blockSize = BlockSizeForFormat( format );
		return std::max( 1, ( width + 3 ) / 4 ) * blockSize;
	}

	int bpe = BitsForFormat( format );
	return width * ( bpe / 8 );
}

/*
========================
idImage::DeriveOpts
========================
*/
ID_INLINE void idImage::DeriveOpts()
{
	if( opts.format == FMT_NONE )
	{
		opts.colorFormat = CFM_DEFAULT;

		switch( usage )
		{
			case TD_COVERAGE:
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_GREEN_ALPHA;
				break;

			case TD_DIFFUSE:
				// TD_DIFFUSE gets only set to when its a diffuse texture for an interaction
				opts.gammaMips = true;
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_YCOCG_DXT5;
				break;

			case TD_SPECULAR:
				opts.gammaMips = true;
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_DEFAULT;
				break;

			case TD_SPECULAR_PBR_RMAO:
				opts.gammaMips = false;
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_DEFAULT;
				break;

			case TD_SPECULAR_PBR_RMAOD:
				opts.gammaMips = false;
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_DEFAULT;
				break;

			case TD_DEFAULT:
				opts.gammaMips = true;
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_DEFAULT;
				break;

			case TD_BUMP:
				opts.format = FMT_DXT5;
				opts.colorFormat = CFM_NORMAL_DXT5;
				break;

			case TD_FONT:
				opts.format = FMT_DXT1;
				opts.colorFormat = CFM_GREEN_ALPHA;
				opts.numLevels = 4; // We only support 4 levels because we align to 16 in the exporter
				opts.gammaMips = true;
				break;

			case TD_LIGHT:
				// RB: TODO check binary format version
				// D3 BFG assets require RGB565 but it introduces color banding
				// mods would prefer FMT_RGBA8
#if defined(STANDALONE)
				opts.format = FMT_RGBA8;
#else
				opts.format = FMT_RGB565;
#endif
				opts.gammaMips = true;
				break;

			case TD_LOOKUP_TABLE_MONO:
				opts.format = FMT_INT8;
				break;

			case TD_LOOKUP_TABLE_ALPHA:
				opts.format = FMT_ALPHA;
				break;

			case TD_LOOKUP_TABLE_RGB1:
			case TD_LOOKUP_TABLE_RGBA:
				opts.format = FMT_RGBA8;
				break;

			case TD_HDR_LIGHTPROBE:
				opts.format = FMT_BC6H;
				break;

			case TD_HDRI:
				opts.format = FMT_BC6H;
				//opts.numLevels = 1;
				break;

			// motorsep 05-17-2015; added this for uncompressed cubemap/skybox textures
			case TD_HIGHQUALITY_CUBE:
				opts.colorFormat = CFM_DEFAULT;
				opts.format = FMT_RGBA8;
				opts.gammaMips = true;
				break;


			//------------------------
			// Render targets only
			//------------------------

			case TD_DEPTH:
				opts.format = FMT_DEPTH;
				break;

			case TD_DEPTH_STENCIL:
				opts.format = FMT_DEPTH_STENCIL;
				break;

			case TD_SHADOW_ARRAY:
				opts.format = FMT_SHADOW_ARRAY;
				break;

			case TD_RG16F:
				opts.format = FMT_RG16F;
				break;

			case TD_RGBA16F:
				opts.format = FMT_RGBA16F;
				break;

			case TD_RGBA16S:
				opts.format = FMT_RGBA16S;
				break;

			case TD_RGBA32F:
				opts.format = FMT_RGBA32F;
				break;

			case TD_R32F:
				opts.format = FMT_R32F;
				break;

			case TD_R8F:
				opts.format = FMT_R8F;
				break;

			default:
				assert( false );
				opts.format = FMT_RGBA8;
		}
	}

	if( opts.numLevels == 0 )
	{
		opts.numLevels = 1;

		if( filter == TF_LINEAR || filter == TF_NEAREST )
		{
			// don't create mip maps if we aren't going to be using them
		}
		else
		{
			int	temp_width = opts.width;
			int	temp_height = opts.height;
			while( temp_width > 1 || temp_height > 1 )
			{
				temp_width >>= 1;
				temp_height >>= 1;
				if( ( opts.format == FMT_DXT1 || opts.format == FMT_DXT5 || opts.format == FMT_BC6H || opts.format == FMT_ETC1_RGB8_OES ) &&
						( ( temp_width & 0x3 ) != 0 || ( temp_height & 0x3 ) != 0 ) )
				{
					break;
				}
				opts.numLevels++;
			}
		}
	}
}

/*
========================
idImage::AllocImage
========================
*/
void idImage::AllocImage( const idImageOpts& imgOpts, textureFilter_t tf, textureRepeat_t tr )
{
	filter = tf;
	repeat = tr;
	opts = imgOpts;
	DeriveOpts();
	AllocImage();
}

/*
================
GenerateImage
================
*/
void idImage::GenerateImage( const byte* pic, int width, int height, textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm, nvrhi::ICommandList* commandList, bool isRenderTarget, bool isUAV, uint sampleCount, cubeFiles_t _cubeFiles )
{
	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	cubeFiles = _cubeFiles;

	opts.textureType = ( sampleCount > 1 ) ? DTT_2D_MULTISAMPLE : DTT_2D;
	opts.width = width;
	opts.height = height;
	opts.numLevels = 0;
	opts.samples = sampleCount;
	opts.isRenderTarget = isRenderTarget;
	opts.isUAV = isUAV;

	// RB
	if( cubeFiles == CF_2D_PACKED_MIPCHAIN )
	{
		opts.width = width * ( 2.0f / 3.0f );
	}

	DeriveOpts();

	// RB: allow pic == NULL for internal framebuffer images
	if( pic == NULL || opts.textureType == DTT_2D_MULTISAMPLE )
	{
		AllocImage();
		isLoaded = true;
	}
	else
	{
		idBinaryImage im( GetName() );
		if( cubeFiles == CF_2D_PACKED_MIPCHAIN )
		{
			im.Load2DAtlasMipchainFromMemory( width, opts.height, pic, opts.numLevels, opts.format, opts.colorFormat );
		}
		else
		{
			im.Load2DFromMemory( width, height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips );
		}

		// don't show binarize info for generated images
		common->LoadPacifierBinarizeEnd();

		AllocImage();

#if defined( USE_NVRHI ) && !defined( DMAP )
		if( commandList )
		{
			commandList->beginTrackingTextureState( texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common );

			for( int i = 0; i < im.NumImages(); i++ )
			{
				const bimageImage_t& img = im.GetImageHeader( i );
				const byte* data = im.GetImageData( i );

				int rowPitch = GetRowPitch( opts.format, img.width );
				commandList->writeTexture( texture, img.destZ, img.level, data, rowPitch );
			}

			commandList->setPermanentTextureState( texture, nvrhi::ResourceStates::ShaderResource );
			commandList->commitBarriers();
		}
#else
		/*
		for( int i = 0; i < im.NumImages(); i++ )
		{
			const bimageImage_t& img = im.GetImageHeader( i );
			const byte* data = im.GetImageData( i );
			SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
		}
		*/
#endif

		isLoaded = true;
	}
	// RB end
}

/*
====================
GenerateCubeImage

Non-square cube sides are not allowed
====================
*/
void idImage::GenerateCubeImage( const byte* pic[6], int size, textureFilter_t filterParm, textureUsage_t usageParm, nvrhi::ICommandList* commandList )
{
	PurgeImage();

	filter = filterParm;
	repeat = TR_CLAMP;
	usage = usageParm;
	cubeFiles = CF_NATIVE;

	opts.textureType = DTT_CUBIC;
	opts.width = size;
	opts.height = size;
	opts.numLevels = 0;
	DeriveOpts();

	// if we don't have a rendering context, just return after we
	// have filled in the parms.  We must have the values set, or
	// an image match from a shader before the render starts would miss
	// the generated texture
#if !defined( DMAP )
	if( !tr.IsInitialized() )
	{
		return;
	}
#endif

	idBinaryImage im( GetName() );
	im.LoadCubeFromMemory( size, pic, opts.numLevels, opts.format, opts.gammaMips );

	// don't show binarize info for generated images
	common->LoadPacifierBinarizeEnd();

	AllocImage();

#if defined( USE_NVRHI ) && !defined( DMAP )
	//const nvrhi::FormatInfo& info = nvrhi::getFormatInfo( texture->getDesc().format );
	//bytesPerPixel = info.bytesPerBlock;

	commandList->beginTrackingTextureState( texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common );

	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );

		commandList->writeTexture( texture, 0, img.level, data, GetRowPitch( opts.format, img.width ) );
	}

	commandList->setPermanentTextureState( texture, nvrhi::ResourceStates::ShaderResource );
	commandList->commitBarriers();
#else
	/*
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
	*/
#endif

	isLoaded = true;
}

// RB begin
void idImage::GenerateShadowArray( int width, int height, textureFilter_t filterParm, textureRepeat_t repeatParm, textureUsage_t usageParm, nvrhi::ICommandList* commandList )
{
	PurgeImage();

	filter = filterParm;
	repeat = repeatParm;
	usage = usageParm;
	cubeFiles = CF_2D_ARRAY;

	opts.textureType = DTT_2D_ARRAY;
	opts.width = width;
	opts.height = height;
	opts.numLevels = 0;
	opts.isRenderTarget = true;

	DeriveOpts();

	// The image will be uploaded to the gpu on a deferred state.
	AllocImage();

	isLoaded = true;
}
// RB end


/*
===============
GetGeneratedName

name contains GetName() upon entry
===============
*/
void idImage::GetGeneratedName( idStr& _name, const textureUsage_t& _usage, const cubeFiles_t& _cube )
{
	idStrStatic< 64 > extension;

	_name.ExtractFileExtension( extension );
	_name.StripFileExtension();

	_name += va( "#__%02d%02d", ( int )_usage, ( int )_cube );
	if( extension.Length() > 0 )
	{
		_name.SetFileExtension( extension );
	}
}

/*
===============
ActuallyLoadImage

Absolutely every image goes through this path
On exit, the idImage will have a valid OpenGL texture number that can be bound
===============
*/
void idImage::ActuallyLoadImage( bool fromBackEnd, nvrhi::ICommandList* commandList )
{
	// RB: might have been called doubled by nested LoadDeferredImages
	if( isLoaded )
	{
		return;
	}

	// if we don't have a rendering context yet, just return
	//if( !tr.IsInitialized() )
	//{
	//	return;
	//}

	// this is the ONLY place generatorFunction will ever be called
	if( generatorFunction )
	{
		generatorFunction( this, commandList );
		return;
	}

	// RB: the following does not load the source images from disk because pic is NULL
	// but it tries to get the timestamp to see if we have a newer file than the one in the compressed .bimage

	if( com_productionMode.GetInteger() != 0 )
	{
		sourceFileTime = FILE_NOT_FOUND_TIMESTAMP;
		if( cubeFiles != CF_2D )
		{
			opts.textureType = DTT_CUBIC;
			repeat = TR_CLAMP;
		}
	}
	else
	{
		// RB: added CF_2D_ARRAY
		if( cubeFiles == CF_2D_ARRAY )
		{
			opts.textureType = DTT_2D_ARRAY;
		}
		else if( cubeFiles == CF_NATIVE || cubeFiles == CF_CAMERA || cubeFiles == CF_QUAKE1 || cubeFiles == CF_SINGLE || cubeFiles == CF_PANORAMA )
		{
			opts.textureType = DTT_CUBIC;
			repeat = TR_CLAMP;
			R_LoadCubeImages( GetName(), cubeFiles, NULL, NULL, &sourceFileTime, cubeMapSize );
		}
		else
		{
			opts.textureType = DTT_2D;
			R_LoadImageProgram( GetName(), NULL, NULL, NULL, &sourceFileTime, &usage );
		}
	}

	// RB: PBR HACK - RMAO maps should end with _rmao insted of _s
	if( usage == TD_SPECULAR_PBR_RMAO )
	{
		idStr baseName = imgName;
		baseName.StripFileExtension();

		if( baseName.StripTrailingOnce( "_s" ) )
		{
			imgName = baseName + "_rmao";
		}
	}
	// RB end

	// Figure out opts.colorFormat and opts.format so we can make sure the binary image is up to date
	DeriveOpts();

	idStrStatic< MAX_OSPATH > generatedName = GetName();
	GetGeneratedName( generatedName, usage, cubeFiles );

	//if( generatedName.Find( "textures/base_floor/a_stairs_d02", false ) >= 0 )
	//{
	// #924
	//int c = 1;
	//}

	// RB: try to load the .bimage and skip if sourceFileTime is newer
	idBinaryImage im( generatedName );
	binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );

	// BFHACK, do not want to tweak on buildgame so catch these images here
	if( binaryFileTime == FILE_NOT_FOUND_TIMESTAMP && fileSystem->UsingResourceFiles() )
	{
		int c = 1;
		while( c-- > 0 )
		{
			if( generatedName.Find( "guis/assets/white#__0000", false ) >= 0 )
			{
				generatedName.Replace( "white#__0000", "white#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "guis/assets/white#__0100", false ) >= 0 )
			{
				generatedName.Replace( "white#__0100", "white#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "textures/black#__0100", false ) >= 0 )
			{
				generatedName.Replace( "black#__0100", "black#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "textures/decals/bulletglass1_d#__0100", false ) >= 0 )
			{
				generatedName.Replace( "bulletglass1_d#__0100", "bulletglass1_d#__0200" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
			if( generatedName.Find( "models/monsters/skeleton/skeleton01_d#__1000", false ) >= 0 )
			{
				generatedName.Replace( "skeleton01_d#__1000", "skeleton01_d#__0100" );
				im.SetName( generatedName );
				binaryFileTime = im.LoadFromGeneratedFile( sourceFileTime );
				break;
			}
		}
	}
	const bimageFile_t& header = im.GetFileHeader();

	if( ( fileSystem->InProductionMode() && binaryFileTime != FILE_NOT_FOUND_TIMESTAMP ) || ( ( binaryFileTime != FILE_NOT_FOUND_TIMESTAMP )
			&& ( header.colorFormat == opts.colorFormat )
			// SRS: handle case when image read is cached and RGB565 format conversion is already done
			// RB: allow R11G11B10 instead of BC6
			&& ( header.format == opts.format || ( header.format == FMT_RGBA8 && opts.format == FMT_RGB565 ) || ( header.format == FMT_R11G11B10F && opts.format == FMT_BC6H ) )
			&& ( header.textureType == opts.textureType )
																							) )
	{
		opts.width = header.width;
		opts.height = header.height;
		opts.numLevels = header.numLevels;
		opts.colorFormat = ( textureColor_t )header.colorFormat;
		opts.format = ( textureFormat_t )header.format;
		opts.format = ( textureFormat_t )header.format;
		opts.textureType = ( textureType_t )header.textureType;

		if( cvarSystem->GetCVarBool( "fs_buildresources" ) )
		{
			// for resource gathering write this image to the preload file for this map
			fileSystem->AddImagePreload( GetName(), filter, repeat, usage, cubeFiles );
		}
	}
	else
	{
		// RB: try to read the source image from disk

		idStr binarizeReason = "binarize: unknown reason";
		if( binaryFileTime == FILE_NOT_FOUND_TIMESTAMP )
		{
			binarizeReason = va( "binarize: binary file not found '%s'", generatedName.c_str() );
		}
		else if( header.colorFormat != opts.colorFormat )
		{
			binarizeReason = va( "binarize: mismatched color format '%s'", generatedName.c_str() );
		}
		else if( header.textureType != opts.textureType )
		{
			binarizeReason = va( "binarize: mismatched texture type '%s'", generatedName.c_str() );
		}
		//else if( toolUsage )
		//	binarizeReason = va( "binarize: tool usage '%s'", generatedName.c_str() );

		if( cubeFiles == CF_NATIVE || cubeFiles == CF_CAMERA || cubeFiles == CF_QUAKE1 || cubeFiles == CF_SINGLE ||  cubeFiles == CF_PANORAMA )
		{
			int size;
			byte* pics[6];

			if( !R_LoadCubeImages( GetName(), cubeFiles, pics, &size, &sourceFileTime, cubeMapSize ) || size == 0 )
			{
				idLib::Warning( "Couldn't load cube image: %s", GetName() );
				defaulted = true; // RB
				isLoaded = true;
				return;
			}

			repeat = TR_CLAMP;

			opts.textureType = DTT_CUBIC;
			opts.width = size;
			opts.height = size;
			opts.numLevels = 0;

			DeriveOpts();

			// foresthale 2014-05-30: give a nice progress display when binarizing
			if( !globalImages->cacheImages )
			{
				commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
			}

			if( opts.numLevels > 1 )
			{
				commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 * 4 / 3 );
			}
			else
			{
				commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.width * 6 );
			}

			im.LoadCubeFromMemory( size, ( const byte** )pics, opts.numLevels, opts.format, opts.gammaMips );

			commonLocal.LoadPacifierBinarizeEnd();

			repeat = TR_CLAMP;

			for( int i = 0; i < 6; i++ )
			{
				if( pics[i] )
				{
					Mem_Free( pics[i] );
				}
			}
		}
		else
		{
			int width, height;
			byte* pic;

			// load the full specification, and perform any image program calculations
			R_LoadImageProgram( GetName(), &pic, &width, &height, &sourceFileTime, &usage );

			if( pic == NULL )
			{
				idLib::Warning( "Couldn't load image: %s : %s", GetName(), generatedName.c_str() );

				// create a default so it doesn't get continuously reloaded
				opts.width = 8;
				opts.height = 8;
				opts.numLevels = 1;
				DeriveOpts();

				defaulted = true; // RB

				if( !commandList )
				{
					return;
				}

				AllocImage();

				// default it again because it was unset by AllocImage().PurgeImage()
				defaulted = true;

				// clear the data so it's not left uninitialized
				idTempArray<byte> clear( opts.width * opts.height * 4 );
				memset( clear.Ptr(), 0, clear.Size() );

#if defined( USE_NVRHI ) && !defined( DMAP )
				commandList->beginTrackingTextureState( texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common );
				for( int level = 0; level < opts.numLevels; level++ )
				{
					commandList->writeTexture( texture, 0, level, clear.Ptr(), GetRowPitch( opts.format, opts.width ) );
				}
				commandList->setPermanentTextureState( texture, nvrhi::ResourceStates::ShaderResource );
				commandList->commitBarriers();
#else
				/*
				for( int level = 0; level < opts.numLevels; level++ )
				{
					SubImageUpload( level, 0, 0, 0, opts.width >> level, opts.height >> level, clear.Ptr() );
				}
				*/
#endif
				isLoaded = true;
				return;
			}

			opts.width = width;
			opts.height = height;
			opts.numLevels = 0;

			// RB
			if( cubeFiles == CF_2D_PACKED_MIPCHAIN )
			{
				opts.width = width * ( 2.0f / 3.0f );
			}

			DeriveOpts();

			// RB: convert to compressed DXT or whatever choosen target format
			if( cubeFiles == CF_2D_PACKED_MIPCHAIN )
			{
				if( !globalImages->cacheImages )
				{
					commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
				}
				commonLocal.LoadPacifierBinarizeProgressTotal( width * opts.height );

				im.Load2DAtlasMipchainFromMemory( width, opts.height, pic, opts.numLevels, opts.format, opts.colorFormat );
			}
			else
			{
				if( !globalImages->cacheImages )
				{
					commonLocal.LoadPacifierBinarizeFilename( generatedName.c_str(), binarizeReason.c_str() );
				}

				if( opts.numLevels > 1 )
				{
					commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.height * 4 / 3 );
				}
				else
				{
					commonLocal.LoadPacifierBinarizeProgressTotal( opts.width * opts.height );
				}

				im.Load2DFromMemory( opts.width, opts.height, pic, opts.numLevels, opts.format, opts.colorFormat, opts.gammaMips );
			}
			commonLocal.LoadPacifierBinarizeEnd();

			Mem_Free( pic );
		}

		// RB: write the compressed .bimage which contains the optimized GPU format
		binaryFileTime = im.WriteGeneratedFile( sourceFileTime );
	}

#if !defined( DMAP )
	if( !commandList )
	{
		return;
	}
#endif

	AllocImage();

#if defined( USE_NVRHI ) && !defined( DMAP )
	commandList->beginTrackingTextureState( texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common );

	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* pic = im.GetImageData( i );

		commandList->writeTexture( texture, img.destZ, img.level, pic, GetRowPitch( opts.format, img.width ) );
	}
	commandList->setPermanentTextureState( texture, nvrhi::ResourceStates::ShaderResource );
	commandList->commitBarriers();
#else
	/*
	for( int i = 0; i < im.NumImages(); i++ )
	{
		const bimageImage_t& img = im.GetImageHeader( i );
		const byte* data = im.GetImageData( i );
		SubImageUpload( img.level, 0, 0, img.destZ, img.width, img.height, data );
	}
	*/
#endif

	isLoaded = true;
}

void idImage::DeferredLoadImage()
{
	if( !globalImages->cacheImages )
	{
		globalImages->imagesToLoad.AddUnique( this );
	}
}

void idImage::DeferredPurgeImage()
{
	globalImages->imagesToLoad.Remove( this );
}

/*
=============
RB_UploadScratchImage

if rows = cols * 6, assume it is a cube map animation
=============
*/
void idImage::UploadScratch( const byte* data, int cols, int rows, nvrhi::ICommandList* commandList )
{
#if !defined( DMAP )

	// if rows = cols * 6, assume it is a cube map animation
	if( rows == cols * 6 )
	{
		rows /= 6;
		const byte* pic[6];

		for( int i = 0; i < 6; i++ )
		{
			pic[i] = data + cols * rows * 4 * i;
		}

		if( opts.textureType != DTT_CUBIC || usage != TD_LOOKUP_TABLE_RGBA )
		{
			GenerateCubeImage( pic, cols, TF_LINEAR, TD_LOOKUP_TABLE_RGBA, commandList );
			return;
		}

		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;

			AllocImage();
		}

#if defined( USE_NVRHI )
		SetSamplerState( TF_LINEAR, TR_CLAMP );

		commandList->beginTrackingTextureState( texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common );

		for( int i = 0; i < 6; i++ )
		{
			commandList->writeTexture( texture, i, 0, pic[i], GetRowPitch( opts.format, opts.width ) );
		}

		commandList->setPermanentTextureState( texture, nvrhi::ResourceStates::ShaderResource );
		commandList->commitBarriers();
#else
		/*
		for( int i = 0; i < 6; i++ )
		{
			SubImageUpload( 0, 0, 0, i, opts.width, opts.height, pic[i] );
		}
		*/
#endif
	}
	else
	{
#if defined( USE_NVRHI )
		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;

			AllocImage();
		}

		if( data != NULL && commandList != NULL )
		{
			SetSamplerState( TF_LINEAR, TR_REPEAT );

			commandList->beginTrackingTextureState( texture, nvrhi::AllSubresources, nvrhi::ResourceStates::Common );

			commandList->writeTexture( texture, 0, 0, data, GetRowPitch( opts.format, opts.width ) );
			//commandList->setPermanentTextureState( texture, nvrhi::ResourceStates::ShaderResource );

			commandList->commitBarriers();
		}
#else
		if( opts.textureType != DTT_2D || usage != TD_LOOKUP_TABLE_RGBA )
		{
			GenerateImage( data, cols, rows, TF_LINEAR, TR_REPEAT, TD_LOOKUP_TABLE_RGBA, commandList );
			return;
		}

		if( opts.width != cols || opts.height != rows )
		{
			opts.width = cols;
			opts.height = rows;

			AllocImage();
		}

		SetSamplerState( TF_LINEAR, TR_REPEAT );
		SubImageUpload( 0, 0, 0, 0, opts.width, opts.height, data );
#endif
	}

	isLoaded = true;

#endif
}

/*
==================
StorageSize
==================
*/
int idImage::StorageSize() const
{
	if( !IsLoaded() )
	{
		return 0;
	}

	size_t baseSize = opts.width * opts.height;
	if( opts.numLevels > 1 && !opts.isRenderTarget )
	{
		baseSize *= 4;
		baseSize /= 3;
	}

	baseSize *= BitsForFormat( opts.format );
	baseSize /= 8;

	return int( baseSize );
}

/*
==================
Print
==================
*/
void idImage::Print() const
{
	if( generatorFunction )
	{
		common->Printf( "F" );
	}
	else
	{
		common->Printf( " " );
	}

	switch( opts.textureType )
	{
		case DTT_2D:
			common->Printf( "      " );
			break;
		case DTT_CUBIC:
			common->Printf( "C     " );
			break;

		case DTT_2D_ARRAY:
			common->Printf( "2D-A  " );
			break;

		case DTT_2D_MULTISAMPLE:
			common->Printf( "2D-MS " );
			break;

		default:
			common->Printf( "<BAD TYPE:%i>", opts.textureType );
			break;
	}

	common->Printf( "%4i %4i ",	opts.width, opts.height );

	switch( opts.format )
	{
#define NAME_FORMAT( x ) case FMT_##x: common->Printf( "%-16s ", #x ); break;
			NAME_FORMAT( NONE );
			NAME_FORMAT( RGBA8 );
			NAME_FORMAT( XRGB8 );
			NAME_FORMAT( RGB565 );
			NAME_FORMAT( L8A8 );
			NAME_FORMAT( ALPHA );
			NAME_FORMAT( LUM8 );
			NAME_FORMAT( INT8 );
			NAME_FORMAT( DXT1 );
			NAME_FORMAT( DXT5 );
			// RB begin
			NAME_FORMAT( ETC1_RGB8_OES );
			NAME_FORMAT( SHADOW_ARRAY );
			NAME_FORMAT( RG16F );
			NAME_FORMAT( RGBA16F );
			NAME_FORMAT( RGBA32F );
			NAME_FORMAT( R32F );
			NAME_FORMAT( R8F );
			NAME_FORMAT( R11G11B10F );
			NAME_FORMAT( BC6H );
			NAME_FORMAT( BC7 );
			// RB end
			NAME_FORMAT( DEPTH );
			NAME_FORMAT( DEPTH_STENCIL );
			NAME_FORMAT( X16 );
			NAME_FORMAT( Y16_X16 );
		default:
			common->Printf( "<%3i>", opts.format );
			break;
	}

	switch( filter )
	{
		case TF_DEFAULT:
			common->Printf( "mip  " );
			break;
		case TF_LINEAR:
			common->Printf( "linr " );
			break;
		case TF_NEAREST:
			common->Printf( "nrst " );
			break;
		case TF_NEAREST_MIPMAP:
			common->Printf( "nmip " );
			break;
		default:
			common->Printf( "<BAD FILTER:%i>", filter );
			break;
	}

	switch( repeat )
	{
		case TR_REPEAT:
			common->Printf( "rept " );
			break;
		case TR_CLAMP_TO_ZERO:
			common->Printf( "zero " );
			break;
		case TR_CLAMP_TO_ZERO_ALPHA:
			common->Printf( "azro " );
			break;
		case TR_CLAMP:
			common->Printf( "clmp " );
			break;
		default:
			common->Printf( "<BAD REPEAT:%i>", repeat );
			break;
	}

	common->Printf( "%4ik ", StorageSize() / 1024 );

	common->Printf( " %s\n", GetName() );
}

/*
===============
idImage::Reload
===============
*/
void idImage::Reload( bool force, nvrhi::ICommandList* commandList )
{
	// always regenerate functional images
	if( generatorFunction )
	{
		//common->DPrintf( "regenerating %s.\n", GetName() );
		generatorFunction( this, commandList );
		return;
	}

	// check file times
	if( !force )
	{
		ID_TIME_T current;
		if( cubeFiles == CF_NATIVE || cubeFiles == CF_CAMERA || cubeFiles == CF_QUAKE1 || cubeFiles == CF_SINGLE || cubeFiles == CF_PANORAMA )
		{
			R_LoadCubeImages( imgName, cubeFiles, NULL, NULL, &current );
		}
		else
		{
			// get the current values
			R_LoadImageProgram( imgName, NULL, NULL, NULL, &current );
		}
		if( current <= sourceFileTime )
		{
			return;
		}
	}

	common->DPrintf( "reloading %s.\n", GetName() );

	PurgeImage();

	DeferredLoadImage();
}



