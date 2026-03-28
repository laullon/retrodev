// --------------------------------------------------------------------------------------------------------------
//
// Retrodev Gui
//
// Version check UI -- notification banner and update popup.
//
// (c) TLOTB 2026
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <retrodev.gui.h>

namespace RetrodevGui {

    //
    // Renders the update-available notification.
    // Call once per frame from the main menu bar area (after the menu bar ends)
    // and once from the main view to drive the modal popup.
    //
    class VersionCheckUi {
    public:
        //
        // Drive the version check state machine.  Must be called once per frame
        // before any Render call so state transitions happen at a known point.
        //
        static void Tick();
        //
        // Render the update-available modal popup.  Call every frame from the
        // main view after MainView::Perform() so the popup can overlay the UI.
        //
        static void RenderPopup();
        //
        // Returns true when an update is available and the user has not yet
        // dismissed the notification.  Used by the menu bar to show the badge.
        //
        static bool HasPendingNotification();
        //
        // Programmatically open the update popup (e.g. from Help > Check for Updates).
        //
        static void ShowPopup();

    private:
        //
        // True once Tick has fired the initial async check
        //
        static inline bool m_checkStarted = false;
        //
        // True when the update popup should open on the next frame
        //
        static inline bool m_popupOpen = false;
        //
        // True while the popup modal is showing
        //
        static inline bool m_popupShowing = false;
        //
        // True when an update is available and not yet dismissed by the user
        //
        static inline bool m_pendingNotification = false;
        //
        // True once the initial auto-open notification has been shown, so Tick
        // never re-triggers it after the user dismisses the popup
        //
        static inline bool m_notificationShown = false;
    };

}
