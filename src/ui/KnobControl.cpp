#include "KnobControl.h"
#include <wx/dcbuffer.h>
#include <cmath>

wxDEFINE_EVENT(wxEVT_KNOB_CHANGED, wxCommandEvent);


wxBEGIN_EVENT_TABLE(KnobControl, wxPanel)
    EVT_PAINT(KnobControl::OnPaint)
    EVT_LEFT_DOWN(KnobControl::OnMouseDown)
    EVT_LEFT_UP(KnobControl::OnMouseUp)
    EVT_MOTION(KnobControl::OnMouseMove)
    EVT_MOUSEWHEEL(KnobControl::OnMouseWheel)
    EVT_MOUSE_CAPTURE_LOST(KnobControl::OnMouseCaptureLost)
wxEND_EVENT_TABLE()


KnobControl::KnobControl(wxWindow* parent, wxWindowID id,
                         const wxString& label,
                         float minVal, float maxVal, float defaultVal,
                         const wxPoint& pos, const wxSize& size)
    : wxPanel(parent, id, pos, size)
    , m_value(defaultVal), m_minVal(minVal), m_maxVal(maxVal)
    , m_label(label)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void KnobControl::SetValue(float val, bool notify) {
    m_value = std::max(m_minVal, std::min(m_maxVal, val));
    Refresh();
    if (notify) FireChange();
}

void KnobControl::FireChange() {
    wxCommandEvent evt(wxEVT_KNOB_CHANGED, GetId());
    evt.SetEventObject(this);
    ProcessEvent(evt);
}

void KnobControl::Draw(wxDC& dc) {
    wxSize sz = GetClientSize();
    int w = sz.GetWidth(), h = sz.GetHeight();

    // Background
    dc.SetBackground(wxBrush(wxColour(28, 28, 28)));
    dc.Clear();

    constexpr int LABEL_H = 32;
    int knobH = h - LABEL_H;
    int cx = w / 2, cy = knobH / 2;
    int R = std::min(cx, cy) - 6;

    //  Outer ring 
    dc.SetBrush(wxBrush(wxColour(55, 55, 58)));
    dc.SetPen(wxPen(wxColour(75, 75, 78), 1));
    dc.DrawCircle(cx, cy, R);

    // Knob body 
    dc.SetBrush(wxBrush(wxColour(42, 42, 42)));
    dc.SetPen(wxPen(wxColour(62, 62, 62), 1));
    dc.DrawCircle(cx, cy, R - 5);

    // Travel arc ticks
    dc.SetPen(wxPen(wxColour(95, 95, 95), 1));
    for (int i = 0; i <= 10; ++i) {
        double t     = i / 10.0;
        double angle = (240.0 - t * 300.0) * M_PI / 180.0;
        int r1 = R - 3, r2 = R + 3;
        dc.DrawLine(
            cx + (int)(r1 * std::cos(angle)), cy - (int)(r1 * std::sin(angle)),
            cx + (int)(r2 * std::cos(angle)), cy - (int)(r2 * std::sin(angle))
        );
    }

    //  Indicator line 
    float norm  = (m_value - m_minVal) / (m_maxVal - m_minVal);
    double angle = (240.0 - norm * 300.0) * M_PI / 180.0;
    int lineR = R - 8;
    dc.SetPen(wxPen(wxColour(230, 230, 230), 2));
    dc.DrawLine(cx, cy,
                cx + (int)(lineR * std::cos(angle)),
                cy - (int)(lineR * std::sin(angle)));

    // Center dot
    dc.SetBrush(wxBrush(wxColour(200, 200, 200)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawCircle(cx, cy, 3);

    // Label 
    dc.SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    dc.SetTextForeground(wxColour(210, 210, 195));

    // Support two-line labels splitting with '\n'
    wxString top = m_label, bot;
    int nl = m_label.Find('\n');
    if (nl != wxNOT_FOUND) {
        top = m_label.Left(nl);
        bot = m_label.Mid(nl + 1);
    }

    int labelY = knobH + 4;
    auto drawCentered = [&](const wxString& s, int y) {
        wxSize ts = dc.GetTextExtent(s);
        dc.DrawText(s, (w - ts.x) / 2, y);
    };
    drawCentered(top, labelY);
    if (!bot.IsEmpty()) drawCentered(bot, labelY + 13);

    // Value (small, below label)
    dc.SetFont(wxFont(7, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    dc.SetTextForeground(wxColour(140, 160, 140));
    wxString valStr = wxString::Format("%.1f", m_value);
    wxSize vs = dc.GetTextExtent(valStr);
    dc.DrawText(valStr, (w - vs.x) / 2, labelY + (bot.IsEmpty() ? 13 : 26));
}

void KnobControl::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);
    Draw(dc);
}

void KnobControl::OnMouseDown(wxMouseEvent& evt) {
    m_dragging     = true;
    m_dragStart    = evt.GetPosition();
    m_dragStartVal = m_value;
    CaptureMouse();
}

void KnobControl::OnMouseUp(wxMouseEvent&) {
    if (m_dragging) {
        m_dragging = false;
        if (HasCapture()) ReleaseMouse();
    }
}

void KnobControl::OnMouseMove(wxMouseEvent& evt) {
    if (!m_dragging) return;
    int dy    = m_dragStart.y - evt.GetPosition().y;  // drag up → increase
    float delta = (float)dy / 160.0f * (m_maxVal - m_minVal);
    SetValue(m_dragStartVal + delta, true);
}

void KnobControl::OnMouseWheel(wxMouseEvent& evt) {
    float step = (m_maxVal - m_minVal) / 50.0f;
    SetValue(m_value + (evt.GetWheelRotation() > 0 ? step : -step), true);
}

void KnobControl::OnMouseCaptureLost(wxMouseCaptureLostEvent&) {
    m_dragging = false;
}
