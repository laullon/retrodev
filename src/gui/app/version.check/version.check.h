// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Version check -- platform-agnostic interface for background update detection.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>

namespace RetrodevGui {

    //
    // Version check result states
    //
    enum class VersionCheckState {
        // No check has been requested yet
        Idle,
        // Background fetch is in progress
        Checking,
        // A newer version is available
        UpdateAvailable,
        // Current version is up to date
        UpToDate,
        // Fetch or parse failed (network unreachable, malformed response, etc.)
        Failed
    };

    //
    // Holds the result produced by the background check thread.
    // Fields are only valid when State == UpdateAvailable.
    //
    struct VersionCheckResult {
        VersionCheckState state = VersionCheckState::Idle;
        //
        // Latest version string as returned by the manifest (e.g. "0.9.42")
        //
        std::string latestVersion;
        //
        // Human-readable release notes URL to show alongside the notification
        //
        std::string releaseNotesUrl;
        //
        // Direct download URL opened when the user clicks "Download"
        //
        std::string downloadUrl;
    };

    //
    // Platform-agnostic version check API.
    // The implementation lives under win/ (and future lnx/, osx/).
    //
    // Fetches the JSON manifest from manifestUrl on a background thread.
    // Expected manifest format:
    //   {
    //     "latestVersion":   "0.9.42",
    //     "releaseNotesUrl": "https://...",
    //     "downloadUrl":     "https://..."
    //   }
    //
    class VersionCheck {
    public:
        //
        // Start the background fetch.  Safe to call multiple times -- a check
        // already in progress is ignored.  manifestUrl must be HTTPS.
        //
        static void StartAsync(const std::string& manifestUrl);
        //
        // Poll the result of the last async check.  Safe to call every frame.
        // Returns the current state; result is populated when UpdateAvailable.
        //
        static VersionCheckResult Poll();
        //
        // Cancel any in-progress check and reset to Idle.
        // Must be called before application shutdown.
        //
        static void Shutdown();
    };

}
