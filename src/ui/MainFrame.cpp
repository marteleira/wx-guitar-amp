#include "MainFrame.h"
#include "KnobControl.h"
#include <wx/dcbuffer.h>

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_CLOSE(MainFrame::OnClose)
wxEND_EVENT_TABLE()

static constexpr int HEADER_H = 54;
static constexpr int KNOB_W   = 92;
static constexpr int KNOB_H   = 116;
static constexpr int PAD      = 18;

MainFrame::MainFrame(AudioEngine& engine)
    : wxFrame(nullptr, wxID_ANY, "Marshall JCM800 2203",
              wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER & ~wxMAXIMIZE_BOX)
    , m_engine(engine)
{
    BuildUI();
}

void MainFrame::BuildUI() {
    SetBackgroundColour(wxColour(22, 22, 22));

    // Status bar first so SetSizerAndFit accounts for it
    CreateStatusBar();
    SetStatusText(wxString::Format("Audio running  |  %u Hz  |  JCM800 2203",
                                   m_engine.getSampleRate()));

    // Header panel (expands full width via sizer)
    m_header = new wxPanel(this, wxID_ANY);
    m_header->SetMinSize(wxSize(1, HEADER_H));
    m_header->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_header->Bind(wxEVT_PAINT, &MainFrame::OnHeaderPaint, this);

    // Knob container (knobs here)
    auto* knobPanel = new wxPanel(this, wxID_ANY);
    knobPanel->SetBackgroundColour(wxColour(22, 22, 22));

    struct KnobDef { wxString label; float def; std::atomic<float>* param; };
    KnobDef defs[] = {
        { "PREAMP\nVOLUME", 5.0f, &m_engine.params.preampVol },
        { "BASS",           5.0f, &m_engine.params.bass      },
        { "MIDDLE",         5.0f, &m_engine.params.mid       },
        { "TREBLE",         5.0f, &m_engine.params.treble    },
        { "PRESENCE",       5.0f, &m_engine.params.presence  },
        { "MASTER\nVOLUME", 4.0f, &m_engine.params.master    },
    };
    KnobControl** targets[] = {
        &m_kPreamp, &m_kBass, &m_kMid, &m_kTreble, &m_kPresence, &m_kMaster
    };

    auto* knobRow = new wxBoxSizer(wxHORIZONTAL);
    knobRow->AddSpacer(PAD);
    for (int i = 0; i < 6; ++i) {
        auto* k = new KnobControl(knobPanel, wxID_ANY,
                                  defs[i].label, 0.0f, 10.0f, defs[i].def,
                                  wxDefaultPosition, wxSize(KNOB_W, KNOB_H));
        defs[i].param->store(defs[i].def, std::memory_order_relaxed);
        k->Bind(wxEVT_KNOB_CHANGED, &MainFrame::OnKnobChanged, this);
        *targets[i] = k;
        knobRow->Add(k, 0, wxTOP | wxBOTTOM, PAD);
        if (i < 5) knobRow->AddSpacer(PAD);
    }
    knobRow->AddSpacer(PAD);
    knobPanel->SetSizer(knobRow);

    //  Main vertical sizer
    auto* root = new wxBoxSizer(wxVERTICAL);
    root->Add(m_header,   0, wxEXPAND);
    root->Add(knobPanel,  0, wxEXPAND);
    SetSizerAndFit(root);
    Centre();
}

void MainFrame::OnHeaderPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(m_header);
    wxSize sz = m_header->GetClientSize();

    dc.SetBackground(wxBrush(wxColour(18, 18, 18)));
    dc.Clear();

    // Gold strip
    dc.SetBrush(wxBrush(wxColour(120, 90, 20)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, 220, sz.y);

    //italic script
    dc.SetFont(wxFont(22, wxFONTFAMILY_SWISS, wxFONTSTYLE_ITALIC, wxFONTWEIGHT_BOLD));
    dc.SetTextForeground(wxColour(248, 240, 210));
    dc.DrawText("Marshall", 10, 8);

    // Model label
    dc.SetFont(wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(190, 190, 190));
    dc.DrawText("JCM800  Master Volume  2203", 228, 8);

    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(110, 110, 110));
    dc.DrawText("100W  Super Lead", 228, 30);

    // Power LED (red circle)
    int ledX = sz.x - 22, ledY = sz.y / 2;
    dc.SetBrush(wxBrush(wxColour(255, 40, 40)));
    dc.SetPen(wxPen(wxColour(180, 0, 0), 1));
    dc.DrawCircle(ledX, ledY, 6);
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(wxPen(wxColour(200, 60, 60), 1));
    dc.DrawCircle(ledX, ledY, 10);
}

void MainFrame::OnKnobChanged(wxCommandEvent& evt) {
    auto* knob = dynamic_cast<KnobControl*>(evt.GetEventObject());
    if (!knob) return;

    float v = knob->GetValue();
    if      (knob == m_kPreamp)   m_engine.params.preampVol.store(v, std::memory_order_relaxed);
    else if (knob == m_kBass)     m_engine.params.bass.store(v,      std::memory_order_relaxed);
    else if (knob == m_kMid)      m_engine.params.mid.store(v,       std::memory_order_relaxed);
    else if (knob == m_kTreble)   m_engine.params.treble.store(v,    std::memory_order_relaxed);
    else if (knob == m_kPresence) m_engine.params.presence.store(v,  std::memory_order_relaxed);
    else if (knob == m_kMaster)   m_engine.params.master.store(v,    std::memory_order_relaxed);
}

void MainFrame::OnClose(wxCloseEvent& evt) {
    m_engine.shutdown();
    evt.Skip();
}
