#include <wx/wx.h>
#include "audio/AudioEngine.h"
#include "ui/MainFrame.h"

class GuitarAmpApp : public wxApp {
public:
    bool OnInit() override {
        m_engine = std::make_unique<AudioEngine>();

        if (!m_engine->init()) {
            wxMessageBox(
                "Failed to open audio device.\n\n"
                "Make sure your audio interface is connected and not\n"
                "in use by another application, then try again.",
                "Audio Error", wxOK | wxICON_ERROR);
            return false;
        }

        auto* frame = new MainFrame(*m_engine);
        frame->Show();
        return true;
    }

    int OnExit() override {
        m_engine.reset();
        return 0;
    }

private:
    std::unique_ptr<AudioEngine> m_engine;
};

wxIMPLEMENT_APP(GuitarAmpApp);
