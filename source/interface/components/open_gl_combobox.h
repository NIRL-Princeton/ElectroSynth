//
// Created by Jeffrey Snyder on 11/19/24.
//

#ifndef OPEN_GL_COMBOBOX_H
#define OPEN_GL_COMBOBOX_H
#include "juce_data_structures/juce_data_structures.h"
#include "open_gl_component.h"
#include "open_gl_image_component.h"
#include "default_look_and_feel.h"

class OpenGLComboBox : public OpenGlAutoImageComponent<juce::ComboBox>, juce::ComboBox::Listener{

public:
    OpenGLComboBox() : OpenGlAutoImageComponent<juce::ComboBox> ("Combo box")
    {
        image_component_ = std::make_shared<OpenGlImageComponent> ();
        setLookAndFeel(DefaultLookAndFeel::instance());
        image_component_->setComponent(this);
        addListener(this);
    }
    virtual void resized() override
    {
        OpenGlAutoImageComponent<juce::ComboBox>::resized();
        redoImage();
    }
    void comboBoxChanged(ComboBox *comboBoxThatHasChanged) override
{
    redoImage();
}
};



#endif //OPEN_GL_COMBOBOX_H
