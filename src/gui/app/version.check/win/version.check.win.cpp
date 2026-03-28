// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Version check -- Windows implementation using WinHTTP + std::thread.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#include <app/version.check/version.check.h>
#include <system/version.h>
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>

#pragma comment(lib, "winhttp.lib")

namespace RetrodevGui {

    //
    // Shared state between the background thread and the main thread.
    // Written only by the background thread, read by Poll() on the main thread.
    //
    static std::mutex				s_mutex;
    static VersionCheckResult		s_result;
    static std::atomic<bool>		s_running{ false };
    static std::thread				s_thread;

    //
    // Minimal JSON field extractor -- reads the value of "key" from a flat JSON object.
    // Only handles string values.  Avoids pulling in a full JSON parser for this tiny manifest.
    //
    static std::string ExtractJsonString(const std::string& json, const std::string& key) {
        //
        // Find "key" : "value" -- tolerates spaces around the colon
        //
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos)
            return {};
        size_t colonPos = json.find(':', keyPos + searchKey.size());
        if (colonPos == std::string::npos)
            return {};
        size_t openQuote = json.find('"', colonPos + 1);
        if (openQuote == std::string::npos)
            return {};
        size_t closeQuote = json.find('"', openQuote + 1);
        if (closeQuote == std::string::npos)
            return {};
        return json.substr(openQuote + 1, closeQuote - openQuote - 1);
    }

    //
    // Strict semver comparison: returns true when remote > local.
    // Accepts "major.minor.build" strings; non-numeric suffixes (e.g. "BETA") are ignored.
    //
    static bool IsNewerVersion(const std::string& local, const std::string& remote) {
        auto parse = [](const std::string& v, int& major, int& minor, int& build) {
            major = minor = build = 0;
            sscanf(v.c_str(), "%d.%d.%d", &major, &minor, &build);
        };
        int lMaj, lMin, lBld, rMaj, rMin, rBld;
        parse(local, lMaj, lMin, lBld);
        parse(remote, rMaj, rMin, rBld);
        if (rMaj != lMaj)
            return rMaj > lMaj;
        if (rMin != lMin)
            return rMin > lMin;
        return rBld > lBld;
    }

    //
    // Perform a synchronous HTTPS GET and return the response body as a string.
    // Returns an empty string on any error.
    //
    static std::string HttpsGet(const std::wstring& host, const std::wstring& path) {
        std::string body;
        HINTERNET hSession = WinHttpOpen(
            L"RetroDev/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (!hSession)
            return body;
        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            return body;
        }
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            return body;
        }
        //
        // 8-second send/receive timeout so a slow server does not hang the thread
        //
        DWORD timeout = 8000;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
            && WinHttpReceiveResponse(hRequest, nullptr)) {
            DWORD bytesAvailable = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
                std::string chunk(bytesAvailable, '\0');
                DWORD bytesRead = 0;
                if (WinHttpReadData(hRequest, &chunk[0], bytesAvailable, &bytesRead))
                    body.append(chunk.data(), bytesRead);
            }
        }
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return body;
    }

    //
    // Background thread entry point.
    // Fetches the manifest, parses the version, writes the result under the mutex.
    //
    static void CheckThreadProc(std::string manifestUrl) {
        VersionCheckResult result;
        result.state = VersionCheckState::Failed;
        //
        // Split manifestUrl into host and path.
        // Expected format: https://hostname/path/to/file
        //
        const std::string prefix = "https://";
        if (manifestUrl.rfind(prefix, 0) == 0) {
            std::string remainder = manifestUrl.substr(prefix.size());
            size_t slashPos = remainder.find('/');
            std::string host = (slashPos != std::string::npos) ? remainder.substr(0, slashPos) : remainder;
            std::string path = (slashPos != std::string::npos) ? remainder.substr(slashPos) : "/";
            std::wstring wHost(host.begin(), host.end());
            std::wstring wPath(path.begin(), path.end());
            std::string body = HttpsGet(wHost, wPath);
            if (!body.empty()) {
                result.latestVersion   = ExtractJsonString(body, "latestVersion");
                result.releaseNotesUrl = ExtractJsonString(body, "releaseNotesUrl");
                result.downloadUrl     = ExtractJsonString(body, "downloadUrl");
                //
                // Body received but required field missing -- treat as failed parse
                //
                if (result.latestVersion.empty()) {
                    result.state = VersionCheckState::Failed;
                } else {
                    //
                    // Parse succeeded -- compare versions to resolve final state
                    //
                    std::string current = std::to_string(RetrodevLib::k_versionMajor)
                        + "." + std::to_string(RetrodevLib::k_versionMinor)
                        + "." + std::string(RetrodevLib::k_versionBuild);
                    result.state = IsNewerVersion(current, result.latestVersion)
                        ? VersionCheckState::UpdateAvailable
                        : VersionCheckState::UpToDate;
                }
            }
        }
        std::lock_guard<std::mutex> lock(s_mutex);
        s_result = result;
        s_running.store(false);
    }

    void VersionCheck::StartAsync(const std::string& manifestUrl) {
        bool expected = false;
        if (!s_running.compare_exchange_strong(expected, true))
            return;
        //
        // Join any previously finished thread before launching a new one
        //
        if (s_thread.joinable())
            s_thread.join();
        {
            std::lock_guard<std::mutex> lock(s_mutex);
            s_result = VersionCheckResult{};
            s_result.state = VersionCheckState::Checking;
        }
        s_thread = std::thread(CheckThreadProc, manifestUrl);
    }

    VersionCheckResult VersionCheck::Poll() {
        std::lock_guard<std::mutex> lock(s_mutex);
        return s_result;
    }

    void VersionCheck::Shutdown() {
        s_running.store(false);
        if (s_thread.joinable())
            s_thread.join();
        std::lock_guard<std::mutex> lock(s_mutex);
        s_result = VersionCheckResult{};
    }

}
