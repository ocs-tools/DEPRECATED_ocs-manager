include($${PWD}/qtil/qtil.pri)

unix:!ios:!android {
    INCLUDEPATH += $${PWD}/AppImageUpdate/include

    DEPENDPATH += $${PWD}/AppImageUpdate/include

    LIBS += \
        -L$${PWD}/AppImageUpdate-prebuilt/src/ -lappimageupdate \
        -L$${PWD}/AppImageUpdate-prebuilt/src/elf/ -lelf \
        -L$${PWD}/AppImageUpdate-prebuilt/lib/zsync2/src/ -lzsync2 \
        -L$${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/libzsync/ -lzsync \
        -L$${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/librcksum/ -lrcksum \
        -L$${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/zlib/ -lzsync2_libz \
        -L$${PWD}/AppImageUpdate-prebuilt/lib/ -lcpr \
        $${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/hashlib/hashlib.a

    PRE_TARGETDEPS += \
        $${PWD}/AppImageUpdate-prebuilt/src/libappimageupdate.a \
        $${PWD}/AppImageUpdate-prebuilt/src/elf/libelf.a \
        $${PWD}/AppImageUpdate-prebuilt/lib/zsync2/src/libzsync2.a \
        $${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/libzsync/libzsync.a \
        $${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/librcksum/librcksum.a \
        $${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/zlib/libzsync2_libz.a \
        $${PWD}/AppImageUpdate-prebuilt/lib/libcpr.a \
        $${PWD}/AppImageUpdate-prebuilt/lib/zsync2/lib/hashlib/hashlib.a

    LIBS += -lcurl
}
