/****************************************************************************************
	atomicity.config.h $Revision: 13 $
		<http://rentzsch.com/atomicity>
	
	Copyright © 1998-2002 Red Shed Software. All rights reserved.
	by Jonathan 'Wolf' Rentzsch (jon at redshed dot net)
	
	************************************************************************************/

#ifndef		_atomicity_config_
#define		_atomicity_config_

/****************************************************************************************
	Open Transport Compatibility Options

	Define DontUseOpenTransport *or* UseOpenTransportIfAvailable *or*
	OnlyUseOpenTransport. If you don't define any of these, the default is
	UseOpenTransportIfAvailable.
	
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
#pragma mark	--Open Transport Compatibility Options--

#define		DontUseOpenTransport
//#define		UseOpenTransportIfAvailable
//#define		OnlyUseOpenTransport

#endif	//	_atomicity_config_