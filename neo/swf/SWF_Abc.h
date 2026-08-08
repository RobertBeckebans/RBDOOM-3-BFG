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
#pragma once

#include "SWF_Enums.h"

struct swfMultiname
{
	swfConstantKind_t type;
	uint32 nameIndex;
	uint32 index;
	uint32 nameIndexT;  //second Multiname for types
	uint32 indexT;		//
};

struct swfConstant_pool_info
{
	idList<int32>				integers;		//u30 int_count			s32 integer[int_count]
	idList<uint32>				uIntegers;		//u30 uint_count		u32 uinteger[uint_count]
	idList<double>				doubles;		//u30 double_count		d64 double[double_count]
	idStrList					utf8Strings;	//u30 string_count		string_info string[string_count] (char is 16bit code point)
	idList<swfConstantKind_t>	namespaceKinds;	//u30 namespace_count	namespace_info namespace[namespace_count] (namespaceinfo_t.kind)
	idStrPtrList				namespaceNames; //u30 namespace_count	namespace_info namespace[namespace_count] (namespaceinfo_t.name)
	idList<idStrPtrList>		namespaceSets;	//u30 ns_set_count		ns_set_info ns_set[ns_set_count]
	idList<swfMultiname>		multinameInfos;	//u30 multiname_count	multiname_info multiname[multiname_count]
};

struct swfMetadata_info
{
	struct item   //item_info
	{
		idStrPtr	key;	//u30 key
		idStrPtr	value;	//u30 value
	};
	idStrPtr		name;	//u30 name
	idList<item>	items;	//u30 item_count item_info items[item_count]
};

struct swfOption_info
{
	struct item   //option_detail
	{
		uint32				val;	//u30 val
		swfConstantKind_t	kind;	//u8 kind
	};
	uint32 option_count;	//u30 option_count
	idList<item> options;	//option_detail option[option_count]
};

struct swfMethod_body_info;
struct swfMethod_info
{
	//zero before use.
	enum Flags
	{
		NEED_ARGUMENTS	= 0x01, // Suggests to the run - time that an “arguments” object( as specified by the ActionScript 3.0 Language Reference ) be created.Must not be used together with NEED_REST.See Chapter 3.
		NEED_ACTIVATION = 0x02, // Must be set if this method uses the newactivation opcode.
		NEED_REST		= 0x04, // This flag creates an ActionScript 3.0 rest arguments array.Must not be used with NEED_ARGUMENTS.See Chapter 3.
		HAS_OPTIONAL	= 0x08, // Must be set if this method has optional parameters andthe options field is present in this method_info structure.
		IGNORE_REST		= 0x10,
		NATIVE			= 0x20,
		SET_DXNS		= 0x40, // Must be set if this method uses the dxns or dxnslate opcodes.
		HAS_PARAM_NAMES	= 0x80, // Must be set when the param_names field is present in this method_info structure.
	};

	uint32					paramCount;		//u30 param_count
	swfMultiname*			returnType;		//u30 return_type
	idList<swfMultiname*>	paramTypes;		//u30 param_count //u30 param_type[param_count]
	idStrPtr				name;			//u30 name
	uint8					flags;			//u8 flags
	swfOption_info			options;		//option_info options
	idStrPtrList			paramNames;		// ( param_info )  param_names	u30 param_name[param_count]

	swfMethod_body_info* 	body = nullptr;
};

struct swfTraits_info
{
	enum Attrib   // upper 4 bits of kind
	{
		Final		= 0x1,	// Is used with Trait_Method, Trait_Getter andTrait_Setter.It marks a method that cannot be overridden by a sub - class
		Override	= 0x2, // Is used with Trait_Method, Trait_Getter andTrait_Setter.It marks a method that has been overridden in this class
		Metadata	= 0x4, // Is used to signal that the fields metadata_count and metadata follow the data field in the traits_info entry
	};
	enum Type
	{
		Trait_Slot		= 0,
		Trait_Method	= 1,
		Trait_Getter	= 2,
		Trait_Setter	= 3,
		Trait_Class		= 4,
		Trait_Function  = 5,
		Trait_Const		= 6,
		Trait_Count		= Trait_Const + 1,
		Trait_Mask		= 15
	};
	swfMultiname*				name;		//u30 name
	uint8						kind;		//u8 kind
	void*						data;		//u8 data[]
	idList<swfMetadata_info*>	metadatas;	//u30 metadata_count	u30 metadata[metadata_count]
};

struct swfInstance_info
{
	swfMultiname*			name;			//u30 name
	swfMultiname*			super_name;		//u30 super_name
	swfInstanceFlags_t		flags;			//u8 flags
	uint32					protectedNs;	//u30 protectedNs
	idList<swfMultiname*>	interfaces;		//u30 intrf_count	u30 interface[intrf_count]
	swfMethod_info*			iinit;			//u30 iinit
	idList<swfTraits_info>	traits;			//u30 trait_count	traits_info trait[trait_count]
};

struct swfTrait_slot
{
	uint32			slot_id;	//u30 slot_id
	swfMultiname*	type_name;	//u30 type_name
	uint32			vindex;		//u30 vindex
	uint8			vkind;		//u8 vkind
};

struct swfClass_info
{
	swfMethod_info*			cinit;	//u30 cinit
	idList<swfTraits_info>	traits; //u30 trait_count traits_info traits[trait_count]
};

struct swfTrait_class
{
	uint32			slot_id;	//u30 slot_id
	swfClass_info*	classi;		//u30 classi
};

struct swfTrait_function
{
	uint32				slot_id;	//u30 slot_id
	swfMethod_info* 	func;		//u30 function
};

struct swfTrait_method
{
	uint32				disp_id;	//u30 disp_id
	swfMethod_info* 	method;		//u30 method
};

struct swfScript_info
{
	swfMethod_info*			init;	//u30 init
	idList<swfTraits_info>	traits; //u30 trait_count traits_info traits[trait_count]
};

struct swfException_info
{
	uint32		from;		//u30 from
	uint32		to;			//u30 to
	uint32		target;		//u30 target
	idStrPtr	exc_type;	//u30 exc_type
	idStrPtr	var_name;	//u30 var_name
};

struct swfMethod_body_info
{
	swfMethod_info* 			method;			//u30 method
	uint32						max_stack;		//u30 max_stack
	uint32						localCount;		//u30 local_count
	uint32						initScopeDepth;	//u30 init_scope_depth
	uint32						maxScopeDepth;	//u30 max_scope_depth
	uint32						codeLength;		//u30 code_length
	idSWFBitStream				code;			//u8 code[code_length]
	idList<swfException_info>	exceptions;		//u30 exception_count exception_info exception[exception_count]
	idList<swfTraits_info>		traits;			//u30 trait_count traits_info traits[trait_count]
};

class idSWFScriptObject;
struct SWF_AbcFile
{
	template<class T>
	T* GetTrait( const swfTraits_info& trait, idSWFScriptObject* globals = nullptr );

	void ReadMethodInfo( idSWFBitStream& bitstream, swfMethod_info& newMethod );
	void ReadOptionInfo( idSWFBitStream& bitstream, swfOption_info& newOption );
	void ReadMetaDataInfo( idSWFBitStream& bitstream, swfMetadata_info& newMetadata );
	void ReadInstanceInfo( idSWFBitStream& bitstream, swfInstance_info& newInstancedata );
	void ReadTraitData( idSWFBitStream& bitstream, swfTraits_info& newTraitsData );
	void ReadTraitsInfo( idSWFBitStream& bitstream, swfTraits_info& newTraitsData );
	void ReadClassInfo( idSWFBitStream& bitstream, swfClass_info& newClassData );
	void ReadScriptInfo( idSWFBitStream& bitstream, swfScript_info& newScriptData );
	void ReadMethodBodyInfo( idSWFBitStream& bitstream, swfMethod_body_info& newMethodBody );
	void ReadExceptionInfo( idSWFBitStream& bitstream, swfException_info& newException );

	static idStr asString( swfConstantKind_t kind, swfConstant_pool_info& constant_pool );
	static idStr asString( swfMultiname* mn, swfConstant_pool_info& constant_pool, bool prefix = true );
	static void traceMN( const char* name, swfMultiname* mn, swfConstant_pool_info& constant_pool );
	static void traceConstantPool( swfConstant_pool_info& constant_pool );

	uint16						minor_version;
	uint16						major_version;
	swfConstant_pool_info		constant_pool;
	idList<swfMethod_info>		methods;		//u30 method_count		method_info method[method_count]
	idList<swfMetadata_info>	metadatas;		//u30 metadata_count	metadata_info metadata[metadata_count]
	uint32						class_count;	//u30 class_count
	idList<swfInstance_info>	instances;		//instance_info instance[class_count]
	idList<swfClass_info>		classes;		//class_info class[class_count]
	idList<swfScript_info>		scripts;		//u30 script_count script_info script[script_count]
	idList<swfMethod_body_info>	method_bodies;	//u30 method_body_count method_body_info method_body[method_body_count]

	//Writing the whole bytestream as whole for now, but :
	//	debug info plus unused tags and bytecode should be removed, constant pools need to stay the same
	//	accessibility can also be removed.
	idSWFBitStream				AbcTagData;
	void WriteBinary( idFileLocal& file );
	void LoadBinary( idFile* file );
};

struct SWF_SymbolClass
{
	struct Item
	{
		uint16 tag;
		idStr name;
	};
	idList<Item> symbols;
};

enum SWFAbcOpcode
{
#define ABC_OP(operandCount, canThrow, stack, internalOnly, nameToken)        OP_##nameToken,
#define ABC_UNUSED_OP(operandCount, canThrow, stack, internalOnly, nameToken) ABC_OP(operandCount, canThrow, stack, internalOnly, nameToken)
#include "opcodes.tbl"
#undef ABC_OP
#undef ABC_UNUSED_OP

	//-----
	OP_end_of_op_codes
};

struct AbcOpcodeInfo
{
	int8_t operandCount;    // uses -1 for "invalid", we can avoid that if necessary
	int8_t canThrow;        // always 0 or 1
	int8_t stack;           // stack movement not taking into account run-time names or function arguments
	uint16_t wordCode;      // a map used during translation
	const char* name;       // instruction name or OP_0xNN for undefined instructions #IFDEF DEBUGGER
};

extern const AbcOpcodeInfo opcodeInfo[];
extern const unsigned char kindToPushOp[];
