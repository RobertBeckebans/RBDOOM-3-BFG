cmake_minimum_required ( VERSION 2.8.11 )
project ( jpeg-9b C )

if ( APPLE )
    set ( CMAKE_MACOSX_RPATH OFF )
    set (
        CMAKE_MODULE_LINKER_FLAGS
        "${CMAKE_MODULE_LINKER_FLAGS} -undefined dynamic_lookup"
    )
endif ( APPLE )

# TODO move install headers to 
add_library (
    jpeg-9b SHARED 
    cderror.h
    cdjpeg.h
    jdct.h
    jerror.h # TODO install
    jinclude.h
    jmemsys.h
    jmorecfg.h # TODO install
    jpegint.h
    jpeglib.h # TODO install
    jversion.h
    transupp.h

    jaricom.c jcapimin.c jcapistd.c jcarith.c jccoefct.c jccolor.c
    jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c
    jcomapi.c jcparam.c jcprepct.c jcsample.c jctrans.c jdapimin.c
    jdapistd.c jdarith.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c
    jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c
    jdmerge.c jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c
    jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jquant1.c
    jquant2.c jutils.c jmemmgr.c jmemnobs.c
)

#if(BUILD_BINARIES)
add_executable(
    cjpeg
    cjpeg.c rdppm.c rdgif.c rdtarga.c
    rdrle.c rdbmp.c rdswitch.c cdjpeg.c
    )

add_executable(
    djpeg
    djpeg.c wrppm.c wrgif.c wrtarga.c wrrle.c
    wrbmp.c rdcolmap.c cdjpeg.c
    )

add_executable(
    jpegtran
    jpegtran.c rdswitch.c cdjpeg.c transupp.c
    )

add_executable(
    rdjpgcom
    rdjpgcom.c
    )

add_executable(
    wrjpgcom
    wrjpgcom.c
    )

target_link_libraries( cjpeg jpeg-9b )
target_link_libraries( djpeg jpeg-9b )
target_link_libraries( jpegtran jpeg-9b )
target_link_libraries( rdjpgcom jpeg-9b )
target_link_libraries( wrjpgcom jpeg-9b )
#endif(BUILD_BINARIES)

set ( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11" )

#target_include_directories (
#    jpeg-9b PUBLIC
#    #${IDTECH4_INCLUDE_DIRS}
#)

#install (
#    TARGETS jpeg-9b
#    LIBRARY DESTINATION "lib/${CMAKE_PROJECT_NAME}/libs"
#)
