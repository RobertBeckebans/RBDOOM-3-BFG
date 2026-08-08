/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 2023 Harrie van Ginneken

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

//.AS test
//trace( "hello" )
//trace( "world" )
//var clickCount = 0;
//function playAnimation( event:MouseEvent ) :void {
//	clickCount++;
//	trace( text1.text )
//		text1.text = "clicked me " + clickCount;
//}
//// Register the function as a listener with the button.
//addEventListener( MouseEvent.CLICK, playAnimation );

#include "precompiled.h"
#pragma hdrstop

#include "SWF_Abc.h"

idCVar swf_abc_verbose( "swf_abc_verbose", "0", CVAR_INTEGER, "1 : writes out all abc data read \n 2 : print bytecode " );

#pragma warning( disable: 4189 ) // local variable is initialized but not referenced

void trace( const char* fmt, ... )
{
	if( swf_abc_verbose.GetInteger() == 1 )
	{
		//common->PrintPrefix("[SWF]");
		va_list argptr;
		va_start( argptr, fmt );
		common->VPrintf( fmt, argptr );
		va_end( argptr );
		//common->PrintPrefix("");
	}
}

#define toString(x) asString(x,constant_pool)

idStr SWF_AbcFile::asString( swfConstantKind_t kind, swfConstant_pool_info& constant_pool )
{
	idStr type;
#define switchTrace( n ) case n: type = #n;break;
	switch( kind )
	{
			switchTrace( unused_0x00 );
			switchTrace( Utf8 );
			switchTrace( Int );
			switchTrace( UInt );
			switchTrace( PrivateNs );
			switchTrace( Double );
			switchTrace( Qname );
			switchTrace( Namespace );
			switchTrace( False );
			switchTrace( True );
			switchTrace( Null );
			switchTrace( QnameA );
			switchTrace( RTQname );
			switchTrace( RTQnameA );
			switchTrace( RTQnameL );
			switchTrace( RTQnameLA );
			switchTrace( Multiname );
			switchTrace( MultinameA );
			switchTrace( MultinameL );
			switchTrace( MultinameLA );
			switchTrace( PackageNamespace );
			switchTrace( PackageInternalNs );
			switchTrace( ProtectedNamespace );
			switchTrace( ExplicitNamespace );
			switchTrace( StaticProtectedNs );
	}
	return type;
#undef  switchTrace
}

idStr SWF_AbcFile::asString( swfMultiname* mn, swfConstant_pool_info& constant_pool, bool prefix /*= true*/ )
{
	idStr ret;
	if( prefix )
	{
		ret += toString( mn->type );
	}
	switch( mn->type )
	{
		case RTQnameL:
		case RTQnameLA:
			ret += "null::null";
			break;
		case Qname:
		case QnameA:
			ret += " ";
			ret += *constant_pool.namespaceNames[mn->index];
			if( mn->index != 1 )
			{
				ret += ".";
			}
			ret += constant_pool.utf8Strings[mn->nameIndex];
			break;

		case RTQname:
		case RTQnameA:
			ret += " ";
			ret += constant_pool.utf8Strings[mn->nameIndex];
			break;

		case TypeName:
		case Multiname:
		case MultinameA:
			ret += " ";
			ret += asString( &constant_pool.multinameInfos[mn->index], constant_pool );
			ret += ".";
			ret += constant_pool.utf8Strings[mn->nameIndex];
			break;

		case MultinameL:
		case MultinameLA:
			ret += " ";
			for( auto* str : constant_pool.namespaceSets[mn->index] )
			{
				ret += *str;
				ret += ".";
			};
			break;
		case unused_0x00:
			ret += " unused_0x00";
			break;
		default:
			common->FatalError( "Invalid Multiname type" );
			break;
	}

	return ret;
}

void SWF_AbcFile::traceMN( const char* name, swfMultiname* mn, swfConstant_pool_info& constant_pool )
{
	idStr type = asString( mn, constant_pool );
	trace( "%s %s \n", name, type.c_str() );
}

void SWF_AbcFile::traceConstantPool( swfConstant_pool_info& constant_pool )
{
	int cnt = 0;

	trace( "^8========================================================\n" );
	trace( " constant pool \n" );
	trace( "^8========================================================\n" );

	trace( "^8integers : ^7%i\n", constant_pool.integers.Num() );
	for( auto& t : constant_pool.integers )
	{
		trace( "^8[^7%i^8]\t ^7%i \n", cnt++, t );
	}
	trace( "^8uIntegers ^7: %i\n", constant_pool.uIntegers.Num() );
	cnt = 0;
	for( auto& t : constant_pool.uIntegers )
	{
		trace( "^8[^7%i^8]\t ^7%i \n", cnt++, ( int )t );
	}
	trace( "^8doubles ^7: %i\n", constant_pool.doubles.Num() );
	cnt = 0;
	for( auto& t : constant_pool.doubles )
	{
		trace( "^8[^7%i^8]\t ^7%f \n", cnt++, ( float ) t );
	}
	trace( "^8utf8Strings ^7: %i\n", constant_pool.utf8Strings.Num() );
	cnt = 0;
	for( auto& t : constant_pool.utf8Strings )
	{
		trace( "^8[^7%i^8]\t ^7%s \n", cnt++, t.c_str() );
	}
	trace( "^8namespaceNames ^7: %i\n", constant_pool.namespaceNames.Num() );
	cnt = 0;
	for( auto& t : constant_pool.namespaceNames )
	{
		trace( "^8[^7%i^8]\t ^7%s \n", cnt++, t->c_str() );
	}

	trace( "^8namespaceSets ^7: %i\n", constant_pool.namespaceSets.Num() );
	cnt = 0;
	for( auto& t : constant_pool.namespaceSets )
	{
		for( auto& ts : t )
		{
			trace( "^8[^7%i^8]\t ^7%s \n", cnt++, ts->c_str() );
		}
	}

	trace( "^8multinameInfos ^7: %i\n", constant_pool.multinameInfos.Num() );
	cnt = 0;
	for( auto& t : constant_pool.multinameInfos )
	{
		idStr pre = "^8[^7";
		pre += idStr( cnt++ );
		pre += "^8]^7\t";
		traceMN( pre.c_str(), &t, constant_pool );
	}

	trace( "^8========================================================\n" );
}


void ReadMultiName( idSWFBitStream& bitstream , swfMultiname& target )
{
	target.type = ( swfConstantKind_t )bitstream.ReadU8();
	target.index = 0;
	target.nameIndex = 0;
	switch( target.type )
	{
		case RTQnameL:
		case RTQnameLA:
			//0,0
			break;
		case Qname:
		case QnameA:
			target.index = bitstream.ReadEncodedU32();
			target.nameIndex = bitstream.ReadEncodedU32();
			break;

		case RTQname:
		case RTQnameA:
			target.nameIndex = bitstream.ReadEncodedU32();
			break;

		case TypeName:
			target.nameIndex = bitstream.ReadEncodedU32();
			target.nameIndexT = bitstream.ReadEncodedU32();
			target.indexT = bitstream.ReadEncoded<uint8>();
			break;
		case Multiname:
		case MultinameA:
			target.nameIndex = bitstream.ReadEncodedU32();
			target.index = bitstream.ReadEncodedU32();
			break;

		case MultinameL:
		case MultinameLA:
			target.index = bitstream.ReadEncodedU32();
			break;
		default:
			common->FatalError( "Invalid Multiname type" );
			break;
	}
}

void ReadConstantPoolInfo( idSWFBitStream& bitstream ,  swfConstant_pool_info& target )
{
	/*cpool_info{}*/
	uint32 int_count = bitstream.ReadEncodedU32() ;
	target.integers.Alloc() = 0;
	for( uint i = 1; i < int_count; i++ )
	{
		target.integers.Alloc() = bitstream.ReadEncoded<int32>();
	}

	uint32 uint_count = bitstream.ReadEncodedU32();
	target.uIntegers.Alloc() = 0;
	for( uint i = 1; i < uint_count; i++ )
	{
		target.uIntegers.Alloc() = bitstream.ReadEncodedU32();
	}

	uint32 double_count = bitstream.ReadEncodedU32();
	target.doubles.Alloc() = 0.0;
	for( uint i = 1; i < double_count; i++ )
	{
		//WARNING IEEE-754
		target.doubles.Alloc() = *( double* )bitstream.ReadData( 8 );
	}

	uint32 string_count = bitstream.ReadEncodedU32();
	target.utf8Strings.Alloc().Append( "*" );
	for( uint i = 1; i < string_count; i++ )
	{
		uint32 str_len = bitstream.ReadEncodedU32();
		target.utf8Strings.Alloc().Append( ( char* ) bitstream.ReadData( str_len ), str_len );
	}

	uint32 namespace_count = bitstream.ReadEncodedU32();
	target.namespaceNames.Alloc() = &target.utf8Strings[0];
	for( uint i = 1; i < namespace_count; i++ )
	{
		target.namespaceKinds.Alloc() = ( swfConstantKind_t )bitstream.ReadU8();
		uint32 str_idx = bitstream.ReadEncodedU32();
		target.namespaceNames.Alloc() = &target.utf8Strings[( int )str_idx];
	}

	uint32 namespace_set_count = bitstream.ReadEncodedU32();
	target.namespaceSets.Alloc().Alloc() = target.namespaceNames[0];
	for( uint i = 1; i < namespace_set_count; i++ )
	{
		uint32 count = bitstream.ReadEncodedU32();
		auto& newSet = target.namespaceSets.Alloc();
		for( uint j = 0; j < count; j++ )
		{
			uint32 idx = bitstream.ReadEncodedU32();
			newSet.Alloc() = target.namespaceNames[( int )idx];
		}
	}

	uint32 multiname_count = bitstream.ReadEncodedU32();
	auto& empty = target.multinameInfos.Alloc();
	empty.index = 0;
	empty.nameIndex = 0;
	empty.type = unused_0x00;
	for( uint i = 1; i < multiname_count; i++ )
	{
		auto& newMn = target.multinameInfos.Alloc();
		ReadMultiName( bitstream, newMn );
	}
}

void SWF_AbcFile::ReadOptionInfo( idSWFBitStream& bitstream, swfOption_info& newOption )
{
	newOption.option_count = bitstream.ReadEncodedU32();
	for( uint i = 0; i < newOption.option_count; i++ )
	{
		auto& newItem = newOption.options.Alloc();
		newItem.val = bitstream.ReadEncodedU32();
		newItem.kind = constant_pool.namespaceKinds[bitstream.ReadEncodedU32()];
	}
}

void SWF_AbcFile::ReadMetaDataInfo( idSWFBitStream& bitstream , swfMetadata_info& newMetadata )
{
	newMetadata.name = &constant_pool.utf8Strings[bitstream.ReadEncodedU32()];
	uint32 item_count = bitstream.ReadEncodedU32();
	for( uint i = 0; i < item_count; i++ )
	{
		auto& newItem = newMetadata.items.Alloc();
		newItem.key =  &constant_pool.utf8Strings[bitstream.ReadEncodedU32()];
		newItem.value =  &constant_pool.utf8Strings[bitstream.ReadEncodedU32()];
	}
}

void SWF_AbcFile::ReadTraitData( idSWFBitStream& bitstream, swfTraits_info& newTraitsData )
{
	swfTraits_info::Type kind = ( swfTraits_info::Type )( ( newTraitsData.kind << 4 ) >> 4 ); // snip upper half, lower 4 bits should remain.
	uint8 attrib = ( newTraitsData.kind >> 4 ); // snip lower half, upper 4 bits should remain.

	switch( kind )
	{
		case swfTraits_info::Trait_Slot:
		case swfTraits_info::Trait_Const:
		{
			newTraitsData.data = Mem_ClearedAlloc( sizeof( swfTrait_slot ) , TAG_SWF );
			swfTrait_slot& slot = *( ( swfTrait_slot* )( newTraitsData.data ) );
			slot.slot_id = bitstream.ReadEncodedU32();
			slot.type_name = &constant_pool.multinameInfos[bitstream.ReadEncodedU32()];
			slot.vindex = bitstream.ReadEncodedU32();
			if( slot.vindex != 0 )
			{
				slot.vkind = bitstream.ReadU8();
			}
		}
		break;
		case swfTraits_info::Trait_Method:
		case swfTraits_info::Trait_Getter:
		case swfTraits_info::Trait_Setter:
		{
			newTraitsData.data = Mem_ClearedAlloc( sizeof( swfTrait_method ), TAG_SWF );
			swfTrait_method& method = *( swfTrait_method* )( newTraitsData.data );
			method.disp_id = bitstream.ReadEncodedU32();
			method.method = &methods[bitstream.ReadEncodedU32()];
		}
		break;
		case swfTraits_info::Trait_Class:
		{
			newTraitsData.data = Mem_ClearedAlloc( sizeof( swfTrait_class ), TAG_SWF );
			swfTrait_class& tclass = *( swfTrait_class* )( newTraitsData.data );
			tclass.slot_id = bitstream.ReadEncodedU32();
			tclass.classi = &classes[bitstream.ReadEncodedU32()];
		}
		break;
		case swfTraits_info::Trait_Function:
		{
			newTraitsData.data = Mem_ClearedAlloc( sizeof( swfTrait_function ), TAG_SWF );
			swfTrait_function& func = *( swfTrait_function* )( newTraitsData.data );
			func.slot_id = bitstream.ReadEncodedU32();
			func.func = &methods[bitstream.ReadEncodedU32()];
		}
		break;

		default:
			common->FatalError( "Unknown trait data" );
			break;
	}
}

//struct idSWFScriptObject::swfNamedVar_t ;

template<>
idSWFScriptObject::swfNamedVar_t* SWF_AbcFile::GetTrait( const swfTraits_info& trait, idSWFScriptObject* globals )
{
	swfTraits_info::Type kind = ( swfTraits_info::Type )( ( trait.kind << 4 ) >> 4 );  // snip upper half, lower 4 bits should remain.
	uint8 attrib = ( trait.kind >> 4 ); // snip lower half, upper 4 bits should remain.

	//idSWFScriptObject* newObj = new idSWFScriptObject();
	idSWFScriptObject::swfNamedVar_t* newVar = new idSWFScriptObject::swfNamedVar_t();
	newVar->value.traitsInfo = &trait;
	switch( kind )
	{
		case swfTraits_info::Trait_Slot:
		case swfTraits_info::Trait_Const:
		{
			//member variable.
			swfTrait_slot& slot = *( ( swfTrait_slot* )( trait.data ) );
			newVar->name = constant_pool.utf8Strings[trait.name->nameIndex];
			//if ( slot.slot_id )
			//{
			//	newVar = newObj->GetVariable(slot.slot_id,true);
			//	newVar->name = constant_pool.utf8Strings[trait.name->nameIndex];
			//}else
			//	newVar = newObj->GetVariable(constant_pool.utf8Strings[trait.name->nameIndex].c_str(),true);

			if( slot.vindex != 0 )
				switch( slot.vkind )
				{
					case Utf8:
						newVar->value.SetString( constant_pool.utf8Strings[slot.vindex] );
						break;
					case Int:
						newVar->value.SetInteger( constant_pool.integers[slot.vindex] );
						break;
					case UInt:
						newVar->value.SetInteger( ( int32 )( constant_pool.uIntegers[slot.vindex] ) );
						break;
					case Double: // crap.
						newVar->value.SetFloat( ( float )( constant_pool.doubles[slot.vindex] ) );
						break;
					case Qname:
					{

					}
					case Namespace:
						newVar->value.SetString( *constant_pool.namespaceNames[slot.vindex] );
						break;
					case Multiname:
						break;
					case False:
						newVar->value.SetBool( false );
						break;
					case True:
						newVar->value.SetBool( true );
						break;
					case Null:
						newVar->value.SetUndefined();
						break;
					case QnameA:
					{
						idStr& typeName = constant_pool.utf8Strings[slot.vindex];
						if( globals->HasProperty( typeName.c_str() ) )
						{
							newVar->value.SetObject( globals->GetObject( typeName ) );
						}
						else
						{
							newVar->value.SetUndefined();
						}
						break;
					}
					break;
					case MultinameA:
						break;
					case RTQname:
						break;
					case RTQnameA:
						break;
					case RTQnameL:
						break;
					case RTQnameLA:
						break;
					case NamespaceSet:
						break;
					case PackageNamespace:
						break;
					case PackageInternalNs:
						break;
					case ProtectedNamespace:
						break;
					case ExplicitNamespace:
						break;
					case StaticProtectedNs:
						break;
					case MultinameL:
						break;
					case MultinameLA:
						break;
					case TypeName:
						break;
					default:
						break;
				}
			else
			{
				idStr& typeName = constant_pool.utf8Strings[slot.type_name->nameIndex];
				if( globals->HasProperty( typeName.c_str() ) )
				{
					auto* newobj = idSWFScriptObject::Alloc();
					newobj->SetPrototype( globals->GetObject( typeName.c_str() ) );
					newVar->value.SetObject( newobj );
				}
				else
				{
					newVar->value.SetUndefined();
				}
				break;
			}

			return newVar;
		}
		break;
		case swfTraits_info::Trait_Method:
		case swfTraits_info::Trait_Getter:
		case swfTraits_info::Trait_Setter:
		{
			newVar->name = constant_pool.utf8Strings[trait.name->nameIndex];
			swfTrait_method& method = *( ( swfTrait_method* )( trait.data ) );

			idStrPtr name = method.method->name;
			idStr owner;
			//string method owner
			//common->FatalError("This is wrong. now dont keep using the debug info but start resolving it properly!!!!");
			//check frame scripts!
			int slashPos = name->Find( "/" );
			if( slashPos != -1 )
			{
				owner = idStr( name->c_str(), 0, slashPos );
			}

			slashPos = owner.Find( ":" );
			if( slashPos != -1 )
			{
				owner = idStr( owner.c_str(), slashPos + 1, owner.Length() );
			}

			if( globals->HasProperty( owner.c_str() ) )
			{
				idSWFScriptFunction_Script* func = idSWFScriptFunction_Script::Alloc();
				func->SetAbcFile( this );
				func->SetData( method.method );
				newVar->value = idSWFScriptVar( func ) ;
			}
			else
			{
				newVar->value.SetUndefined();
			}
			break;

		}
		break;
		case swfTraits_info::Trait_Class:
		{
			int a = 0;
			//newTraitsData.data = Mem_ClearedAlloc( sizeof( swfTrait_class ) );
			//swfTrait_class &tclass = *( swfTrait_class * ) ( newTraitsData.data );
			//tclass.slot_id = bitstream.ReadEncodedU32();
			//tclass.classi = &classes[bitstream.ReadEncodedU32()];
		}
		break;
		case swfTraits_info::Trait_Function:
		{
			int a  = 0;
			//newTraitsData.data = Mem_ClearedAlloc( sizeof( swfTrait_function ) );
			//swfTrait_function &func = *( swfTrait_function * ) ( newTraitsData.data );
			//func.slot_id = bitstream.ReadEncodedU32();
			//func.func = &methods[bitstream.ReadEncodedU32()];
		}
		break;

		default:
			common->FatalError( "Unknown trait data" );
			break;
	}

	return newVar;
}

void SWF_AbcFile::ReadTraitsInfo( idSWFBitStream& bitstream, swfTraits_info& newTraitsData )
{
	newTraitsData.name = &constant_pool.multinameInfos[bitstream.ReadEncodedU32()];
	//The kind field contains two four-bit fields. The lower four bits determine the kind of this trait. The
	//	upper four bits comprise a bit vector providing attributes of the trait. See the following tables and
	//	sections for full descriptions.

	newTraitsData.kind = bitstream.ReadU8();
	ReadTraitData( bitstream, newTraitsData );
	uint8 attrib = ( newTraitsData.kind >> 4 ); // snip lower half, upper 4 bits should remain.
	if( ( attrib & swfTraits_info::Attrib::Metadata ) != 0 )
	{
		uint32 meta_count = bitstream.ReadEncodedU32();
		for( uint i = 0 ; i < meta_count; i++ )
		{
			newTraitsData.metadatas.Alloc() = &metadatas[bitstream.ReadEncodedU32()];
			//fix this.
		}
	}
}

void SWF_AbcFile::ReadClassInfo( idSWFBitStream& bitstream, swfClass_info& newClassData )
{
	newClassData.cinit = &methods[bitstream.ReadEncodedU32()];
	uint32 trait_count  = bitstream.ReadEncodedU32();
	newClassData.traits.AssureSize( trait_count );
	for( uint i = 0; i < trait_count; i++ )
	{
		ReadTraitsInfo( bitstream, newClassData.traits[i] );
	}

}

//The last entry in that array is the entry point for the ABC file; that is, the last entry�s
//initialization method contains the first bytecode that�s run when the ABC file is executed.
void SWF_AbcFile::ReadScriptInfo( idSWFBitStream& bitstream, swfScript_info& newScriptData )
{
	uint32 init = bitstream.ReadEncodedU32();
	uint32 trait_count = bitstream.ReadEncodedU32();
	newScriptData.init = &methods[init];
	trace( "%s \n", newScriptData.init->name->c_str() );
	newScriptData.traits.AssureSize( trait_count );
	for( uint i = 0; i < trait_count; i++ )
	{
		ReadTraitsInfo( bitstream, newScriptData.traits[i] );
	}
}

void SWF_AbcFile::ReadMethodBodyInfo( idSWFBitStream& bitstream, swfMethod_body_info& newMethodBody )
{
	newMethodBody.method = &methods[bitstream.ReadEncodedU32()];
	assert( newMethodBody.method->body == nullptr );
	newMethodBody.method->body = &newMethodBody;

	newMethodBody.max_stack = bitstream.ReadEncodedU32();
	newMethodBody.localCount = bitstream.ReadEncodedU32();
	newMethodBody.initScopeDepth = bitstream.ReadEncodedU32();
	newMethodBody.maxScopeDepth = bitstream.ReadEncodedU32();
	newMethodBody.codeLength = bitstream.ReadEncodedU32();
	newMethodBody.code.Load( bitstream.ReadData( newMethodBody.codeLength ), newMethodBody.codeLength, true ); // ( byte * ) Mem_ClearedAlloc( sizeof( byte ) * newMethodBody.codeLength );
	extern void swf_PrintStream( SWF_AbcFile * file , idSWFBitStream & bitstream );
	if( swf_abc_verbose.GetInteger() == 2 )
	{
		common->Printf( "============================\n" );
		common->Printf( "Method %s 's bytecode \n", newMethodBody.method->name->c_str() );
		common->Printf( "============================\n" );
		swf_PrintStream( this, newMethodBody.code );
	}
	//memcpy(newMethodBody.code,bitstream.ReadData(newMethodBody.codeLength),newMethodBody.codeLength);
	uint32 exception_count = bitstream.ReadEncodedU32();
	for( uint i = 0; i < exception_count; i++ )
	{
		auto& newExceptionInfo = newMethodBody.exceptions.Alloc();
		ReadExceptionInfo( bitstream, newExceptionInfo );
	}
	uint32 trait_count = bitstream.ReadEncodedU32();
	for( uint i = 0; i < trait_count; i++ )
	{
		auto& newTrait = newMethodBody.traits.Alloc();
		ReadTraitsInfo( bitstream, newTrait );
	}
}

void SWF_AbcFile::ReadExceptionInfo( idSWFBitStream& bitstream, swfException_info& newException )
{
	newException.from = bitstream.ReadEncodedU32();
	newException.to = bitstream.ReadEncodedU32();
	newException.target = bitstream.ReadEncodedU32();
	newException.exc_type = &constant_pool.utf8Strings[bitstream.ReadEncodedU32()];
	newException.var_name = &constant_pool.utf8Strings[bitstream.ReadEncodedU32()];
}

void SWF_AbcFile::ReadInstanceInfo( idSWFBitStream& bitstream, swfInstance_info& newInstancedata )
{
	newInstancedata.name = &constant_pool.multinameInfos[bitstream.ReadEncodedU32()];
	newInstancedata.super_name = &constant_pool.multinameInfos[bitstream.ReadEncodedU32()];

	if( swf_abc_verbose.GetBool() )
	{
		traceMN( "newInstancedata.name", newInstancedata.name, constant_pool );
		traceMN( "newInstancedata.super_name", newInstancedata.super_name, constant_pool );
	}

	newInstancedata.flags = ( swfInstanceFlags_t )bitstream.ReadU8();
	if( ( newInstancedata.flags & swfInstanceFlags_t::ClassProtectedNs ) != 0 )
	{
		newInstancedata.protectedNs = bitstream.ReadEncodedU32();
	}
	uint32 interface_count = bitstream.ReadEncodedU32();

	for( uint i = 0; i < interface_count; i++ )
	{
		newInstancedata.interfaces.Alloc() = &constant_pool.multinameInfos[bitstream.ReadEncodedU32()];
	}

	newInstancedata.iinit = &methods[bitstream.ReadEncodedU32()];

	uint32 trait_count = bitstream.ReadEncodedU32();

	for( uint i = 0; i < trait_count; i++ )
	{
		auto& newTrait = newInstancedata.traits.Alloc();
		ReadTraitsInfo( bitstream, newTrait );
	}

}

void SWF_AbcFile::ReadMethodInfo( idSWFBitStream& bitstream , swfMethod_info& newMethod )
{
	uint32 idx = 0;
	newMethod.paramCount = bitstream.ReadEncodedU32();
	idx =  bitstream.ReadEncodedU32();
	newMethod.returnType =  &constant_pool.multinameInfos[idx];

	for( uint i = 0; i < newMethod.paramCount; i++ )
	{
		idx = bitstream.ReadEncodedU32();
		newMethod.paramTypes.Alloc() = &constant_pool.multinameInfos[idx];
	}
	idx = bitstream.ReadEncodedU32();
	newMethod.name = &constant_pool.utf8Strings[idx];
	newMethod.flags = bitstream.ReadU8();
	newMethod.options.option_count = 0;
	if( ( newMethod.flags & swfMethod_info::HAS_OPTIONAL ) != 0 )
	{
		ReadOptionInfo( bitstream, newMethod.options );
	}

	trace( "newMethod.name %s \n", newMethod.name->c_str() );

	if( ( newMethod.flags & swfMethod_info::HAS_PARAM_NAMES ) != 0 )
	{
		trace( "newMethod.params %i \n", ( int )newMethod.paramCount );

		for( uint i = 0; i < newMethod.paramCount; i++ )
		{
			idx = bitstream.ReadEncodedU32();
			newMethod.paramNames.Alloc() = &constant_pool.utf8Strings[idx];

			trace( "newMethod.param %s \n", constant_pool.utf8Strings[idx].c_str() );
		}
	}

}


void SWF_AbcFile::WriteBinary( idFileLocal& file )
{
	file->WriteBig( ( int )AbcTagData.Length() );
	file->Write( AbcTagData.Ptr(), AbcTagData.Length() );
}

void SWF_AbcFile::LoadBinary( idFile* file )
{
	uint32 len;
	file->ReadBig( len );
	AbcTagData.LoadFromFile( file, len );
}

//Remove Accessibility
void idSWF::DoABC( idSWFBitStream& bitstream )
{
	SWF_AbcFile& newAbcFile = abcFile;
	int strmSize = bitstream.Length() + 6; // codeLength(uin16) + recordLength(uin32)

	if( !newAbcFile.AbcTagData.Length() )
	{
		newAbcFile.AbcTagData.Load( bitstream.Ptr(), bitstream.Length(), true );
	}

	uint32 flags = bitstream.ReadU32();
	idStr name = bitstream.ReadString();
	int dataSize = bitstream.Length() - name.Length();
	common->Printf( "DoABC %s flags %i tagsize %i bytecode size %i \n", name.c_str(), flags, strmSize, dataSize );

	bitstream.ReadLittle( newAbcFile.minor_version );
	bitstream.ReadLittle( newAbcFile.major_version );

	ReadConstantPoolInfo( bitstream, newAbcFile.constant_pool );
	SWF_AbcFile::traceConstantPool( newAbcFile.constant_pool );

	uint32 method_count = bitstream.ReadEncodedU32() ;
	newAbcFile.methods.AssureSize( method_count );
	trace( "method_count %i \n", method_count );
	for( uint i = 0; i < method_count; i++ )
	{
		auto& newMethod = newAbcFile.methods[i];
		newAbcFile.ReadMethodInfo( bitstream, newMethod );
	}

	uint32 meta_count = bitstream.ReadEncodedU32();
	newAbcFile.metadatas.AssureSize( meta_count );
	trace( "meta_count %i \n", meta_count );
	for( uint i = 0; i < meta_count; i++ )
	{
		auto& newMeta = newAbcFile.metadatas[i];
		newAbcFile.ReadMetaDataInfo( bitstream, newMeta );
	}

	newAbcFile.class_count = bitstream.ReadEncodedU32();
	newAbcFile.instances.AssureSize( newAbcFile.class_count );
	trace( "class_count %i (Instance) \n", newAbcFile.class_count );
	for( uint i = 0; i < newAbcFile.class_count ; i++ )
	{
		auto& newInstance = newAbcFile.instances[i];
		newAbcFile.ReadInstanceInfo( bitstream, newInstance );
	}

	trace( "class_count %i (Class) \n", newAbcFile.class_count );
	newAbcFile.classes.AssureSize( newAbcFile.class_count );
	for( uint i = 0; i < newAbcFile.class_count ; i++ )
	{
		auto& newClass = newAbcFile.classes[i];
		newAbcFile.ReadClassInfo( bitstream, newClass );
	}

	uint32 script_count = bitstream.ReadEncodedU32();
	newAbcFile.scripts.AssureSize( script_count );
	trace( "script_count %i \n", script_count );
	for( uint i = 0; i < script_count; i++ )
	{
		auto& newScript = newAbcFile.scripts[i];
		newAbcFile.ReadScriptInfo( bitstream, newScript );
	}

	uint32 methBody_count = bitstream.ReadEncodedU32();
	newAbcFile.method_bodies.AssureSize( methBody_count );
	trace( "methBody_count %i \n", methBody_count );
	for( uint i = 0; i < methBody_count; i++ )
	{
		auto& newMethBody = newAbcFile.method_bodies[i];
		newAbcFile.ReadMethodBodyInfo( bitstream, newMethBody );
	}
}

void idSWF::SymbolClass( idSWFBitStream& bitstream )
{

	//Header		RECORDHEADER	Tag type = 76
	//NumSymbols	UI16			Number of symbols that will be associated by this tag.
	//Tag1			U16				The 16 - bit character tag ID for the symbol to associate
	//Name1			STRING			The fully - qualified name of the ActionScript 3.0 class with which to associate this symbol.The class must have already been declared by a DoABC tag.
	//... ... ...
	//TagN			U16				Tag ID for symbol N
	//NameN STRING Fully - qualified class name for symbol N
	uint16 numSymbols = bitstream.ReadU16();
	for( uint i = 0 ; i < numSymbols; i++ )
	{
		auto& newSymbol = symbolClasses.symbols.Alloc();
		newSymbol.tag  = bitstream.ReadU16();
		newSymbol.name = bitstream.ReadString();
		trace( "SymbolClass ^5%i ^7tag ^5%i  ^2%s \n", i, newSymbol.tag, newSymbol.name.c_str() );
	}
}


