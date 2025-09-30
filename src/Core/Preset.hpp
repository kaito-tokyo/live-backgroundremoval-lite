/*
Background Removal Lite
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

#include <cmath>

namespace KaitoTokyo {
namespace BackgroundRemovalLite {

enum class FilterLevel : int {
	Default = 0,
	Passthrough = 100,
	Segmentation = 200,
	GuidedFilter = 300,
};

struct Preset {
	FilterLevel filterLevel;

	int selfieSegmenterFps;

	double gfEpsDb;
	double gfEps;

	double maskGamma;
	double maskLowerBoundDb;
	double maskLowerBound;
	double maskUpperBoundMarginDb;
	double maskUpperBound;

	static float dbToLinearAmp(float db) noexcept { return std::pow(10.0f, db / 20.0f); }
	static float dbToLinearPow(float db) noexcept { return std::pow(10.0f, db / 10.0f); }
};

} // namespace BackgroundRemovalLite
} // namespace KaitoTokyo
