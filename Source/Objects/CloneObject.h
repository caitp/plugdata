/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C" {
t_glist* clone_get_instance(t_gobj*, int);
int clone_get_n(t_gobj*);
}

class CloneObject final : public TextBase {

    pd::Patch::Ptr subpatch;

public:
    CloneObject(void* obj, Object* object)
        : TextBase(obj, object)
    {
        auto* gobj = static_cast<t_gobj*>(ptr);
        if (clone_get_n(gobj) > 0) {
            subpatch = new pd::Patch(clone_get_instance(gobj, 0), cnv->pd, false);
        } else {
            subpatch = new pd::Patch(nullptr, nullptr, false);
        }
    }

    ~CloneObject()
    {
        closeOpenedSubpatchers();
    }

    pd::Patch::Ptr getPatch() override
    {
        return subpatch;
    }

    String getText() override
    {
        auto* sym = static_cast<t_fake_clone*>(ptr)->x_s;

        if (!sym || !sym->s_name)
            return "";

        return String::fromUTF8(sym->s_name);
    }

    bool canOpenFromMenu() override
    {
        return true;
    }

    void openFromMenu() override
    {
        openSubpatch();
    }

    std::vector<hash32> getAllMessages() override
    {
        return {
            hash("vis")
        };
    }

    void receiveObjectMessage(String const& symbol, std::vector<pd::Atom>& atoms) override
    {
        switch (hash(symbol)) {
        case hash("vis"): {
            if (atoms.size() > 2) {
                // TODO: implement this!
            }
            break;
        }
        default:
            break;
        }
    }
};
