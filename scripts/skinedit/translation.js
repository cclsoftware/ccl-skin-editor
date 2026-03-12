const kPackageID = "ccl.skineditor.skinedittasks";

// String Extractor searches for lines with "JSTRANSLATE"
function JSTRANSLATE (key)
{
	var theStrings = Host.Locales.getStrings (kPackageID)
	return theStrings.getString (key)
}

/*
This section will be parsed by String Extractor

BEGIN_XSTRINGS "SkinEdit"

	Task Names
	XSTRING "Arrange Horizontally"
	XSTRING "Arrange Vertically"
	
END_XSTRINGS
*/
