# Copyright (c) 2023 teknopaul

include(ImportedTargetHelpers)

find_package_config_mode_with_fallback(RubberBand RubberBand::RubberBandStretcher
	LIBRARY_NAMES "rubberband" "librubberband" "librubberband-1"
	INCLUDE_NAMES "rubberband/RubberBandStretcher.h"
	PKG_CONFIG rubberband
)

determine_version_from_source(RubberBand_VERSION RubberBand::RubberBandStretcher [[
	#include <iostream>
	#include <rubberband/RubberBandStretcher.h>

	auto main() -> int
	{
		std::cout << std::to_string(RUBBERBAND_API_MAJOR_VERSION) < "." << std::to_string(RUBBERBAND_API_MINOR_VERSION)
	}
]])

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(RubberBand
	REQUIRED_VARS RubberBand_LIBRARY RubberBand_INCLUDE_DIRS
	VERSION_VAR RubberBand_VERSION
)
