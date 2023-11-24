INCLUDE(BuildPlugin)

LINK_LIBRARIES(${RubberBand_LIBRARY})
BUILD_PLUGIN(voxpop Voxpop.cpp Voxpop.h MOCFILES Voxpop.h EMBEDDED_RESOURCES *.png)


