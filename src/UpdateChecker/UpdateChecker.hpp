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

#include <plugin-support.h>
#include <obs.h>

#include "obs-bridge-utils/Logger.hpp"

#include "MyCprUtils.hpp"

namespace kaito_tokyo {
namespace obs_backgroundremoval_lite {

class LatestVersion {
public:
	/**
     * @brief Constructs a LatestVersion object.
     * @param version The version string (e.g., "1.2.3").
     */
	explicit LatestVersion(const std::string &_version, const kaito_tokyo::obs_bridge_utils::ILogger &logger)
		: version(_version),
		  logger(logger)
	{
	}

	/**
     * @brief Compares this version with the current version to check for updates.
     * @param currentVersion The version string of the currently running software.
     * @return True if an update is available, false otherwise.
     */
	bool isUpdateAvailable(const std::string &currentVersion) const noexcept
	{
		if (version.empty()) {
			logger.info("Latest version information is not available.");
			return false;
		}

		semver::version latest, current;

		if (!semver::parse(version, latest)) {
			logger.warn("Failed to parse latest version: " + version);
			return false;
		}

		if (!semver::parse(currentVersion, current)) {
			obs_log(LOG_WARNING, "[obs-showdraw] Failed to parse current version: %s",
				currentVersion.data());
			return false;
		}

		return latest > current;
	}

	/**
     * @brief Gets the version string.
     * @return A constant reference to the version string.
     */
	const std::string &toString() const noexcept { return version; }

private:
	const std::string version;
	const kaito_tokyo::obs_bridge_utils::ILogger &logger;
};

/**
 * @brief Performs a synchronous check for software updates.
 *
 * @note The calling context is expected to handle any desired asynchronous
 * execution.
 */
class UpdateChecker {
public:
	UpdateChecker() = default;

	/**
     * @brief Fetches the latest version information from the remote server.
     * @return An std::optional<LatestVersion> containing the latest version if successful,
     * or std::nullopt on failure.
     */
	std::optional<LatestVersion> fetch()
	{
		// Use a fully qualified name to avoid `using namespace` in a header file.
		// MyCprSession is assumed to be in the kaito_tokyo::obs_backgroundremoval_lite namespace.
		kaito_tokyo::obs_backgroundremoval_lite::MyCprSession session;
		session.SetUrl(cpr::Url{"https://obs-backgroundremoval-lite.kaito.tokyo/metadata/latest-version.txt"});
		cpr::Response r = session.Get();

		if (r.status_code == 200) {
			return LatestVersion(r.text);
		} else {
			obs_log(LOG_WARNING, "[obs-showdraw] Failed to fetch latest version information: HTTP %ld",
				r.status_code);
			return std::nullopt;
		}
	}
};

} // namespace obs_backgroundremoval_lite
} // namespace kaito_tokyo
