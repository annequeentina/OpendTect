#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2014
________________________________________________________________________

-*/

#include "basicmod.h"


namespace OD
{

/*!\brief Fundamental orientation in 2D UIs */

enum Orientation
{
    Horizontal=0,
    Vertical=1
};


/*!\brief State of check objects */

enum CheckState
{
    Unchecked=0,
    PartiallyChecked=1,
    Checked=2
};


/*!\brief OpendTect flat slice types */

enum SliceType
{
    InlineSlice=0,
    CrosslineSlice=1,
    ZSlice=2
};


/*!\brief What to choose from any list-type UI object */

enum ChoiceMode
{
    ChooseNone=0,
    ChooseOnlyOne=1,
    ChooseAtLeastOne=2,
    ChooseZeroOrMore=3
};


/*!\brief How to select files or directories */

enum FileSelectionMode
{
    SelectFileForRead,	/*!< The name of a single existing file. */
    SelectFileForWrite,	/*!< The name of a file, whether it exists or not. */
    SelectMultiFile,	/*!< The names of zero or more existing files. */
    SelectDirectory	/*!< The name of a directory. */
};


/*!\brief File content types, for which operations may be known. */

enum FileContentType
{
    GeneralContent=0,
    ImageContent,
    TextContent,
    HtmlContent
};


/*!\brief Actions that appear often on buttons and menus. */

enum StdActionType
{
    NoIcon=0,
    Apply,
    Cancel,
    Define,
    Delete,
    Edit,
    Help,
    Ok,
    Options,
    Properties,
    Examine,
    Rename,
    Remove,
    Save,
    SaveAs,
    Select,
    Settings,
    Unload,
    Reload
};

} // namespace OD


inline bool isHorizontal( OD::Orientation orient )
				{ return orient == OD::Horizontal; }
inline bool isVertical( OD::Orientation orient )
				{ return orient == OD::Vertical; }
inline bool isMultiChoice( OD::ChoiceMode cm )
				{ return cm > 1; }
inline bool isOptional( OD::ChoiceMode cm )
				{ return cm == OD::ChooseZeroOrMore; }
inline bool isFile( OD::FileSelectionMode mode )
				{ return mode != OD::SelectDirectory; }
inline bool isDirectory( OD::FileSelectionMode mode )
				{ return mode == OD::SelectDirectory; }
inline bool isSingle( OD::FileSelectionMode mode )
				{ return mode != OD::SelectMultiFile; }
inline bool isForRead( OD::FileSelectionMode mode )
				{ return mode != OD::SelectFileForWrite; }
