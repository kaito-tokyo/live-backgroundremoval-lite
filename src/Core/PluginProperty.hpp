/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

namespace KaitoTokyo {
namespace LiveBackgroundRemovalLite {

enum class FilterLevel : int {
	Default = 0,
	Passthrough = 100,
	Segmentation = 200,
	MotionIntensityThresholding = 300,
	GuidedFilter = 400,
	TimeAveragedFilter = 500,
};

struct PluginProperty {
	int numThreads = 2;
	int subsamplingRate = 4;

	FilterLevel filterLevel = FilterLevel::Default;

	double motionIntensityThresholdPowDb = -40.0;

	double guidedFilterEpsPowDb = -40.0;

	double timeAveragedFilteringAlpha = 0.25;

	double maskGamma = 2.5;
	double maskLowerBoundAmpDb = -25.0;
	double maskUpperBoundMarginAmpDb = -25.0;
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
