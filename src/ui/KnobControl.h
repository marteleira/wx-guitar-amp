#pragma once
#include <wx/wx.h>

wxDECLARE_EVENT(wxEVT_KNOB_CHANGED, wxCommandEvent);


class KnobControl : public wxPanel {
public:
    KnobControl(wxWindow* parent, wxWindowID id,
                const wxString& label,
                float minVal = 0.0f, float maxVal = 10.0f, float defaultVal = 5.0f,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxSize(88, 110));

    float GetValue() const { return m_value; }
    void  SetValue(float val, bool notify = false);

private:
    void OnPaint(wxPaintEvent&);
    void OnMouseDown(wxMouseEvent&);
    void OnMouseUp(wxMouseEvent&);
    void OnMouseMove(wxMouseEvent&);
    void OnMouseWheel(wxMouseEvent&);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent&);

    void Draw(wxDC& dc);
    void FireChange();

    float    m_value, m_minVal, m_maxVal;
    wxString m_label;
    bool     m_dragging     = false;
    wxPoint  m_dragStart;
    float    m_dragStartVal = 0.0f;

    wxDECLARE_EVENT_TABLE();
};
