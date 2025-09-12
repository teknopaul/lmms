INCLUDE(BuildPlugin)

BUILD_PLUGIN(beziersynth BezierSynth.cpp BezierOsc.cpp BezierOsc.h BezierSynth.h MOCFILES BezierOsc.h BezierSynth.h EMBEDDED_RESOURCES *.png)
