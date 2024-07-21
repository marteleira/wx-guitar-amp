#include "MainFrame.h"
#include "KnobControl.h"
#include "SpectrumView.h"
#include <wx/dcbuffer.h>

enum {
    ID_AMP_MODEL     = wxID_HIGHEST + 1,
    ID_INPUT_DEVICE,
    ID_OUTPUT_DEVICE,
    ID_INPUT_GAIN,
    ID_OUTPUT_GAIN,
    ID_TIMER,
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_CLOSE(MainFrame::OnClose)
    EVT_CHOICE(ID_AMP_MODEL,     MainFrame::OnModelChanged)
    EVT_CHOICE(ID_INPUT_DEVICE,  MainFrame::OnInputDevice)
    EVT_CHOICE(ID_OUTPUT_DEVICE, MainFrame::OnOutputDevice)
    EVT_SLIDER(ID_INPUT_GAIN,    MainFrame::OnInputGain)
    EVT_SLIDER(ID_OUTPUT_GAIN,   MainFrame::OnOutputGain)
    EVT_TIMER (ID_TIMER,         MainFrame::OnTimer)
wxEND_EVENT_TABLE()

static constexpr int HEADER_H  = 54;
static constexpr int IO_H      = 170;
static constexpr int KNOB_W    = 92;
static constexpr int KNOB_H    = 116;
static constexpr int PAD       = 18;
static constexpr int SIDE_W    = 150;   // width of each I/O side column

// Helpers

static wxStaticText* label(wxWindow* p, const wxString& s,
                           int pt = 8, bool bold = false) {
    auto* t = new wxStaticText(p, wxID_ANY, s);
    t->SetFont(wxFont(pt, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
                      bold ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL));
    t->SetForegroundColour(wxColour(190, 190, 175));
    return t;
}

static wxStaticText* dimLabel(wxWindow* p, const wxString& s) {
    auto* t = new wxStaticText(p, wxID_ANY, s);
    t->SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    t->SetForegroundColour(wxColour(100, 100, 90));
    return t;
}

// Constructor

MainFrame::MainFrame(AudioEngine& engine)
    : wxFrame(nullptr, wxID_ANY, "Amp Simulator",
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX)
    , m_engine(engine)
    , m_timer(this, ID_TIMER)
{
    BuildUI();
    m_timer.Start(50);   // 20 fps refresh for spectrum + peak meters
}

// UI construction 

void MainFrame::BuildUI() {
    SetBackgroundColour(wxColour(22, 22, 22));

    CreateStatusBar();
    SetStatusText(wxString::Format("Audio running  |  %u Hz ",
                                   m_engine.getSampleRate()));

    // Header 
    m_header = new wxPanel(this, wxID_ANY);
    m_header->SetMinSize(wxSize(1, HEADER_H));
    m_header->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_header->Bind(wxEVT_PAINT, &MainFrame::OnHeaderPaint, this);

    // Model selector bar 
    auto* modelBar = new wxPanel(this, wxID_ANY);
    modelBar->SetBackgroundColour(wxColour(14, 14, 14));
    modelBar->SetMinSize(wxSize(1, 30));

    wxArrayString modelNames;
    for (int i = 0; i < m_engine.ampModelCount(); ++i)
        modelNames.Add(wxString::FromUTF8(m_engine.ampModel(i).name()));
    m_modelChoice = new wxChoice(modelBar, ID_AMP_MODEL,
                                 wxDefaultPosition, wxDefaultSize, modelNames);
    m_modelChoice->SetSelection(0);
    m_modelChoice->SetBackgroundColour(wxColour(35, 35, 35));
    m_modelChoice->SetForegroundColour(wxColour(200, 200, 185));

    auto* mbLabel = new wxStaticText(modelBar, wxID_ANY, "Amp Model:");
    mbLabel->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    mbLabel->SetForegroundColour(wxColour(110, 110, 100));

    auto* modelRow = new wxBoxSizer(wxHORIZONTAL);
    modelRow->AddStretchSpacer();
    modelRow->Add(mbLabel,       0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    modelRow->Add(m_modelChoice, 0, wxALIGN_CENTER_VERTICAL);
    modelRow->AddStretchSpacer();
    modelBar->SetSizer(modelRow);

    // I/O panel
    auto* ioPanel = new wxPanel(this, wxID_ANY);
    ioPanel->SetBackgroundColour(wxColour(18, 18, 18));
    ioPanel->SetMinSize(wxSize(1, IO_H));

    // Populate device lists
    auto inDevs  = m_engine.enumerateInputDevices();
    auto outDevs = m_engine.enumerateOutputDevices();

    auto makeChoice = [](wxWindow* p, wxWindowID id, const std::vector<DeviceInfo>& devs) {
        wxArrayString items;
        int defIdx = 0;
        for (auto& d : devs) {
            items.Add(wxString::FromUTF8(d.name));
            if (d.isDefault) defIdx = d.index;
        }
        auto* c = new wxChoice(p, id, wxDefaultPosition, wxDefaultSize, items);
        c->SetSelection(defIdx);
        c->SetBackgroundColour(wxColour(35, 35, 35));
        c->SetForegroundColour(wxColour(200, 200, 185));
        return c;
    };

    //  Left column: INPUT
    auto* inCol = new wxPanel(ioPanel, wxID_ANY);
    inCol->SetBackgroundColour(wxColour(18, 18, 18));
    inCol->SetMinSize(wxSize(SIDE_W, IO_H));

    m_inDevChoice  = makeChoice(inCol, ID_INPUT_DEVICE, inDevs);
    m_inGainSlider = new wxSlider(inCol, ID_INPUT_GAIN, 100, 0, 200,
                                  wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL);
    m_inPeakLabel  = new wxStaticText(inCol, wxID_ANY, "-inf dBFS",
                                      wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
    m_inPeakLabel->SetFont(wxFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    m_inPeakLabel->SetForegroundColour(wxColour(60, 200, 80));
    m_inPeakLabel->SetMinSize(wxSize(SIDE_W - 10, -1));

    auto* inSizer = new wxBoxSizer(wxVERTICAL);
    inSizer->AddSpacer(8);
    inSizer->Add(label(inCol, "INPUT", 9, true), 0, wxLEFT, 8);
    inSizer->AddSpacer(6);
    inSizer->Add(dimLabel(inCol, " Device"), 0, wxLEFT, 4);
    inSizer->Add(m_inDevChoice, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    inSizer->AddSpacer(8);
    inSizer->Add(dimLabel(inCol, " Gain"), 0, wxLEFT, 4);
    inSizer->Add(m_inGainSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    inSizer->AddSpacer(8);
    inSizer->Add(dimLabel(inCol, " Peak"), 0, wxLEFT, 4);
    inSizer->Add(m_inPeakLabel, 0, wxLEFT, 8);
    inCol->SetSizer(inSizer);

    // Right column: OUTPUT 
    auto* outCol = new wxPanel(ioPanel, wxID_ANY);
    outCol->SetBackgroundColour(wxColour(18, 18, 18));
    outCol->SetMinSize(wxSize(SIDE_W, IO_H));

    m_outDevChoice  = makeChoice(outCol, ID_OUTPUT_DEVICE, outDevs);
    m_outGainSlider = new wxSlider(outCol, ID_OUTPUT_GAIN, 100, 0, 200,
                                   wxDefaultPosition, wxSize(-1, -1), wxSL_HORIZONTAL);
    m_outPeakLabel  = new wxStaticText(outCol, wxID_ANY, "-inf dBFS",
                                       wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
    m_outPeakLabel->SetFont(wxFont(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    m_outPeakLabel->SetForegroundColour(wxColour(60, 200, 80));
    m_outPeakLabel->SetMinSize(wxSize(SIDE_W - 10, -1));

    auto* outSizer = new wxBoxSizer(wxVERTICAL);
    outSizer->AddSpacer(8);
    outSizer->Add(label(outCol, "OUTPUT", 9, true), 0, wxLEFT, 8);
    outSizer->AddSpacer(6);
    outSizer->Add(dimLabel(outCol, " Device"), 0, wxLEFT, 4);
    outSizer->Add(m_outDevChoice, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    outSizer->AddSpacer(8);
    outSizer->Add(dimLabel(outCol, " Gain"), 0, wxLEFT, 4);
    outSizer->Add(m_outGainSlider, 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    outSizer->AddSpacer(8);
    outSizer->Add(dimLabel(outCol, " Peak"), 0, wxLEFT, 4);
    outSizer->Add(m_outPeakLabel, 0, wxLEFT, 8);
    outCol->SetSizer(outSizer);

    // Spectrum center 
    m_spectrum = new SpectrumView(ioPanel, m_engine, wxSize(300, IO_H));

    // I/O panel row sizer 
    auto* ioRow = new wxBoxSizer(wxHORIZONTAL);
    ioRow->Add(inCol,      0, wxEXPAND);
    ioRow->Add(m_spectrum, 1, wxEXPAND);
    ioRow->Add(outCol,     0, wxEXPAND);
    ioPanel->SetSizer(ioRow);

    // Init per-model saved knob state from each model's curated defaults
    for (int mi = 0; mi < AMP_MODEL_COUNT; ++mi) {
        auto defs = m_engine.ampModel(mi).knobDefaults();
        for (int i = 0; i < 6; ++i)
            m_savedKnobs[mi][i] = defs[i];
    }

    // Knob panel — labels and initial values come from model 0 (JCM800)
    auto* knobPanel = new wxPanel(this, wxID_ANY);
    knobPanel->SetBackgroundColour(wxColour(22, 22, 22));

    std::atomic<float>* kParams[] = {
        &m_engine.params.preampVol, &m_engine.params.bass,
        &m_engine.params.mid,       &m_engine.params.treble,
        &m_engine.params.presence,  &m_engine.params.master,
    };
    KnobControl** kTargets[] = {
        &m_kPreamp, &m_kBass, &m_kMid, &m_kTreble, &m_kPresence, &m_kMaster
    };
    auto initLabels = m_engine.ampModel(0).knobLabels();

    auto* knobRow = new wxBoxSizer(wxHORIZONTAL);
    knobRow->AddSpacer(PAD);
    for (int i = 0; i < 6; ++i) {
        float v = m_savedKnobs[0][i];
        auto* k = new KnobControl(knobPanel, wxID_ANY,
                                  initLabels[i], 0.0f, 10.0f, v,
                                  wxDefaultPosition, wxSize(KNOB_W, KNOB_H));
        kParams[i]->store(v, std::memory_order_relaxed);
        k->Bind(wxEVT_KNOB_CHANGED, &MainFrame::OnKnobChanged, this);
        *kTargets[i] = k;
        knobRow->Add(k, 0, wxTOP | wxBOTTOM, PAD);
        if (i < 5) knobRow->AddSpacer(PAD);
    }
    knobRow->AddSpacer(PAD);
    knobPanel->SetSizer(knobRow);

    // Root sizer 
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(m_header,   0, wxEXPAND);
    root->Add(modelBar,   0, wxEXPAND);
    root->Add(ioPanel,    0, wxEXPAND);
    root->Add(knobPanel,  0, wxEXPAND);
    SetSizerAndFit(root);
    Centre();
}

// Event handlers 

// Knob state helpers

void MainFrame::SyncKnobsToSaved() {
    int idx = m_engine.currentAmpModel();
    KnobControl* ks[] = { m_kPreamp, m_kBass, m_kMid, m_kTreble, m_kPresence, m_kMaster };
    for (int i = 0; i < 6; ++i)
        m_savedKnobs[idx][i] = ks[i]->GetValue();
}

void MainFrame::RestoreModelKnobs(int index) {
    const float* v = m_savedKnobs[index];
    auto labels = m_engine.ampModel(index).knobLabels();
    KnobControl* ks[] = { m_kPreamp, m_kBass, m_kMid, m_kTreble, m_kPresence, m_kMaster };

    // notify=false: avoid OnKnobChanged -> SyncKnobsToSaved firing mid-loop,
    // which would overwrite the not-yet-set knobs with stale BuildUI values.
    for (int i = 0; i < 6; ++i) {
        ks[i]->SetKnobLabel(labels[i]);
        ks[i]->SetValue(v[i], false);
    }

    // Push all values to the audio engine atomics in one shot
    m_engine.params.preampVol.store(v[0], std::memory_order_relaxed);
    m_engine.params.bass.store     (v[1], std::memory_order_relaxed);
    m_engine.params.mid.store      (v[2], std::memory_order_relaxed);
    m_engine.params.treble.store   (v[3], std::memory_order_relaxed);
    m_engine.params.presence.store (v[4], std::memory_order_relaxed);
    m_engine.params.master.store   (v[5], std::memory_order_relaxed);

    m_header->Refresh();
}

// Persistence

AppState MainFrame::getState() {
    SyncKnobsToSaved();
    AppState s;
    s.currentModel     = m_engine.currentAmpModel();
    s.inputGain        = m_inGainSlider->GetValue();
    s.outputGain       = m_outGainSlider->GetValue();
    s.inputDeviceName  = m_inDevChoice->GetStringSelection().ToStdString();
    s.outputDeviceName = m_outDevChoice->GetStringSelection().ToStdString();
    for (int m = 0; m < AMP_MODEL_COUNT; ++m)
        for (int i = 0; i < 6; ++i)
            s.models[m].vals[i] = m_savedKnobs[m][i];
    return s;
}

void MainFrame::applyState(const AppState& s) {
    // Load all per-model knob values
    for (int m = 0; m < AMP_MODEL_COUNT; ++m)
        for (int i = 0; i < 6; ++i)
            m_savedKnobs[m][i] = s.models[m].vals[i];

    // Switch to saved model
    int idx = (s.currentModel >= 0 && s.currentModel < 3) ? s.currentModel : 0;
    m_modelChoice->SetSelection(idx);
    m_engine.setAmpModel(idx);
    RestoreModelKnobs(idx);

    // Gains
    m_inGainSlider->SetValue(s.inputGain);
    m_outGainSlider->SetValue(s.outputGain);
    m_engine.params.inputGain.store (s.inputGain  / 100.0f, std::memory_order_relaxed);
    m_engine.params.outputGain.store(s.outputGain / 100.0f, std::memory_order_relaxed);

    // Devices — find by name, restart if something changed
    bool devChanged = false;
    auto findDevice = [&](wxChoice* choice, const std::string& name) -> int {
        if (name.empty()) return -1;
        for (int i = 0; i < (int)choice->GetCount(); ++i)
            if (choice->GetString(i).ToStdString() == name) return i;
        return -1;
    };

    int inIdx  = findDevice(m_inDevChoice,  s.inputDeviceName);
    int outIdx = findDevice(m_outDevChoice, s.outputDeviceName);

    if (inIdx >= 0 && inIdx != m_inDevChoice->GetSelection()) {
        m_inDevChoice->SetSelection(inIdx);
        m_engine.setInputDevice(inIdx);
        devChanged = true;
    }
    if (outIdx >= 0 && outIdx != m_outDevChoice->GetSelection()) {
        m_outDevChoice->SetSelection(outIdx);
        m_engine.setOutputDevice(outIdx);
        devChanged = true;
    }
    if (devChanged) {
        m_engine.restart();
        SetStatusText(wxString::Format("Audio running  |  %u Hz", m_engine.getSampleRate()));
    }
}

// Model switch 

void MainFrame::OnModelChanged(wxCommandEvent& evt) {
    SyncKnobsToSaved();          // save current model's knob positions
    int sel = evt.GetSelection();
    m_engine.setAmpModel(sel);
    RestoreModelKnobs(sel);      // load saved positions for new model
}

void MainFrame::OnHeaderPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(m_header);
    wxSize sz = m_header->GetClientSize();

    const auto& m   = m_engine.ampModel(m_engine.currentAmpModel());
    int modelIdx     = m_engine.currentAmpModel();

    // Strip colour + brand text colour per brand
    struct BrandTheme { wxColour strip; wxColour text; };
    BrandTheme themes[] = {
        { wxColour(120,  90,  20), wxColour(248, 240, 210) },  // 0 Marshall — gold
        { wxColour( 35,  35,  40), wxColour(160, 200, 255) },  // 1 Fender — dark/blue
        { wxColour(170,  65,   0), wxColour(255, 200, 100) },  // 2 Orange — orange
        { wxColour( 18,  18,  40), wxColour(220, 220, 185) },  // 3 Vox — dark navy/cream
    };
    auto theme = (modelIdx >= 0 && modelIdx < AMP_MODEL_COUNT) ? themes[modelIdx] : themes[0];

    dc.SetBackground(wxBrush(wxColour(18, 18, 18)));
    dc.Clear();

    dc.SetBrush(wxBrush(theme.strip));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, 220, sz.y);

    dc.SetFont(wxFont(22, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD));
    dc.SetTextForeground(theme.text);
    dc.DrawText(m.brand(), 10, 8);

    dc.SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(190, 190, 190));
    dc.DrawText(m.name(), 228, 8);

    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(110, 110, 110));
    dc.DrawText(m.subtitle(), 228, 30);

    int ledX = sz.x - 22, ledY = sz.y / 2;
    dc.SetBrush(wxBrush(wxColour(255, 40, 40)));
    dc.SetPen(wxPen(wxColour(180, 0, 0), 1));
    dc.DrawCircle(ledX, ledY, 6);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColour(200, 60, 60), 1));
    dc.DrawCircle(ledX, ledY, 10);
}

void MainFrame::OnKnobChanged(wxCommandEvent& evt) {
    auto* k = dynamic_cast<KnobControl*>(evt.GetEventObject());
    if (!k) return;
    float v = k->GetValue();
    if      (k == m_kPreamp)   m_engine.params.preampVol.store(v, std::memory_order_relaxed);
    else if (k == m_kBass)     m_engine.params.bass.store(v,      std::memory_order_relaxed);
    else if (k == m_kMid)      m_engine.params.mid.store(v,       std::memory_order_relaxed);
    else if (k == m_kTreble)   m_engine.params.treble.store(v,    std::memory_order_relaxed);
    else if (k == m_kPresence) m_engine.params.presence.store(v,  std::memory_order_relaxed);
    else if (k == m_kMaster)   m_engine.params.master.store(v,    std::memory_order_relaxed);
    SyncKnobsToSaved();
}

void MainFrame::OnInputDevice(wxCommandEvent& evt) {
    m_engine.setInputDevice(evt.GetSelection());
    m_engine.restart();
    SetStatusText(wxString::Format("Audio running  |  %u Hz ",
                                   m_engine.getSampleRate()));
}

void MainFrame::OnOutputDevice(wxCommandEvent& evt) {
    m_engine.setOutputDevice(evt.GetSelection());
    m_engine.restart();
    SetStatusText(wxString::Format("Audio running  |  %u Hz  ",
                                   m_engine.getSampleRate()));
}

void MainFrame::OnInputGain(wxCommandEvent&) {
    float gain = m_inGainSlider->GetValue() / 100.0f;  // 0–2.0
    m_engine.params.inputGain.store(gain, std::memory_order_relaxed);
}

void MainFrame::OnOutputGain(wxCommandEvent&) {
    float gain = m_outGainSlider->GetValue() / 100.0f;
    m_engine.params.outputGain.store(gain, std::memory_order_relaxed);
}

void MainFrame::OnTimer(wxTimerEvent&) {
    // Update spectrum
    m_spectrum->UpdateAndRefresh();

    // Update peak labels
    auto fmtPeak = [](float db) -> wxString {
        if (db < -99.0f) return "-inf dBFS";
        return wxString::Format("%+.1f dBFS", db);
    };

    float inDb  = m_engine.inputPeakDb();
    float outDb = m_engine.outputPeakDb();

    m_inPeakLabel->SetLabel(fmtPeak(inDb));
    m_outPeakLabel->SetLabel(fmtPeak(outDb));

    // Colour: green → yellow → red as level rises
    auto peakColor = [](float db) -> wxColour {
        if (db > -6.0f)  return wxColour(220, 60,  40);   // red (clipping danger)
        if (db > -18.0f) return wxColour(220, 200, 40);   // yellow (hot)
        return                   wxColour(60,  200, 80);   // green (safe)
    };
    m_inPeakLabel->SetForegroundColour(peakColor(inDb));
    m_outPeakLabel->SetForegroundColour(peakColor(outDb));
}

void MainFrame::OnClose(wxCloseEvent& evt) {
    m_timer.Stop();
    Persistence::save(getState(), Persistence::defaultPath());
    m_engine.shutdown();
    evt.Skip();
}
