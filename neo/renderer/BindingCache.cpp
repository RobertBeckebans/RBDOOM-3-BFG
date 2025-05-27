/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
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

#include "BindingCache.h"

BindingCache::BindingCache()
	: device( nullptr )
	, bindingSets()
	, bindingHash()
	, mutex()
{
}

void BindingCache::Init( nvrhi::IDevice* _device )
{
	device = _device;
}

nvrhi::BindingSetHandle BindingCache::GetCachedBindingSet( const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout )
{
	size_t hash = 0;
	nvrhi::hash_combine( hash, desc );
	nvrhi::hash_combine( hash, layout );

	mutex.Lock();

	nvrhi::BindingSetHandle result = nullptr;
	for( int i = bindingHash.First( hash ); i != -1; i = bindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle bindingSet = bindingSets[i];
		if( *bindingSet->getDesc() == desc )
		{
			result = bindingSet;
			break;
		}
	}

	mutex.Unlock();

	if( result )
	{
		assert( result->getDesc() && *result->getDesc() == desc );
	}

	return result;
}

nvrhi::BindingSetHandle BindingCache::GetOrCreateBindingSet( const nvrhi::BindingSetDesc& desc, nvrhi::IBindingLayout* layout )
{
	size_t hash = 0;
	nvrhi::hash_combine( hash, desc );
	nvrhi::hash_combine( hash, layout );

	mutex.Lock();

	nvrhi::BindingSetHandle result = nullptr;
	for( int i = bindingHash.First( hash ); i != -1; i = bindingHash.Next( i ) )
	{
		nvrhi::BindingSetHandle bindingSet = bindingSets[i];
		if( *bindingSet->getDesc() == desc )
		{
			result = bindingSet;
			break;
		}
	}

	mutex.Unlock();

	if( !result )
	{
		mutex.Lock();

		result = device->createBindingSet( desc, layout );

		int entryIndex = bindingSets.Append( result );
		bindingHash.Add( hash, entryIndex );

		mutex.Unlock();
	}

	if( result )
	{
		assert( result->getDesc() && *result->getDesc() == desc );
	}

	return result;
}

void BindingCache::Clear()
{
	// RB FIXME void StaticDescriptorHeap::releaseDescriptors(DescriptorIndex baseIndex, uint32_t count)
	// will try to gain a conflicting mutex lock and cause an abort signal

	mutex.Lock();
	for( int i = 0; i < bindingSets.Num(); i++ )
	{
		bindingSets[i].Reset();
	}
	bindingSets.Clear();
	bindingHash.Clear();
	mutex.Unlock();
}

void SamplerCache::Init( nvrhi::IDevice* _device )
{
	device = _device;
}

void SamplerCache::Clear()
{
	mutex.Lock();
	samplers.Clear();
	samplerHash.Clear();
	mutex.Unlock();
}

nvrhi::SamplerHandle SamplerCache::GetOrCreateSampler( nvrhi::SamplerDesc desc )
{
#if 1
	size_t hash = std::hash<nvrhi::SamplerDesc> {}( desc );

	mutex.Lock();

	nvrhi::SamplerHandle result = nullptr;
	for( int i = samplerHash.First( hash ); i != -1; i = samplerHash.Next( i ) )
	{
		nvrhi::SamplerHandle sampler = samplers[i];
		if( sampler->getDesc() == desc )
		{
			result = sampler;
			break;
		}
	}

	mutex.Unlock();

	if( !result )
	{
		mutex.Lock();

		result = device->createSampler( desc );

		int entryIndex = samplers.Append( result );
		samplerHash.Add( hash, entryIndex );

		mutex.Unlock();
	}

	if( result )
	{
		assert( result->getDesc() == desc );
	}

	return result;
#else
	return device->createSampler( desc );
#endif
}
