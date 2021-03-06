/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Lammertink
 Date:		25/08/1999
________________________________________________________________________

-*/

#include "uistrings.h"
#include "dbkey.h"

static const char* joinstring = "%1 %2";

uiString uiStrings::phrAdd( const uiString& string )
{ return toUiString(joinstring).arg( sAdd() ).arg( string ); }

uiString uiStrings::phrASCII( const uiString& string )
{ return toUiString(joinstring).arg( sASCII() ).arg( string ); }

uiString uiStrings::phrCalculate( const uiString& string )
{ return toUiString(joinstring).arg(sCalculate()).arg(string); }

uiString uiStrings::phrCalculateFrom( const uiString& string )
{ return toUiString(joinstring).arg(sCalculateFrom()).arg(string); }

uiString uiStrings::phrCrossline( const uiString& string )
{ return phrJoinStrings( sCrossline(), string ); }

uiString uiStrings::phrTODONotImpl( const char* clssnm )
{ return toUiString( "[%1] TODO: Not Implemented" ).arg( clssnm ); }

uiString uiStrings::phrNotImplInThisVersion( const char* fromver )
{ return tr("Not implemented in this version of OpendTect."
	  "\nPlease use version %1 or higher").arg( fromver ); }

uiString uiStrings::phrThreeDots( const uiString& string, bool immediate )
{ return immediate ? string : toUiString( "%1 ..." ).arg( string ); }

uiString uiStrings::phrPlsSelectAtLeastOne( const uiString& string )
{ return tr("Please select at least one %1").arg( string ); }

uiString uiStrings::phrPlsSpecifyAtLeastOne( const uiString& string )
{ return tr("Please specify at least one %1").arg( string ); }

uiString uiStrings::phrSelect( const uiString& string )
{ return toUiString(joinstring).arg( sSelect() ).arg( string ); }

uiString uiStrings::phrSelectObjectWrongType( const uiString& string )
{ return toUiString(joinstring).arg(tr("Select object is not a ")).arg(string);}

uiString uiStrings::phrDoesntExist(const uiString& string, int num )
{ return tr( "%1 does not exist", 0, num ).arg( string ); }

uiString uiStrings::phrExport( const uiString& string )
{ return toUiString(joinstring).arg( sExport() ).arg( string ); }

uiString uiStrings::phrImport( const uiString& string )
{ return toUiString(joinstring).arg( sImport() ).arg( string ); }

uiString uiStrings::phrInternalError( const uiString& string )
{ return tr( "Internal Error (pease contact support@dgbes.com):\n%1")
	 .arg( string ); }

uiString uiStrings::phrInternalError( const char* string )
{ return tr( "Internal Error (pease contact support@dgbes.com):\n%1")
	 .arg( string ); }

uiString uiStrings::phrCannotAdd( const uiString& string )
{ return toUiString(joinstring).arg(sCannotAdd()).arg(string); }

uiString uiStrings::phrCannotCompute( const uiString& string )
{ return toUiString(joinstring).arg(sCannotCompute()).arg(string); }

uiString uiStrings::phrCannotCopy( const uiString& string )
{ return toUiString(joinstring).arg(sCannotCopy()).arg(string); }

uiString uiStrings::phrCannotCreate( const uiString& string )
{ return tr("Cannot create %1").arg( string ); }

uiString uiStrings::phrCannotCreateDBEntryFor(const uiString& string)
{ return phrCannotCreate( tr("database entry for %1").arg(string) ); }

uiString uiStrings::phrCannotCreateDirectory( const uiString& string )
{ return phrCannotCreate( tr("directory %1").arg(string) ); }

uiString uiStrings::phrCannotEdit( const uiString& string )
{ return toUiString(joinstring).arg(sCannotEdit()).arg(string); }

uiString uiStrings::phrCannotExtract( const uiString& string )
{ return toUiString(joinstring).arg(sCannotExtract()).arg(string); }

uiString uiStrings::phrCannotFind( const uiString& string )
{ return toUiString(joinstring).arg(sCannotFind()).arg(string); }

uiString uiStrings::phrCannotImport( const uiString& string )
{ return toUiString(joinstring).arg(sCannotImport()).arg(string); }

uiString uiStrings::phrCannotLoad( const uiString& string )
{ return toUiString(joinstring).arg(sCannotLoad()).arg(string); }

uiString uiStrings::phrCannotOpen( const uiString& string )
{ return toUiString(joinstring).arg(sCannotOpen()).arg( string ); }

uiString uiStrings::phrCannotParse( const uiString& string )
{ return toUiString(joinstring).arg(sCannotParse()).arg(string); }

uiString uiStrings::phrCannotFindDBEntry( const uiString& string )
{ return phrCannotFind( tr("database entry for %1").arg( string ) ); }

uiString uiStrings::phrCannotRead( const uiString& string )
{ return tr("Cannot read %1").arg( string ); }

uiString uiStrings::phrCannotRemove( const uiString& string )
{ return toUiString(joinstring).arg(sCannotRemove()).arg(string); }

uiString uiStrings::phrCannotWrite( const uiString& string )
{ return toUiString(joinstring).arg(sCannotWrite()).arg( string ); }

uiString uiStrings::phrCannotWriteDBEntry( const uiString& string )
{ return phrCannotWrite( tr("database entry for %1").arg(string) ); }

uiString uiStrings::phrCannotSave( const uiString& string )
{ return toUiString(joinstring).arg(sCannotSave()).arg(string); }

uiString uiStrings::phrCannotStart( const uiString& string )
{ return toUiString(joinstring).arg(sCannotStart()).arg(string); }

uiString uiStrings::phrCannotUnZip( const uiString& string )
{ return toUiString(joinstring).arg(sCannotUnZip()).arg(string); }

uiString uiStrings::phrCannotZip( const uiString& string )
{ return toUiString(joinstring).arg(sCannotZip()).arg(string); }

uiString uiStrings::phrCheck( const uiString& string )
{ return toUiString(joinstring).arg(sCheck()).arg(string); }

uiString uiStrings::phrCopy( const uiString& string )
{ return toUiString(joinstring).arg(sCopy()).arg(string); }

uiString uiStrings::phrCreate( const uiString& string )
{ return toUiString(joinstring).arg(sCreate()).arg(string); }

uiString uiStrings::phrCreateNew( const uiString& string )
{ return toUiString(joinstring).arg(sCreateNew()).arg(string); }

uiString uiStrings::phrCrossPlot( const uiString& string )
{ return toUiString(joinstring).arg(sCrossPlot()).arg(string); }

uiString uiStrings::phrColonString( const uiString& string )
{ return tr(": %1").arg( string ); }

uiString uiStrings::phrData( const uiString& string )
{ return toUiString(joinstring).arg(sData()).arg(string); }

uiString uiStrings::phrDelete( const uiString& string )
{ return toUiString(joinstring).arg(sDelete()).arg(string); }

uiString uiStrings::phrEdit( const uiString& string )
{ return toUiString(joinstring).arg( sEdit() ).arg( string ); }

uiString uiStrings::phrEnter( const uiString& string )
{ return toUiString(joinstring).arg(sEnter()).arg(string); }

uiString uiStrings::phrExistsConinue( const uiString& string, bool overwrite )
{
    return tr( "%1 exists. %2?")
	.arg( string )
	.arg( overwrite ? sOverwrite() : sContinue() );
}

uiString uiStrings::phrExtract( const uiString& string )
{ return toUiString(joinstring).arg(sExtract()).arg(string); }

uiString uiStrings::phrGenerating( const uiString& string )
{ return toUiString(joinstring).arg(sGenerating()).arg(string); }

uiString uiStrings::phrHandling( const uiString& string )
{ return tr( "Handling %1").arg( string ); }

uiString uiStrings::phrHandled( const uiString& string )
{ return tr( "%1 handled").arg( string ); }

uiString uiStrings::phrInline( const uiString& string )
{ return phrJoinStrings( sInline(), string ); }

uiString uiStrings::phrInput( const uiString& string )
{ return toUiString(joinstring).arg( sInput() ).arg( string ); }

uiString uiStrings::phrInsert( const uiString& string )
{ return phrJoinStrings( sInsert(), string ); }

uiString uiStrings::phrInvalid( const uiString& string )
{ return toUiString(joinstring).arg(sInvalid()).arg(string); }

uiString uiStrings::phrJoinStrings( const uiString& a, const uiString& b )
{ return toUiString(joinstring).arg( a ).arg( b ); }

uiString uiStrings::phrJoinStrings( const uiString& a, const uiString& b,
				    const uiString& c)
{ return toUiString("%1 %2 %3").arg( a ).arg( b ).arg( c ); }

uiString uiStrings::phrManage( const uiString& string )
{ return toUiString(joinstring).arg(sManage()).arg(string); }

uiString uiStrings::phrModify( const uiString& string )
{ return toUiString(joinstring).arg(sModify()).arg(string); }

uiString uiStrings::phrMerge( const uiString& string )
{ return toUiString(joinstring).arg(sMerge()).arg(string); }

uiString uiStrings::phrOpen( const uiString& string )
{ return toUiString(joinstring).arg(sOpen()).arg(string); }

uiString uiStrings::phrOutput( const uiString& string )
{ return toUiString(joinstring).arg( sOutput() ).arg( string ); }

uiString uiStrings::phrPlsContactSupport( bool firstdoc )
{
    if ( !firstdoc )
	return tr( "Please contact OpendTect support at support@dgbes.com." );
    return tr( "Please consult the documentation at opendtect.org."
	    "\nIf that fails you may want to contact OpendTect support at "
	    "support@dgbes.com.");
}

uiString uiStrings::phrReading( const uiString& string )
{ return tr( "Reading %1").arg( string ); }

uiString uiStrings::phrRead( const uiString& string )
{ return tr( "%1 read").arg( string ); }

uiString uiStrings::phrRemove( const uiString& string )
{ return toUiString(joinstring).arg(sRemove()).arg(string); }

uiString uiStrings::phrRemoveSelected( const uiString& string )
{ return toUiString(joinstring).arg(sRemoveSelected()).arg(string); }

uiString uiStrings::phrRename( const uiString& string )
{ return toUiString(joinstring).arg(sRename()).arg(string); }

uiString uiStrings::phrSelectPos( const uiString& string )
{ return toUiString(joinstring).arg(sSelectPos()).arg(string); }

uiString uiStrings::phrSetAs( const uiString& string )
{ return toUiString(joinstring).arg(sSetAs()).arg(string); }

uiString uiStrings::phrSuccessfullyExported( const uiString& string )
{ return tr( "Successfully exported %1").arg( string );}

uiString uiStrings::phrZIn( const uiString& string )
{ return tr( "Z in %1" ).arg( string ); }

uiString uiStrings::phrWriting( const uiString& string )
{ return tr( "Writing %1").arg( string ); }

uiString uiStrings::phrWritten( const uiString& string )
{ return tr( "%1 written").arg( string ); }

uiString uiStrings::phrSave( const uiString& string )
{ return toUiString(joinstring).arg(sSave()).arg(string); }

uiString uiStrings::phrSaveAs( const uiString& string )
{ return tr( "Save %1 as" ).arg( string ); }

uiString uiStrings::phrShowIn( const uiString& string )
{ return toUiString(joinstring).arg(sShowIn()).arg(string); }

uiString uiStrings::phrSpecify( const uiString& string )
{ return toUiString(joinstring).arg(sSpecify()).arg(string); }

uiString uiStrings::phrStorageDir( const uiString& string )
{ return toUiString(joinstring).arg(sStorageDir()).arg(string); }

uiString uiStrings::phrLoad( const uiString& string )
{ return toUiString(joinstring).arg(sLoad()).arg(string); }

uiString uiStrings::phrLoading( const uiString& string )
{ return tr("Loading %1").arg(string); }

uiString uiStrings::phrXcoordinate( const uiString& string )
{ return toUiString(joinstring).arg(sXcoordinate()).arg(string); }

uiString uiStrings::phrYcoordinate( const uiString& string )
{ return toUiString(joinstring).arg(sYcoordinate()).arg(string); }

uiString uiStrings::phrZRange( const uiString& string )
{ return toUiString(joinstring).arg(sZRange()).arg(string); }

uiString uiStrings::s2D()
{ return tr("2D"); }

uiString uiStrings::s3D()
{ return tr("3D"); }

uiString uiStrings::sAdd()
{ return tr("Add"); }

uiString uiStrings::sASCII()
{ return tr("ASCII"); }

uiString uiStrings::sBatchProgram()
{ return mJoinUiStrs(sBatch(),sProgram()); }

uiString uiStrings::sBatchProgramFailedStart()
{ return tr("Batch program failed to start"); }

uiString uiStrings::sColorTable(int num)
{ return tr("Color Table",0,num); }

uiString uiStrings::sCreate()
{ return tr("Create"); }

uiString uiStrings::sCalculate()
{ return tr("Calculate"); }

uiString uiStrings::sCannotAdd()
{ return tr("Cannot add"); }

uiString uiStrings::sCannotCompute()
{ return tr("Cannot compute"); }

uiString uiStrings::sCannotCopy()
{ return tr("Cannot copy"); }

uiString uiStrings::sCannotEdit()
{ return tr("Cannot edit"); }

uiString uiStrings::sCannotExtract()
{ return tr("Cannot extract"); }

uiString uiStrings::sCannotFind()
{ return tr("Cannot find"); }

uiString uiStrings::sCalculateFrom()
{ return tr("Calculate From"); }

uiString uiStrings::sCannotImport()
{ return tr("Cannot Import"); }

uiString uiStrings::sCannotLoad()
{ return tr("Cannot load"); }

uiString uiStrings::sCannotSave()
{ return tr("Cannot Save"); }

uiString uiStrings::sCannotWrite()
{ return tr("Cannot Write"); }

uiString uiStrings::sCannotUnZip()
{ return tr("Cannot UnZip"); }

uiString uiStrings::sCannotZip()
{ return tr("Cannot Zip"); }

uiString uiStrings::sCantCreateHor()
{ return phrCannotCreate( tr("horizon") ); }

uiString uiStrings::sCantFindAttrName()
{ return phrCannotFind( tr("attribute name") ); }

uiString uiStrings::sCantFindODB()
{ return phrCannotFind( tr("object in data base") ); }

uiString uiStrings::sCantFindSurf()
{ return phrCannotFind( tr("surface") ); }

uiString uiStrings::sCannotOpen()
{ return tr("Cannot open"); }

uiString uiStrings::sCannotParse()
{ return tr("Cannot parse"); }

uiString uiStrings::sCantReadHor()
{ return phrCannotRead( tr("horizon") ); }

uiString uiStrings::sCantReadInp()
{ return phrCannotRead( tr("input") ); }

uiString uiStrings::sCantWriteSettings()
{ return phrCannotWrite(tr("settings"));}

uiString uiStrings::sCantOpenInpFile( int num )
{ return phrCannotOpen( tr("input file", 0, num ) ); }

uiString uiStrings::sCannotStart()
{ return tr("Cannot Start"); }

uiString uiStrings::sCheck()
{ return tr("Check"); }

uiString uiStrings::sCheckPermissions()
{ return tr("Please check your permissions."); }

uiString uiStrings::sOutput()
{ return tr("Output"); }

uiString uiStrings::sCantOpenOutpFile( int num )
{ return phrCannotOpen( tr("output file", 0, num ) ); }

uiString uiStrings::sCannotRemove()
{ return tr("Cannot remove"); }

uiString uiStrings::sCopy()
{ return tr("Copy"); }

uiString uiStrings::sCreateNew()
{ return mJoinUiStrs(sCreate(),sNew()); }

uiString uiStrings::sCreateOutput()
{ return mJoinUiStrs(sCreate(),sOutput()); }

uiString uiStrings::sCreateProbDesFunc()
{ return phrCreate( sProbDensFunc(false) ); }

uiString uiStrings::sCrossPlot()
{ return tr("Cross Plot"); }

uiString uiStrings::sData()
{ return tr("Data"); }

uiString uiStrings::sDelete()
{ return tr("Delete"); }

uiString uiStrings::sEdit()
{ return tr("Edit"); }

uiString uiStrings::sEnter()
{ return tr("Enter"); }

uiString uiStrings::sEnterValidName()
{ return uiStrings::phrEnter(tr("a valid name")); }

uiString uiStrings::sExport()
{ return tr("Export"); }

uiString uiStrings::sExtract()
{ return tr("Extract"); }

uiString uiStrings::sFault( int num )
{ return tr("Fault", 0, num ); }

uiString uiStrings::sFaultStickSet( int num )
{ return tr( "FaultStickSet", 0, num ); }

uiString uiStrings::sFrequency( int num )
{
    return tr( "Frequency", 0, num );
}

uiString uiStrings::sHelp()
{ return tr("Help"); }

uiString uiStrings::sHistogram( )
{ return tr("Histogram"); }

uiString uiStrings::sHorizon( int num )
{ return tr("Horizon", 0, num ); }

uiString uiStrings::sImport()
{ return tr("Import"); }

uiString uiStrings::sInput()
{ return tr("Input"); }

uiString uiStrings::sInputFile()
{ return phrInput( sFile().toLower() ); }

uiString uiStrings::sInputSelection()
{ return phrInput( sSelection().toLower() ); }

uiString uiStrings::sInputASCIIFile()
{ return phrInput( phrASCII( sFile() )); }

uiString uiStrings::sInputParamsMissing()
{ return tr("Input parameters missing"); }

uiString uiStrings::sInsert()
{ return tr("Insert"); }

uiString uiStrings::sInvalid()
{ return tr("Invalid"); }

uiString uiStrings::sLatitude( bool abbrev )
{ return abbrev ? tr("Lat") : tr("Latitude"); }

uiString uiStrings::sLoad()
{ return tr("Load"); }

uiString uiStrings::sLogs()
{ return sLog(mPlural); }

uiString uiStrings::sLongitude( bool abbrev )
{ return abbrev ? tr("Long") : tr("Longitude"); }

uiString uiStrings::sManage()
{ return tr("Manage"); }

uiString uiStrings::sMarker( int num )
{ return tr("Marker", 0, num); }

uiString uiStrings::sMerge()
{ return tr("Merge"); }

uiString uiStrings::sModify()
{ return tr("Modify"); }

uiString uiStrings::sNew()
{ return tr("New"); }

uiString uiStrings::sOpen()
{ return tr("Open" ); }

uiString uiStrings::sOptions()
{ return tr("Options"); }

uiString uiStrings::sOutputSelection()
{ return phrOutput(sSelection().toLower()); }

uiString uiStrings::sOutputASCIIFile()
{ return phrOutput( phrASCII( sFile() )); }

uiString uiStrings::sOutputFileExistsOverwrite()
{ return phrExistsConinue( tr("Output file"), true); }

uiString uiStrings::sProbDensFunc( bool abbrevation, int num )
{
    return abbrevation
        ? tr( "PDF", 0, num )
        : tr("Probability Density Function", 0, num );
}

uiString uiStrings::sProperties()
{ return tr("Properties"); }

uiString uiStrings::sRemove()
{ return tr("Remove"); }

uiString uiStrings::sRemoveSelected()
{ return tr("Remove Selected"); }

uiString uiStrings::sRename()
{ return tr("Rename"); }

uiString uiStrings::sSave()
{ return tr("Save"); }

uiString uiStrings::sSaveAs()
{ return tr("Save as"); }

uiString uiStrings::sSelect()
{ return tr("Select"); }

uiString uiStrings::sSelectPos()
{ return tr("Select Position"); }

uiString uiStrings::sSelOutpFile()
{ return uiStrings::phrSelect(tr("output file")); }

uiString uiStrings::sSelection( int num )
{ return tr("Selection", 0, num ); }

uiString uiStrings::sSetting( int num )
{ return tr("Setting", 0, num ); }

uiString uiStrings::sSetAs()
{ return tr("Set As"); }

uiString uiStrings::sShift()
{ return tr("Shift" ); }

uiString uiStrings::sShowIn()
{ return tr("Show in"); }

uiString uiStrings::sSpecify()
{ return tr("Specify"); }

uiString uiStrings::sSpecifyOut()
{ return uiStrings::phrJoinStrings(tr("Specify"), uiStrings::sOutput()); }


uiString uiStrings::sStorageDir()
{ return tr("Storage Directory"); }

uiString uiStrings::sStored()
{ return tr("Stored" ); }

uiString uiStrings::sStratigraphy()
{ return tr( "Stratigraphy" ); }

uiString uiStrings::sTrack()
{ return tr("Track" ); }

uiString uiStrings::sVolume(int num)
{ return tr("Volume",0,num); }

uiString uiStrings::sWaveNumber( int num )
{ return tr("Wavenumber", 0, num ); }

uiString uiStrings::sWavelet( int num )
{ return tr("Wavelet", 0, num ); }

uiString uiStrings::sWell( int num )
{ return tr("Well", 0, num ); }

uiString uiStrings::sWellLog( int num )
{ return tr("Well log", 0, num ); }

uiString uiStrings::sDistUnitString(bool isfeet,bool abb, bool withparentheses)
{
    return withparentheses
	? toUiString("(%1)").arg( sDistUnitString( isfeet, abb, false ) )
	: isfeet
	    ? abb ? tr("ft") : tr("feet" )
	    : abb ? tr("m") : tr("meter");
}

uiString uiStrings::sTimeUnitString( bool abb )
{ return abb ? tr( "s" ) : uiStrings::sSec(); }

uiString uiStrings::sXcoordinate()
{ return tr("X-coordinate"); }

uiString uiStrings::sYcoordinate()
{ return tr("Y-coordinate"); }uiString uiStrings::sZRange()
{ return tr("Z Range"); }


uiString uiStrings::sVolDataName(bool is2d, bool is3d, bool isprestack,
			     bool both_2d_3d_in_context,
			     bool both_pre_post_in_context )
{
    if ( is2d && is3d )
	return tr( "Seismic data" );

    if ( is2d )
    {
	if ( isprestack )
	{
	    if ( both_2d_3d_in_context )
	    {
		return tr( "Prestack 2D Data" );
	    }

	    return tr( "Prestack Data" );
	}

	if ( both_2d_3d_in_context )
	{
	    if ( both_pre_post_in_context )
	    {
		return tr( "Poststack 2D Data" );
	    }

	    return tr("2D Data (attribute)");
	}

	if ( both_pre_post_in_context )
	{
	    return tr("Poststack Data");
	}

	return tr("2D Data (attribute)");
    }

    if ( is3d )
    {
	if ( isprestack )
	{
	    if ( both_2d_3d_in_context )
	    {
		return tr( "Prestack 3D Data");
	    }

	    return tr( "Prestack Data" );
	}

	return tr("Cube");
    }

    return tr("Data");
}
