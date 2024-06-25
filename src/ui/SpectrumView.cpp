#include "SpectrumView.h"
#include "dsp/Fft.h"
#include <wx/dcbuffer.h>
#include <cmath>
#include <algorithm>

wxBEGIN_EVENT_TABLE(SpectrumView, wxPanel)
    EVT_PAINT(SpectrumView::OnPaint)
wxEND_EVENT_TABLE()

SpectrumView::SpectrumView(wxWindow* parent, const AudioEngine& engine, const wxSize& size)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, size)
    , m_engine(engine)
    , m_inBuf(FFT_SIZE, 0.0f)
    , m_outBuf(FFT_SIZE, 0.0f)
    , m_inMag(FFT_SIZE / 2, DB_MIN)
    , m_outMag(FFT_SIZE / 2, DB_MIN)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(size);
}

// ─── Frequency ↔ screen mapping (logarithmic) ─────────────────────────────────

int SpectrumView::FreqToX(float hz, int w) const {
    if (hz < FREQ_MIN) return 0;
    float t = std::log(hz / FREQ_MIN) / std::log(FREQ_MAX / FREQ_MIN);
    return (int)(t * w);
}

int SpectrumView::DbToY(float db, int h) const {
    float t = (db - DB_MIN) / (DB_MAX - DB_MIN);
    return (int)((1.0f - t) * h);
}

// ─── FFT + magnitude ──────────────────────────────────────────────────────────

void SpectrumView::ComputeMagnitude(const std::vector<float>& samples, std::vector<float>& mag) {
    std::vector<std::complex<float>> buf(FFT_SIZE);
    for (int i = 0; i < FFT_SIZE; ++i) {
        // Hanning window
        float w = 0.5f * (1.0f - std::cos(2.0f * (float)M_PI * i / (FFT_SIZE - 1)));
        buf[i]  = { samples[i] * w, 0.0f };
    }
    fftInPlace(buf);

    mag.resize(FFT_SIZE / 2);
    float scale = 2.0f / FFT_SIZE;
    for (int i = 0; i < FFT_SIZE / 2; ++i) {
        float m = std::abs(buf[i]) * scale;
        mag[i] = (m > 1e-9f) ? 20.0f * std::log10(m) : -120.0f;
    }
}

// ─── Drawing ─────────────────────────────────────────────────────────────────

void SpectrumView::DrawGrid(wxDC& dc, int w, int h) {
    dc.SetPen(wxPen(wxColour(50, 50, 50), 1));

    // Frequency grid: 50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k Hz
    for (float hz : {50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f}) {
        int x = FreqToX(hz, w);
        dc.DrawLine(x, 0, x, h);
    }

    // dB grid: -60, -40, -20, 0
    dc.SetPen(wxPen(wxColour(45, 45, 45), 1));
    for (float db : {-60.f, -40.f, -20.f, 0.f}) {
        int y = DbToY(db, h);
        dc.DrawLine(0, y, w, y);
    }

    // Labels
    dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(80, 80, 80));

    for (auto [hz, label] : std::initializer_list<std::pair<float, const char*>>{
        {100.f, "100"}, {1000.f, "1k"}, {10000.f, "10k"}
    }) {
        dc.DrawText(label, FreqToX(hz, w) + 2, h - 14);
    }

    for (auto [db, label] : std::initializer_list<std::pair<float, const char*>>{
        {-60.f, "-60"}, {-40.f, "-40"}, {-20.f, "-20"}
    }) {
        dc.DrawText(label, 2, DbToY(db, h) - 10);
    }
}

void SpectrumView::DrawMagnitude(wxDC& dc, const std::vector<float>& mag,
                                 int w, int h,
                                 const wxColour& fill, const wxColour& line) {
    unsigned int sr = m_engine.getSampleRate();
    int bins = (int)mag.size();

    std::vector<wxPoint> pts;
    pts.emplace_back(0, h);  // start at bottom-left

    for (int k = 1; k < bins; ++k) {
        float hz = (float)k * sr / FFT_SIZE;
        if (hz < FREQ_MIN || hz > FREQ_MAX) continue;
        int x = FreqToX(hz, w);
        int y = DbToY(std::max(DB_MIN, mag[k]), h);
        pts.emplace_back(x, y);
    }
    pts.emplace_back(w, h);  // close at bottom-right

    // Filled area
    dc.SetBrush(wxBrush(fill));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawPolygon((int)pts.size(), pts.data());

    // Line on top (skip first/last floor anchor points)
    dc.SetPen(wxPen(line, 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawLines((int)pts.size() - 2, pts.data() + 1);
}

void SpectrumView::UpdateAndRefresh() {
    m_engine.copyInputSamples(m_inBuf.data(),   FFT_SIZE);
    m_engine.copyOutputSamples(m_outBuf.data(), FFT_SIZE);
    ComputeMagnitude(m_inBuf,  m_inMag);
    ComputeMagnitude(m_outBuf, m_outMag);
    Refresh();
}

void SpectrumView::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    wxSize sz = GetClientSize();
    int w = sz.x, h = sz.y;

    dc.SetBackground(wxBrush(wxColour(14, 14, 14)));
    dc.Clear();

    DrawGrid(dc, w, h);

    // Output first (behind), then input on top
    DrawMagnitude(dc, m_outMag, w, h,
                  wxColour(20, 80, 20, 120),    // semi-transparent green fill
                  wxColour(60, 180, 60));

    DrawMagnitude(dc, m_inMag, w, h,
                  wxColour(20, 50, 100, 100),   // semi-transparent blue fill
                  wxColour(60, 140, 220));

    // Legend
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(60, 140, 220));
    dc.DrawText("IN",  w - 28, 4);
    dc.SetTextForeground(wxColour(60, 180, 60));
    dc.DrawText("OUT", w - 28, 16);
}
