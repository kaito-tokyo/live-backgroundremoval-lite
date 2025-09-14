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

#pragma once

#include <optional>
#include <string>

#include <cpr/cpr.h>
#include <semver.hpp>

#include <obs-bridge-utils/ILogger.hpp>

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

/**
 * @brief Performs a synchronous check for software updates.
 *
 * @note The calling context is expected to handle any desired asynchronous
 * execution.
 */
template<typename TCprSession = cpr::Session> class UpdateChecker {
public:
	static_assert(std::is_base_of_v<cpr::Session, TCprSession>, "TCprSession must inherit from cpr::Session");

	UpdateChecker(const kaito_tokyo::obs_bridge_utils::ILogger &_logger) : logger(_logger) {}

	/**
     * @brief Fetches the latest version information from the remote server.
     * @return An std::optional<LatestVersion> containing the latest version if successful,
     * or std::nullopt on failure.
     */
	std::optional<std::string> fetch()
	{
		// Use a fully qualified name to avoid `using namespace` in a header file.
		// MyCprSession is assumed to be in the kaito_tokyo::obs_backgroundremoval_lite namespace.
		TCprSession session;
		session.SetUrl(cpr::Url{"https://obs-backgroundremoval-lite.kaito.tokyo/metadata/latest-version.txt"});
		cpr::Response r = session.Get();

		if (r.status_code == 200) {
			return {r.text};
		} else {
			logger.warn("Failed to fetch latest version information: HTTP {}", r.status_code);
			return std::nullopt;
		}
	}

	/**
     * @brief Compares this version with the current version to check for updates.
     * @param currentVersion The version string of the currently running software.
     * @return True if an update is available, false otherwise.
     */
	bool isUpdateAvailable(const std::string &latestVersion, const std::string &currentVersion) const noexcept
	{
		if (latestVersion.empty()) {
			logger.info("Latest version information is not available.");
			return false;
		}

		semver::version latest, current;

		if (!semver::parse(latestVersion, latest)) {
			logger.warn("Failed to parse latest version: {}", latestVersion);
			return false;
		}

		if (!semver::parse(currentVersion, current)) {
			logger.warn("Failed to parse current version: {}", currentVersion);
			return false;
		}

		return latest > current;
	}

private:
	const kaito_tokyo::obs_bridge_utils::ILogger &logger;
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
