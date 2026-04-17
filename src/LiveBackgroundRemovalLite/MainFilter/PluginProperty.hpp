// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter {

enum class FilterLevel : int {
	Default = 0,
	Passthrough = 100,
	Segmentation = 200,
	MotionIntensityThresholding = 300,
	GuidedFilter = 400,
	TimeAveragedFilter = 500,
};

struct PluginProperty {
	int numThreads = 1;
	int subsamplingRate = 4;

	FilterLevel filterLevel = FilterLevel::Default;

	double motionIntensityThresholdPowDb = -40.0;

	double guidedFilterEpsPowDb = -40.0;

	double timeAveragedFilteringAlpha = 0.25;

	double maskGamma = 2.5;
	double maskLowerBoundAmpDb = -25.0;
	double maskUpperBoundMarginAmpDb = -25.0;

	int blurSize = 0;
};

} // namespace KaitoTokyo::LiveBackgroundRemovalLite::MainFilter
