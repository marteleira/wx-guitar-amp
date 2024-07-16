#pragma once
#include <wx/wx.h>
#include "audio/AudioEngine.h"
#include "Persistence.h"

class KnobControl;
class SpectrumView;

class MainFrame : public wxFrame {
public:
    explicit MainFrame(AudioEngine& engine);

    void applyState(const AppState& s);
    AppState getState();

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

    // Sync current knob positions into m_savedKnobs[currentModel]
    void SyncKnobsToSaved();
    // Push m_savedKnobs[index] into the knob widgets + update labels
    void RestoreModelKnobs(int index);

    KnobControl* knobs() const;  // returns array view via inline helper below

    AudioEngine& m_engine;

    wxPanel*  m_header      = nullptr;
    wxChoice* m_modelChoice = nullptr;

    wxChoice*     m_inDevChoice  = nullptr;
    wxChoice*     m_outDevChoice = nullptr;
    wxSlider*     m_inGainSlider  = nullptr;
    wxSlider*     m_outGainSlider = nullptr;
    wxStaticText* m_inPeakLabel   = nullptr;
    wxStaticText* m_outPeakLabel  = nullptr;
    SpectrumView* m_spectrum      = nullptr;

    KnobControl* m_kPreamp   = nullptr;
    KnobControl* m_kBass     = nullptr;
    KnobControl* m_kMid      = nullptr;
    KnobControl* m_kTreble   = nullptr;
    KnobControl* m_kPresence = nullptr;
    KnobControl* m_kMaster   = nullptr;

    // Per-model knob state — updated on every knob change and on model switch
    float m_savedKnobs[3][6] = {};

    wxTimer m_timer;

    wxDECLARE_EVENT_TABLE();
};
