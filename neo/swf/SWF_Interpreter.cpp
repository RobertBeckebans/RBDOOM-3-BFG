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
#include "precompiled.h"
#pragma hdrstop

//#include "SWF_Bitstream.h"
//#include "SWF_Enums.h"
//#include "SWF_Abc.h"
//#include "SWF_ScriptObject.h"
//#include "SWF_ScriptFunction.h"
//#include "framework/Common.h"

inline int32 readS24( const byte* pc )
{
	return ( ( uint16_t* )pc )[0] | ( ( int8_t* )pc )[2] << 16;
}

inline uint32 readU30( const uint8_t*& pc )
{
	uint32 result = 0;
	for( int i = 0; i < 5; i++ )
	{
		byte b = *pc++;
		result |= ( b & 0x7F ) << ( 7 * i );
		if( ( b & 0x80 ) == 0 )
		{
			return result;
		}
	}
	return result;
}

const AbcOpcodeInfo opcodeInfo[] =
{
	// For stack movement ("stk") only constant movement is accounted for; variable movement,
	// as for arguments to CALL, CONSTRUCT, APPLYTYPE, et al, and for run-time parts of
	// names, must be handled separately.

#define ABC_OP(operandCount, canThrow, stack, internalOnly, nameToken)        { operandCount, canThrow, stack , OP_##nameToken	, #nameToken },
#define ABC_UNUSED_OP(operandCount, canThrow, stack, internalOnly, nameToken) { operandCount, canThrow, stack ,	0				, #nameToken },

#include "opcodes.tbl"

#undef ABC_OP
#undef ABC_UNUSED_OP
#undef ABC_OP_F
};

void debugfile( SWF_AbcFile* file,  idSWFBitStream& bitstream )
{
	common->Printf( " debugfile %s\n", file->constant_pool.utf8Strings[bitstream.ReadEncodedU32()].c_str() );
}

void debugline( SWF_AbcFile* file,  idSWFBitStream& bitstream )
{
	common->Printf( " debugline %i\n", ( int )bitstream.ReadEncodedU32() );
}

void swf_PrintStream( SWF_AbcFile* file, idSWFBitStream& bitstream )
{
	//(case.*OP_)([A-Za-z0-9]*_?[A-Za-z0-9]*)(.*\n)(.*)
	idSWFStack stack;
	static idList<AbcOpcodeInfo*> codeMap;
	idStr type;
	const AbcOpcodeInfo* info = nullptr;
	while( bitstream.Tell() < bitstream.Length() )
	{
#define DoWordCode( n ) case OP_##n: type = #n; info = &opcodeInfo[opCode]; break;
#define ExecWordCode( n ) case OP_##n: type = #n; info = &opcodeInfo[opCode]; n(file,bitstream); continue;
		SWFAbcOpcode opCode = ( SWFAbcOpcode ) bitstream.ReadU8();
		switch( opCode )
		{
				DoWordCode( bkpt );
				DoWordCode( nop );
				DoWordCode( throw );
				DoWordCode( getsuper );
				DoWordCode( setsuper );
				DoWordCode( dxns );
				DoWordCode( dxnslate );
				DoWordCode( kill );
				DoWordCode( label );
				DoWordCode( ifnlt );
				DoWordCode( ifnle );
				DoWordCode( ifngt );
				DoWordCode( ifnge );
				DoWordCode( jump );
				DoWordCode( iftrue );
				DoWordCode( iffalse );
				DoWordCode( ifeq );
				DoWordCode( ifne );
				DoWordCode( iflt );
				DoWordCode( ifle );
				DoWordCode( ifgt );
				DoWordCode( ifge );
				DoWordCode( ifstricteq );
				DoWordCode( ifstrictne );
				DoWordCode( lookupswitch );
				DoWordCode( pushwith );
				DoWordCode( popscope );
				DoWordCode( nextname );
				DoWordCode( hasnext );
				DoWordCode( pushnull );
				DoWordCode( pushundefined );
				DoWordCode( DISABLED_pushfloat );
				DoWordCode( nextvalue );
				DoWordCode( pushbyte );
				DoWordCode( pushshort );
				DoWordCode( pushtrue );
				DoWordCode( pushfalse );
				DoWordCode( pushnan );
				DoWordCode( pop );
				DoWordCode( dup );
				DoWordCode( swap );
				DoWordCode( pushstring );
				DoWordCode( pushint );
				DoWordCode( pushuint );
				DoWordCode( pushdouble );
				DoWordCode( pushscope );
				DoWordCode( pushnamespace );
				DoWordCode( hasnext2 );
				DoWordCode( lix8 );
				DoWordCode( lix16 );
				DoWordCode( li8 );
				DoWordCode( li16 );
				DoWordCode( li32 );
				DoWordCode( lf32 );
				DoWordCode( lf64 );
				DoWordCode( si8 );
				DoWordCode( si16 );
				DoWordCode( si32 );
				DoWordCode( sf32 );
				DoWordCode( sf64 );
				DoWordCode( newfunction );
				DoWordCode( call );
				DoWordCode( construct );
				DoWordCode( callmethod );
				DoWordCode( callstatic );
				DoWordCode( callsuper );
				DoWordCode( callproperty );
				DoWordCode( returnvoid );
				DoWordCode( returnvalue );
				DoWordCode( constructsuper );
				DoWordCode( constructprop );
				DoWordCode( callsuperid );
				DoWordCode( callproplex );
				DoWordCode( callinterface );
				DoWordCode( callsupervoid );
				DoWordCode( callpropvoid );
				DoWordCode( sxi1 );
				DoWordCode( sxi8 );
				DoWordCode( sxi16 );
				DoWordCode( applytype );
				DoWordCode( DISABLED_pushfloat4 );
				DoWordCode( newobject );
				DoWordCode( newarray );
				DoWordCode( newactivation );
				DoWordCode( newclass );
				DoWordCode( getdescendants );
				DoWordCode( newcatch );
				DoWordCode( findpropglobalstrict );
				DoWordCode( findpropglobal );
				DoWordCode( findpropstrict );
				DoWordCode( findproperty );
				DoWordCode( finddef );
				DoWordCode( getlex );
				DoWordCode( setproperty );
				DoWordCode( getlocal );
				DoWordCode( setlocal );
				DoWordCode( getglobalscope );
				DoWordCode( getscopeobject );
				DoWordCode( getproperty );
				DoWordCode( getouterscope );
				DoWordCode( initproperty );
				DoWordCode( deleteproperty );
				DoWordCode( getslot );
				DoWordCode( setslot );
				DoWordCode( getglobalslot );
				DoWordCode( setglobalslot );
				DoWordCode( convert_s );
				DoWordCode( esc_xelem );
				DoWordCode( esc_xattr );
				DoWordCode( convert_i );
				DoWordCode( convert_u );
				DoWordCode( convert_d );
				DoWordCode( convert_b );
				DoWordCode( convert_o );
				DoWordCode( checkfilter );
				//DoWordCode ( DISABLED_convert );
				//DoWordCode ( DISABLED_unplus );
				//DoWordCode ( DISABLED_convert );
				DoWordCode( coerce );
				DoWordCode( coerce_b );
				DoWordCode( coerce_a );
				DoWordCode( coerce_i );
				DoWordCode( coerce_d );
				DoWordCode( coerce_s );
				DoWordCode( astype );
				DoWordCode( astypelate );
				DoWordCode( coerce_u );
				DoWordCode( coerce_o );
				DoWordCode( negate );
				DoWordCode( increment );
				DoWordCode( inclocal );
				DoWordCode( decrement );
				DoWordCode( declocal );
				DoWordCode( typeof );
				DoWordCode( not );
				DoWordCode( bitnot );
				DoWordCode( add );
				DoWordCode( subtract );
				DoWordCode( multiply );
				DoWordCode( divide );
				DoWordCode( modulo );
				DoWordCode( lshift );
				DoWordCode( rshift );
				DoWordCode( urshift );
				DoWordCode( bitand );
				DoWordCode( bitor );
				DoWordCode( bitxor );
				DoWordCode( equals );
				DoWordCode( strictequals );
				DoWordCode( lessthan );
				DoWordCode( lessequals );
				DoWordCode( greaterthan );
				DoWordCode( greaterequals );
				DoWordCode( instanceof );
				DoWordCode( istype );
				DoWordCode( istypelate );
				DoWordCode( in );
				DoWordCode( increment_i );
				DoWordCode( decrement_i );
				DoWordCode( inclocal_i );
				DoWordCode( declocal_i );
				DoWordCode( negate_i );
				DoWordCode( add_i );
				DoWordCode( subtract_i );
				DoWordCode( multiply_i );
				DoWordCode( getlocal0 );
				DoWordCode( getlocal1 );
				DoWordCode( getlocal2 );
				DoWordCode( getlocal3 );
				DoWordCode( setlocal0 );
				DoWordCode( setlocal1 );
				DoWordCode( setlocal2 );
				DoWordCode( setlocal3 );
				DoWordCode( debug );
				ExecWordCode( debugline );
				ExecWordCode( debugfile );
				DoWordCode( bkptline );
				DoWordCode( timestamp );
				DoWordCode( restargc );
				DoWordCode( restarg );
			default:
				common->Printf( "default %s %s\n", type.c_str( ) , info ? info->name : "Empty" );
		}
		static const char* tabs[] = { " ", "  ", "   ", "    ", "      ", "       ", "        ", "         ", "          ", "           ", "            ", "             ", "              ", "               ", "                "};
		if( info && info->operandCount > 0 )
		{
			bitstream.ReadData( info->operandCount );
		}
		common->Printf( " %s %s o %s%i  \t s %s%i \n" ,
						info ? info->name : type.c_str( ),
						tabs[int( 18 - ( int( idStr::Length( info->name ) ) ) )],
						info->operandCount > 0 ? "^2" : "^1" ,
						info->operandCount,
						info->stack < 0 ? "^2" : "^1",
						info->stack
					  );
	}
	bitstream.Rewind();
#undef DoWordCode
#undef ExecWordCode
}

void idSWFScriptFunction_Script::findproperty( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32()];
	const idStrPtr propName = ( idStrPtr )&cp.utf8Strings[mn.nameIndex];
	//search up scope stack.
	for( int i = scope.Num() - 1; i >= 0; i-- )
	{
		auto* s = scope[i];
		while( s )
			if( s->HasProperty( propName->c_str() ) )
			{
				stack.Alloc() = s->Get( propName->c_str() );
				return;
			}
			else if( s->GetPrototype() && s->GetPrototype()->GetPrototype() )
			{
				s = s->GetPrototype()->GetPrototype();
			}
			else
			{
				s = NULL;
			}
	}
	auto prop = scope[0]->GetVariable( *propName, true );
	stack.Alloc() = prop->value;
}

void idSWFScriptFunction_Script::findpropstrict( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
	const idStrPtr propName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];
	//search up scope stack.
	for( int i = scope.Num( ) - 1; i >= 0; i-- )
	{
		auto* s = scope[i];
		while( s )
			if( s->HasProperty( propName->c_str( ) ) )
			{
				stack.Alloc( ) = s->Get( propName->c_str( ) );
				return;
			}
			else if( s->GetPrototype( ) && s->GetPrototype( )->GetPrototype( ) )
			{
				s = s->GetPrototype( )->GetPrototype( );
			}
			else
			{
				s = NULL;
			}
	}
	common->Warning( "idSWFScriptFunction_Script::findpropstrict cant find %s", propName->c_str() );
	stack.Alloc().SetObject( idSWFScriptObject::Alloc() );
	stack.A().GetObject()->Release();
}

void idSWFScriptFunction_Script::getlex( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
	const idStrPtr propName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];
	//search up scope stack.
	for( int i = scope.Num( ) - 1; i >= 0; i-- )
	{
		auto* s = scope[i];
		while( s )
			if( s->HasProperty( propName->c_str( ) ) )
			{
				stack.Alloc( ) = s->Get( propName->c_str( ) );
				return;
			}
			else if( s->GetPrototype( ) && s->GetPrototype( )->GetPrototype() )
			{
				s = s->GetPrototype( )->GetPrototype();
			}
			else
			{
				s = NULL;
			}
	}
}

void idSWFScriptFunction_Script::getscopeobject( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	uint8 index = bitstream.ReadEncodedU32();
	stack.Alloc() = scope[( scope.Num() - 1 ) - index];
}

void idSWFScriptFunction_Script::pushscope( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	if( stack.Num() > 0 )
	{
		if( stack.A().IsObject() )
		{
			auto stackOBj = stack.A().GetObject();
			stackOBj->AddRef();
			scope.Alloc() = stackOBj;

		}
		else
		{
			common->DWarning( "tried to push a non object onto scope" );
		}
		stack.Pop( 1 );
	}
}

void idSWFScriptFunction_Script::getlocal0( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	stack.Alloc() = registers[0];
}

//Classes are constructed implicitly
void idSWFScriptFunction_Script::newclass( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& ci = file->classes[bitstream.ReadEncodedU32( )];
	idSWFScriptVar  base = stack.A();
	stack.Pop( 1 );
	idSWFScriptVar* thisObj = &registers[0];
	common->FatalError( "Bytestream corrupted? Classes should already be created in CreateAbcObjects()!" );
}

void idSWFScriptFunction_Script::callpropvoid( SWF_AbcFile* file, idSWFStack& stack, idSWFBitStream& bitstream )
{
	const auto& cp = file->constant_pool;
	const auto& mn = file->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
	const idStrPtr funcName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];

	uint32 arg_count = bitstream.ReadEncodedU32();

	//Todo Optimize: search for addFrameScript string index in constantpool!
	if( *funcName == "addFrameScript" )
	{
		stack.Pop( arg_count );
		arg_count = 0;
	}
	idSWFParmList parms( arg_count );

	for( int i = 0; i < parms.Num( ); i++ )
	{
		parms[parms.Num() - 1 - i] = stack.A();
		stack.Pop( 1 );
	}
	if( stack.Num() )
	{
		idSWFScriptVar& item = stack.A();
		if( item.IsFunction() )
		{
			auto func = ( ( idSWFScriptFunction_Script* )item.GetFunction() );
			if( !func->GetScope()->Num() )
			{
				func->SetScope( *GetScope() );
			}
			item.GetFunction()->Call( registers[0].GetObject(), parms );
		}
		else if( item.IsObject() )
		{
			auto func = item.GetObject()->Get( funcName->c_str() );
			if( func.IsFunction() )
			{
				if( !( ( idSWFScriptFunction_Script* )func.GetFunction() )->GetScope()->Num() )
				{
					( ( idSWFScriptFunction_Script* )func.GetFunction() )->SetScope( *GetScope() );
				}
				func.GetFunction()->Call( item.GetObject(), parms );
			}
		}
		stack.Pop( 1 );
	}
}

/*
========================
idSWFScriptFunction_Script::RunAbc bytecode
========================
*/
idSWFScriptVar idSWFScriptFunction_Script::RunAbc( idSWFScriptObject* thisObject, idSWFStack& stack, idSWFBitStream& bitstream )
{
	static int abcCallstackLevel = -1;
	assert( abcFile );
	idSWFScriptVar returnValue;

	idSWFSpriteInstance* thisSprite = thisObject->GetSprite();
	idSWFSpriteInstance* currentTarget = thisSprite;

	if( currentTarget == NULL )
	{
		thisSprite = currentTarget = defaultSprite;
	}

	abcCallstackLevel++;
	while( bitstream.Tell( ) < bitstream.Length( ) )
	{
#define ExecWordCode( n ) case OP_##n: n(abcFile,stack,bitstream); continue;
#define InlineWordCode( n ) case OP_##n:
		SWFAbcOpcode opCode = ( SWFAbcOpcode ) bitstream.ReadU8();
		switch( opCode )
		{
				//ExecWordCode( bkpt );
				//ExecWordCode( throw );
				//ExecWordCode( getsuper );
				//ExecWordCode( setsuper );
				//ExecWordCode( dxns );
				//ExecWordCode( dxnslate );
				InlineWordCode( kill )
				{
					registers[bitstream.ReadEncodedU32()].SetUndefined();
					continue;
				}
				InlineWordCode( nop );
				InlineWordCode( label )
				{
					continue;
				}
				InlineWordCode( ifnlt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() < rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() < rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() < rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifnle )
				{
					int offset = bitstream.ReadS24();
					const auto& lH = stack.B();
					const auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() <= rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() <= rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() <= rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifngt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() > rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() > rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() > rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifnge )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = !( lH.ToString() >= rH.ToString() );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = !( lH.ToFloat() >= rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = !( lH.ToInteger() >= rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( jump )
				{
					int offset = bitstream.ReadS24();
					bitstream.Seek( offset );
					continue;
				}
				InlineWordCode( iftrue )
				{
					int offset = bitstream.ReadS24();
					idSWFScriptVar value = stack.A();
					stack.Pop( 1 );
					bool condition = value.ToBool();
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( iffalse )
				{
					int offset = bitstream.ReadS24();
					if( !stack.A().ToBool() )
					{
						bitstream.Seek( offset );
					}
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( ifeq )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() == rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() == rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() == rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifne )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() != rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() != rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() != rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( iflt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() < rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() < rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() < rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifle )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() <= rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() <= rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() <= rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifgt )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() > rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() > rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() > rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifge )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							condition = lH.ToString() >= rH.ToString();
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							condition = lH.ToFloat() >= rH.ToFloat();
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							condition = lH.ToInteger() >= rH.ToInteger();
							break;
						default:
							common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifstricteq )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					if( lH.GetType() != rH.GetType() )
					{
						condition = false;
					}
					else
					{
						switch( lH.GetType() )
						{
							case idSWFScriptVar::SWF_VAR_STRING:
								condition = lH.ToString() == rH.ToString();
								break;
							case idSWFScriptVar::SWF_VAR_FLOAT:
								condition = lH.ToFloat() == rH.ToFloat();
								break;
							case idSWFScriptVar::SWF_VAR_INTEGER:
								condition = lH.ToInteger() == rH.ToInteger();
								break;
							default:
								common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
						}
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				InlineWordCode( ifstrictne )
				{
					int offset = bitstream.ReadS24();
					auto& lH = stack.B();
					auto& rH = stack.A();
					stack.Pop( 2 );
					bool condition = false;
					if( lH.GetType() != rH.GetType() )
					{
						condition = true;
					}
					else
					{
						switch( lH.GetType() )
						{
							case idSWFScriptVar::SWF_VAR_STRING:
								condition = ( lH.ToString() != rH.ToString() );
								break;
							case idSWFScriptVar::SWF_VAR_FLOAT:
								condition = lH.ToFloat() != rH.ToFloat();
								break;
							case idSWFScriptVar::SWF_VAR_INTEGER:
								condition = lH.ToInteger() != rH.ToInteger();
								break;
							default:
								common->Warning( " Tried to compare incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
						}
					}
					if( condition )
					{
						bitstream.Seek( offset );
					}
					continue;
				}
				//ExecWordCode( lookupswitch );
				//ExecWordCode( pushwith );
				InlineWordCode( popscope )
				{
					scope[scope.Num() - 1]->Release();
					scope.SetNum( scope.Num() - 1 );
					continue;
				}
				//ExecWordCode( nextname );
				//ExecWordCode( hasnext );
				InlineWordCode( pushnull )
				{
					stack.Append( idSWFScriptVar( ( idSWFScriptObject* )( NULL ) ) );
					continue;
				}
				InlineWordCode( pushundefined )
				{
					stack.Append( idSWFScriptVar() );
					continue;
				}
				//ExecWordCode( nextvalue );
				InlineWordCode( pushbyte )
				{
					stack.Append( idSWFScriptVar( ( int )bitstream.ReadU8() ) );
					continue;
				}
				InlineWordCode( pushshort )
				{
					stack.Append( idSWFScriptVar( ( int )bitstream.ReadEncodedU32() ) );
					continue;
				}
				InlineWordCode( pushtrue )
				{
					stack.Append( idSWFScriptVar( true ) );
					continue;
				}
				InlineWordCode( pushfalse )
				{
					stack.Append( idSWFScriptVar( false ) );
					continue;
				}
				//ExecWordCode ( pushnan );
				InlineWordCode( pop )
				{
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( dup )
				{
					stack.Alloc() = idSWFScriptVar( stack.A() );
					continue;
				}
				InlineWordCode( swap )
				{
					common->FatalError( "swap not implemented" );
					continue;
				}
				InlineWordCode( pushstring )
				{
					const auto& cp = abcFile->constant_pool.utf8Strings;
					const auto& mn = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptString( mn ) );
					continue;
				}
				InlineWordCode( pushint )
				{
					const auto& cp = abcFile->constant_pool.integers;
					const auto& val = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptVar( val ) );
					continue;
				}
				InlineWordCode( pushuint )
				{
					const auto& cp = abcFile->constant_pool.uIntegers;
					const auto& val = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptVar( ( int )val ) );
					continue;
				}
				InlineWordCode( pushdouble )
				{
					const auto& cp = abcFile->constant_pool.doubles;
					const auto& val = cp[bitstream.ReadEncodedU32( )];
					stack.Append( idSWFScriptVar( ( float )val ) );
					continue;
				}
				ExecWordCode( pushscope )
				//ExecWordCode( pushnamespace );
				//ExecWordCode( hasnext2 );
				//ExecWordCode( lix8 );
				//ExecWordCode( lix16 );
				//ExecWordCode( li8 );
				//ExecWordCode( li16 );
				//ExecWordCode( li32 );
				//ExecWordCode( lf32 );
				//ExecWordCode( lf64 );
				//ExecWordCode( si8 );
				//ExecWordCode( si8 );
				//ExecWordCode( si16 );
				//ExecWordCode( si32 );
				//ExecWordCode( sf32 );
				//ExecWordCode( sf64 );
				InlineWordCode( newfunction )
				{
					const auto& cp = abcFile->constant_pool;
					auto& method = abcFile->methods[bitstream.ReadEncodedU32()];
					idSWFScriptFunction_Script* func = idSWFScriptFunction_Script::Alloc();
					func->SetAbcFile( abcFile );
					func->methodInfo = &method;
					func->SetScope( scope );
					func->SetConstants( constants );
					func->SetDefaultSprite( defaultSprite );
					stack.Alloc() = idSWFScriptVar( func );
					stack.A().GetFunction()->Release();
					continue;
				}
				InlineWordCode( call )
				InlineWordCode( construct )
				InlineWordCode( callmethod )
				InlineWordCode( callstatic )
				InlineWordCode( callsuper )
				{
					common->FatalError( "Not implemented" );
					continue;
				}
				InlineWordCode( callproperty )
				{
					//fold this with callpropvoid.
					const auto& cp = abcFile->constant_pool;
					const auto& mn = abcFile->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
					const idStrPtr funcName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];
					uint32 arg_count = bitstream.ReadEncodedU32( );

					idSWFParmList parms( arg_count );

					for( int i = 0; i < parms.Num( ); i++ )
					{
						parms[parms.Num() - 1 - i] = stack.A( );
						stack.Pop( 1 );
					}
					idSWFScriptVar& item = stack.A( );

					if( item.IsFunction( ) )
					{
						stack.Pop( 1 );
						stack.Alloc() = item.GetFunction( )->Call( registers[0].GetObject( ), parms );
					}
					else if( item.IsObject( ) )
					{
						auto func = item.GetObject()->Get( funcName->c_str() );
						if( !func.IsFunction() ) // search up scope
						{
							for( int i = scope.Num() - 1; i >= 0; i-- )
							{
								auto* s = scope[i];
								while( s )
								{
									if( s->HasProperty( funcName->c_str() ) )
									{
										func = s->Get( funcName->c_str() );
										s = NULL;
										i = -1;
										break;
									}
									else if( s->GetPrototype() && s->GetPrototype()->GetPrototype() )
									{
										s = s->GetPrototype()->GetPrototype();
									}
									else
									{
										s = NULL;
									}
								}
							}
						}
						if( func.IsFunction( ) )
						{
							stack.Alloc() = func.GetFunction( )->Call( item.GetObject( ), parms );
						}
					}
					continue;
				}
				InlineWordCode( returnvalue )
				{
					returnValue = stack.A();
					[[fallthrough]];
				}
				InlineWordCode( returnvoid )
				{
					if( scope.Num() )
					{
						scope[scope.Num() - 1]->Release();
						scope.SetNum( scope.Num() - 1 );
					}
					continue;
				}
				InlineWordCode( constructsuper )
				{
					uint32 args = bitstream.ReadEncodedU32( );
					stack.Pop( args );
					continue;
				}
				InlineWordCode( constructprop )
				{
					//no need to call constructors for props that
					const auto& cp = abcFile->constant_pool;
					const auto& mn = abcFile->constant_pool.multinameInfos[bitstream.ReadEncodedU32( )];
					const idStrPtr propName = ( idStrPtr ) &cp.utf8Strings[mn.nameIndex];
					uint32 arg_count = bitstream.ReadEncodedU32( );
					if( *propName == "Array" )
					{
						for( int i = 0; i < arg_count; i++ )
						{
							stack[stack.Num() - ( arg_count + 1 )].GetObject()->Set( i, stack[stack.Num() - arg_count + i] );
						}
					}
					stack.Pop( arg_count );
					continue;
				}
				//ExecWordCode( callsuperid );
				//ExecWordCode( callproplex );
				//ExecWordCode( callinterface );
				//ExecWordCode( callsupervoid );
				ExecWordCode( callpropvoid );
				//ExecWordCode( sxi1 );
				//ExecWordCode( sxi8 );
				//ExecWordCode( sxi16 );
				//ExecWordCode( applytype );
				//ExecWordCode( DISABLED_pushfloat4 );
				InlineWordCode( newarray )
				{
					auto* newArray = idSWFScriptObject::Alloc();

					newArray->MakeArray();

					uint32 args = bitstream.ReadEncodedU32();
					for( int i = 0; i < args; i++ )
					{
						newArray->Set( i, stack.A() );
						stack.Pop( 1 );
					}

					idSWFScriptVar baseObjConstructor = scope[0]->Get( "Object" );
					idSWFScriptFunction* baseObj = baseObjConstructor.GetFunction();
					newArray->SetPrototype( baseObj->GetPrototype() );
					stack.Alloc().SetObject( newArray );

					newArray->Release();
					continue;
				}
				InlineWordCode( newobject );
				InlineWordCode( newactivation )
				{
					idSWFScriptObject* object = idSWFScriptObject::Alloc();
					idSWFScriptVar baseObjConstructor = scope[0]->Get( "Object" );
					idSWFScriptFunction* baseObj = baseObjConstructor.GetFunction();
					object->SetPrototype( baseObj->GetPrototype() );
					stack.Alloc().SetObject( object );
					object->Release();
					continue;
				}
				ExecWordCode( newclass );
				//ExecWordCode ( getdescendants );
				//ExecWordCode ( newcatch );
				//ExecWordCode ( findpropglobalstrict );
				//ExecWordCode ( findpropglobal );
				ExecWordCode( findproperty );
				ExecWordCode( findpropstrict );

				//ExecWordCode ( finddef );
				ExecWordCode( getlex );
				InlineWordCode( setproperty )
				{
					const auto& cp = abcFile->constant_pool;
					const auto& mn = cp.multinameInfos[bitstream.ReadEncodedU32()];
					const auto& n = cp.utf8Strings[mn.nameIndex];
					idStr index = n;
					idSWFScriptObject* target = nullptr;

					if( mn.nameIndex && !stack.B().IsObject() )
					{
						target = scope[0];
					}
					else if( !mn.nameIndex )
					{
						index = stack.B().ToString();
						idSWFScriptVar val = stack.A();
						stack.Pop( 2 );
						target = stack.A().GetObject();
						stack.Alloc() = val;
					}
					else if( stack.B().IsObject() )
					{
						target = stack.B().GetObject();
					}
					target->Set( index, stack.A() );
					stack.Pop( 2 );
					continue;
				}
				InlineWordCode( getlocal )
				{
					stack.Alloc() = registers[bitstream.ReadEncodedU32()];
					continue;
				}
				InlineWordCode( setlocal );
				{
					registers[bitstream.ReadEncodedU32()] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				//ExecWordCode ( getglobalscope );
				ExecWordCode( getscopeobject );
				InlineWordCode( getproperty )
				{
					const auto& cp = abcFile->constant_pool;
					const auto& mn = cp.multinameInfos[bitstream.ReadEncodedU32()];
					const auto& n = cp.utf8Strings[mn.nameIndex];

					idStr index = n;
					idSWFScriptObject* target = nullptr;

					if( mn.nameIndex && !stack.A().IsObject() )
					{
						target = scope[0];
					}
					else if( !mn.nameIndex )
					{
						target = stack.B().GetObject();
						index = stack.A().ToString();
						stack.Pop( 1 );
					}
					else
					{
						target = stack.A().GetObject();
					}

					stack.Pop( 1 );

					if( target->HasProperty( index.c_str() ) )
					{
						stack.Append( target->Get( index.c_str() ) );
					}
					else
					{
						stack.Alloc().SetObject( idSWFScriptObject::Alloc() );
						stack.A().GetObject()->Release();
					}

					continue;
				}
				//ExecWordCode ( getouterscope );
				InlineWordCode( initproperty )
				{
					const auto& cp = abcFile->constant_pool;
					const auto& mn = cp.multinameInfos[bitstream.ReadEncodedU32( )];
					const auto& n = cp.utf8Strings[mn.nameIndex];

					idSWFScriptVar value = stack.A( );
					stack.Pop( 1 );
					stack.A().GetObject()->Set( n, value );
					continue;
				}
				//ExecWordCode ( 0x69 );
				//ExecWordCode ( deleteproperty );
				//ExecWordCode ( 0x6B );
				InlineWordCode( getslot )
				{
					if( stack.A().IsObject() )
					{
						stack.Append( stack.A().GetObject()->Get( bitstream.ReadEncodedU32() ) );
					}
					continue;
				}

				InlineWordCode( setslot )
				{
					auto var = stack.A();

					if( stack.B().IsUndefined() || stack.B().IsNULL() )
					{
						stack.B().SetObject( idSWFScriptObject::Alloc() );
						stack.B().GetObject()->MakeArray();
					}

					stack.B().GetObject()->Set( bitstream.ReadEncodedU32(), var );
					continue;
				}
				//ExecWordCode ( getglobalslot );
				//ExecWordCode ( setglobalslot );
				//ExecWordCode ( convert_s );
				//ExecWordCode ( esc_xelem );
				//ExecWordCode ( esc_xattr );
				//ExecWordCode ( convert_i );
				//ExecWordCode ( convert_u );
				//ExecWordCode ( convert_d );
				//ExecWordCode ( convert_b );
				//ExecWordCode ( convert_o );
				//ExecWordCode ( checkfilter );
				//ExecWordCode ( DISABLED_convert );
				//ExecWordCode ( DISABLED_unplus );
				//ExecWordCode ( DISABLED_convert );
				//ExecWordCode ( coerce );
				//ExecWordCode ( coerce_b );
				InlineWordCode( coerce_a )
				{
					auto var = stack.A();
					stack.Pop( 1 );
					if( !stack.A().IsValid() )
					{
						stack.A().SetNULL();
						stack.Append( var );
					}
					else
					{
						if( !var.IsUndefined() )
						{
							stack.Append( var.ToString() );
						}
						else
						{
							stack.Append( var );
						}

					}

					continue;
				}
				//ExecWordCode ( coerce_i );
				//ExecWordCode ( coerce_d );
				//ExecWordCode ( coerce_s );
				//ExecWordCode ( astype );
				//ExecWordCode ( astypelate );
				//ExecWordCode ( coerce_u );
				//ExecWordCode ( coerce_o );
				InlineWordCode( negate_i )
				InlineWordCode( negate )
				{
					auto& val = stack.A( );
					idSWFScriptVar result;
					switch( val.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( -val.ToFloat() );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( -val.ToInteger() );
							continue;
						default:
							common->Warning( " Tried to increment incompatible type %s", val.TypeOf( ) );
					}
					continue;
				}
				InlineWordCode( increment_i )
				InlineWordCode( increment )
				{
					auto& val = stack.A( );
					idSWFScriptVar result;
					switch( val.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( val.ToFloat() + 1.0f );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( val.ToInteger() + 1 );
							continue;
						default:
							common->Warning( " Tried to increment incompatible type %s", val.TypeOf( ) );
					}
					continue;
				}
				InlineWordCode( inclocal_i )
				InlineWordCode( inclocal )
				{
					auto& val = registers[bitstream.ReadEncodedU32()];
					idSWFScriptVar result;
					switch( val.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( val.ToFloat() + 1.0f );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( val.ToInteger() + 1 );
							continue;
						default:
							common->Warning( " Tried to increment incompatible type %s", val.TypeOf() );
					}
					continue;
				}
				InlineWordCode( decrement_i )
				InlineWordCode( decrement )
				{
					auto& val = stack.A();
					idSWFScriptVar result;
					switch( val.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( val.ToFloat() - 1.0f );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( val.ToInteger() + 1 );
							continue;
						default:
							common->Warning( " Tried to decrement incompatible type %s", val.TypeOf() );
					}
					continue;
				}
				InlineWordCode( declocal_i );
				InlineWordCode( declocal );
				{
					auto& val = registers[bitstream.ReadEncodedU32()];
					idSWFScriptVar result;
					switch( val.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							val.SetFloat( val.ToFloat() - 1.0f );
							continue;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							val.SetInteger( val.ToInteger() - 1 );
							continue;
						default:
							common->Warning( " Tried to decrement incompatible type %s", val.TypeOf() );
					}
					continue;
				}
				//ExecWordCode ( typeof );
				InlineWordCode( not )
				{
					stack.A().SetBool( !stack.A().ToBool() );
					continue;
				}
				//ExecWordCode ( bitnot );
				InlineWordCode( add_i )
				InlineWordCode( add )
				{
					auto& lH = stack.B();
					auto& rH = stack.A();
					idSWFScriptVar result;
					switch( lH.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_STRING:
							result.SetString( lH.ToString( ) + rH.ToString( ) );
							break;
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat( ) + rH.ToFloat( ) );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger( ) + rH.ToInteger( ) );
							break;
						case idSWFScriptVar::SWF_VAR_FUNCTION:
							result.SetString( lH.ToString( ) + rH.ToString( ) );
							break;
						default:
							common->Warning( " Tried to add incompatible types %s + %s", lH.TypeOf( ), rH.TypeOf( ) );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				InlineWordCode( subtract_i )
				InlineWordCode( subtract )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					idSWFScriptVar result;
					switch( lH.GetType( ) )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat( ) - rH.ToFloat( ) );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger( ) - rH.ToInteger( ) );
							break;
						default:
							common->Warning( " Tried to subtract incompatible types %s + %s", lH.TypeOf( ), rH.TypeOf( ) );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				InlineWordCode( multiply_i )
				InlineWordCode( multiply )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					idSWFScriptVar result;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat() * rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger() * rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to multiply incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				InlineWordCode( divide )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					idSWFScriptVar result;
					switch( lH.GetType() )
					{
						case idSWFScriptVar::SWF_VAR_FLOAT:
							result.SetFloat( lH.ToFloat() / rH.ToFloat() );
							break;
						case idSWFScriptVar::SWF_VAR_INTEGER:
							result.SetInteger( lH.ToInteger() / rH.ToInteger() );
							break;
						default:
							common->Warning( " Tried to divide incompatible types %s + %s", lH.TypeOf(), rH.TypeOf() );
					}

					stack.Pop( 2 );
					stack.Alloc() = result;
					continue;
				}
				//ExecWordCode ( modulo );
				//ExecWordCode ( lshift );
				//ExecWordCode ( rshift );
				//ExecWordCode ( urshift );
				//ExecWordCode ( bitand );
				//ExecWordCode ( bitor );
				//ExecWordCode ( bitxor );
				InlineWordCode( equals )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					stack.Pop( 2 );
					stack.Alloc() = lH.AbstractEquals( rH );
					continue;
				}
				InlineWordCode( strictequals )
				{
					auto& lH = stack.A();
					auto& rH = stack.B();
					stack.Pop( 2 );
					stack.Alloc() = lH.StrictEquals( rH );
					continue;
				}
				//ExecWordCode ( lessthan );
				//ExecWordCode ( lessequals );
				//ExecWordCode ( greaterthan );
				//ExecWordCode ( greaterequals );
				//ExecWordCode ( instanceof );
				//ExecWordCode ( istype );
				//ExecWordCode ( istypelate );
				//ExecWordCode ( in );
				InlineWordCode( getlocal0 )
				{
					stack.Alloc() = registers[0];
					continue;
				}
				InlineWordCode( getlocal1 )
				{
					stack.Alloc() = registers[1];
					continue;
				}
				InlineWordCode( getlocal2 )
				{
					stack.Alloc() = registers[2];
					continue;
				}
				InlineWordCode( getlocal3 )
				{
					stack.Alloc() = registers[3];
					continue;
				}
				InlineWordCode( setlocal0 )
				{
					registers[0] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( setlocal1 )
				{
					registers[1] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( setlocal2 )
				{
					registers[2] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( setlocal3 )
				{
					registers[3] = stack.A();
					stack.Pop( 1 );
					continue;
				}
				InlineWordCode( debug )
				{
					uint8 type = bitstream.ReadU8();
					uint32 index = bitstream.ReadEncodedU32();
					uint8 reg = bitstream.ReadU8();
					uint32 extra = bitstream.ReadEncodedU32();
					continue;
				}
				InlineWordCode( debugline )
				InlineWordCode( debugfile )
				{
					uint32 nr = bitstream.ReadEncodedU32();
					continue;
				}
			//ExecWordCode ( bkptline );
			//ExecWordCode ( timestamp );
			//ExecWordCode ( restargc );
			//ExecWordCode ( restarg );
			//ExecWordCode ( codes );
			default:
			{
				const AbcOpcodeInfo* info = &opcodeInfo[opCode];
				common->DWarning( "^5Unhandled Opcode %s\n", info ? info->name : "Empty" );
#ifdef _WIN32
				DebugBreak();
#endif
			}

		}
	}
	abcCallstackLevel--;
	return returnValue;
}
