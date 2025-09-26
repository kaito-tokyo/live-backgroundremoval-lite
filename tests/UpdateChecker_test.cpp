/*
Background Removal Lite
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

TEST(UpdateCheckerTest, Fetch)
{
	NullLogger logger;
	UpdateChecker checker(logger);
	auto latestVersion = checker.fetch();
	ASSERT_TRUE(latestVersion.has_value());
	EXPECT_FALSE(latestVersion->empty());
}

TEST(LatestVersionTest, IsUpdateAvailable)
{
	NullLogger logger;
	UpdateChecker checker(logger);

	EXPECT_TRUE(checker.isUpdateAvailable("1.0.0", "0.9.0"));
	EXPECT_FALSE(checker.isUpdateAvailable("1.0.0", "1.0.0"));
	EXPECT_FALSE(checker.isUpdateAvailable("1.0.0", "1.1.0"));

	EXPECT_TRUE(checker.isUpdateAvailable("2.0.0-beta", "1.0.0"));
	EXPECT_TRUE(checker.isUpdateAvailable("2.0.0-beta", "2.0.0-alpha"));
	EXPECT_FALSE(checker.isUpdateAvailable("2.0.0-beta", "2.0.0-beta"));
	EXPECT_FALSE(checker.isUpdateAvailable("2.0.0-beta", "2.0.0"));

	EXPECT_FALSE(checker.isUpdateAvailable("", "1.0.0"));
}
