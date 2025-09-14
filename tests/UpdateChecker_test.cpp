/*
obs-showdraw
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include "UpdateChecker/UpdateChecker.hpp"

#include <gtest/gtest.h>

#include "NullLogger.hpp"

using namespace kaito_tokyo::obs_backgroundremoval_lite;

// TEST(UpdateCheckerTest, Fetch)
// {
// 	NullLogger logger;
// 	UpdateChecker checker(logger);
// 	auto latestVersion = checker.fetch();
// 	ASSERT_TRUE(latestVersion.has_value());
// 	EXPECT_FALSE(latestVersion->toString().empty());
// }

// TEST(LatestVersionTest, IsUpdateAvailable)
// {
// 	LatestVersion v100("1.0.0");
// 	EXPECT_TRUE(v100.isUpdateAvailable("0.9.0"));
// 	EXPECT_FALSE(v100.isUpdateAvailable("1.0.0"));
// 	EXPECT_FALSE(v100.isUpdateAvailable("1.1.0"));

// 	LatestVersion v200b("2.0.0-beta");
// 	EXPECT_TRUE(v200b.isUpdateAvailable("1.0.0"));
// 	EXPECT_TRUE(v200b.isUpdateAvailable("2.0.0-alpha"));
// 	EXPECT_FALSE(v200b.isUpdateAvailable("2.0.0-beta"));
// 	EXPECT_FALSE(v200b.isUpdateAvailable("2.0.0"));

// 	LatestVersion vempty("");
// 	EXPECT_FALSE(vempty.isUpdateAvailable("1.0.0"));
// }
