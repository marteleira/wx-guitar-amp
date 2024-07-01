#pragma once
#include <wx/wx.h>
#include "audio/AudioEngine.h"

class KnobControl;
class SpectrumView;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(AudioEngine& engine);

private:
    void BuildUI();
    void OnKnobChanged(wxCommandEvent&);
    void OnHeaderPaint(wxPaintEvent&);
    void OnModelChanged(wxCommandEvent&);
    void OnInputDevice(wxCommandEvent&);
    void OnOutputDevice(wxCommandEvent&);
    void OnInputGain(wxCommandEvent&);
    void OnOutputGain(wxCommandEvent&);
    void OnTimer(wxTimerEvent&);
    void OnClose(wxCloseEvent&);
    void ApplyModelToUI(int index);

    AudioEngine& m_engine;

    wxPanel*  m_header      = nullptr;
    wxChoice* m_modelChoice = nullptr;

    // I/O panel widgets
    wxChoice*     m_inDevChoice  = nullptr;
    wxChoice*     m_outDevChoice = nullptr;
    wxSlider*     m_inGainSlider  = nullptr;
    wxSlider*     m_outGainSlider = nullptr;
    wxStaticText* m_inPeakLabel   = nullptr;
    wxStaticText* m_outPeakLabel  = nullptr;
    SpectrumView* m_spectrum      = nullptr;

    // Amp knobs
    KnobControl* m_kPreamp   = nullptr;
    KnobControl* m_kBass     = nullptr;
    KnobControl* m_kMid      = nullptr;
    KnobControl* m_kTreble   = nullptr;
    KnobControl* m_kPresence = nullptr;
    KnobControl* m_kMaster   = nullptr;

    wxTimer m_timer;

    wxDECLARE_EVENT_TABLE();
};
