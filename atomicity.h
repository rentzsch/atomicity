/****************************************************************************************
	atomicity.h $Revision: 48 $
		<http://rentzsch.com/atomicity>
	
	Copyright © 1998-2002 Red Shed Software. All rights reserved.
	by Jonathan 'Wolf' Rentzsch (jon at redshed dot net)
		
	This code requires a 68020 or later or any PowerPC. No habla 68000 or x86.
	
	************************************************************************************/

#ifndef		_atomicity_
#define		_atomicity_

#include	<stddef.h>
#include	"atomicity.config.h"

#ifdef		__cplusplus
extern	"C"	{
#endif	//	__cplusplus

#define		kAtomicAlignment	4

/****************************************************************************************
*	
*	Atomic Basis
*	
****************************************************************************************/
#pragma mark	(Atomic Basis)

//	Performs the following atomically:
//	if( *address == oldValue ) {
//		*address = newValue;
//		return 1;
//	} else {
//		return 0;
//	}
//	Resistant to non-modifing preemptions.

	extern
	pascal
	long
MyAtomicStore(
	long	oldValue,
	long	newValue,
	void	*address );

//	Performs the following atomically:
//	if( *address == oldValue ) {
//		*address = newValue;
//		return 1;
//	} else {
//		return 0;
//	}
//	Resistant to non-modifing preemptions.

	extern
	pascal
	long
MyAtomicStorePtr(
	void	*oldValue,
	void	*newValue,
	void	*address );

//	Performs the following atomically:
//	if( *address == oldValue && *address2 == oldValue2 ) {
//		*address = newValue;
//		return 1;
//	} else {
//		return 0;
//	}
//	Resistant to non-modifing preemptions.

	extern
	pascal
	long
AtomicStore2(
	long	oldValue,
	long	newValue,
	void	*address,
	long	oldValue2,
	void	*address2 );

//	Performs the following atomically:
//	if( *address == oldValue && *address2 == oldValue2 ) {
//		*address = newValue;
//		return 1;
//	} else {
//		return 0;
//	}
//	Resistant to non-modifing preemptions.

	extern
	pascal
	long
AtomicStore2Ptr(
	void	*oldValue,
	void	*newValue,
	void	*address,
	void	*oldValue2,
	void	*address2 );

/****************************************************************************************
*	
*	Atomic Stacks
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Stacks)

typedef	struct	AtomicElement	AtomicElement, AtomicStack;

struct	AtomicElement	{
	volatile	AtomicElement	*next;
	
#ifdef	__cplusplus
	AtomicElement() : next( nil ){}
#endif
};

/**************************
*	
*	Atomic Stack Functions
*
*	An atomic stack is an atomic last-in-first-out list.
*
*	These are the lowest level functions to atomically manipulate stacks.
*	
**************************/
#pragma mark	(Atomic Stack Functions)

//	Atomically set element->next to stack->next, set stack->next to element.

	extern
	void
MyPushAtomicStack(
	AtomicElement	*element,
	AtomicStack		*stack );

//	Atomically store stack->next, set stack->next to stack->next->next, and return the
//	first element. It's okay if stack->next == nil: PopAtomicStack() will return nil.

	extern
	AtomicElement*
MyPopAtomicStack(
	AtomicStack		*stack );

//	Atomically set toStack->next to fromStack->next and set fromStack->next to nil.
//	It's okay if fromStack->next == nil: StealAtomicStack() will return set toStack->next
//	to nil.

	extern
	void
StealAtomicStack(
	AtomicStack		*fromStack,
	AtomicStack		*toStack );

//	Simply returns stack->next.

	extern
	AtomicElement*
PeekAtomicStack(
	AtomicStack		*stack );

/**************************
*	
*	Atomic Stack Offset Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the coercions and field offset.
*	
**************************/
#pragma mark	(Atomic Stack Offset Functions)

//	Adds offset to element, and calls PushAtomicStack().

	extern
	void
PushAtomicStackOff(
	void			*element,
	AtomicStack		*stack,
	size_t			offset );

//	Calls PopAtomicStack(), if the result is not nil, subtracts offset from the result
//	and returns it.

	extern
	AtomicElement*
PopAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset );

//	Calls PeekAtomicStack(), if the result is not nil, subtracts offset from the result
//	and returns it.

	extern
	AtomicElement*
PeekAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset );

/**************************
*	
*	Atomic Stack Type Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the structure and field names. It does
*	the coercions and offsets for you.
*	
**************************/
#pragma mark	(Atomic Stack Type Functions)

#define		PushAtomicStackType( ELEMENT, STACK, STRUCTURE, FIELD )	\
				PushAtomicStackOff( (ELEMENT), (STACK), offsetof( STRUCTURE, FIELD ) )

#define		PopAtomicStackType( STACK, STRUCTURE, FIELD )	\
				((STRUCTURE*) PopAtomicStackOff( (STACK), offsetof( STRUCTURE, FIELD ) ))

#define		PeekAtomicStackType( STACK, STRUCTURE, FIELD )	\
				((STRUCTURE*) PeekAtomicStackOff( (STACK), offsetof( STRUCTURE, FIELD ) ))

/****************************************************************************************
*	
*	Atomic Locks
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Locks)

typedef	volatile	unsigned	long	AtomicLock;

	extern
	long	//	Non-zero if successfully grabbed.
MyGrabAtomicLock(
	AtomicLock	*lock );

	extern
	void
ReleaseAtomicLock(
	AtomicLock	*lock );

/****************************************************************************************
*	
*	Atomic Flags
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Flags)

typedef	unsigned long				AtomicFlag;
typedef	volatile	unsigned long	AtomicFlags;

#define	kNoAtomicFlags	(AtomicFlags)0x00000000
#define	kNoAtomicFlag	(AtomicFlag) 0x00000000

#define	kAtomicFlag1	(AtomicFlag) 0x00000001
#define	kAtomicFlag2	(AtomicFlag) 0x00000002
#define	kAtomicFlag3	(AtomicFlag) 0x00000004
#define	kAtomicFlag4	(AtomicFlag) 0x00000008
#define	kAtomicFlag5	(AtomicFlag) 0x00000010
#define	kAtomicFlag6	(AtomicFlag) 0x00000020
#define	kAtomicFlag7	(AtomicFlag) 0x00000040
#define	kAtomicFlag8	(AtomicFlag) 0x00000080
#define	kAtomicFlag9	(AtomicFlag) 0x00000100
#define	kAtomicFlag10	(AtomicFlag) 0x00000200
#define	kAtomicFlag11	(AtomicFlag) 0x00000400
#define	kAtomicFlag12	(AtomicFlag) 0x00000800
#define	kAtomicFlag13	(AtomicFlag) 0x00001000
#define	kAtomicFlag14	(AtomicFlag) 0x00002000
#define	kAtomicFlag15	(AtomicFlag) 0x00004000
#define	kAtomicFlag16	(AtomicFlag) 0x00008000
#define	kAtomicFlag17	(AtomicFlag) 0x00010000
#define	kAtomicFlag18	(AtomicFlag) 0x00020000
#define	kAtomicFlag19	(AtomicFlag) 0x00040000
#define	kAtomicFlag20	(AtomicFlag) 0x00080000
#define	kAtomicFlag21	(AtomicFlag) 0x00100000
#define	kAtomicFlag22	(AtomicFlag) 0x00200000
#define	kAtomicFlag23	(AtomicFlag) 0x00400000
#define	kAtomicFlag24	(AtomicFlag) 0x00800000
#define	kAtomicFlag25	(AtomicFlag) 0x01000000
#define	kAtomicFlag26	(AtomicFlag) 0x02000000
#define	kAtomicFlag27	(AtomicFlag) 0x04000000
#define	kAtomicFlag28	(AtomicFlag) 0x08000000
#define	kAtomicFlag29	(AtomicFlag) 0x10000000
#define	kAtomicFlag30	(AtomicFlag) 0x20000000
#define	kAtomicFlag31	(AtomicFlag) 0x40000000
#define	kAtomicFlag32	(AtomicFlag) 0x80000000

	extern
	Boolean	//	The value of the flag within the flags.
GetAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag );

	extern
	Boolean	//	True if wasn't already set.
SetAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag );

	extern
	Boolean	//	True if wasn't already clear.
ClearAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag );

	extern
	Boolean
FlipAtomicFlag(
	AtomicFlags	*flags,
	AtomicFlag	flag,
	Boolean		value );

/****************************************************************************************
	Open Transport Compatibility Options

	Define DontUseOpenTransport *or* UseOpenTransportIfAvailable *or*
	OnlyUseOpenTransport in the file atomicity.config.h. If you don't define any of
	these, the default is UseOpenTransportIfAvailable.
	
	If you define UseOpenTransportIfAvailable or OnlyUseOpenTransport, you *must* call
	InitOpenTransport() in the beginning of your application. If you don't want to
	juggle whether or not to call InitOpenTransport(), simply always call
	OpenAtomicity(). If you defined UseOpenTransportIfAvailable or OnlyUseOpenTransport,
	it maps to InitOpenTransport(), otherwise it maps to noErr.
	
	DontUseOpenTransport:
		o	Open Transport Libraries: Not Required.
		o	Runtime Overhead: None.
	
	UseOpenTransportIfAvailable:
		o	Open Transport Libraries: Required (preferably weakly imported).
		o	Runtime Overhead: Small (larger if requirements checking is turned on).
	
	OnlyUseOpenTransport:
		o	Open Transport Libraries: Required (preferably strongly imported).
		o	Runtime Overhead: None.
	
	************************************************************************************/
#pragma mark	-
#pragma mark	(Open Transport Compatibility Options)

#if		defined(DontUseOpenTransport)
	#define	AtomicStore( OLD, NEW, ADDRESS )	MyAtomicStore( OLD, NEW, ADDRESS )
	#define	AtomicStorePtr( OLD, NEW, ADDRESS )	MyAtomicStorePtr( OLD, NEW, ADDRESS )
	#define	PushAtomicStack( ELEMENT, STACK )	MyPushAtomicStack( ELEMENT, STACK )
	#define	PopAtomicStack( STACK )				MyPopAtomicStack( STACK )
	#define	GrabAtomicLock( LOCK )				MyGrabAtomicLock( LOCK )

	#if	TARGET_API_MAC_CARBON
		#define	OpenAtomicity( flags, context )	noErr
		#define	CloseAtomicity( context )		
	#else
		#define	OpenAtomicity()					noErr
		#define	CloseAtomicity()				
	#endif
	
	#define	UseOTAtomicity( VALUE )				
#elif	defined(OnlyUseOpenTransport)
	#include	"OpenTransport.h"
	#define	AtomicStore( OLD, NEW, ADDRESS )	OTCompareAndSwap32( (UInt32) OLD, (UInt32) NEW, (UInt32*) ADDRESS )
	#define	AtomicStorePtr( OLD, NEW, ADDRESS )	OTCompareAndSwapPtr( OLD, NEW, (void**)ADDRESS )
	#define	PushAtomicStack( ELEMENT, STACK )	OTLIFOEnqueue( (OTLIFO*) STACK, (OTLink*) ELEMENT )
	#define	PopAtomicStack( STACK )				(AtomicElement*) OTLIFODequeue( (OTLIFO*) STACK );
	#define	GrabAtomicLock( LOCK )				AtomicStore( 0, 1, LOCK )

	#if	TARGET_API_MAC_CARBON
		#define	OpenAtomicity( flags, context )	InitOpenTransportInContext( flags, context )
		#define	CloseAtomicity( context )		CloseOpenTransportInContext( context )
	#else
		#define	OpenAtomicity()					InitOpenTransport()
		#define	CloseAtomicity()				CloseOpenTransport()
	#endif
	
	#define	UseOTAtomicity( VALUE )				
#else	//	UseOpenTransportIfAvailable
	#include	"OpenTransport.h"
	extern	long								AtomicStore( long oldValue, long newValue, void *address );
	extern	long								AtomicStorePtr( void *oldValue, void *newValue, void *address );
	extern	void								PushAtomicStack( AtomicElement *element, AtomicStack *stack );
	extern	AtomicElement* 						PopAtomicStack( AtomicStack *stack );
	extern	Boolean								GrabAtomicLock( AtomicLock *lock );
	
	#if	TARGET_API_MAC_CARBON
		extern	OSStatus						OpenAtomicity( OTInitializationFlags flags, OTClientContextPtr *outClientContext );
		extern	void							CloseAtomicity( OTClientContextPtr clientContext );
	#else
		extern	OSStatus						OpenAtomicity();
		extern	void							CloseAtomicity();
	#endif
	
	extern	void								UseOTAtomicity( Boolean value );
#endif

/****************************************************************************************
*	
*	Guarded Atomic Stacks
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Stacks)

typedef	struct	GuardedAtomicElement	GuardedAtomicElement;

struct	GuardedAtomicElement	{
	volatile	GuardedAtomicElement	*next;
	volatile	void					*list;
	
#ifdef	__cplusplus
	GuardedAtomicElement() : next( nil ), list( nil ){}
#endif
};

	extern
	long
GrabAtomicElement(
	GuardedAtomicElement	*element,
	void					*list );

	extern
	void
ReleaseAtomicElement(
	GuardedAtomicElement	*element );

/**************************
*	
*	Guarded Atomic Stack Functions
*
*	A guarded atomic stack doesn't allow an element to be pushed more than once.
*	Pushing an element more than once on a stack corrupts the stack into a nasty
*	circular list. These functions test whether the element in already on a stack
*	before blindly pushing the element, saving your stack from corruption.
*	
**************************/
#pragma mark	(Guarded Atomic Stack Functions)

//	Call GrabAtomicLock() on element->lock. If successful, call PushAtomicStack() and
//	return true. Otherwise return false.

	extern
	long
PushGuardedAtomicStack(
	GuardedAtomicElement	*element,
	AtomicStack				*stack );

//	Call PopAtomicStack(). If the result isn't nil, call ReleaseAtomicLock() on the popped
//	element->lock and return the popped element. Otherwise return nil.

	extern
	GuardedAtomicElement*
PopGuardedAtomicStack(
	AtomicStack				*stack );

//	Atomically set toStack->next to fromStack->next and set fromStack->next to nil.
//	It's okay if fromStack->next == nil: StealAtomicStack() will return set toStack->next
//	to nil. It also sets every element's list field from the stolen stack to toStack.

	extern
	void
StealGuardedAtomicStack(
	AtomicStack		*fromStack,
	AtomicStack		*toStack );

//	Simply returns stack->next.

	extern
	GuardedAtomicElement*
PeekGuardedAtomicStack(
	AtomicStack				*stack );

/**************************
*	
*	Guarded Atomic Stack Offset Functions
*
*	Convenience routines for when the GuardedAtomicElement field in your structure
*	can't be the first field. You supply the coercions and field offset.
*	
**************************/
#pragma mark	(Guarded Atomic Stack Offset Functions)

//	Adds offset to element, and calls PushGuardedAtomicStack().

	extern
	long
PushGuardedAtomicStackOff(
	void			*element,
	AtomicStack		*stack,
	size_t			offset );

//	Calls PopGuardedAtomicStack(), if the result is not nil, subtracts offset from
//	the result and returns it.

	extern
	GuardedAtomicElement*
PopGuardedAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset );

//	Calls PeekGuardedAtomicStack(), if the result is not nil, subtracts offset from
//	the result and returns it.

	extern
	GuardedAtomicElement*
PeekGuardedAtomicStackOff(
	AtomicStack		*stack,
	size_t			offset );

/**************************
*	
*	Guarded Atomic Stack Type Functions
*
*	Convenience routines for when the GuardedAtomicElement field in your structure
*	can't be the first field. You supply the structure and field names. It does
*	the coercions and offsets for you.
*	
**************************/
#pragma mark	(Guarded Atomic Stack Type Functions)

#define		PushGuardedAtomicStackType( ELEMENT, STACK, STRUCTURE, FIELD )	\
				PushGuardedAtomicStackOff( (ELEMENT), (STACK), offsetof( STRUCTURE, FIELD ) )

#define		PopGuardedAtomicStackType( STACK, STRUCTURE, FIELD )	\
				((STRUCTURE*) PopGuardedAtomicStackOff( (STACK), offsetof( STRUCTURE, FIELD ) ))

#define		PeekGuardedAtomicStackType( STACK, STRUCTURE, FIELD )	\
				((STRUCTURE*) PeekGuardedAtomicStackOff( (STACK), offsetof( STRUCTURE, FIELD ) ))

#define		RemoveGuardedAtomicStackType( ELEMENT, STACK, STRUCTURE, FIELD )	\
				RemoveGuardedAtomicStackOff( (ELEMENT), (STACK), offsetof( STRUCTURE, FIELD ) )

/****************************************************************************************
*	
*	Atomic Queues
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Queues)

typedef	struct	{
	AtomicStack		input;
	AtomicStack		output;
}	AtomicQueue;

/**************************
*	
*	Atomic Queue Functions
*
*	An atomic queue is an atomic first-in-first-out list.
*	
**************************/
#pragma mark	(Atomic Queue Functions)

//	Simply calls PushAtomicStack() with queue->input.

	extern
	void
PushAtomicQueue(
	AtomicElement	*element,
	AtomicQueue		*queue );

//	Simply calls PushAtomicStack() with queue->output.

	extern
	void
PushFrontAtomicQueue(
	AtomicElement	*element,
	AtomicQueue		*queue );

//	Calls PopAtomicStack() with queue->output. If the result is not nil,
//	PopAtomicQueue() returns it. Otherwise PopAtomicQueue() calls
//	StealAtomicList() on queue->input. If the result is not nil,
//	PopAtomicQueue() calls PopAtomicStack() on each element in the stolen stack,
//	then calling PushAtomicStack() with queue->output, effectively "reversing" the
//	stack into a queue.

	extern
	AtomicElement*
PopAtomicQueue(
	AtomicQueue		*queue );

//	Calls PopAtomicQueue(). If the result is not nil, then it calls PushAtomicStack()
//	with the result on queue->output.

	extern
	AtomicElement*
PeekAtomicQueue(
	AtomicQueue		*queue );

/**************************
*	
*	Atomic Queue Offset Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the coercions and field offset.
*	
**************************/
#pragma mark	(Atomic Queue Offset Functions)

//	Adds offset to element, and calls PushAtomicQueue().

	extern
	void
PushAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset );

//	Adds offset to element, and calls PushFrontAtomicQueue().

	extern
	void
PushFrontAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset );

//	Calls PopAtomicQueue(), if the result is not nil, subtracts offset from
//	the result and returns it.

	extern
	AtomicElement*
PopAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset );

//	Calls PeekAtomicStack(). If the result is not nil, subtracts offset from
//	the result and returns it.

	extern
	AtomicElement*
PeekAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset );

/**************************
*	
*	Atomic Queue Type Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the structure and field names. It does
*	the coercions and offsets for you.
*	
**************************/
#pragma mark	(Atomic Queue Type Functions)

#define		PushAtomicQueueType( ELEMENT, QUEUE, STRUCTURE, FIELD )	\
				PushAtomicQueueOff( (ELEMENT), (QUEUE), offsetof( STRUCTURE, FIELD ) )

#define		PushFrontAtomicQueueType( ELEMENT, QUEUE, STRUCTURE, FIELD )	\
				PushFrontAtomicQueueOff( (ELEMENT), (QUEUE), offsetof( STRUCTURE, FIELD ) )

#define		PopAtomicQueueType( QUEUE, STRUCTURE, FIELD )	\
				((STRUCTURE*) PopAtomicQueueOff( (QUEUE), offsetof( STRUCTURE, FIELD ) ))

#define		PeekAtomicQueueType( QUEUE, STRUCTURE, FIELD )	\
				((STRUCTURE*) PeekAtomicQueueOff( (QUEUE), offsetof( STRUCTURE, FIELD ) ))

/****************************************************************************************
*	
*	Guarded Atomic Queues
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Queues)

/**************************
*	
*	Guarded Atomic Queue Functions
*
*	A guarded atomic queue is an atomic queue that doesn't allow an element to be
*	pushed more than once.
*	
**************************/
#pragma mark	(Guarded Atomic Queue Functions)

//	Simply calls PushGuardedAtomicStack() with queue->input.

	extern
	long
PushGuardedAtomicQueue(
	GuardedAtomicElement	*element,
	AtomicQueue				*queue );

//	Simply calls PushGuardedAtomicStack() with queue->output.

	extern
	long
PushFrontGuardedAtomicQueue(
	GuardedAtomicElement	*element,
	AtomicQueue				*queue );

//	Calls PopGuardedAtomicStack() with queue->output. If the result is not nil,
//	PopGuardedAtomicQueue() returns it. Otherwise PopGuardedAtomicQueue() calls
//	StealAtomicList() on queue->input. If the result is not nil,
//	PopGuardedAtomicQueue() calls PopAtomicStack() on each element in the stolen stack,
//	then calling PushAtomicStack() with queue->output, effectively "reversing" the
//	stack into a queue.

	extern
	GuardedAtomicElement*
PopGuardedAtomicQueue(
	AtomicQueue		*queue );

//	If queue->output.next is nil, then PeekGuardedAtomicQueue() calls
//	StealAtomicList() on queue->input. If the result is not nil,
//	PopGuardedAtomicQueue() calls PopAtomicStack() on each element in the stolen stack,
//	then calling PushAtomicStack() with queue->output, effectively "reversing" the
//	stack into a queue. PeekGuardedAtomicQueue() returns queue->output.next.

	extern
	GuardedAtomicElement*
PeekGuardedAtomicQueue(
	AtomicQueue		*queue );

/**************************
*	
*	Guarded Atomic Queue Offset Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the coercions and field offset.
*	
**************************/
#pragma mark	(Guarded Atomic Queue Offset Functions)

//	Adds offset to element, and calls PushGuardedAtomicQueue().

	extern
	long
PushGuardedAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset );

//	Adds offset to element, and calls PushFrontGuardedAtomicQueue().

	extern
	long
PushFrontGuardedAtomicQueueOff(
	void			*element,
	AtomicQueue		*queue,
	size_t			offset );

//	Calls PopGuardedAtomicQueue(), if the result is not nil, subtracts offset from
//	the result and returns it.

	extern
	GuardedAtomicElement*
PopGuardedAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset );

//	Calls PekkGuardedAtomicQueue(), if the result is not nil, subtracts offset from
//	the result and returns it.

	extern
	GuardedAtomicElement*
PeekGuardedAtomicQueueOff(
	AtomicQueue		*queue,
	size_t			offset );

/**************************
*	
*	Guarded Atomic Queue Type Functions
*
*	Convenience routines for when the GuardedAtomicElement field in your structure
*	can't be the first field. You supply the structure and field names. It does
*	the coercions and offsets for you.
*	
**************************/
#pragma mark	(Guarded Atomic Queue Type Functions)

#define		PushGuardedAtomicQueueType( ELEMENT, QUEUE, STRUCTURE, FIELD )	\
				PushGuardedAtomicQueueOff( (ELEMENT), (QUEUE), offsetof( STRUCTURE, FIELD ) )

#define		PushFrontGuardedAtomicQueueType( ELEMENT, QUEUE, STRUCTURE, FIELD )	\
				PushFrontGuardedAtomicQueueOff( (ELEMENT), (QUEUE), offsetof( STRUCTURE, FIELD ) )

#define		PopGuardedAtomicQueueType( QUEUE, STRUCTURE, FIELD )	\
				((STRUCTURE*) PopGuardedAtomicQueueOff( (QUEUE), offsetof( STRUCTURE, FIELD ) ))

#define		PeekGuardedAtomicQueueType( QUEUE, STRUCTURE, FIELD )	\
				((STRUCTURE*) PeekGuardedAtomicQueueOff( (QUEUE), offsetof( STRUCTURE, FIELD ) ))

/****************************************************************************************
*	
*	Atomic Lists
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Atomic Lists)

typedef	AtomicElement	AtomicList;

/**************************
*	
*	Atomic List Functions
*	
**************************/
#pragma mark	(Atomic List Functions)

//	Atomically set element->next to list->next, set list->next to element.
//	Constant speed, O(1).

	extern
	void
PutFirstAtomicList(
	AtomicElement	*element,
	AtomicList		*list );

//	Sets element->next to after->next. Atomically set after->next to element.
//	Constant speed, O(1).

	extern
	long
PutAfterAtomicList(
	AtomicElement	*element,
	AtomicElement	*after,
	AtomicElement	*oldElement,
	AtomicList		*list );

//	Set element->next to nil. Atomically walk the list until we find the last element.
//	(we'll call this element "lastElement") and set lastElement->next to element.
//	Linear speed, O(n).

	extern
	void
PutLastAtomicList(
	AtomicElement	*element,
	AtomicList		*list );

//	Atomically store list->next, set list->next to list->next->next, and return the
//	first element. It's okay if list->next == nil: RemoveFirstAtomicList() will return nil.
//	Constant speed, O(1).

	extern
	AtomicElement*
RemoveFirstAtomicList(
	AtomicList		*list );

//	Atomically walk the list until the we find the element before "element" (we'll call
//	this element "previousElement") and set previousElement->next to element->next.
//	Linear speed, O(n).

	extern
	long
RemoveAtomicList(
	AtomicElement	*element,
	AtomicList		*list );

//	Peek at the first element in the list. Only useful in its [Off|Type] varients.
//	Constant speed, O(1).

#define		PeekAtomicList( LIST )	PeekAtomicStack( LIST );

//	Atomically steal an entire list.
//	Constant speed, O(1).

#define		StealAtomicList( FROMLIST, TOLIST )	StealAtomicStack( FROMLIST, TOLIST );

//	Atomically walk the list. Upon beginning, set *current to nil. Returns false (0)
//	if interrupted, true if uninterrupted. You must start over upon an interruption.
//	Constant speed, O(1).

	extern
	long
IterateAtomicList(
	AtomicElement	**current,
	AtomicElement	**next,
	AtomicList		*list );

/**************************
*	
*	Atomic List Offset Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the coercions and field offset.
*	
**************************/
#pragma mark	(Atomic List Offset Functions)

//	

	extern
	void
PutFirstAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset );

//	

	extern
	long
PutAfterAtomicListOff(
	void		*element,
	void		*after,
	void		*oldElement,
	AtomicList	*list,
	size_t		offset );

//	

	extern
	void
PutLastAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset );

//	

	extern
	AtomicElement*
RemoveFirstAtomicListOff(
	AtomicList		*list,
	size_t			offset );

//	

	extern
	long
RemoveAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset );

//

#define		PeekAtomicListOff( LIST, OFFSET )	PeekAtomicStackOff( LIST, OFFSET )

//	

	extern
	long
IterateAtomicListOff(
	void		**current,
	void		**next,
	AtomicList	*list,
	size_t		offset );

/**************************
*	
*	Atomic List Type Functions
*
*	Convenience routines for when the AtomicElement field in your structure
*	can't be the first field. You supply the structure and field names. It does
*	the coercions and offsets for you.
*	
**************************/
#pragma mark	(Atomic List Type Functions)

#define		PutFirstAtomicListType( ELEMENT, LIST, STRUCTURE, FIELD )	\
				PutFirstAtomicListOff( (ELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		PutAfterAtomicListType( ELEMENT, AFTER, OLDELEMENT, LIST, STRUCTURE, FIELD )	\
				PutAfterAtomicListOff( (ELEMENT), (AFTER), (OLDELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		PutLastAtomicListType( ELEMENT, LIST, STRUCTURE, FIELD )	\
				PutLastAtomicListOff( (ELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		RemoveFirstAtomicListType( LIST, STRUCTURE, FIELD )	\
				((STRUCTURE*) RemoveFirstAtomicListOff( (LIST), offsetof( STRUCTURE, FIELD ) ))

#define		RemoveAtomicListType( ELEMENT, LIST, STRUCTURE, FIELD )	\
				RemoveAtomicListOff( (ELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		PeekAtomicListType( LIST, STRUCTURE, FIELD )	\
				PeekAtomicStackType( LIST, STRUCTURE, FIELD )

#define		IterateAtomicListType( CURRENT, NEXT, LIST, STRUCTURE, FIELD )	\
				IterateAtomicListOff( (CURRENT), (NEXT), (LIST), offsetof( STRUCTURE, FIELD ) )

/****************************************************************************************
*	
*	Guarded Atomic Lists
*	
****************************************************************************************/
#pragma mark	-
#pragma mark	(Guarded Atomic Lists)

/**************************
*	
*	Guarded Atomic List Functions
*
*	A guarded atomic list doesn't allow an element to be pushed more than once.
*	Pushing an element more than once on a list corrupts the stack into a nasty
*	circular list. These functions test whether the element in already on a list
*	before blindly pushing the element, saving your list from corruption.
*	
**************************/
#pragma mark	(Guarded Atomic List Functions)

//	Call GrabAtomicLock() on element->lock. If successful, call PutFirstAtomicList() and
//	return true. Otherwise return false.

	extern
	long
PutFirstGuardedAtomicList(
	GuardedAtomicElement	*element,
	AtomicList				*list );

//	

	extern
	long
PutAfterGuardedAtomicList(
	GuardedAtomicElement	*element,
	GuardedAtomicElement	*after,
	GuardedAtomicElement	*oldElement,
	AtomicList				*list );

//	Call GrabAtomicLock() on element->lock. If successful, call PutLastAtomicList() and
//	return true. Otherwise return false.

	extern
	long
PutLastGuardedAtomicList(
	GuardedAtomicElement	*element,
	AtomicList				*list );

//	Call RemoveFirstAtomicList(). If the result is not nil, call ReleaseAtomicLock() on
//	it and return it. Otherwise just return nil.

	extern
	GuardedAtomicElement*
RemoveFirstGuardedAtomicList(
	AtomicList		*list );

//	Call RemoveAtomicList(). If the result is not zero, then call ReleaseAtomicLock()
//	on element->lock. Return the result.

	extern
	long
RemoveGuardedAtomicList(
	GuardedAtomicElement	*element,
	AtomicList				*list );

//

#define		PeekGuardedAtomicList( LIST )	PeekGuardedAtomicStack( LIST )

//

#define		StealGuardedAtomicList( FROMLIST, TOLIST )	StealGuardedAtomicStack( FROMLIST, TOLIST )

//	

	extern
	long
IterateGuardedAtomicList(
	GuardedAtomicElement	**current,
	GuardedAtomicElement	**next,
	AtomicList				*list );

/**************************
*	
*	Guarded Atomic List Offset Functions
*
*	Convenience routines for when the GuardedAtomicElement field in your structure
*	can't be the first field. You supply the coercions and field offset.
*	
**************************/
#pragma mark	(Guarded Atomic List Offset Functions)

//	Adds offset to element, and calls PutFirstGuardedAtomicList().

	extern
	long
PutFirstGuardedAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset );

//	

	extern
	long
PutAfterGuardedAtomicListOff(
	void		*element,
	void		*after,
	void		*oldElement,
	AtomicList	*list,
	size_t		offset );

//	

	extern
	long
PutLastGuardedAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset );

//	

	extern
	GuardedAtomicElement*
RemoveFirstGuardedAtomicListOff(
	AtomicList		*list,
	size_t			offset );

//	

	extern
	long
RemoveGuardedAtomicListOff(
	void		*element,
	AtomicList	*list,
	size_t		offset );

//

#define		PeekGuardedAtomicListOff( LIST )	PeekGuardedAtomicStackOff( LIST )

//	

	extern
	long
IterateGuardedAtomicListOff(
	void		**current,
	void		**next,
	AtomicList	*list,
	size_t		offset );

/**************************
*	
*	Guarded Atomic List Type Functions
*
*	Convenience routines for when the GuardedAtomicElement field in your structure
*	can't be the first field. You supply the structure and field names. It does
*	the coercions and offsets for you.
*	
**************************/
#pragma mark	(Guarded Atomic List Type Functions)

#define		PutFirstGuardedAtomicListType( ELEMENT, LIST, STRUCTURE, FIELD )	\
				PutFirstGuardedAtomicListOff( (ELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		PutAfterGuardedAtomicListType( ELEMENT, AFTER, OLDELEMENT, LIST, STRUCTURE, FIELD )	\
				PutAfterGuardedAtomicListOff( (ELEMENT), (AFTER), (OLDELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		PutLastGuardedAtomicListType( ELEMENT, LIST, STRUCTURE, FIELD )	\
				PutLastGuardedAtomicListOff( (ELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		RemoveFirstGuardedAtomicListType( LIST, STRUCTURE, FIELD )	\
				((STRUCTURE*) RemoveFirstGuardedAtomicListOff( (LIST), offsetof( STRUCTURE, FIELD ) ))

#define		RemoveGuardedAtomicListType( ELEMENT, LIST, STRUCTURE, FIELD )	\
				RemoveGuardedAtomicListOff( (ELEMENT), (LIST), offsetof( STRUCTURE, FIELD ) )

#define		PeekGuardedAtomicListType( LIST, STRUCTURE, FIELD )	\
				PeekGuardedAtomicStackType( LIST, STRUCTURE, FIELD )

#define		IterateGuardedAtomicListType( CURRENT, NEXT, LIST, STRUCTURE, FIELD )	\
				IterateGuardedAtomicListOff( (void**)(CURRENT), (void**)(NEXT), (LIST), offsetof( STRUCTURE, FIELD ) )

#ifdef	__cplusplus
}
#endif

#endif	//	_atomicity_