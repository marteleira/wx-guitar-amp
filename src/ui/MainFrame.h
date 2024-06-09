#pragma once
#include <wx/wx.h>
#include "audio/AudioEngine.h"

class KnobControl;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(AudioEngine& engine);

private:
    void BuildUI();
    void OnKnobChanged(wxCommandEvent& evt);
    void OnHeaderPaint(wxPaintEvent& evt);
    void OnClose(wxCloseEvent& evt);

    AudioEngine& m_engine;

    wxPanel*     m_header = nullptr;
    KnobControl* m_kPreamp   = nullptr;
    KnobControl* m_kBass     = nullptr;
    KnobControl* m_kMid      = nullptr;
    KnobControl* m_kTreble   = nullptr;
    KnobControl* m_kPresence = nullptr;
    KnobControl* m_kMaster   = nullptr;

    wxDECLARE_EVENT_TABLE();
};
