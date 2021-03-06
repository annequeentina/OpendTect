#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

# OD_INIT_MODULE - Macro that setups a number of variables for compiling
#		   OpendTect.
#
# Input variables:
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
# OD_SUBSYSTEM                          : "od" or "dgb"
# OD_MODULE_DEPS			: List of other modules that this
#					  module is dependent on.
# OD_MODULE_SOURCES			: Sources that should go into the library
# OD_USEBATCH				: Whether to include include/Batch 
# OD_USECOIN				: Dependency on Coin is enabled if set.
# OD_USEQT				: Dependency on Qt is enabled if set.
#					  value should be either Core, Sql, Gui
#					  or OpenGL
# OD_USEZLIB				: Dependency on zlib is enabled if set.
# OD_USEOSG				: Dependency on OSG is enabled if set.
# OD_IS_PLUGIN				: Tells if this is a plugin (if set)
# OD_PLUGINMODULES			: A list of eventual sub-modules of
#					  a plugin. Each submodule must have an
#					  plugins/{OD_MODULE_NAME}/src/${PLUGINMODULE}/CMakeFile.txt
#####################################
#
# Output variables:
#
# OD_${OD_MODULE_NAME}_INCLUDEPATH 	: The path(s) to the headerfiles of the
#					  module. Normally one single one, but
#					  may have multiple paths in plugins.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_RUNTIMEPATH	: The runtime path for its own library, 
#					  and all external libraries it is
#					  dependent on.
# OD_MODULE_NAMES_${OD_SUBSYSTEM}	: A list of all modules
# OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}  : A list of all non-plugin modules
#####################################
#
# Internal variables
#
# OD_MODULE_INCLUDEPATH		: The includepath needed to compile the source-
#				  files of the module.
# OD_MODULE_INCLUDESYSPATH	: The system includepath needed to compile the
#				  source- files of the module.
# OD_MODULE_RUNTIMEPATH		: All directories that are needed at runtime
# OD_MODULE_INTERNAL_LIBS	: All OD libraries needed for the module
# OD_MODULE_EXTERNAL_LIBS	: All external libraries needed for the module

macro( OD_INIT_MODULE )

get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )

set ( OD_MODULE_HAS_LIBRARY ${OD_MODULE_SOURCES} )

#test for keywords not allowed in source

#Add this module to the list
if ( OD_MODULE_HAS_LIBRARY )
    set( OD_MODULE_NAMES_${OD_SUBSYSTEM} ${OD_MODULE_NAMES_${OD_SUBSYSTEM}}
					 ${OD_MODULE_NAME} PARENT_SCOPE )

    #Create init-header 
    OD_CREATE_INIT_HEADER()
endif(  OD_MODULE_HAS_LIBRARY )

#Add all module dependencies
set( OD_${OD_MODULE_NAME}_DEPS ${OD_MODULE_DEPS} ${OD_EXT_MODULE_DEPS} )

#Setup all deps and set runtime and includepath
if( OD_MODULE_DEPS )
    OD_GET_ALL_DEPS( ${OD_MODULE_NAME} OD_MODULE_INTERNAL_LIBS )
    foreach ( DEP ${OD_MODULE_INTERNAL_LIBS} )
	#Add dependencies to include-path
	if( WIN32 )
	    if ( ${DEP} MATCHES dGBCommon )
		set( OD_MODULE_LINK_OPTIONS /NODEFAULTLIB:\"libcmt.lib\" )
	    endif()
	endif()
	list(APPEND OD_MODULE_INCLUDEPATH ${OD_${DEP}_INCLUDEPATH} )
	list(APPEND OD_MODULE_RUNTIMEPATH ${OD_${DEP}_RUNTIMEPATH} )
    endforeach()
endif()

if(OD_USEOSG)
    OD_SETUP_OSG()
endif()

#Add Qt-stuff
if(OD_USEQT)
   OD_SETUP_QT()
endif(OD_USEQT)

#Must be after QT
if( (UNIX OR WIN32)  AND OD_USEZLIB )
    OD_SETUP_ZLIB()
    if ( EXISTS ${ZLIB_INCLUDE_DIR} )
	list(APPEND OD_MODULE_INCLUDESYSPATH ${ZLIB_INCLUDE_DIR} )
	list(APPEND OD_MODULE_EXTERNAL_LIBS ${ZLIB_LIBRARY} )
    endif()
endif()

if(OD_USEPROJ4)
    OD_SETUP_PROJ4()
endif()

#Add current module to include-path
set( CMAKE_BINARY_DIR "${OD_BINARY_BASEDIR}" )
if ( OD_MODULE_HAS_LIBRARY )
    if (OD_IS_PLUGIN)
        set( PLUGINDIR ${CMAKE_CURRENT_SOURCE_DIR} )
	list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH ${PLUGINDIR} ${INITHEADER_DIR} )
	foreach ( OD_PLUGINSUBDIR ${OD_PLUGINMODULES} )
	    list(APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
		${PLUGINDIR}/include/${OD_PLUGINSUBDIR})
	    include(${PLUGINDIR}/src/${OD_PLUGINSUBDIR}/CMakeLists.txt)
	endforeach()

	# Record alo-entries
	if ( NOT DEFINED OD_NO_ALO_ENTRY )
	    set( OD_ALO_NAME ${OD_MODULE_NAME} )
	    OD_ADD_ALO_ENTRIES( ${OD_PLUGIN_ALO_EXEC} )
	endif()
    else()
	set( OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}
	     ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}}
		${OD_MODULE_NAME} PARENT_SCOPE )
	list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
	    ${INITHEADER_DIR} )

	if ( EXISTS ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	    list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH 
		${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	endif()

	if ( EXISTS ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	    list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH 
		${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	endif()
    endif(OD_IS_PLUGIN)

    list( REMOVE_DUPLICATES OD_${OD_MODULE_NAME}_INCLUDEPATH )

    #Add all headerfiles to be included in the library (nice in IDEs)
    foreach ( INCDIR ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )
	file ( GLOB INCFILES ${INCDIR}/*.h )
	file ( GLOB XPMFILES ${INCDIR}/*.xpm )
	file ( GLOB INFILES ${INCDIR}/*.in )
	set( INCFILES ${INCFILES} ${XPMFILES} ${INFILES} )
	if ( INCFILES )
	    list ( APPEND OD_MODULE_INCFILES ${INCFILES} )
	endif( INCFILES )
    endforeach()

    if (NOT OD_IS_PLUGIN)
	file ( GLOB SRCHEADERFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h )
    endif()

endif ( OD_MODULE_HAS_LIBRARY )

list ( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${OD_LIB_OUTPUT_PATH} )
guess_runtime_library_dirs( EXTERNAL_RUNTIMEPATH ${OD_MODULE_EXTERNAL_LIBS} )
list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${EXTERNAL_RUNTIMEPATH} )
list( APPEND OD_MODULE_INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )
list( APPEND OD_MODULE_RUNTIMEPATH ${OD_${OD_MODULE_NAME}_RUNTIMEPATH} )
list( REMOVE_DUPLICATES OD_${OD_MODULE_NAME}_RUNTIMEPATH )

#Clean up the lists
list( REMOVE_DUPLICATES OD_MODULE_INCLUDEPATH )
if( OD_MODULE_RUNTIMEPATH )
    list( REMOVE_DUPLICATES OD_MODULE_RUNTIMEPATH)
endif()

if ( OD_MODULE_HAS_LIBRARY )
    #Export dependencies
    set( OD_${OD_MODULE_NAME}_DEPS ${OD_MODULE_DEPS} PARENT_SCOPE )
    set( OD_${OD_MODULE_NAME}_INCLUDEPATH
	 ${OD_${OD_MODULE_NAME}_INCLUDEPATH} PARENT_SCOPE)
    set( OD_${OD_MODULE_NAME}_RUNTIMEPATH
	 ${OD_${OD_MODULE_NAME}_RUNTIMEPATH} PARENT_SCOPE)
endif( OD_MODULE_HAS_LIBRARY )

#Extract static libraries
foreach( STATIC_LIB ${OD_MODULE_STATIC_LIBS} )
    if ( NOT EXISTS ${STATIC_LIB} )
	message( FATAL_ERROR "${STATIC_LIB} does not exist" )
    endif()

    get_filename_component( STATIC_LIB_NAME ${STATIC_LIB} NAME )
    set( STATIC_LIB_DIR
         ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${OD_MODULE_NAME}.dir/${STATIC_LIB_NAME}.dir )
    if ( WIN32 )
	set ( SHARED_LIB_COMMAND ${OpendTect_DIR}/dtect/extract_static_lib.cmd ${STATIC_LIB} ${OD_PLFSUBDIR} )
    else()
	set ( SHARED_LIB_COMMAND ${CMAKE_AR} x ${STATIC_LIB} )
    endif()
    if ( NOT EXISTS ${STATIC_LIB_DIR} )
	file ( MAKE_DIRECTORY ${STATIC_LIB_DIR} )
    endif()

    execute_process( 
	COMMAND ${SHARED_LIB_COMMAND}
	WORKING_DIRECTORY ${STATIC_LIB_DIR} )

    if ( WIN32 )
	file ( GLOB STATIC_LIB_FILES ${STATIC_LIB_DIR}/*.obj )
    else()
	file( GLOB STATIC_LIB_FILES ${STATIC_LIB_DIR}/*${CMAKE_C_OUTPUT_EXTENSION} )
    endif()

    list( APPEND OD_STATIC_OUTFILES ${STATIC_LIB_FILES} )

    add_custom_command( OUTPUT ${STATIC_LIB_FILES}
	        COMMAND ${SHARED_LIB_COMMAND}
                WORKING_DIRECTORY ${STATIC_LIB_DIR}
		DEPENDS ${STATIC_LIB}
                COMMENT "Extracting shared library ${STATIC_LIB_NAME}" )

    foreach( STATIC_LIB_FILE ${STATIC_LIB_FILES} )
	set_property( DIRECTORY APPEND
		PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${STATIC_LIB_FILE} )
    endforeach()

endforeach()

#Setup library & its deps
if( OD_MODULE_HAS_LIBRARY AND OD_LIB_LINKER_NEEDS_ALL_LIBS )
    set( OD_LIB_DEP_LIBS ${EXTRA_LIBS} ${OD_MODULE_DEPS} )
    if ( NOT OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	list( APPEND OD_LIB_DEP_LIBS ${OD_MODULE_INTERNAL_LIBS} )
    endif()
else()
    set( OD_EXEC_DEP_LIBS ${EXTRA_LIBS} ${OD_MODULE_INTERNAL_LIBS} )
endif()

if ( OD_MODULE_HAS_LIBRARY )
    add_library( ${OD_MODULE_NAME} SHARED ${OD_MODULE_SOURCES}
		 ${QT_MOC_OUTFILES}
		 ${OD_MODULE_INCFILES}
		 ${OD_STATIC_OUTFILES} )
    target_link_libraries(
	    ${OD_MODULE_NAME}
	    ${OD_LIB_DEP_LIBS}
	    ${OD_MODULE_EXTERNAL_LIBS}
	 )

    set ( TARGET_PROPERTIES ${OD_MODULE_NAME}
	    PROPERTIES 
	    LINK_FLAGS "${OD_PLATFORM_LINK_OPTIONS} ${OD_MODULE_LINK_OPTIONS}"
	    LABELS ${OD_MODULE_NAME}
	    ARCHIVE_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_DEBUG}" 
	    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_DEBUG}"
	    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_DEBUG}"
	    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_RELEASE}"
	    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_RELEASE}"
	    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_RELEASE}"
	    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OD_LIB_OUTPUT_RELPATH}"
	    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OD_LIB_OUTPUT_RELPATH}"
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OD_LIB_OUTPUT_RELPATH}" )

    if ( OD_SET_TARGET_PROPERTIES )
	list ( APPEND TARGET_PROPERTIES 
	    VERSION ${OD_BUILD_VERSION}
	    SOVERSION ${OD_API_VERSION} )
    endif( OD_SET_TARGET_PROPERTIES )

    set_target_properties( ${TARGET_PROPERTIES} )
    OD_GENERATE_SYMBOLS( ${OD_MODULE_NAME} )

    install( TARGETS
	    ${OD_MODULE_NAME}
	    RUNTIME
		DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}
		CONFIGURATIONS "Debug"
	    LIBRARY
		DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}
		CONFIGURATIONS "Debug"
	    ARCHIVE DESTINATION lib )
    install( TARGETS
	    ${OD_MODULE_NAME}
	    RUNTIME
		DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
		CONFIGURATIONS Release
	    LIBRARY
		DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
		CONFIGURATIONS Release
	    ARCHIVE DESTINATION lib )

    #Add to list of all files
    OD_ADD_SOURCE_FILES( ${OD_MODULE_SOURCES} ${OD_MODULE_INCFILES} ${SRCHEADERFILES} )

endif ( OD_MODULE_HAS_LIBRARY )

OD_ADD_SOURCE_FILES( CMakeLists.txt )

#Setup common things for batch-programs
if( OD_MODULE_BATCHPROGS OR OD_MODULE_GUI_PROGS )
    set ( OD_USEBATCH 1 )
    OD_ADD_SOURCE_FILES( ${OD_MODULE_PROGS} ${OD_MODULE_BATCHPROGS}
			 ${OD_MODULE_GUI_PROGS} )
endif()

set ( OD_RUNTIMELIBS ${OD_MODULE_DEPS})
if ( OD_MODULE_HAS_LIBRARY )
    list ( APPEND OD_RUNTIMELIBS ${OD_MODULE_NAME} )
endif ( OD_MODULE_HAS_LIBRARY )

#Add executable targets
if( OD_MODULE_PROGS OR OD_MODULE_GUI_PROGS OR OD_ELEVATED_PERMISSIONS_PROGS OR OD_ELEVATED_PERMISSIONS_GUI_PROGS )

    foreach ( EXEC ${OD_MODULE_PROGS} ${OD_MODULE_GUI_PROGS} ${OD_ELEVATED_PERMISSIONS_PROGS} ${OD_ELEVATED_PERMISSIONS_GUI_PROGS} )
	get_filename_component( TARGET_NAME ${EXEC} NAME_WE )

	#Check if from GUI list
	list ( FIND OD_MODULE_GUI_PROGS ${EXEC} INDEX )
	if ( NOT ${INDEX} EQUAL -1 )
	    set( OD_EXEC_GUI_SYSTEM ${OD_GUI_SYSTEM} )
	endif()
	list ( FIND OD_ELEVATED_PERMISSIONS_GUI_PROGS ${EXEC} INDEX )
	if ( NOT ${INDEX} EQUAL -1 )
	    set( OD_EXEC_GUI_SYSTEM ${OD_GUI_SYSTEM} )
	endif()

	add_executable( ${TARGET_NAME} ${OD_EXEC_GUI_SYSTEM} ${EXEC} 
			${OD_${TARGET_NAME}_RESOURCE} )
	if ( OD_EXECUTABLE_COMPILE_FLAGS )
	    set_source_files_properties( ${EXEC} PROPERTIES COMPILE_FLAGS
				     ${OD_EXECUTABLE_COMPILE_FLAGS} )
	endif( OD_EXECUTABLE_COMPILE_FLAGS )

	set ( OD_LINK_FLAGS "${OD_PLATFORM_LINK_OPTIONS} ${OD_MODULE_LINK_OPTIONS}" )
	
	#Check if from Elevated permission list
	if ( WIN32 )
	    list ( FIND OD_ELEVATED_PERMISSIONS_PROGS ${EXEC} INDEX )
	    if ( NOT ${INDEX} EQUAL -1 )
		set ( OD_LINK_FLAGS "${OD_LINK_FLAGS} ${OD_UAC_LINKFLAGS}" )
	    endif()
	    list ( FIND OD_ELEVATED_PERMISSIONS_GUI_PROGS ${EXEC} INDEX )
	    if ( NOT ${INDEX} EQUAL -1 )
		set ( OD_LINK_FLAGS "${OD_LINK_FLAGS} ${OD_UAC_LINKFLAGS}" )
	    endif()
	endif()
			
	set( TARGET_PROPERTIES ${TARGET_NAME}
	    PROPERTIES 
	    LINK_FLAGS "${OD_LINK_FLAGS}"
	    LABELS ${OD_MODULE_NAME}
	    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG}"
	    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE}"
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OD_EXEC_OUTPUT_RELPATH}" )

	if ( OD_SET_TARGET_PROPERTIES )
	    list ( APPEND TARGET_PROPERTIES 
		VERSION ${OD_BUILD_VERSION} )
	endif( OD_SET_TARGET_PROPERTIES )
	if ( WIN32 AND OD_SET_LINKFLAGS_UAC )
	    list ( APPEND TARGET_PROPERTIES LINK_FLAGS_RELEASE ${OD_UAC_LINKFLAGS} )
	endif()

	set_target_properties( ${TARGET_PROPERTIES} )
	target_link_libraries(
	    ${TARGET_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${OD_RUNTIMELIBS} )
        if( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TARGET_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH}
		${OD_EXEC_OUTPUT_PATH}
		ENVIRONMENT
		WORK=${OD_BINARY_BASEDIR})
        endif( OD_CREATE_LAUNCHERS )
	install( TARGETS
		${TARGET_NAME}
		RUNTIME DESTINATION ${OD_EXEC_INSTALL_PATH_DEBUG} 
		CONFIGURATIONS "Debug" )
	install( TARGETS
		${TARGET_NAME}
		RUNTIME DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE} 
		CONFIGURATIONS "Release" )

	OD_GENERATE_SYMBOLS( ${TARGET_NAME} )

    endforeach()

endif()

if(OD_MODULE_BATCHPROGS)
    #Add dep on Batch if there are batch-progs
    set ( PROGRAM_RUNTIMELIBS ${OD_RUNTIMELIBS} "Batch" "Network" )

    foreach ( EXEC ${OD_MODULE_BATCHPROGS} )
	get_filename_component( TARGET_NAME ${EXEC} NAME_WE )
	add_executable( ${TARGET_NAME} ${EXEC} )
	if ( OD_EXECUTABLE_COMPILE_FLAGS )
	    set_source_files_properties( ${EXEC} PROPERTIES COMPILE_FLAGS
				     ${OD_EXECUTABLE_COMPILE_FLAGS} )
	endif( OD_EXECUTABLE_COMPILE_FLAGS )
	target_link_libraries(
	    ${TARGET_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${PROGRAM_RUNTIMELIBS} )

	set( TARGET_PROPERTIES ${TARGET_NAME}
	    PROPERTIES 
	    COMPILE_DEFINITIONS __prog__
	    LINK_FLAGS "${OD_PLATFORM_LINK_OPTIONS} ${OD_MODULE_LINK_OPTIONS}"
	    LABELS ${OD_MODULE_NAME}
	    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG}"
	    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE}"
	    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${OD_EXEC_OUTPUT_RELPATH}" )

	if ( OD_SET_TARGET_PROPERTIES )
	    list ( APPEND TARGET_PROPERTIES 
		VERSION ${OD_BUILD_VERSION} )
	endif( OD_SET_TARGET_PROPERTIES )

	set_target_properties( ${TARGET_PROPERTIES} )

	if( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TARGET_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH} )
	endif( OD_CREATE_LAUNCHERS )
	install( TARGETS
		${TARGET_NAME}
		RUNTIME DESTINATION ${OD_EXEC_INSTALL_PATH_DEBUG} 
		CONFIGURATIONS "Debug" )
	install( TARGETS
		${TARGET_NAME}
		RUNTIME DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE} 
		CONFIGURATIONS "Release" )

	OD_GENERATE_SYMBOLS( ${TARGET_NAME} )
    endforeach()

endif( OD_MODULE_BATCHPROGS )

set ( OD_MODULE_TESTS_IGNORE_CONTINUOUS "" )
set ( OD_MODULE_TESTS_IGNORE_EXPERIMENTAL "" )
foreach ( TEST_FILE ${OD_NIGHTLY_TEST_PROGS} ${OD_BATCH_TEST_PROGS} )
    get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
    set ( TEST_NAME test_${TEST_NAME} )

    set ( OD_MODULE_TESTS_IGNORE_CONTINUOUS ${OD_MODULE_TESTS_IGNORE_CONTINUOUS} ${TEST_NAME} )
    set ( OD_MODULE_TESTS_IGNORE_EXPERIMENTAL ${OD_MODULE_TESTS_IGNORE_EXPERIMENTAL} ${TEST_NAME} )
endforeach()

set ( OD_TESTS_IGNORE_CONTINUOUS ${OD_TESTS_IGNORE_CONTINUOUS} ${OD_MODULE_TESTS_IGNORE_CONTINUOUS} PARENT_SCOPE )
set ( OD_TESTS_IGNORE_EXPERIMENTAL ${OD_TESTS_IGNORE_EXPERIMENTAL} ${OD_MODULE_TESTS_IGNORE_EXPERIMENTAL} PARENT_SCOPE )

foreach ( TEST_FILE ${OD_TEST_PROGS} ${OD_BATCH_TEST_PROGS} ${OD_NIGHTLY_TEST_PROGS} )

    set ( PROGRAM_RUNTIMELIBS ${OD_RUNTIMELIBS} )

    #Add dep on Batch if there are batch-progs
    set ( INDEX -1 )
    if ( OD_BATCH_TEST_PROGS )
	list ( FIND OD_BATCH_TEST_PROGS ${TEST_FILE} INDEX )
    endif()

    if ( NOT (${INDEX} EQUAL -1) )
	list( APPEND PROGRAM_RUNTIMELIBS "Batch" "Network" )
	set( EXTRA_TARGET_PROP COMPILE_DEFINITIONS __prog__ )
    endif()

    get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
    set ( PARAMETER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tests/${TEST_NAME}.par )
    set ( TEST_NAME test_${TEST_NAME} )
    add_executable( ${TEST_NAME} ${OD_EXEC_GUI_SYSTEM} tests/${TEST_FILE} )
    if ( OD_EXECUTABLE_COMPILE_FLAGS )
	set_source_files_properties( tests/${TEST_FILE} PROPERTIES COMPILE_FLAGS
				 ${OD_EXECUTABLE_COMPILE_FLAGS} )
    endif( OD_EXECUTABLE_COMPILE_FLAGS )
    OD_ADD_SOURCE_FILES( tests/${TEST_FILE} )

    set_target_properties( ${TEST_NAME}
	    PROPERTIES 
	    ${EXTRA_TARGET_PROP}
	    LINK_FLAGS "${OD_PLATFORM_LINK_OPTIONS} ${OD_MODULE_LINK_OPTIONS}"
	    LABELS ${OD_MODULE_NAME}
	    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG}"
	    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE}" )
    target_link_libraries(
	    ${TEST_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${PROGRAM_RUNTIMELIBS} )
    if( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TEST_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH}
		${OD_EXEC_OUTPUT_PATH}
		ENVIRONMENT
		WORK=${OD_BINARY_BASEDIR})
    endif( OD_CREATE_LAUNCHERS )

    ADD_TEST_PROGRAM( ${TEST_NAME} )

    set_property( TEST ${TEST_NAME} PROPERTY ${OD_MODULE_NAME} )
endforeach()


if( OD_USEBATCH )
    list(APPEND OD_MODULE_INCLUDEPATH ${OD_Batch_INCLUDEPATH} )
endif( OD_USEBATCH )

#Set current include_path
include_directories( SYSTEM ${OD_MODULE_INCLUDESYSPATH} )
include_directories( ${OD_MODULE_INCLUDEPATH} )

endmacro( OD_INIT_MODULE )

# OD_GET_ALL_DEPS( MODULE LISTNAME ) - dumps all deps to MODULE into LISTNAME
macro( OD_GET_ALL_DEPS MODULE DEPS )
    foreach ( DEPLIB ${OD_${MODULE}_DEPS} )
	OD_GET_ALL_DEPS_ADD( ${DEPLIB} ${DEPS} )
    endforeach()
endmacro( OD_GET_ALL_DEPS )

macro( OD_GET_ALL_DEPS_ADD DEP DEPLIST )
    list ( FIND ${DEPLIST} ${DEP} INDEX )
    if ( ${INDEX} EQUAL -1 )
        list ( APPEND ${DEPLIST} ${DEP} )
        foreach ( DEPLIB ${OD_${DEP}_DEPS} )
            OD_GET_ALL_DEPS_ADD( ${DEPLIB} ${DEPLIST} )
        endforeach()
    endif()

endmacro ( OD_GET_ALL_DEPS_ADD )



# OD_ADD_PLUGIN_SOURCES(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_SOURCES

macro ( OD_ADD_PLUGIN_SOURCES )
    foreach ( SRC ${ARGV} )
	list( APPEND OD_MODULE_SOURCES src/${OD_PLUGINSUBDIR}/${SRC} )
    endforeach()
endmacro()

# OD_ADD_PLUGIN_EXECS(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_PROGS

macro ( OD_ADD_PLUGIN_EXECS )
    foreach ( SRC ${ARGV} )
	list( APPEND OD_MODULE_PROGS src/${OD_PLUGINSUBDIR}/${SRC} )
    endforeach()
endmacro()


# OD_ADD_PLUGIN_BATCHPROGS(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_BATCHPROGS

macro ( OD_ADD_PLUGIN_BATCHPROGS )
    foreach ( SRC ${ARGV} )
	list( APPEND OD_MODULE_BATCHPROGS src/${OD_PLUGINSUBDIR}/${SRC} )
    endforeach()
endmacro()

#Get current year
macro ( OD_CURRENT_YEAR RESULT)
    if (WIN32)
        execute_process(COMMAND "powershell" "(get-date).year" OUTPUT_VARIABLE ${RESULT})
    elseif(UNIX)
        execute_process(COMMAND "date" "+%Y" OUTPUT_VARIABLE ${RESULT})
    endif()
    string(REPLACE "\n" "" "${RESULT}" ${${RESULT}} ) 
endmacro (OD_CURRENT_YEAR )

macro ( OD_CURRENT_MONTH RESULT )
    if (WIN32)
	execute_process(COMMAND "powershell" "(get-date).month" OUTPUT_VARIABLE ${RESULT})
    elseif( UNIX )
	execute_process(COMMAND "date" "+%B" OUTPUT_VARIABLE ${RESULT})
	string(REPLACE "\n" "" "${RESULT}" ${${RESULT}} )
    endif()
endmacro( OD_CURRENT_MONTH )

macro ( OD_CURRENT_DATE RESULT )
    if ( WIN32 )
	execute_process(COMMAND "powershell" "(get-date)" OUTPUT_VARIABLE ${RESULT})
    elseif( UNIX )
	execute_process(COMMAND "date" "+%c" OUTPUT_VARIABLE ${RESULT})
    endif()
    string( REPLACE "\n" "" ${RESULT} ${${RESULT}} ) 
endmacro ( OD_CURRENT_DATE )


#Adds lists of files to global file-list
macro ( OD_ADD_SOURCE_FILES )
    foreach ( THEFILE ${ARGV} )
	get_filename_component( PATH ${THEFILE} ABSOLUTE )
	string( FIND ${PATH} "${CMAKE_BINARY_DIR}" ISBINARYEDIR )
	if ( "${ISBINARYEDIR}" EQUAL -1 )
	    file ( RELATIVE_PATH RELPATH ${CMAKE_BINARY_DIR} ${PATH} )
	else() #Getting relative path of files generated by cmake under CMAKE_BINARY_DIR
	    file ( RELATIVE_PATH RELPATH ${CMAKE_BINARY_DIR} ${PATH} )
	endif()

	file ( APPEND ${OD_SOURCELIST_FILE} ${RELPATH} "\n" )
    endforeach()
endmacro()
