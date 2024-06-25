#pragma once
#include <wx/wx.h>
#include <vector>
#include <complex>
#include "audio/AudioEngine.h"

class SpectrumView : public wxPanel {
public:
    SpectrumView(wxWindow* parent, const AudioEngine& engine,
                 const wxSize& size = wxSize(400, 160));

    void UpdateAndRefresh();

private:
    void OnPaint(wxPaintEvent&);

    void ComputeMagnitude(const std::vector<float>& samples, std::vector<float>& outMag);
    void DrawGrid(wxDC& dc, int w, int h);
    void DrawMagnitude(wxDC& dc, const std::vector<float>& mag,
                       int w, int h, const wxColour& fill, const wxColour& line);

    int FreqToX(float hz, int w)  const;
    int DbToY  (float db, int h)  const;

    const AudioEngine& m_engine;

    static constexpr int   FFT_SIZE = 1024;
    static constexpr float DB_MIN   = -80.0f;
    static constexpr float DB_MAX   =   0.0f;
    static constexpr float FREQ_MIN =  20.0f;
    static constexpr float FREQ_MAX = 20000.0f;

    std::vector<float> m_inBuf, m_outBuf;
    std::vector<float> m_inMag, m_outMag;

    wxDECLARE_EVENT_TABLE();
};
