/****************************************************************************************
	atomicity.mbug.r $Revision: 17 $
		<http://rentzsch.com/atomicity>
	
	Copyright © 1998-2002 Red Shed Software. All rights reserved.
	by Jonathan 'Wolf' Rentzsch (jon at redshed dot net)
	
	************************************************************************************/

type 'mxwt' {
	integer = $$Countof(TemplateArray);		//	numberOfTemplates
	array TemplateArray {
		pstring;							//	templateName
		integer = $$Countof(FieldArray);	//	numberOfFields
		array FieldArray {
			pstring;						//	fieldName
			pstring;						//	typeName
			integer;						//	count
		};
	};
};

resource 'mxwt' (128, "atomicity") {{
	"AtomicElement", {
		"next",
		"^AtomicElement",
		1
	},
	"AtomicStack", {
		"head",
		"Pointer",
		1
	},
	"GuardedAtomicElement", {
		"next",
		"^GuardedAtomicElement",
		1,
		
		"list",
		"Pointer",
		1
	},
	"AtomicQueue", {
		"input",
		"AtomicStack",
		1,
		
		"output",
		"AtomicStack",
		1
	},
	"AtomicList", {
		"head",
		"Pointer",
		1
	},
}};