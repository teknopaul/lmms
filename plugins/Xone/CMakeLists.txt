INCLUDE(BuildPlugin)

BUILD_PLUGIN(xone Xone.h XoneControlDialog.h XoneControls.h Xone.cpp XoneControls.cpp XoneControlDialog.cpp MOCFILES Xone.h XoneControls.h XoneControlDialog.h EMBEDDED_RESOURCES artwork.png logo.png)
