/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

// Inherit to customise drawing
class MIDIKeyboard : public MidiKeyboardComponent {

    Object* object;

    bool toggleMode = false;
    std::set<int> heldKeys;

public:
    std::function<void(int, int)> noteOn;
    std::function<void(int)> noteOff;

    MIDIKeyboard(Object* parent, MidiKeyboardState& stateToUse, Orientation orientationToUse)
        : MidiKeyboardComponent(stateToUse, orientationToUse)
        , object(parent)
    {
        // Make sure nothing is drawn outside of our custom draw functions
        setColour(MidiKeyboardComponent::whiteNoteColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::keySeparatorLineColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::keyDownOverlayColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::textLabelColourId, Colours::transparentBlack);
        setColour(MidiKeyboardComponent::shadowColourId, Colours::transparentBlack);
    }

    /*  Return the amount of white notes in the current displayed range.
     *  We use this to calculate & resize the keyboard width when more range is added
     *  because setKeyWidth sets the width of white keys
     */
    int getCountOfWhiteNotesInRange()
    {
        /*
         ┌──┬─┬─┬─┬──┬──┬─┬─┬─┬─┬─┬──┐
         │  │┼│ │┼│  │  │┼│ │┼│ │┼│  │
         │  │┼│ │┼│  │  │┼│ │┼│ │┼│  │
         │  └┼┘ └┼┘  │  └┼┘ └┼┘ └┼┘  │
         │ 0 │ 2 │ 4 │ 5 │ 7 │ 9 │11 │
         └───┴───┴───┴───┴───┴───┴───┘
         */
        int count = 0;
        for (int i = getRangeStart(); i <= getRangeEnd(); i++) {
            if (i % 12 == 0 || i % 12 == 2 || i % 12 == 4 || i % 12 == 5 || i % 12 == 7 || i % 12 == 9 || i % 12 == 11) {
                count++;
            }
        }
        return count;
    }

    bool mouseDownOnKey(int midiNoteNumber, MouseEvent const& e) override
    {
        if (toggleMode) {
            if (heldKeys.count(midiNoteNumber)) {
                heldKeys.erase(midiNoteNumber);
                noteOff(midiNoteNumber);
            } else {
                heldKeys.insert(midiNoteNumber);
                noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);
            }
        } else {
            heldKeys.insert(midiNoteNumber);
            noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);
        }

        repaint();
        return false;
    }

    bool mouseDraggedToKey(int midiNoteNumber, MouseEvent const& e) override
    {
        if (!toggleMode && !heldKeys.count(midiNoteNumber)) {
            for (auto& note : heldKeys) {
                noteOff(note);
            }

            heldKeys.clear();

            heldKeys.insert(midiNoteNumber);
            noteOn(midiNoteNumber, getNoteAndVelocityAtPosition(e.position).velocity * 127);

            repaint();
        }

        return true;
    }

    void mouseUpOnKey(int midiNoteNumber, MouseEvent const& e) override
    {
        if (!toggleMode) {
            heldKeys.erase(midiNoteNumber);
            noteOff(midiNoteNumber);
        }

        repaint();
    }

    // Override to fix bug in JUCE
    void mouseUp(MouseEvent const& e) override
    {
        auto keys = heldKeys;
        for (auto& key : keys) {
            mouseUpOnKey(key, e);
        }
    }

    void setToggleMode(bool enableToggleMode)
    {
        toggleMode = enableToggleMode;
    }

    void drawWhiteNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour lineColour, Colour textColour) override
    {
        // TODO: this should be a theme preference, or setting for keyboard
        // yeah but we can set a less ugly default colour for now!

        isDown = heldKeys.count(midiNoteNumber);

        auto c = Colour(225, 225, 225);
        if (isOver)
            c = Colour(235, 235, 235);
        if (isDown)
            c = object->findColour(PlugDataColour::dataColourId);

        area = area.reduced(0.0f, 0.5f);

        g.setColour(c);

        // Rounded first and last keys to fix objects
        if (midiNoteNumber == getRangeStart()) {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), Corners::objectCornerRadius, Corners::objectCornerRadius, true, false, true, false);

            g.fillPath(keyPath);
        } else if (midiNoteNumber == getRangeEnd()) {
            Path keyPath;
            keyPath.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), Corners::objectCornerRadius, Corners::objectCornerRadius, false, true, false, true);

            g.fillPath(keyPath);
        } else {
            g.fillRect(area);
        }

        // don't draw the first separator line to fix object look
        if (midiNoteNumber != getRangeStart()) {
            g.setColour(object->findColour(PlugDataColour::outlineColourId));
            g.fillRect(area.withWidth(1.0f));
        }

        // draw C octave numbers
        if (!(midiNoteNumber % 12)) {
            Array<int> glyphs;
            Array<float> offsets;
            auto font = Fonts::getCurrentFont();
            Path p;
            Path outline;
            font.getGlyphPositions(String(floor(midiNoteNumber / 12) - 1), glyphs, offsets);

            auto rectangle = area.withTrimmedTop(area.proportionOfHeight(0.8f)).reduced(area.getWidth() / 6.0f);

            int prev_size = 0;
            AffineTransform transform;
            for (auto glyph : glyphs) {
                font.getTypefacePtr()->getOutlineForGlyph(glyph, p);
                if (glyphs.size() > 1) {
                    prev_size = outline.getBounds().getWidth();
                }
                transform = AffineTransform::scale(20).followedBy(AffineTransform::translation(prev_size, 0.0));
                outline.addPath(p, transform);
                p.clear();
            }

            // TODO: C octave number text colour should be a theme prefernece or setting
            g.setColour(Colour(90, 90, 90));
            g.fillPath(outline, outline.getTransformToScaleToFit(rectangle, true));
        }
    }

    void drawBlackNote(int midiNoteNumber, Graphics& g, Rectangle<float> area, bool isDown, bool isOver, Colour noteFillColour) override
    {
        // TODO: this should be a theme preference, or setting for keyboard
        auto c = Colour(90, 90, 90);

        isDown = heldKeys.count(midiNoteNumber);

        if (isOver)
            c = Colour(101, 101, 101);
        if (isDown)
            c = object->findColour(PlugDataColour::dataColourId).darker(0.5f);

        g.setColour(c);
        g.fillRect(area);
    }
};
// ELSE keyboard
class KeyboardObject final : public ObjectBase
    , public Timer {

    Value lowC;
    Value octaves;
    int numWhiteKeys = 0;

    Value sendSymbol;
    Value receiveSymbol;
    Value toggleMode;

    MidiKeyboardState state;
    MIDIKeyboard keyboard;
    int keyRatio = 5;

public:
    KeyboardObject(void* ptr, Object* object)
        : ObjectBase(ptr, object)
        , keyboard(object, state, MidiKeyboardComponent::horizontalKeyboard)
    {
        keyboard.setMidiChannel(1);
        keyboard.setScrollButtonsVisible(false);

        keyboard.noteOn = [this](int note, int velocity) {
            auto* elseKeyboard = static_cast<t_fake_keyboard*>(this->ptr);

            int ac = 2;
            t_atom at[2];
            SETFLOAT(at, note);
            SETFLOAT(at + 1, velocity);

            pd->lockAudioThread();

            outlet_list(elseKeyboard->x_out, gensym("list"), ac, at);
            if (elseKeyboard->x_send != gensym("") && elseKeyboard->x_send->s_thing)
                pd_list(elseKeyboard->x_send->s_thing, gensym("list"), ac, at);

            pd->unlockAudioThread();
        };

        keyboard.noteOff = [this](int note) {
            auto* elseKeyboard = static_cast<t_fake_keyboard*>(this->ptr);

            pd->lockAudioThread();

            int ac = 2;
            t_atom at[2];
            SETFLOAT(at, note);
            SETFLOAT(at + 1, 0);

            outlet_list(elseKeyboard->x_out, gensym("list"), ac, at);
            if (elseKeyboard->x_send != gensym("") && elseKeyboard->x_send->s_thing)
                pd_list(elseKeyboard->x_send->s_thing, gensym("list"), ac, at);

            pd->unlockAudioThread();
        };

        addAndMakeVisible(keyboard);

        objectParameters.addParamInt("Start octave", cGeneral, &lowC, 2);
        objectParameters.addParamInt("Num. octaves", cGeneral, &octaves, 4);
        objectParameters.addParamBool("Toggle Mode", cGeneral, &toggleMode, { "Off", "On" }, 0);
        objectParameters.addParamReceiveSymbol(&receiveSymbol);
        objectParameters.addParamSendSymbol(&sendSymbol);

        startTimer(150);
    }

    void update() override
    {
        auto* elseKeyboard = static_cast<t_fake_keyboard*>(ptr);
        lowC.setValue(elseKeyboard->x_low_c);
        octaves.setValue(elseKeyboard->x_octaves);
        toggleMode.setValue(elseKeyboard->x_toggle_mode);

        auto sndSym = String::fromUTF8(elseKeyboard->x_send->s_name);
        auto rcvSym = String::fromUTF8(elseKeyboard->x_receive->s_name);

        sendSymbol = sndSym != "empty" ? sndSym : "";
        receiveSymbol = rcvSym != "empty" ? rcvSym : "";

        MessageManager::callAsync([this] {
            updateAspectRatio();

            // Call async to make sure pd obj has updated
            object->updateBounds();
        });
    }

    Rectangle<int> getPdBounds() override
    {
        pd->lockAudioThread();

        int x, y, w, h;
        libpd_get_object_bounds(cnv->patch.getPointer(), ptr, &x, &y, &w, &h);

        auto* elseKeyboard = static_cast<t_fake_keyboard*>(ptr);
        auto bounds = Rectangle<int>(x, y, elseKeyboard->x_space * numWhiteKeys, elseKeyboard->x_height);

        pd->unlockAudioThread();

        return bounds;
    }

    void setPdBounds(Rectangle<int> b) override
    {
        libpd_moveobj(cnv->patch.getPointer(), static_cast<t_gobj*>(ptr), b.getX(), b.getY());

        auto* elseKeyboard = static_cast<t_fake_keyboard*>(ptr);
        elseKeyboard->x_height = b.getHeight();
    }

    void resized() override
    {
        float keyWidth = static_cast<float>(object->getHeight() - Object::doubleMargin) / keyRatio;

        if (keyWidth <= 0)
            return;

        keyboard.setKeyWidth(keyWidth);

        auto* elseKeyboard = static_cast<t_fake_keyboard*>(ptr);
        elseKeyboard->x_space = keyWidth;

        keyboard.setSize(keyWidth * numWhiteKeys, object->getHeight() - Object::doubleMargin);
    }

    void updateAspectRatio()
    {
        int numOctaves = getValue<int>(octaves);
        int lowest = getValue<int>(lowC);
        int highest = std::clamp<int>(lowest + 1 + numOctaves, 0, 11);
        keyboard.setAvailableRange(((lowest + 1) * 12), std::min((highest * 12) - 1, 127));

        float horizontalLength = keyboard.getTotalKeyboardWidth();

        // we only need to get the amount of white notes when the number of keys has changed
        numWhiteKeys = keyboard.getCountOfWhiteNotesInRange();

        object->setSize(horizontalLength + Object::doubleMargin, object->getHeight());
        constrainer->setFixedAspectRatio(horizontalLength / static_cast<float>(object->getHeight() - Object::doubleMargin));
        constrainer->setMinimumSize((object->minimumSize / 5.0f) * numWhiteKeys, object->minimumSize);
    }

    void valueChanged(Value& value) override
    {
        auto* elseKeyboard = static_cast<t_fake_keyboard*>(ptr);

        if (value.refersToSameSourceAs(lowC)) {
            lowC = std::clamp<int>(getValue<int>(lowC), -1, 9);
            elseKeyboard->x_low_c = getValue<int>(lowC);
            updateAspectRatio();
        } else if (value.refersToSameSourceAs(octaves)) {
            octaves = std::clamp<int>(getValue<int>(octaves), 1, 11);
            elseKeyboard->x_octaves = getValue<int>(octaves);
            updateAspectRatio();
        } else if (value.refersToSameSourceAs(sendSymbol)) {
            auto symbol = sendSymbol.toString();
            pd->sendDirectMessage(ptr, "send", { symbol });
        } else if (value.refersToSameSourceAs(receiveSymbol)) {
            auto symbol = receiveSymbol.toString();
            pd->sendDirectMessage(ptr, "receive", { symbol });
        } else if (value.refersToSameSourceAs(toggleMode)) {
            auto toggle = getValue<int>(toggleMode);
            pd->sendDirectMessage(ptr, "toggle", { toggle });
            keyboard.setToggleMode(toggle);
        }
    }

    void updateValue()
    {
        auto* elseKeyboard = static_cast<t_fake_keyboard*>(ptr);

        for (int i = keyboard.getRangeStart(); i < keyboard.getRangeEnd(); i++) {
            if (elseKeyboard->x_tgl_notes[i] && !(state.isNoteOn(2, i) && state.isNoteOn(1, i))) {
                state.noteOn(2, i, 1.0f);
            }
            if (!elseKeyboard->x_tgl_notes[i] && !(state.isNoteOn(2, i) && state.isNoteOn(1, i))) {
                state.noteOff(2, i, 1.0f);
            }
        }
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("float"),
            hash("list"),
            hash("set"),
            hash("lowc"),
            hash("oct"),
            hash("8ves"),
            hash("send"),
            hash("receive"),
            hash("toggle")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("float"):
        case hash("list"):
        case hash("set"): {
            updateValue();
            break;
        }
        case hash("lowc"): {
            setParameterExcludingListener(lowC, static_cast<int>(atoms[0].getFloat()));
            updateAspectRatio();
            break;
        }
        case hash("oct"): {
            setParameterExcludingListener(lowC, std::clamp<int>(getValue<int>(lowC) + static_cast<int>(atoms[0].getFloat()), -1, 9));
            updateAspectRatio();
            break;
        }
        case hash("8ves"): {
            setParameterExcludingListener(octaves, static_cast<int>(atoms[0].getFloat()));
            updateAspectRatio();
            break;
        }
        case hash("send"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(sendSymbol, atoms[0].getSymbol());
            break;
        }
        case hash("receive"): {
            if (atoms.size() >= 1)
                setParameterExcludingListener(receiveSymbol, atoms[0].getSymbol());
            break;
        }
        case hash("toggle"): {
            setParameterExcludingListener(toggleMode, atoms[0].getFloat());
            keyboard.setToggleMode(static_cast<bool>(atoms[0].getFloat()));
        }
        default:
            break;
        }
    }

    void timerCallback() override
    {
        updateValue();
    }

    void paintOverChildren(Graphics& g) override
    {
        bool selected = object->isSelected() && !cnv->isGraph;
        auto outlineColour = object->findColour(selected ? PlugDataColour::objectSelectedOutlineColourId : PlugDataColour::objectOutlineColourId);

        g.setColour(outlineColour);
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), Corners::objectCornerRadius, 1.0f);
    }
};
