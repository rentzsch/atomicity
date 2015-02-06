/****************************************************************************************
	atomicity.c $Revision: 74 $
		<http://redshed.net/atomicity>
	
	Copyright © 1998-2002 Red Shed Software. All rights reserved.
	by Jonathan 'Wolf' Rentzsch (jon at redshed dot net)
	
	This code requires a 68020 or later or any PowerPC. No habla 68000 or x86.
	
	************************************************************************************/

#include	"atomicity.h"

/**************************
*	
*	Requirements
*	
**************************/
#pragma mark	(Requirements)

#include	"require.h"	//	Comment out this line to disable requirement checking.
#ifndef	GenerateRequirements
	#define	Require( CODE )
	#define	RequireIf( TEST, CODE )
	#define	RequirePtr( PTR )
	#define	RequirePtrAlign( PTR, ALIGN )
	#define	RequirePtrAlignIfNotNil( PTR, ALIGN )
	#define	RequirePtrIfNotNil( PTR )
	#define	RequirePtrAlignIf( TEST, PTR, ALIGN )
#endif
#define	RequireOffset( OFFSET )	Require( OFFSET < 1024 )

/**************************
*	
*	Conditional Compilation Flags
*	
**************************/
#pragma mark	-
#pragma mark	(Conditional Compilation Flags)

#if	0	//	Set to 1 if you're not using Universal Interfaces 3 or later.
	#define	TARGET_CPU_PPC		1
	#define	TARGET_CPU_68K		0
	#define	TARGET_RT_MAC_CFM	0
	
	/*	Psuedocode explaining the above conditional compilation flags.
		You'll need to set these flags manually if you're not using
		Universal Interfaces 3 or later.
		
	if( TARGET_CPU_PPC == 1 ) {
		generate PowerPC code;
	} else if( TARGET_CPU_68K == 1 ) {
		if( TARGET_RT_MAC_CFM == 1 ) {
			generate CFM68K code;
		} else {
			generate Classic 68K code;
		}
	}*/
#endif

/**************************
*	
*	Types
*	
**************************/
#pragma mark	-
#pragma mark	(Types)

typedef	struct	{
	char		padding[ kAtomicAlignment ];
	AtomicStack	stack;
}	AlignedAtomicStack;

/**************************
*	
*	Funky Protos
*	
**************************/
#pragma mark	-
#pragma mark	(Funky Protos)

#if		defined(DontUseOpenTransport)
#elif	defined(OnlyUseOpenTransport)
#else	//	UseOpenTransportIfAvailable
	Boolean
IsOpenTransportAvailable();
#endif

	AtomicStack*
AlignAtomicStack(
	AlignedAtomicStack	*stack );

	void
AddOffset(
	void	**ptr,
	size_t	offset );

	void
SubOffset(
	void	**ptr,
	size_t	offset );

/****************************************************************************************
	Some of the functions below are written in assembly language to access atomic
	instructions inaccessable from a high-level language like C. We want to compile to
	differing platforms from a single source file, so we use conditional #defines to
	generate 68K or PowerPC code depending on what the compiler is generating.
	
	In addition to conditionally generating 68K or PowerPC assembly, when generating 68K
	code, we conditionally generate Classic 68K or CFM-68K code.
	
	Classic 68K:
		Classic 68K doesn't define a standard parameter passing convention, so we use
		the Pascal calling convention. We use the 'pascal' keyword so the compiler
		generates code that uses the Pascal calling convention.
		
		The Pascal calling convention requires the caller make space for the callee's
		result (if any) and all parameters. The parameters are pushed from left to right,
		followed by the return address. The callee is responsible for popping all
		paramaters before returning to the caller. The function result (if any) is left
		on the stack.
		
		The Pascal calling convention doesn't explictly state what registers should be
		saved, so we use the rather strict CFM-68K register preservation guideline, which
		requires we preserve registers d3-d7 and a2-a7. That leaves us with d0, d1, d2
		and a0, a1.
	CFM-68K:
		CFM-68K defines a standard parameter passing convention, so the 'pascal'
		keyword (required when generating Classic 68K code) is ignored.
		
		Under CFM-68K, parameters are pushed from right to left onto the stack,
		followed by the return address. The function's result (if any) is returned
		in register D0. CFM-68K requires we preserve registers d3-d7 and a2-a7. That
		leaves us with d0, d1, d2 and a0, a1.
	PowerPC:
		PowerPC code always uses CFM parameter passing conventions, so the 'pascal'
		keyword (required when generating Classic 68K code) is ignored.
		
		Under PowerPC, parameters are passed in registers left to right, starting with
		register r3. The function's result (if any) is stored in register r3.
	
	************************************************************************************/

/****************************************************************************************
*	
*	Atomic Basis Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Basis Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Sep 15, 1999	Created.
	
	************************************************************************************/

	asm
	pascal
	long
MyAtomicStore(
	long	/*oldValue*/,
	long	/*newValue*/,
	void	*/*address*/ )
{
#if		TARGET_CPU_PPC
retry:
	lwarx	r6, 0, r5		//	currentValue = *address;
	cmpw	r6, r3			//	if( currentValue != oldValue )
	bne		fail			//		goto fail;
	sync					//	Sync with pending stores.
	stwcx.	r4, 0, r5		//	if( reservation == address ) *address = newValue;
	bne-	retry			//	else goto retry;
	sync					//	Broadcast store.
	li		r3, 1			//	Return true.
	blr						//	We're outta here.
fail:
	sync					//	Sync with pending stores.
	stwcx.	r6, 0, r5		//	Clear reservation.
	li		r3, 0			//	Return false.
	blr						//	We're outta here.
#elif	TARGET_CPU_68K
	#if	TARGET_RT_MAC_CFM
		movea.l	(a7)+, a0		//	Pop return address.
		move.l	(a7)+, d0		//	Pop oldValue param.
		move.l	(a7)+, d1		//	Pop newValue param.
		movea.l	(a7)+, a1		//	Pop address param.
		dc.l	0x0ED10040		//	CWPro5 doesn't know cas. Here's the raw opcode.
	//	cas.l	d0, d1, (a1)	//	if( *address == oldValue ) *address = newValue;
		bne.s	fail			//	else goto fail.
		moveq.l	#1, d0			//	Return true in register d0.
		jmp		(a0)			//	We're outta here.
	fail:
		moveq.l	#0, d0			//	Return false in register d0.
		jmp		(a0)			//	We're outta here.
	#else
		movea.l	(a7)+, a0		//	Pop return address.
		movea.l	(a7)+, a1		//	Pop address param.
		move.l	(a7)+, d0		//	Pop newValue param.
		move.l	(a7)+, d1		//	Pop oldValue param.
		dc.l	0x0ED10001		//	CWPro5 doesn't know cas. Here's the raw opcode.
	//	cas.l	d1, d0, (a1)	//	If the *address == oldValue, *address = newValue.
		bne.s	fail			//	Else fail.
		move.l	#1, (a7)		//	Return true.
		jmp		(a0)			//	We're outta here.
	fail:
		move.l	#0, (a7)		//	Return false.
		jmp		(a0)			//	We're outta here.
	#endif
#else
	#error	atomicity doesnt support this processor
#endif
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	pascal
	long
MyAtomicStorePtr(
	void	*oldValue,
	void	*newValue,
	void	*address )
{
	RequirePtrIfNotNil( oldValue );
	RequirePtrIfNotNil( newValue );
	RequirePtrAlign( address, kAtomicAlignment );
	
	return( MyAtomicStore( (long) oldValue, (long) newValue, address ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	asm
	pascal
	long
AtomicStore2(
	long	/*oldValue*/,
	long	/*newValue*/,
	void	*/*address*/,
	long	/*oldValue2*/,
	void	*/*address2*/ )
{
#if		TARGET_CPU_PPC
retry:
	lwarx	r8, 0, r5		//	currentValue = *address;
	cmpw	r8, r3			//	if( currentValue != oldValue )
	bne		fail			//		goto fail;
	lwz		r9, 0(r7)		//	currentValue2 = *address2;
	cmpw	r9, r6			//	if( currentValue2 != oldValue2 )
	bne		fail			//		goto fail;
	sync					//	Sync with pending stores.
	stwcx.	r4, 0, r5		//	if( reservation == address ) *address = newValue;
	bne-	retry			//	else goto retry;
	sync					//	Broadcast store.
	li		r3, 1			//	Return true.
	blr						//	We're outta here.
fail:
	sync					//	Sync with pending stores.
	stwcx.	r8, 0, r5		//	Clear the reservation.
	li		r3, 0			//	Return false.
	blr						//	We're outta here.
#elif	TARGET_CPU_68K
	#if	TARGET_RT_MAC_CFM
		move.l	4(a7), d0				//	Load oldValue param.
		move.l	8(a7), d1				//	Load newValue param.
		movea.l	12(a7), a0				//	Load address param.
		move.l	16(a7), d2				//	Load oldValue2 param.
		movea.l	20(a7), a1				//	Load address2 param.
		dc.l	0x0EFC8040				//	CWPro5 doesn't know
		dc.w	0x9082					//	cas2. Here's the raw opcodes.
	//	cas2.l	d0:d2, d1:d2, (a0):(a1)	//	if( *a0 == d0 && *a1 == d2 ) {
										//		*a0 = d1; *a1 = d2 /*nop*/;
		bne.s	fail					//	} else fail;
		move.l	#1, d0					//	Set result to true.
		bra.s	done
	fail:
		move.l	#0, d0					//	Set result to false.
	done:
		movea.l	(a7)+, a0				//	Pop return address.
		add.l	#20, a7					//	Pop all parameters.
		jmp		(a0)					//	We're outta here.
	#else
		move.l	20(a7), d0				//	Load oldValue param.
		move.l	16(a7), d1				//	Load newValue param.
		movea.l	12(a7), a0				//	Load address param.
		move.l	8(a7), d2				//	Load oldValue2 param.
		movea.l	4(a7), a1				//	Load address2 param.
		dc.l	0x0EFC8040				//	CWPro5 doesn't know
		dc.w	0x9082					//	cas2. Here's the raw opcodes.
	//	cas2.l	d0:d2, d1:d2, (a0):(a1)	//	if( *a0 == d0 && *a1 == d2 ) {
										//		*a0 = d1; *a1 = d2 /*nop*/;
		bne.s	fail					//	} else fail;
		move.l	#1, d0					//	Set result to true.
		bra.s	done
	fail:
		move.l	#0, d0					//	Set result to false.
	done:
		movea.l	(a7)+, a0				//	Pop return address.
		add.l	#20, a7					//	Pop all parameters.
		move.l	d0, (a7)				//	Return the result.
		jmp		(a0)					//	We're outta here.
	#endif
#else
	#error	atomicity doesnt support this processor
#endif
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	pascal
	long
AtomicStore2Ptr(
	void	*oldValue,
	void	*newValue,
	void	*address,
	void	*oldValue2,
	void	*address2 )
{
	RequirePtrIfNotNil( oldValue );
	RequirePtrIfNotNil( newValue );
	RequirePtrAlign( address, kAtomicAlignment );
	RequirePtrIfNotNil( oldValue2 );
	RequirePtrAlign( address2, kAtomicAlignment );
	
	return( AtomicStore2( (long) oldValue, (long) newValue, address,
								(long) oldValue2, address2 ) );
}

/****************************************************************************************
*	
*	Atomic Stack Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Stack Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Oct 26, 1998	Created.
	wolf		Mon, Nov 9, 1998	PowerPC code rolled in.
	wolf		Tue, Mar 23, 1999	Rewrote PowerPC code. After more study of the
									programming model, I learned I need not save and
									restore the Link Register (I don't modify it) and
									the Condition Register (the cr0 field is marked
									volatile). This saves 8 instructions including
									two loads and two stores.
									However, I now use a more complex double-load-with-
									sync technique to eliminate the possibility of
									livelock in a multiprocessor environment. The
									overhead of the sync instruction probably negates
									the speed gain of omitting the 8 instructions.
	wolf		Thu, Apr 1, 1999	Rewrote PowerPC code to be a tad more efficient.
	wolf		Fri, Sep 10, 1999	Rewrote in C.
	
	************************************************************************************/

	void
MyPushAtomicStack(
	AtomicElement	*element,
	AtomicStack		*stack )
{
	AtomicElement	*next;
	long			stored;
	
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( stack, kAtomicAlignment );
	
	do {
		next = (AtomicElement*) stack->next;
		element->next = next;
		stored = AtomicStorePtr( next, element, stack );
	} while( !stored );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Oct 26, 1998	Created.
	wolf		Mon, Nov 9, 1998	PowerPC code rolled in.
	wolf		Wed, Nov 25, 1998	Fixed function result bug in Classic 68K code.
									I was returning the result in the space allocated
									for the stack parameter, not the result space right
									above it.
	wolf		Tue, Mar 23, 1999	Rewrote PowerPC code. After more study of the
									programming model, I learned I need not save and
									restore the Link Register (I don't modify it) and
									the Condition Register (the cr0 field is marked
									volatile). This saves 8 instructions including
									two loads and two stores.
	wolf		Thu, Apr 1, 1999	I moved the test for a nil stack out of the
									reservation code, which should help performance.
	wolf		Fri, Sep 10, 1999	Rewrote in C.
	
	************************************************************************************/

	AtomicElement*
MyPopAtomicStack(
	AtomicStack		*stack )
{
	AtomicElement	*next, *element;
	long			stored;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	
	do {
		element = (AtomicElement*) stack->next;
		RequirePtrAlignIfNotNil( element, kAtomicAlignment );
		if( element ) {
			next = (AtomicElement*) element->next;
			stored = AtomicStorePtr( element, next, stack );
		} else {
			stored = true;
		}
	} while( !stored );
	
	return( (AtomicElement*) element );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Oct 26, 1998	Created.
	wolf		Mon, Nov 9, 1998	PowerPC code rolled in.
	wolf		Wed, Nov 25, 1998	Fixed function result bug in Classic 68K code.
									I was returning the result in the space allocated
									for the stack parameter, not the result space right
									above it.
	wolf		Thu, Mar 4, 1999	On the PowerPC side, I mistakenly moved r4 (which
									is always set to nil) into the result register, r3.
									This resulted in StealAtomicStack() always returning
									nil when executing PowerPC. Now I correctly move r5
									into r3.
									It's very strange that I didn't catch this obvious
									error in my test matrix. Perhaps I forgot to test the
									PowerPC side and only tested the 68K sides.
	wolf		Thu, Apr 1, 1999	I moved the test for a nil stack out of the
									reservation code, which should help performance.
	wolf		Fri, Sep 10, 1999	Rewrote in C.
	wolf		Mon, Nov 1, 1999	Modified to take two parameters instead of one. This
									is conceptually clearer.
	
	************************************************************************************/

	void 
StealAtomicStack(
	AtomicStack		*fromStack,
	AtomicStack		*toStack )
{
	AtomicElement	*result;
	long			stored;
	
	RequirePtrAlign( fromStack, kAtomicAlignment );
	RequirePtrAlign( toStack, kAtomicAlignment );
	
	do {
		result = (AtomicElement*) fromStack->next;
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
		stored = AtomicStorePtr( result, nil, fromStack );
	} while( !stored );
	
	toStack->next = result;
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Apr 30, 1999	Created.
	
	************************************************************************************/

	AtomicElement*
PeekAtomicStack(
	AtomicStack		*stack )
{
	AtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	
	result = (AtomicElement*) stack->next;
	RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	return( result );
}

/****************************************************************************************
*	
*	Atomic Stack Offset Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Stack Offset Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	void
PushAtomicStackOff(
	void			*element,
	AtomicStack		*stack,
	size_t			offset )
{
	RequirePtr( element );
	RequireOffset( offset );
	RequirePtrAlign( stack, kAtomicAlignment );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	PushAtomicStack( (AtomicElement*) element, stack );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	AtomicElement*
PopAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset )
{
	AtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PopAtomicStack( stack );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	SubOffset( (void**) &result, offset );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Apr 30, 1999	Created.
	
	************************************************************************************/

	AtomicElement*
PeekAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset )
{
	AtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PeekAtomicStack( stack );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
*	
*	Atomic Lock Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Lock Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Mar 31, 1999	Created.
	wolf		Thu, Apr 1, 1999	I moved the test for a nonzero lock out of the
									reservation code, which should help performance.
	wolf		Fri, Sep 10, 1999	Rewrote in C.
	
	************************************************************************************/

	long
MyGrabAtomicLock(
	AtomicLock	*lock )
{
	RequirePtrAlign( (void*) lock, kAtomicAlignment );
	
	return( AtomicStore( 0, 1, (void*) lock ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Mar 23, 1999	Created.
	
	************************************************************************************/

	void
ReleaseAtomicLock(
	AtomicLock	*lock )
{
	RequirePtrAlign( (void*) lock, kAtomicAlignment );
	Require( *lock );
	
	*lock = 0;
}


/****************************************************************************************
*	
*	Atomic Flags Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Flags Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Apr 7, 2000	Created.
	
	************************************************************************************/

	Boolean
GetAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag )
{
	RequirePtrAlign( (void*) flags, kAtomicAlignment );
	
	return( ( *flags & flag ) == flag );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Apr 7, 2000	Created.
	wolf		Sat, Jul 1, 2000	Added explicit casts to long.
	
	************************************************************************************/

	Boolean
SetAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag )
{
	AtomicFlags	flagsCopy;
	long		stored;
	Boolean		wasClear;
	
	RequirePtrAlign( (void*) flags, kAtomicAlignment );
	
	do {
		flagsCopy = *flags;
		wasClear = (flagsCopy & flag) == 0;
		stored = AtomicStore( (long) flagsCopy, (long) (flagsCopy | flag), (void*) flags );
	} while( !stored );
	
	return( wasClear );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Apr 7, 2000	Created.
	wolf		Sat, Jul 1, 2000	Added explicit casts to long.
	
	************************************************************************************/

	Boolean
ClearAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag )
{
	AtomicFlags	flagsCopy;
	long		stored;
	Boolean		wasSet;
	
	RequirePtrAlign( (void*) flags, kAtomicAlignment );
	
	do {
		flagsCopy = *flags;
		wasSet = (flagsCopy & flag) == flag;
		stored = AtomicStore( (long) flagsCopy, (long) (flagsCopy & (~flag)), (void*) flags );
	} while( !stored );
	
	return( wasSet );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Apr 7, 2000	Created.
	
	************************************************************************************/

	Boolean
FlipAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag,
	Boolean		value )
{
	Boolean	result;
	
	RequirePtrAlign( (void*) flags, kAtomicAlignment );
	Require( value == true || value == false );
	
	if( value )
		result = SetAtomicFlag( flags, flag );
	else
		result = ClearAtomicFlag( flags, flag );
	
	return( result );
}

/****************************************************************************************
*	
*	Open Transport Compatibility Layer
*
*	The the following functions (PushAtomicStack, PopAtomicStack,
*	IsOpenTransportAvailable, OpenAtomicity, CloseAtomicity and UseOTAtomicity) are only
*	compiled if UseOpenTransportIfAvailable is defined. Otherwise zero-runtime-overhead
*	macros are used.
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Open Transport Compatibility Layer)

#if		defined(DontUseOpenTransport)
#elif	defined(OnlyUseOpenTransport)
#else	//	UseOpenTransportIfAvailable

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
AtomicStore(
	long	oldValue,
	long	newValue,
	void	*address )
{
	long	result;
	
	RequirePtrAlign( address, kAtomicAlignment );
	
	if( IsOpenTransportAvailable() )
		result = OTCompareAndSwap32( (UInt32) oldValue, (UInt32) newValue, (UInt32*) address );
	else
		result = MyAtomicStore( oldValue, newValue, address );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
AtomicStorePtr(
	void	*oldValue,
	void	*newValue,
	void	*address )
{
	long	result;
	
	RequirePtrIfNotNil( oldValue );
	RequirePtrIfNotNil( newValue );
	RequirePtrAlign( address, kAtomicAlignment );
	
	if( IsOpenTransportAvailable() )
		result = OTCompareAndSwapPtr( oldValue, newValue, (void**) address );
	else
		result = MyAtomicStorePtr( oldValue, newValue, address );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Mar 29, 1999	Created.
	
	************************************************************************************/

	void
PushAtomicStack(
	AtomicElement	*element,
	AtomicStack		*stack )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( stack, kAtomicAlignment );
	
	if( IsOpenTransportAvailable() )
		OTLIFOEnqueue( (OTLIFO*) stack, (OTLink*) element );
	else
		MyPushAtomicStack( element, stack );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Mar 29, 1999	Created.
	
	************************************************************************************/

	AtomicElement*
PopAtomicStack(
	AtomicStack	*stack )
{
	AtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	
	if( IsOpenTransportAvailable() )
		result = (AtomicElement*) OTLIFODequeue( (OTLIFO*) stack );
	else
		result = MyPopAtomicStack( stack );
	
	RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Mar 31, 1999	Created.
	
	************************************************************************************/

	Boolean
GrabAtomicLock(
	AtomicLock	*lock )
{
	Boolean	result;
	
	RequirePtrAlign( (void*) lock, kAtomicAlignment );
	
	if( IsOpenTransportAvailable() )
		result = OTCompareAndSwap32( 0, 1, (unsigned long*) lock );
	else
		result = MyGrabAtomicLock( lock ) == true;
	
	return( result );
}

Boolean	gHasOT = (Boolean) -1;

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Mar 29, 1999	Created.
	
	************************************************************************************/

	Boolean
IsOpenTransportAvailable()
{
	long	ignored;
	
	if( gHasOT == -1 ) {
		if( !Gestalt( gestaltOpenTpt, &ignored ) && OTLIFOEnqueue != nil )
			gHasOT = true;
		else
			gHasOT = false;
	}
	
	return( gHasOT );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Aug 23, 1999	Created.
	wolf		Sun, Jan 30, 2000	Carbonized.
	
	************************************************************************************/

	OSStatus
#if	TARGET_API_MAC_CARBON
OpenAtomicity(
	OTInitializationFlags 	flags,
	OTClientContextPtr *	outClientContext )
#else
OpenAtomicity()
#endif
{
	OSStatus	err;
	
#if	TARGET_API_MAC_CARBON
	err = InitOpenTransportInContext( flags, outClientContext );
#else
	err = InitOpenTransport();
#endif
	
	if( err ) {
		gHasOT = false;
	} else {
		if( OTLIFOEnqueue == nil )
			gHasOT = false;
		else
			gHasOT = true;
	}
	
	return( err );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Aug 23, 1999	Created.
	wolf		Sun, Jan 30, 2000	Carbonized.
	
	************************************************************************************/

	void
#if	TARGET_API_MAC_CARBON
CloseAtomicity(
	OTClientContextPtr	clientContext )
#else
CloseAtomicity()
#endif
{
	if( gHasOT == true ) {
#if	TARGET_API_MAC_CARBON
		CloseOpenTransportInContext( clientContext );
#else
		CloseOpenTransport();
#endif
	}
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Aug 24, 1999	Created.
	
	************************************************************************************/

	void
UseOTAtomicity(
	Boolean	value )
{
	gHasOT = value;
}

#endif	//	UseOpenTransportIfAvailable

/****************************************************************************************
*	
*	Guarded Atomic Stack Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Stack Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Sep 28, 1999	Created.
	
	************************************************************************************/

	long
GrabAtomicElement(
	GuardedAtomicElement	*element,
	void					*list )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( (void*) element->list, kAtomicAlignment );
	RequirePtrAlignIfNotNil( list, kAtomicAlignment );
	
	return( AtomicStorePtr( nil, list, &element->list ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Sep 28, 1999	Created.
	
	************************************************************************************/

	void
ReleaseAtomicElement(
	GuardedAtomicElement	*element )
{
	RequirePtrAlign( element, kAtomicAlignment );
	
	element->list = nil;
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Nov 23, 1998	Created.
	wolf		Mon, Nov 30, 1998	Broke out element count incrementing code into its
									own atomic function: IncrementGuardedAtomicStack().
									It takes three instructions to increment a value in
									memory on a PowerPC, so we need to use the special
									syncronization instructions offered by the PowerPC.
	wolf		Wed, Mar 31, 1999	Migrated to Atomic Locks.
	wolf		Sat, Jul 1, 2000	Changed result type from Boolean to long to placate
									compiler.
	
	************************************************************************************/

	long
PushGuardedAtomicStack(
	GuardedAtomicElement	*element,
	AtomicStack				*stack )
{
	long	result;
	
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( stack, kAtomicAlignment );
	
	result = GrabAtomicElement( element, stack );
	if( result )
		PushAtomicStack( (AtomicElement*) element, stack );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Nov 23, 1998	Created.
	wolf		Wed, Mar 31, 1999	Migrated to Atomic Locks.
	wolf		Mon, Sep 13, 1999	Added requirement that a freshly popped element's
									lock should have been set before we clear it.
	
	************************************************************************************/

	GuardedAtomicElement*
PopGuardedAtomicStack(
	AtomicStack				*stack )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	
	result = (GuardedAtomicElement*) PopAtomicStack( stack );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );

	if( result ) {
		Require( result->list == stack );
		ReleaseAtomicElement( result );
	}
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Nov 2, 1999	Created.
	
	************************************************************************************/

	void
StealGuardedAtomicStack(
	AtomicStack		*fromStack,
	AtomicStack		*toStack )
{
	GuardedAtomicElement	*result;
	long					stored;
	
	RequirePtrAlign( fromStack, kAtomicAlignment );
	RequirePtrAlign( toStack, kAtomicAlignment );
	
	do {
		result = (GuardedAtomicElement*) fromStack->next;
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
		stored = AtomicStorePtr( result, nil, fromStack );
	} while( !stored );
	
	toStack->next = (AtomicElement*) result;
	
	while( result ) {
		result->list = toStack;
		result = (GuardedAtomicElement*) result->next;
	}
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, May 3, 1999	Created.
	wolf		Mon, Sep 13, 1999	Added the requirement that if a element is
									successfully peeked at, its lock must be set.
	
	************************************************************************************/

	GuardedAtomicElement*
PeekGuardedAtomicStack(
	AtomicStack		*stack )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	
	result = (GuardedAtomicElement*) stack->next;
		RequireIf( result, result->list == stack );
	
	return( result );
}

/****************************************************************************************
*	
*	Guarded Atomic Stack Offset Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Stack Offset Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	long
PushGuardedAtomicStackOff(
	void			*element,
	AtomicStack		*stack,
	size_t			offset )
{
	long	result;
	
	RequirePtr( element );
	RequirePtrAlign( stack, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
		RequirePtrAlignIfNotNil( (void*)((GuardedAtomicElement*) element)->list, kAtomicAlignment );
	result = PushGuardedAtomicStack( (GuardedAtomicElement*) element, stack );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == stack );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	GuardedAtomicElement*
PopGuardedAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PopGuardedAtomicStack( stack );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
		RequireIf( result, ((GuardedAtomicElement*) result)->list == nil );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, May 3, 1999	Created.
	
	************************************************************************************/

	GuardedAtomicElement*
PeekGuardedAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( stack, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PeekGuardedAtomicStack( stack );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
		RequireIf( result, ((GuardedAtomicElement*) result)->list == stack );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
*	
*	Atomic Queue Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Queue Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 18, 1998	Created.
	
	An atomic queue is simply a queue that uses an atomic stack as its input list.
	
	************************************************************************************/

	void
PushAtomicQueue(
	AtomicElement	*element,
	AtomicQueue		*queue )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( queue, kAtomicAlignment );
	
	PushAtomicStack( element, &queue->input );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Mar 16, 2000	Created.
	
	************************************************************************************/

	void
PushFrontAtomicQueue(
	AtomicElement	*element,
	AtomicQueue		*queue )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( queue, kAtomicAlignment );
	
	PushAtomicStack( element, &queue->output );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 18, 1998	Created.
	wolf		Thu, Jan 20, 2000	stolenStack is now explicitly aligned.
	
	When the client wants the next element in the queue, we try to pop the element
	off the output stack. If the output stack is empty, then we have to refill the output
	stack from the input stack.
	
	We do this by atomically "stealing" the entire stack. Once we have the stack, we
	"reverse" the stack (make a first-in-last-out list into a first-in-first-out list) by
	popping each element from the stolen stack and pushing it onto the output stack.
	
	If all this stealing, popping and pushing sounds expensive, it isn't. The reversal
	operates in linear time, which is acceptable. However, if you don't need the
	sequential access of a queue, by all means use a stack, as it operates in fast
	constant time.
	
	************************************************************************************/

	AtomicElement*
PopAtomicQueue(
	AtomicQueue			*queue )
{
	AlignedAtomicStack	alignedStolenStack;
	AtomicStack			*stolenStack;
	AtomicElement		*current;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	
	current = PopAtomicStack( &queue->output );
	RequirePtrAlignIfNotNil( current, kAtomicAlignment );
	
	stolenStack = AlignAtomicStack( &alignedStolenStack );
	RequirePtrAlign( stolenStack, kAtomicAlignment );
	
	if( current == nil ) {
		//	Nothing to pop. Refill the queue from the input stack.
		StealAtomicStack( &queue->input, stolenStack );
		current = PopAtomicStack( stolenStack );
		while( current ) {
			RequirePtrAlign( current, kAtomicAlignment );
			PushAtomicStack( current, &queue->output );
			current = PopAtomicStack( stolenStack );
		}
		current = PopAtomicStack( &queue->output );
		RequirePtrAlignIfNotNil( current, kAtomicAlignment );
	}
	
	return( current );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Apr 15, 1999	Created.
	wolf		Mon, Sep 13, 1999	Rewrote. Before, I would do an PopAtomicQueue()
									followed by a PushAtomicStack() on the output stack.
									While this was easy, this actually modified stack,
									and quickly put it back into place, which created a
									small window where someone could preempt and Pop or
									Peek the queue and find something different than
									what is really there. While I haven't seen this
									cause any trouble yet, I figure I'll just Do The
									Right Thing.
	wolf		Thu, Jan 20, 2000	stolenStack now explicitly aligned.
	wolf		Wed, Jun 4, 2003	Was accidently calling PopAtomicStack instead of
									PeekAtomicStack. Fixed.
	
	************************************************************************************/

	AtomicElement*
PeekAtomicQueue(
	AtomicQueue		*queue )
{
	AlignedAtomicStack	alignedStolenStack;
	AtomicStack			*stolenStack;
	AtomicElement		*current;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	
	current = PeekAtomicStack( &queue->output );
	RequirePtrAlignIfNotNil( current, kAtomicAlignment );
	
	stolenStack = AlignAtomicStack( &alignedStolenStack );
	RequirePtrAlign( stolenStack, kAtomicAlignment );
	
	if( current == nil ) {
		//	Nothing to pop. Refill the queue from the input stack.
		StealAtomicStack( &queue->input, stolenStack );
		current = PopAtomicStack( stolenStack );
		while( current ) {
			RequirePtrAlign( current, kAtomicAlignment );
			PushAtomicStack( current, &queue->output );
			current = PopAtomicStack( stolenStack );
		}
		current = PeekAtomicStack( &queue->output );
		RequirePtrAlignIfNotNil( current, kAtomicAlignment );
	}
	
	return( current );
}

/****************************************************************************************
*	
*	Atomic Queue Offset Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Queue Offset Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	void
PushAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset )
{
	RequirePtr( element );
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	PushAtomicQueue( (AtomicElement*) element, queue );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Mar 16, 2000	Created.
	
	************************************************************************************/

	void
PushFrontAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset )
{
	RequirePtr( element );
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	PushFrontAtomicQueue( (AtomicElement*) element, queue );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	AtomicElement*
PopAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset )
{
	AtomicElement	*result;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PopAtomicQueue( queue );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	SubOffset( (void**)&result, offset );
		
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Apr 15, 1999	Created.
	
	************************************************************************************/

	AtomicElement*
PeekAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset )
{
	AtomicElement	*result;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PeekAtomicQueue( queue );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
*	
*	Guarded Atomic Queue Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Queue Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Nov 23, 1998	Created.
	
	************************************************************************************/

	long
PushGuardedAtomicQueue(
	GuardedAtomicElement	*element,
	AtomicQueue				*queue )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( queue, kAtomicAlignment );
	
	return( PushGuardedAtomicStack( element, &queue->input ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Mar 16, 2000	Created.
	
	************************************************************************************/

	long
PushFrontGuardedAtomicQueue(
	GuardedAtomicElement	*element,
	AtomicQueue				*queue )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( queue, kAtomicAlignment );
	
	return( PushGuardedAtomicStack( element, &queue->output ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Nov 23, 1998	Created.
	wolf		Thu, Jan 20, 2000	stolenStack now explicitly aligned.
	
	************************************************************************************/

	GuardedAtomicElement*
PopGuardedAtomicQueue(
	AtomicQueue				*queue )
{
	AlignedAtomicStack		alignedStolenStack;
	AtomicStack				*stolenStack;
	GuardedAtomicElement	*current;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	
	current = PopGuardedAtomicStack( &queue->output );
	RequirePtrAlignIfNotNil( current, kAtomicAlignment );
	
	stolenStack = AlignAtomicStack( &alignedStolenStack );
	RequirePtrAlign( stolenStack, kAtomicAlignment );
	
	if( current == nil ) {
		//	Nothing to pop. Refill the queue from the input stack.
		StealGuardedAtomicStack( &queue->input, stolenStack );
		current = PopGuardedAtomicStack( stolenStack );
		while( current ) {
			RequirePtrAlign( current, kAtomicAlignment );
			PushGuardedAtomicStack( current, &queue->output );
			current = PopGuardedAtomicStack( stolenStack );
		}
		current = PopGuardedAtomicStack( &queue->output );
		RequirePtrAlignIfNotNil( current, kAtomicAlignment );
	}
	
	return( current );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Apr 15, 1999	Created.
	wolf		Thu, Jan 20, 2000	stolenStack now explicitly aligned.
	wolf		Sat, Feb 19, 2000	Removed a wrong requirement.
	wolf		Mon, Feb 21, 2000	Fixed an insanely stupid bug. I was pushing the
									freshly stolen elements back into the stolen list,
									resulting in a nice infinite loop.
	
	************************************************************************************/

	GuardedAtomicElement*
PeekGuardedAtomicQueue(
	AtomicQueue		*queue )
{
	AlignedAtomicStack		alignedStolenStack;
	AtomicStack				*stolenStack;
	GuardedAtomicElement	*current;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	
	stolenStack = AlignAtomicStack( &alignedStolenStack );
	RequirePtrAlign( stolenStack, kAtomicAlignment );
	
	if( queue->output.next == nil ) {
		//	Nothing to pop. Refill the queue from the input stack.
		StealGuardedAtomicStack( &queue->input, stolenStack );
		current = PopGuardedAtomicStack( stolenStack );
		while( current ) {
			RequirePtrAlign( current, kAtomicAlignment );
			PushGuardedAtomicStack( current, &queue->output );
			current = PopGuardedAtomicStack( stolenStack );
		}
	}
	
	return( (GuardedAtomicElement*) queue->output.next );
}

/****************************************************************************************
*	
*	Guarded Atomic Queue Offset Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Queue Offset Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	long
PushGuardedAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset )
{
	long	result;
	
	RequirePtr( element );
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
		RequirePtrAlignIfNotNil( (void*)((GuardedAtomicElement*) element)->list, kAtomicAlignment );
	result = PushGuardedAtomicQueue( (GuardedAtomicElement*) element, queue );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == queue );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Mar 16, 2000	Created.
	wolf		Tue, Apr 11, 2000	Fixed a bug in a requirement.
	
	************************************************************************************/

	long
PushFrontGuardedAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset )
{
	long	result;
	
	RequirePtr( element );
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
		RequirePtrAlignIfNotNil( (void*)((GuardedAtomicElement*) element)->list, kAtomicAlignment );
	result = PushFrontGuardedAtomicQueue( (GuardedAtomicElement*) element, queue );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == &queue->output );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Nov 25, 1998	Created.
	
	************************************************************************************/

	GuardedAtomicElement*
PopGuardedAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PopGuardedAtomicQueue( queue );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
		RequireIf( result, ((GuardedAtomicElement*) result)->list == nil );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Apr 15, 1999	Created.
	wolf		Mon, Feb 21, 2000	Fixed a broken requirement.
	
	************************************************************************************/

	GuardedAtomicElement*
PeekGuardedAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( queue, kAtomicAlignment );
	RequireOffset( offset );
	
	result = PeekGuardedAtomicQueue( queue );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
		RequireIf( result, ((GuardedAtomicElement*) result)->list == &queue->output );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
*	
*	Atomic List Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic List Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Sep 15, 1999	Created.
	
	************************************************************************************/

	void
PutFirstAtomicList(
	AtomicElement	*element,
	AtomicList		*list )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	PushAtomicStack( element, list );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 20, 1999	Created.
	
	************************************************************************************/

	long
PutAfterAtomicList(
	AtomicElement	*element,
	AtomicElement	*after,
	AtomicElement	*oldElement,
	AtomicList		*list )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( after, kAtomicAlignment );
	RequirePtrAlignIfNotNil( oldElement, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	if( after ) {
		element->next = oldElement;
		return( AtomicStorePtr( oldElement, element, after ) );
	} else {
		PutFirstAtomicList( element, list );
		return( true );
	}
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Sep 14, 1999	Created.
	
	************************************************************************************/

	void
PutLastAtomicList(
	AtomicElement	*element,
	AtomicList		*list )
{
	AtomicElement	*current, *next, *temp;
	long			stored;
	
	element->next = nil;
	
restart:
	current = list;
	next = (AtomicElement*) current->next;
	for(;;) {
		if( next ) {
			temp = (AtomicElement*) next->next;
			stored = AtomicStorePtr( next, next, current );
			if( !stored )
				goto restart;
			current = next;
			next = temp;
		} else {
			stored = AtomicStorePtr( next, element, current );
			if( stored )
				return;
			else
				goto restart;
		}
	}
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Sep 15, 1999	Created.
	
	************************************************************************************/

	AtomicElement*
RemoveFirstAtomicList(
	AtomicList		*list )
{
	AtomicElement	*result;
	
	RequirePtrAlign( list, kAtomicAlignment );
	
	result = PopAtomicStack( list );
	RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	if( result )
		++result->next;	//	Superfical, meaningless modification.
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Wed, Sep 15, 1999	Created.
	
	************************************************************************************/

	long
RemoveAtomicList(
	AtomicElement	*element,
	AtomicList		*list )
{
	AtomicElement	*current, *next, *newNext, *temp;
	long			stored;
	
restart:
	current = list;
	next = (AtomicElement*) current->next;
	while( next ) {
		if( next == element ) {
			newNext = (AtomicElement*) element->next;
			stored = AtomicStore2Ptr( next, newNext, current, newNext, element );
			if( stored ) {
				++element->next;	//	Superfical, meaningless modification.
				return( true );
			} else
				goto restart;
		} else {
			temp = (AtomicElement*) next->next;
			stored = AtomicStorePtr( next, next, current );
			if( !stored )
				goto restart;
			current = next;
			next = temp;
		}
	}
	return( false );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 20, 1999	Created.
	wolf		Wed, Sep 29, 1999	When *current == nil and the stack was empty, *next
									was being set to *nil -- not a good thing. Fixed.
	
	************************************************************************************/

	long
IterateAtomicList(
	AtomicElement	**current,
	AtomicElement	**next,
	AtomicList		*list )
{
	AtomicElement	*temp;
	
	RequirePtr( current );
	RequirePtrAlignIfNotNil( *current, kAtomicAlignment );
	RequirePtr( next );
	RequirePtrAlignIf( *current && *next, *next, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	if( *current == nil ) {
		*current = (AtomicElement*) list->next;
		*next = *current ? ((AtomicElement*) (**current).next) : nil;
	} else {
		temp = *next ? ((AtomicElement*) (**next).next) : nil;
		if( !AtomicStorePtr( *next, *next, *current ) )
			return( false );
		*current = *next;
		*next = temp;
	}
	return( true );
}

/****************************************************************************************
*	
*	Atomic List Offset Functions
*	
****************************************************************************************/
#pragma mark-
#pragma mark	(Atomic List Offset Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	void
PutFirstAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset )
{
	RequirePtr( element );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	PutFirstAtomicList( (AtomicElement*) element, list );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 20, 1999	Created.
	
	************************************************************************************/

	long
PutAfterAtomicListOff(
	void		*element,
	void		*after,
	void		*oldElement,
	AtomicList	*list,
	size_t		offset )
{
	RequirePtr( element );
	RequirePtrIfNotNil( after );
	RequirePtrIfNotNil( oldElement );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	AddOffset( &after, offset );
		RequirePtrAlignIfNotNil( after, kAtomicAlignment );
	AddOffset( &oldElement, offset );
		RequirePtrAlignIfNotNil( oldElement, kAtomicAlignment );
	return( PutAfterAtomicList( (AtomicElement*) element, (AtomicElement*) after,
								(AtomicElement*) oldElement, list ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	void
PutLastAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset )
{
	RequirePtr( element );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	PutLastAtomicList( (AtomicElement*) element, list );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	AtomicElement*
RemoveFirstAtomicListOff(
	AtomicList		*list,
	size_t			offset )
{
	AtomicElement	*result;
	
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	result = RemoveFirstAtomicList( list );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	SubOffset( (void**)&result, offset );
		
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
RemoveAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
	return( RemoveAtomicList( (AtomicElement*) element, list ) );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Tue, Sep 21, 1999	Created.
	wolf		Thu, Sep 23, 1999	Testing brought up an interesting case. *current is
									allowed to be set to nil, which tells
									IterateAtomicList() to start from the beginning.
									In that case, the value in *next should be ingored
									since it will merely be overwritten. Thus in the
									test program's subroutine call, it was passing an
									uninitalized *next value.
									However, AddOffset() wants a valid pointer if not
									nil. So I added an if statement to only call
									AddOffset() on next when *current is not nil.
	
	************************************************************************************/

	long
IterateAtomicListOff(
	void		**current,
	void		**next,
	AtomicList	*list,
	size_t		offset )
{
	long	result;
	
	RequirePtr( current );
	RequirePtrIfNotNil( *current );
	RequirePtr( next );
	RequirePtrAlignIf( *current && *next, *next, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( current, offset );
		RequirePtrAlignIfNotNil( *current, kAtomicAlignment );
	if( *current ) {
		AddOffset( next, offset );
		RequirePtrAlignIfNotNil( *next, kAtomicAlignment );
	}
	result = IterateAtomicList( (AtomicElement**) current, (AtomicElement**) next, list );
	SubOffset( current, offset );
	SubOffset( next, offset );
	
	return( result );
}

/****************************************************************************************
*	
*	Guarded Atomic List Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic List Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
PutFirstGuardedAtomicList(
	GuardedAtomicElement	*element,
	AtomicList				*list )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( (void*) element->list, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	if( GrabAtomicElement( element, list ) ) {
		PutFirstAtomicList( (AtomicElement*) element, list );
		Require( element->list == list );
		return( true );
	} else
		return( false );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 20, 1999	Created.
	
	************************************************************************************/

	long
PutAfterGuardedAtomicList(
	GuardedAtomicElement	*element,
	GuardedAtomicElement	*after,
	GuardedAtomicElement	*oldElement,
	AtomicList				*list )
{
	long	result;
	
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( (void*) element->list, kAtomicAlignment );
	RequirePtrAlignIfNotNil( after, kAtomicAlignment );
	RequireIf( after, after->list == list );
	RequirePtrAlignIfNotNil( oldElement, kAtomicAlignment );
	RequireIf( oldElement, oldElement->list == list );
	RequirePtrAlign( list, kAtomicAlignment );
	
	result = GrabAtomicElement( element, list );
	if( result ) {
		Require( element->list == list );
		result = PutAfterAtomicList( (AtomicElement*) element, (AtomicElement*) after,
										(AtomicElement*) oldElement, list );
			RequireIf( result, element->list == list );
			RequireIf( result && after, after->list == list );
			RequireIf( result && oldElement, oldElement->list == list );
		if( !result )
			ReleaseAtomicElement( element );
	}
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
PutLastGuardedAtomicList(
	GuardedAtomicElement	*element,
	AtomicList				*list )
{
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( (void*) element->list, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	if( GrabAtomicElement( element, list ) ) {
		PutLastAtomicList( (AtomicElement*) element, list );
		Require( element->list == list );
		return( true );
	} else
		return( false );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	GuardedAtomicElement*
RemoveFirstGuardedAtomicList(
	AtomicList		*list )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( list, kAtomicAlignment );
	
	result = (GuardedAtomicElement*) RemoveFirstAtomicList( list );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	if( result ) {
		Require( result->list == list );
		ReleaseAtomicElement( result );
	}
	
	RequireIf( result, result->list == nil );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
RemoveGuardedAtomicList(
	GuardedAtomicElement	*element,
	AtomicList				*list )
{
	long	result;
	
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( (void*) element->list, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	result = RemoveAtomicList( (AtomicElement*) element, list );
	if( result ) {
		Require( element->list == list );
		ReleaseAtomicElement( element );
	}
	
	RequireIf( result, element->list == nil );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 27, 1999	Created.
	
	************************************************************************************/

	long
IterateGuardedAtomicList(
	GuardedAtomicElement	**current,
	GuardedAtomicElement	**next,
	AtomicList				*list )
{
	long	result;
	
	RequirePtr( current );
	RequirePtrAlignIfNotNil( *current, kAtomicAlignment );
	RequirePtr( next );
	RequirePtrAlignIf( *current && *next, *next, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	
	result = IterateAtomicList( (AtomicElement**) current, (AtomicElement**) next, list );
		RequireIf( result && *current, (**current).list == list );
		RequireIf( result && *next, (**next).list == list );
	
	return( result );
}

/****************************************************************************************
*	
*	Guarded Atomic List Offset Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic List Offset Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	long
PutFirstGuardedAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset )
{
	long	result;
	
	RequirePtr( element );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
		RequirePtrAlignIfNotNil( (void*)((GuardedAtomicElement*) element)->list, kAtomicAlignment );
	result = PutFirstGuardedAtomicList( (GuardedAtomicElement*) element, list );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == list );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 20, 1999	Created.
	
	************************************************************************************/

	long
PutAfterGuardedAtomicListOff(
	void		*element,
	void		*after,
	void		*oldElement,
	AtomicList	*list,
	size_t		offset )
{
	long	result;
	
	RequirePtrAlign( element, kAtomicAlignment );
	RequirePtrAlignIfNotNil( after, kAtomicAlignment );
	RequirePtrAlignIfNotNil( oldElement, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
		RequirePtrAlignIfNotNil( (void*)((GuardedAtomicElement*) element)->list, kAtomicAlignment );
	AddOffset( &after, offset );
		RequirePtrAlignIfNotNil( after, kAtomicAlignment );
	AddOffset( &oldElement, offset );
		RequirePtrAlignIfNotNil( oldElement, kAtomicAlignment );
	result = PutAfterGuardedAtomicList( (GuardedAtomicElement*) element,
										(GuardedAtomicElement*) after,
										(GuardedAtomicElement*) oldElement,
										list );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == list );
		RequireIf( result && after, ((GuardedAtomicElement*) after)->list == list );
		RequireIf( result && oldElement, ((GuardedAtomicElement*) oldElement)->list == list );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Sep 17, 1999	Created.
	
	************************************************************************************/

	long
PutLastGuardedAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset )
{
	long	result;
	
	RequirePtr( element );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
		RequirePtrAlignIfNotNil( (void*)((GuardedAtomicElement*) element)->list, kAtomicAlignment );
	result = PutLastGuardedAtomicList( (GuardedAtomicElement*) element, list );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == list );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Sep 16, 1999	Created.
	
	************************************************************************************/

	GuardedAtomicElement*
RemoveFirstGuardedAtomicListOff(
	AtomicList		*list,
	size_t			offset )
{
	GuardedAtomicElement	*result;
	
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	result = RemoveFirstGuardedAtomicList( list );
		RequirePtrAlignIfNotNil( result, kAtomicAlignment );
	SubOffset( (void**)&result, offset );
	
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Fri, Sep 17, 1999	Created.
	
	************************************************************************************/

	long
RemoveGuardedAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset )
{
	long	result;
	
	RequirePtr( element );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( &element, offset );
		RequirePtrAlign( element, kAtomicAlignment );
	result = RemoveGuardedAtomicList( (GuardedAtomicElement*) element, list );
		RequireIf( result, ((GuardedAtomicElement*) element)->list == nil );
	return( result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 27, 1999	Created.
	
	************************************************************************************/

	long
IterateGuardedAtomicListOff(
	void		**current,
	void		**next,
	AtomicList	*list,
	size_t		offset )
{
	long	result;
	
	RequirePtr( current );
	RequirePtrIfNotNil( *current );
	RequirePtr( next );
	RequirePtrAlignIf( *current && *next, *next, kAtomicAlignment );
	RequirePtrAlign( list, kAtomicAlignment );
	RequireOffset( offset );
	
	AddOffset( current, offset );
		RequirePtrAlignIfNotNil( *current, kAtomicAlignment );
	if( *current ) {
		AddOffset( next, offset );
		RequirePtrAlignIfNotNil( *next, kAtomicAlignment );
	}
	result = IterateGuardedAtomicList( (GuardedAtomicElement**) current,
										(GuardedAtomicElement**) next, list );
		RequireIf( result && *current, ((GuardedAtomicElement*) *current)->list == list );
		RequireIf( result && *next, ((GuardedAtomicElement*) *next)->list = list );
	SubOffset( current, offset );
	SubOffset( next, offset );
	
	return( result );
}

/****************************************************************************************
*	
*	Private Pointer Manipulation Functions
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Private Pointer Manipulation Functions)

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Thu, Jan 20, 2000	Created.
	wolf		Sat, Jan 22, 2000	Now aligns the stack based on the kAtomicAlignment
									constant.
	
	************************************************************************************/

	AtomicStack*
AlignAtomicStack(
	AlignedAtomicStack	*stack )
{
	long		result = (long) stack;
	
	RequirePtr( stack );
	
	result &= ~(kAtomicAlignment - 1);	//	Strip lower (kAtomicAlignment log 2) bits.
	result += kAtomicAlignment;			//	Move pointer up kAtomicAlignment bytes.
	RequirePtrAlign( (void*) result, kAtomicAlignment );
	return( (AtomicStack*) result );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 13, 1999	Created.
	
	************************************************************************************/

	void
AddOffset(
	void	**ptr,
	size_t	offset )
{
	char	*p;
	
	RequirePtr( ptr );
	RequirePtrIfNotNil( *ptr );
	RequireOffset( offset );
	
	p = (char*) *ptr;
	if( p ) {
		p += offset;
		*ptr = p;
	}
	RequirePtrIfNotNil( *ptr );
}

/****************************************************************************************
	Commenter	Date				Comment
	---------	-----------------	-----------------------------------------------------
	wolf		Mon, Sep 13, 1999	Created.
	
	************************************************************************************/

	void
SubOffset(
	void	**ptr,
	size_t	offset )
{
	char	*p;
	
	RequirePtr( ptr );
	RequirePtrIfNotNil( *ptr );
	RequireOffset( offset );
	
	p = (char*) *ptr;
	if( p ) {
		p -= offset;
		*ptr = p;
	}
	RequirePtrIfNotNil( *ptr );
}