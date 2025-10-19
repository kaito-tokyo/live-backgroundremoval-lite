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
	GuidedFilter = 300,
	TimeAveragedFilter = 400,
};

struct PluginProperty {
	int ncnnGpuIndex = -1;
	int ncnnNumThreads = 2;

	FilterLevel filterLevel = FilterLevel::Default;

	int selfieSegmenterFps = 15;

	int subsamplingRate = 4;

	double guidedFilterEpsPowDb = -40.0;

	double maskGamma = 2.5;
	double maskLowerBoundAmpDb = -25.0;
	double maskUpperBoundMarginAmpDb = -25.0;

	double timeAveragedFilteringAlpha = 0.1;
};

} // namespace LiveBackgroundRemovalLite
} // namespace KaitoTokyo
