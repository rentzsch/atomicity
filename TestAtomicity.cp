/****************************************************************************************
	TestAtomicity.cp $Revision: 13 $
		<http://rentzsch.com/require>
	
	Copyright © 1998-2002 Red Shed Software. All rights reserved.
	by Jonathan 'Wolf' Rentzsch (jon at redshed dot net)
	
	************************************************************************************/

#include	"atomicity.h"
#include	"require.h"
#include	"Aligned.h"

typedef	struct	{
	OSType			tag;
	AtomicElement	element;
	OSType			tag2;
}	TypedAtomicElement;

typedef	struct	{
	OSType					tag;
	GuardedAtomicElement	element;
	OSType					tag2;
}	TypedGuardedAtomicElement;

void	TestAtomicBasis();
void	TestAtomicStacks();
void	TestAtomicStackTypes();
void	TestAtomicLocks();
void	TestAtomicFlags();
void	TestGuardedAtomicStacks();
void	TestGuardedAtomicStackTypes();
void	TestAtomicQueues();
//void	TestAtomicQueueTypes();
//void	TestGuardedAtomicQueues();
//void	TestGuardedAtomicQueueTypes();
void	TestAtomicLists();
void	TestAtomicListIteration();
void	TestAtomicListIterationTypes();
void	TestGuardedAtomicLists();
void	TestGuardedAtomicListIterationTypes();

	void
main()
{
	if( !OpenAtomicity() ) {
		TestAtomicBasis();
		TestAtomicStacks();
		TestAtomicStackTypes();
		TestAtomicLocks();
		TestAtomicFlags();
		TestGuardedAtomicStacks();
		TestGuardedAtomicStackTypes();
		TestAtomicQueues();
		//TestAtomicQueueTypes();
		//TestGuardedAtomicQueues();
		//TestGuardedAtomicQueueTypes();
		TestAtomicLists();
		TestAtomicListIteration();
		TestAtomicListIterationTypes();
		TestGuardedAtomicLists();
		TestGuardedAtomicListIterationTypes();
		
		CloseAtomicity();
	}
}

void	TestAtomicBasis()
{
	long	test1 = 0xAAAAAAAA, *test1p = &test1;
	long	test2 = 0xBBBBBBBB, *test2p = &test2;
	long	success, count;
	
	//	Test single atomic stores.
	success = AtomicStore( 0xAAAAAAAA, 0xCCCCCCCC, test1p );
		Require( success );
	success = AtomicStore( 0xAAAAAAAA, 0xCCCCCCCC, test1p );
		Require( !success );
	success = AtomicStore( 0xCCCCCCCC, 0xAAAAAAAA, test1p );
		Require( success );
	
	//	Test a bunch of single atomic stores.
	for( count = 0x7FFF; count; --count ) {
		success = AtomicStore( 0xAAAAAAAA, 0xCCCCCCCC, test1p );
		Require( success );
		Require( test1 == 0xCCCCCCCC );
		test1 = 0xAAAAAAAA;
	}
	
	//	Test double atomic stores.
	success = AtomicStore2( 0xAAAAAAAA, 0xCCCCCCCC, test1p, 0xBBBBBBBB, test2p );
		Require( success );
	success = AtomicStore2( 0xAAAAAAAA, 0xCCCCCCCC, test1p, 0xBBBBBBBB, test2p );
		Require( !success );
	success = AtomicStore2( 0xCCCCCCCC, 0xAAAAAAAA, test1p, 0xBBBBBBBB, test2p );
		Require( success );
	
		Require( test1 == 0xAAAAAAAA );
		Require( test2 == 0xBBBBBBBB );
	success = AtomicStore2( 0xAAAAAAAA, 0xCCCCCCCC, test1p, 0xDDDDDDDD, test2p );
		Require( !success );
		Require( test1 == 0xAAAAAAAA );
		Require( test2 == 0xBBBBBBBB );
		
	/*{
		long	test = 0xAAAAAAAA;
		long	one = AtomicLoad( &test );
				Require( one == test );
		long	stored = AtomicStore( one, 0xBBBBBBBB, &test );
				Require( stored );
	}
	{
		//	The following loop will result in apparently random requirement failures,
		//	even though the same code above will probably run without incident.
		//	This illustrates how interrupts can and will execute between an atomic load
		//	and store and make the store fail. Note this code will only randomly fail on
		//	a PowerPC -- a 68K (emulated or real) will pass this test without incident.
		for( long i = 0; i < 0x7FFFF; ++i ) {
			long	test = 0xAAAAAAAA;
			long	one = AtomicLoad( &test );
					Require( one == test );
			long	stored = AtomicStoreX( one, 0xBBBBBBBB, &test );
					RequireSupport( if( !stored ) { RequireSwitch( i ); } );
		}
	}
	{
		long	test1 = 0xAAAAAAAA, test2 = 0xBBBBBBBB;
		long	one = AtomicLoad( &test1 );
				Require( one = test1 );
		long	two = *&test2;
		long	stored = AtomicStore2( one, 0xCCCCCCCC, &test1, two, &test2 );
				Require( stored );
	}*/
}

void	TestAtomicStacks()
{
	Aligned<AtomicElement>	one, two, three, four;
							Require( one->next == nil );
							Require( two->next == nil );
							Require( three->next == nil );
							Require( four->next == nil );
	AtomicElement			*result;
	Aligned<AtomicStack>	stack;
							Require( stack->next == nil );
	
	//	Push one item and pop it off.
	PushAtomicStack( &one, &stack );
	result = PopAtomicStack( &stack );
		Require( result == &one );
		RequirePtrAlign( result, 4 );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	
	//	Push two items and pop them off.
	PushAtomicStack( &one, &stack );
	PushAtomicStack( &two, &stack );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	
	//	Push three items and pop them off.
	PushAtomicStack( &one, &stack );
	PushAtomicStack( &two, &stack );
	PushAtomicStack( &three, &stack );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	
	//	Push one, push two, pop two, push three, pop three, pop one.
	PushAtomicStack( &one, &stack );
	PushAtomicStack( &two, &stack );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	PushAtomicStack( &three, &stack );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	/*
	//	Push one item and remove it.
	PushAtomicStack( &one, &stack );
	removed = RemoveAtomicStack( &one, &stack );
		Require( removed );
	removed = RemoveAtomicStack( &one, &stack );
		Require( !removed );
	
	//	Push one, push two, remove one, remove one (should fail), pop two.
	PushAtomicStack( &one, &stack );
	PushAtomicStack( &two, &stack );
	removed = RemoveAtomicStack( &one, &stack );
		Require( removed );
	removed = RemoveAtomicStack( &one, &stack );
		Require( !removed );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &two );

	//	Push one, push two, push three, remove two, pop three, pop one.
	PushAtomicStack( &one, &stack );
	PushAtomicStack( &two, &stack );
	PushAtomicStack( &three, &stack );
	removed = RemoveAtomicStack( &two, &stack );
		Require( removed );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	
	//	Push one, push two, push three, push four, remove two, remove three, remove one, pop four.
	PushAtomicStack( &one, &stack );
	PushAtomicStack( &two, &stack );
	PushAtomicStack( &three, &stack );
	PushAtomicStack( &four, &stack );
	removed = RemoveAtomicStack( &two, &stack );
		Require( removed );
	removed = RemoveAtomicStack( &three, &stack );
		Require( removed );
	removed = RemoveAtomicStack( &one, &stack );
		Require( removed );
	result = PopAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &four );
	result = PopAtomicStack( &stack );
		Require( result == nil );
	*/
}

void	TestAtomicStackTypes()
{
	Aligned<TypedAtomicElement>	one, two, three;
								Require( one->element.next == nil );
								Require( two->element.next == nil );
								Require( three->element.next == nil );
	TypedAtomicElement			*result;
	Aligned<AtomicStack>		stack;
								Require( stack->next == nil );
	
	one->tag = one->tag2 = 'one ';
	two->tag = two->tag2 = 'two ';
	three->tag = three->tag2 = 'thre';
	
	//	Push one item and pop it off.
	PushAtomicStackType( &one, &stack, TypedAtomicElement, element );
	
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	
	//	Push two items and pop them off.
	PushAtomicStackType( &one, &stack, TypedAtomicElement, element );
	PushAtomicStackType( &two, &stack, TypedAtomicElement, element );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	
	//	Push three items and pop them off.
	PushAtomicStackType( &one, &stack, TypedAtomicElement, element );
	PushAtomicStackType( &two, &stack, TypedAtomicElement, element );
	PushAtomicStackType( &three, &stack, TypedAtomicElement, element );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	
	//	Push one, push two, pop two, push three, pop three, pop one.
	PushAtomicStackType( &one, &stack, TypedAtomicElement, element );
	PushAtomicStackType( &two, &stack, TypedAtomicElement, element );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	PushAtomicStackType( &three, &stack, TypedAtomicElement, element );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	result = PopAtomicStackType( &stack, TypedAtomicElement, element );
		Require( result == nil );
	
	//	Make sure tags survived.
		Require( one->tag == 'one ' );
		Require( one->tag2 == 'one ' );
		Require( two->tag == 'two ' );
		Require( two->tag2 == 'two ' );
		Require( three->tag == 'thre' );
		Require( three->tag2 == 'thre' );
}

void	TestAtomicLocks()
{
	Aligned<AtomicLock>		lock = 0;
	Boolean					result;
	
	result = GrabAtomicLock( &lock );
		Require( result );
	result = GrabAtomicLock( &lock );
		Require( !result );
	result = GrabAtomicLock( &lock );
		Require( !result );
	
	ReleaseAtomicLock( &lock );
	result = GrabAtomicLock( &lock );
		Require( result );
}

void	TestAtomicFlags()
{
	Aligned<AtomicFlag>	flag = 0;
	AtomicFlag			*flagp = &flag;
	Boolean				result;
	
	//	Set flag 1, set flag 1 (fail), clear flag 1.
	result = SetAtomicFlag( &flag, kAtomicFlag1 );
		Require( result );
		Require( *flagp == kAtomicFlag1 );
	result = SetAtomicFlag( &flag, kAtomicFlag1 );
		Require( !result );
		Require( *flagp == kAtomicFlag1 );
	result = ClearAtomicFlag( &flag, kAtomicFlag1 );
		Require( result );
		Require( *flagp == 0 );
	
	//	Set flag 1, set flag 2, clear flag 3 (fail), clear flag 1, clear flag 2
	result = SetAtomicFlag( &flag, kAtomicFlag1 );
		Require( result );
		Require( *flagp == kAtomicFlag1 );
	result = SetAtomicFlag( &flag, kAtomicFlag2 );
		Require( result );
		Require( *flagp == kAtomicFlag1 | kAtomicFlag2 );
	result = ClearAtomicFlag( &flag, kAtomicFlag3 );
		Require( !result );
		Require( *flagp == kAtomicFlag1 | kAtomicFlag2 );
	result = ClearAtomicFlag( &flag, kAtomicFlag1 );
		Require( result );
		Require( *flagp == kAtomicFlag2 );
	result = ClearAtomicFlag( &flag, kAtomicFlag2 );
		Require( result );
		Require( *flagp == 0 );
}

void	TestGuardedAtomicStacks()
{
	Aligned<GuardedAtomicElement>	one, two, three;
									Require( one->next == nil );
									Require( one->list == nil );
									Require( two->next == nil );
									Require( two->list == nil );
									Require( three->next == nil );
									Require( three->list == nil );
	GuardedAtomicElement			*result;
	long							pushed;
	Aligned<AtomicStack>			stack;
	
	//	Push one item and pop it off.
	PushGuardedAtomicStack( &one, &stack );
		Require( one->list == &stack );
	result = PopGuardedAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
		Require( result->list == nil );
	result = PopGuardedAtomicStack( &stack );
		Require( result == nil );
	result = PopGuardedAtomicStack( &stack );
		Require( result == nil );
	
	//	Push one, two, two, three and pop off three, two and one.
	//	Two shouldn't be allowed to be pushed twice.
	pushed = PushGuardedAtomicStack( &one, &stack );
		Require( pushed );
	pushed = PushGuardedAtomicStack( &two, &stack );
		Require( pushed );
	pushed = PushGuardedAtomicStack( &two, &stack );
		Require( !pushed );
	pushed = PushGuardedAtomicStack( &three, &stack );
		Require( pushed );
	result = PopGuardedAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopGuardedAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopGuardedAtomicStack( &stack );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopGuardedAtomicStack( &stack );
		Require( result == nil );
	result = PopGuardedAtomicStack( &stack );
		Require( result == nil );
}

void	TestGuardedAtomicStackTypes()
{
	Aligned<TypedGuardedAtomicElement>	one, two, three;
										Require( one->element.next == nil );
										Require( one->element.list == nil );
										Require( two->element.next == nil );
										Require( two->element.list == nil );
										Require( three->element.next == nil );
										Require( three->element.list == nil );
	TypedGuardedAtomicElement			*result;
	long								pushed;
	Aligned<AtomicStack>				stack;
	
	one->tag = one->tag2 = 'one ';
	two->tag = two->tag2 = 'two ';
	three->tag = three->tag2 = 'thre';
	
	//	Push one item and pop it off.
	PushGuardedAtomicStackType( &one, &stack, TypedGuardedAtomicElement, element );
		Require( one->element.list == &stack );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
		Require( result->element.list == nil );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		Require( result == nil );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		Require( result == nil );
	
	//	Push one, two, two, three and pop off three, two and one.
	//	Two shouldn't be allowed to be pushed twice.
	pushed = PushGuardedAtomicStackType( &one, &stack, TypedGuardedAtomicElement, element );
		Require( pushed );
	pushed = PushGuardedAtomicStackType( &two, &stack, TypedGuardedAtomicElement, element );
		Require( pushed );
	pushed = PushGuardedAtomicStackType( &two, &stack, TypedGuardedAtomicElement, element );
		Require( !pushed );
	pushed = PushGuardedAtomicStackType( &three, &stack, TypedGuardedAtomicElement, element );
		Require( pushed );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		Require( result == nil );
	result = PopGuardedAtomicStackType( &stack, TypedGuardedAtomicElement, element );
		Require( result == nil );
	
	//	Make sure tags survived.
		Require( one->tag == 'one ' );
		Require( one->tag2 == 'one ' );
		Require( two->tag == 'two ' );
		Require( two->tag2 == 'two ' );
		Require( three->tag == 'thre' );
		Require( three->tag2 == 'thre' );
}

void	TestAtomicQueues()
{
	Aligned<AtomicElement>	one, two, three;
	AtomicElement			*result;
	Aligned<AtomicQueue>	queue;
	
	//	Push and pop one element.
	PushAtomicQueue( &one, &queue );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicQueue( &queue );
		Require( result == nil );
	result = PopAtomicQueue( &queue );
		Require( result == nil );
	
	//	Push one, two, three and pop one, two and three.
	PushAtomicQueue( &one, &queue );
	PushAtomicQueue( &two, &queue );
	PushAtomicQueue( &three, &queue );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicQueue( &queue );
		Require( result == nil );
	result = PopAtomicQueue( &queue );
		Require( result == nil );
	
	//	Push one, push two, pop one, push three, pop two, pop three.
	PushAtomicQueue( &one, &queue );
	PushAtomicQueue( &two, &queue );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	PushAtomicQueue( &three, &queue );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = PopAtomicQueue( &queue );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = PopAtomicQueue( &queue );
		Require( result == nil );
	result = PopAtomicQueue( &queue );
		Require( result == nil );
}

void	TestAtomicLists()
{
	Aligned<AtomicElement>	one, two, three, four;
							Require( one->next == nil );
							Require( two->next == nil );
							Require( three->next == nil );
							Require( four->next == nil );
	AtomicElement			*result;
	long					success;
	Aligned<AtomicList>		list;
							Require( list->next == nil );
	
	//	PutFirst one, RemoveFirst one, RemoveFirst (fail).
	PutFirstAtomicList( &one, &list );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = RemoveFirstAtomicList( &list );
		Require( result == nil );
	
	//	PutFirst one, Remove one, Remove one (fail).
	PutFirstAtomicList( &one, &list );
	success = RemoveAtomicList( &one, &list );
		Require( success );
	success = RemoveAtomicList( &one, &list );
		Require( !success );
		
	//	PutFirst one, Remove two (fail), Remove one, Remove one (fail).
	PutFirstAtomicList( &one, &list );
	success = RemoveAtomicList( &two, &list );
		Require( !success );
	success = RemoveAtomicList( &one, &list );
		Require( success );
	success = RemoveAtomicList( &one, &list );
		Require( !success );
	
	//	PutLast one, RemoveFirst one, RemoveFirst (fail).
	PutLastAtomicList( &one, &list );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = RemoveFirstAtomicList( &list );
		Require( result == nil );
	
	//	PutLast one, PutLast two, RemoveFirst one, RemoveFirst two, RemoveFirst (fail).
	PutLastAtomicList( &one, &list );
	PutLastAtomicList( &two, &list );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = RemoveFirstAtomicList( &list );
		Require( result == nil );
	
	//	PutLast one, PutLast two, Remove two, Remove two (fail), PutLast three,
	//	PutLast two, RemoveFirst one, RemoveFirst three, RemoveFirst two, RemoveFirst (fail).
	PutLastAtomicList( &one, &list );
	PutLastAtomicList( &two, &list );
	success = RemoveAtomicList( &two, &list );
		Require( success );
	success = RemoveAtomicList( &two, &list );
		Require( !success );
	PutLastAtomicList( &three, &list );
	PutLastAtomicList( &two, &list );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = RemoveFirstAtomicList( &list );
		Require( result == nil );
	
	//	PutLast {one, two, three, four}, Remove three, Remove three (fail),
	//	Remove two, PutLast three, PutLast two, RemoveFirst one, RemoveFirst four,
	//	RemoveFirst three, RemoveFirst two, RemoveFirst (fail).
	PutLastAtomicList( &one, &list );
	PutLastAtomicList( &two, &list );
	PutLastAtomicList( &three, &list );
	PutLastAtomicList( &four, &list );
	success = RemoveAtomicList( &three, &list );
		Require( success );
	success = RemoveAtomicList( &three, &list );
		Require( !success );
	success = RemoveAtomicList( &two, &list );
		Require( success );
	success = RemoveAtomicList( &two, &list );
		Require( !success );
	PutLastAtomicList( &three, &list );
	PutLastAtomicList( &two, &list );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &four );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &three );
	result = RemoveFirstAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &two );
	result = RemoveFirstAtomicList( &list );
		Require( result == nil );
		
	//	PutLast {one, two, three, four}, rotate the list 13 times,
	//	Remove {four, four (fail), three, three (fail), two, two (fail), one, one (fail)}
	//	RemoveFirst (fail).
	PutLastAtomicList( &one, &list );
	PutLastAtomicList( &two, &list );
	PutLastAtomicList( &three, &list );
	PutLastAtomicList( &four, &list );
	for( long count = 13; count; --count ) {
		result = RemoveFirstAtomicList( &list );
			RequirePtrAlign( result, 4 );
		PutLastAtomicList( result, &list );
	}
	success = RemoveAtomicList( &four, &list );
		Require( success );
	success = RemoveAtomicList( &four, &list );
		Require( !success );
	success = RemoveAtomicList( &three, &list );
		Require( success );
	success = RemoveAtomicList( &three, &list );
		Require( !success );
	success = RemoveAtomicList( &two, &list );
		Require( success );
	success = RemoveAtomicList( &two, &list );
		Require( !success );
	success = RemoveAtomicList( &one, &list );
		Require( success );
	success = RemoveAtomicList( &one, &list );
		Require( !success );
	result = RemoveFirstAtomicList( &list );
		Require( result == nil );
}

void	TestAtomicListIteration()
{
	Aligned<AtomicElement>	one, two, three, four;
							Require( one->next == nil );
							Require( two->next == nil );
							Require( three->next == nil );
							Require( four->next == nil );
	AtomicElement			*current, *next;
	long					success;
	Aligned<AtomicList>		list;
							Require( list->next == nil );
	
	//	PutLast {one, two, three, four}.
	PutLastAtomicList( &one, &list );
	PutLastAtomicList( &two, &list );
	PutLastAtomicList( &three, &list );
	PutLastAtomicList( &four, &list );
	
	//	Walk through list.
	current = nil;
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &two );
		Require( next == &three );
	
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &three );
		Require( next == &four );
	
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &four );
		Require( next == nil );
	
	success = IterateAtomicList( &current, &next, &list );
		Require( current == nil );
		Require( success );
		Require( next == nil );
	
	//	Walk to one, remove three, walk to two, remove four, walk to end.
	current = nil;
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	success = RemoveAtomicList( &three, &list );
		Require( success );
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &two );
		Require( next == &four );
	success = RemoveAtomicList( &four, &list );
		Require( success );
	success = IterateAtomicList( &current, &next, &list );
		Require( !success );
	
	//	Put back {three, four}, Walk to one, walk to two, remove two, walk to next (fail).
	PutLastAtomicList( &three, &list );
	PutLastAtomicList( &four, &list );
	current = nil;
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	success = IterateAtomicList( &current, &next, &list );
		Require( success );
		Require( current == &two );
		Require( next == &three );
	success = RemoveAtomicList( &two, &list );
		Require( success );
	success = IterateAtomicList( &current, &next, &list );
		Require( !success );
	
	//	Put two after one (fail), Put two after one.
	success = PutAfterAtomicList( &two, &one, nil, &list );
		Require( !success );
		Require( one->next == &three );
	success = PutAfterAtomicList( &two, &one, one->next, &list );
		Require( success );
		Require( one->next == &two );
		Require( two->next == &three );
}

void	TestAtomicListIterationTypes()
{
	Aligned<TypedAtomicElement>		one, two, three, four;
									Require( one->element.next == nil );
									Require( two->element.next == nil );
									Require( three->element.next == nil );
									Require( four->element.next == nil );
	TypedAtomicElement				*current = nil, *next = nil;
	long							success;
	Aligned<AtomicList>				list;
									Require( list->next == nil );
	
	one->tag = one->tag2 = 'one ';
	two->tag = two->tag2 = 'two ';
	three->tag = three->tag2 = 'thre';
	four->tag = four->tag2 = 'four';
	
	//	PutLast {one, two, three, four}.
	PutLastAtomicListType( &one, &list, TypedAtomicElement, element );
	PutLastAtomicListType( &two, &list, TypedAtomicElement, element );
	PutLastAtomicListType( &three, &list, TypedAtomicElement, element );
	PutLastAtomicListType( &four, &list, TypedAtomicElement, element );
	
	//	Walk through list.
	current = nil;
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &two );
		Require( next == &three );
	
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &three );
		Require( next == &four );
	
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &four );
		Require( next == nil );
	
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( current == nil );
		Require( success );
		Require( next == nil );
	
	//	Walk to one, remove three, walk to two, remove four, walk to end.
	current = nil;
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	success = RemoveAtomicListType( &three, &list, TypedAtomicElement, element );
		Require( success );
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &two );
		Require( next == &four );
	success = RemoveAtomicListType( &four, &list, TypedAtomicElement, element );
		Require( success );
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( !success );
	
	//	Put back {three, four}, Walk to one, walk to two, remove two, walk to next (fail).
	PutLastAtomicListType( &three, &list, TypedAtomicElement, element );
	PutLastAtomicListType( &four, &list, TypedAtomicElement, element );
	current = nil;
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &two );
		Require( next == &three );
	success = RemoveAtomicListType( &two, &list, TypedAtomicElement, element );
		Require( success );
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( !success );
	
	//	Put two after one (fail), Put two after one.
	success = PutAfterAtomicListType( &two, &one, nil, &list, TypedAtomicElement, element );
		Require( !success );
		Require( one->element.next == &three->element );
	current = nil;
	success = IterateAtomicListType( &current, &next, &list, TypedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &three );
	success = PutAfterAtomicListType( &two, current, next, &list, TypedAtomicElement, element );
		Require( success );
		Require( one->element.next == &two->element );
		Require( two->element.next == &three->element );
	
	//	Make sure tags survived
		Require( one->tag == 'one ' );
		Require( one->tag2 == 'one ' );
		Require( two->tag == 'two ' );
		Require( two->tag2 == 'two ' );
		Require( three->tag == 'thre' );
		Require( three->tag2 == 'thre' );
		Require( four->tag == 'four' );
		Require( four->tag2 == 'four' );
}

void	TestGuardedAtomicLists()
{
	Aligned<GuardedAtomicElement>	one, two, three;
									Require( one->next == nil );
									Require( one->list == nil );
									Require( two->next == nil );
									Require( two->list == nil );
									Require( three->next == nil );
									Require( three->list == nil );
	GuardedAtomicElement			*result;
	long							success;
	Aligned<AtomicList>				list;
	
	//	PutFirst one, RemoveFirst one, RemoveFirst (fail).
	success = PutFirstGuardedAtomicList( &one, &list );
		Require( success );
		Require( one->list );
	result = RemoveFirstGuardedAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
		Require( !one->list );
	result = RemoveFirstGuardedAtomicList( &list );
		Require( result == nil );
	
	//	PutFirst one, PutFirst one (fail), RemoveFirst one, RemoveFirst (fail).
	success = PutFirstGuardedAtomicList( &one, &list );
		Require( success );
		Require( one->list );
	success = PutFirstGuardedAtomicList( &one, &list );
		Require( !success );
		Require( one->list );
	result = RemoveFirstGuardedAtomicList( &list );
		RequirePtrAlign( result, 4 );
		Require( result == &one );
		Require( !one->list );
	result = RemoveFirstGuardedAtomicList( &list );
		Require( result == nil );
}

void	TestGuardedAtomicListIterationTypes()
{
	Aligned<TypedGuardedAtomicElement>		one, two, three, four;
									Require( one->element.next == nil );
									Require( two->element.next == nil );
									Require( three->element.next == nil );
									Require( four->element.next == nil );
	TypedGuardedAtomicElement				*current = nil, *next = nil;
	long							success;
	Aligned<AtomicList>				list;
									Require( list->next == nil );
	
	one->tag = one->tag2 = 'one ';
	two->tag = two->tag2 = 'two ';
	three->tag = three->tag2 = 'thre';
	four->tag = four->tag2 = 'four';
	
	//	PutLast {one, two, three, four}.
	PutLastGuardedAtomicListType( &one, &list, TypedGuardedAtomicElement, element );
	PutLastGuardedAtomicListType( &two, &list, TypedGuardedAtomicElement, element );
	PutLastGuardedAtomicListType( &three, &list, TypedGuardedAtomicElement, element );
	PutLastGuardedAtomicListType( &four, &list, TypedGuardedAtomicElement, element );
	
	//	Walk through list.
	current = nil;
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &two );
		Require( next == &three );
	
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &three );
		Require( next == &four );
	
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &four );
		Require( next == nil );
	
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( current == nil );
		Require( success );
		Require( next == nil );
	
	//	Walk to one, remove three, walk to two, remove four, walk to end.
	current = nil;
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	success = RemoveGuardedAtomicListType( &three, &list, TypedGuardedAtomicElement, element );
		Require( success );
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &two );
		Require( next == &four );
	success = RemoveGuardedAtomicListType( &four, &list, TypedGuardedAtomicElement, element );
		Require( success );
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( !success );
	
	//	Put back {three, four}, Walk to one, walk to two, remove two, walk to next (fail).
	PutLastGuardedAtomicListType( &three, &list, TypedGuardedAtomicElement, element );
	PutLastGuardedAtomicListType( &four, &list, TypedGuardedAtomicElement, element );
	current = nil;
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &two );
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &two );
		Require( next == &three );
	success = RemoveGuardedAtomicListType( &two, &list, TypedGuardedAtomicElement, element );
		Require( success );
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( !success );
	
	//	Put two after one (fail), Put two after one.
	success = PutAfterGuardedAtomicListType( &two, &one, nil, &list, TypedGuardedAtomicElement, element );
		Require( !success );
		Require( one->element.next == &three->element );
	current = nil;
	success = IterateGuardedAtomicListType( &current, &next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( current == &one );
		Require( next == &three );
	success = PutAfterGuardedAtomicListType( &two, current, next, &list, TypedGuardedAtomicElement, element );
		Require( success );
		Require( one->element.next == &two->element );
		Require( two->element.next == &three->element );
		Require( three->element.next == &four->element );
		Require( four->element.next == nil );
	
	//	Put three after one (fail because it's already in the list).
	success = PutAfterGuardedAtomicListType( &three, &one, &two, &list, TypedGuardedAtomicElement, element );
		Require( !success );
		Require( one->element.next == &two->element );
		Require( two->element.next == &three->element );
		Require( three->element.next == &four->element );
		Require( four->element.next == nil );
	
	//	Make sure tags survived
		Require( one->tag == 'one ' );
		Require( one->tag2 == 'one ' );
		Require( two->tag == 'two ' );
		Require( two->tag2 == 'two ' );
		Require( three->tag == 'thre' );
		Require( three->tag2 == 'thre' );
		Require( four->tag == 'four' );
		Require( four->tag2 == 'four' );
}