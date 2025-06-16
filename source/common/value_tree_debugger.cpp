//
// Created by Jim Credland on 21/06/2015.
//
#include "juce_gui_basics/juce_gui_basics.h"
#include "value_tree_debugger.h"
/** Display a tree. */
class ValueTreeDebuggerMain :
public juce::Component
{
public:
    class PropertyEditor :
public juce::PropertyPanel
    {
    public:
        PropertyEditor()
        {
            noEditValue = "not editable";
        }

        void setSource (juce::ValueTree& newSource)
        {
            clear();

            tree = newSource;

            const int maxChars = 200;

            juce::Array<juce::PropertyComponent*> pc;

            for (int i = 0; i < tree.getNumProperties(); ++i)
            {
                const juce::Identifier name = tree.getPropertyName (i).toString();
                juce::Value v = tree.getPropertyAsValue (name, nullptr);
                juce::TextPropertyComponent* tpc;

                if (v.getValue().isObject())
                {
                    tpc = new juce::TextPropertyComponent (noEditValue, name.toString(), maxChars, false);
                    tpc->setEnabled (false);
                }
                else
                {
                    tpc = new juce::TextPropertyComponent (v, name.toString(), maxChars, false);
                }

                pc.add (tpc);
            }

            addProperties (pc);
        }

    private:
        juce::Value noEditValue;
        juce::ValueTree tree;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyEditor);
    };

    class Item :
        public juce::TreeViewItem,
        public juce::ValueTree::Listener
    {
    public:
        Item (PropertyEditor* propertiesEditor, juce::ValueTree tree)
            :
            propertiesEditor (propertiesEditor),
            t (tree)
        {
            t.addListener (this);
        }

        ~Item()
        {
            clearSubItems();
        }

        bool mightContainSubItems()
        {
            return t.getNumChildren() > 0;
        }

        void itemOpennessChanged (bool isNowOpen)
        {
            if (isNowOpen) updateSubItems();
        }

        void updateSubItems()
        {
            std::unique_ptr<juce::XmlElement> openness = getOpennessState();
            clearSubItems();
            int children = t.getNumChildren();

            for (int i = 0; i < children; ++i)
                addSubItem (new Item (propertiesEditor, t.getChild (i)));

            if (openness.get())
                restoreOpennessState (*openness.get());
        }

        void paintItem (juce::Graphics& g, int w, int h)
        {

            juce::Font font;
            juce::Font smallFont (11.0);


            if (isSelected())
                g.fillAll (juce::Colours::white);


            const float padding = 20.0f;

            juce::String typeName = t.getType().toString();

            const float nameWidth = font.getStringWidthFloat (typeName);
            const float propertyX = padding + nameWidth;

            g.setColour (juce::Colours::black);

            g.setFont (font);

            g.drawText (t.getType().toString(), 0, 0, w, h, juce::Justification::left, false);
            g.setColour (juce::Colours::blue);

            juce::String propertySummary;

            for (int i = 0; i < t.getNumProperties(); ++i)
            {
                const juce::Identifier name = t.getPropertyName (i).toString();
                juce::String propertyValue = t.getProperty (name).toString();;
#ifdef JCF_SERIALIZER

                /* Code for handling valuetree serialization classes I use internally.
                 They allow ReferenceCountedObjects that are stored in the tree
                 to save, load and display debug information easily. You won't
                 need this but it makes my life easier to leave it here - Jim. */
                if (t[name].isObject())
                {
                    ReferenceCountedObject* p = t[name].getObject();

                    if (Serializable* s = dynamic_cast<Serializable*> (p))
                        propertyValue = "[[" + s->getDebugInformation() + "]]";
                }

#endif
                propertySummary += " " + name.toString() + "=" + propertyValue;
            }

            g.drawText (propertySummary, propertyX, 0, w - propertyX, h, juce::Justification::left, true);
        }

        void itemSelectionChanged (bool isNowSelected)
        {
            if (isNowSelected)
            {
                t.removeListener (this);
                propertiesEditor->setSource (t);
                t.addListener (this);
            }
        }

        /* Enormous list of ValueTree::Listener options... */
        void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
        {
            if (t != treeWhosePropertyHasChanged) return;

            t.removeListener (this);
            //            if (isSelected())
            //                propertiesEditor->setSource(t);
            repaintItem();
            t.addListener (this);
        }

        void valueTreeChildAdded (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenAdded)
        {
            if (parentTree == t)
                updateSubItems();

            treeHasChanged();
        }

        void valueTreeChildRemoved (juce::ValueTree& parentTree, juce::ValueTree& childWhichHasBeenRemoved, int)
        {
            if (parentTree == t)
                updateSubItems();

            treeHasChanged();
        }
        void valueTreeChildOrderChanged (juce::ValueTree& parentTreeWhoseChildrenHaveMoved, int, int)
        {
            if (parentTreeWhoseChildrenHaveMoved == t)
                updateSubItems();

            treeHasChanged();
        }
        void valueTreeParentChanged (juce::ValueTree& treeWhoseParentHasChanged)
        {
            treeHasChanged();
        }
        void valueTreeRedirected (juce::ValueTree& treeWhichHasBeenChanged)
        {
            if (treeWhichHasBeenChanged == t)
                updateSubItems();

            treeHasChanged();
        }

        /* Works only if the ValueTree isn't updated between calls to getUniqueName. */
        juce::String getUniqueName() const
        {
            if (!t.getParent().isValid()) return "1";

            return juce::String (t.getParent().indexOf (t));
        }

    private:
        PropertyEditor* propertiesEditor;
        juce::ValueTree t;
        juce::Array<juce::Identifier> currentProperties;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Item);
    };

    ValueTreeDebuggerMain() :
        layoutResizer (&layout, 1, false)
    {
        layout.setItemLayout (0, -0.1, -0.9, -0.6);
        layout.setItemLayout (1, 5, 5, 5);
        layout.setItemLayout (2, -0.1, -0.9, -0.4);

        setSize (1000, 700);
        addAndMakeVisible (treeView);
        addAndMakeVisible (propertyEditor);
        addAndMakeVisible (layoutResizer);
    }
    ~ValueTreeDebuggerMain()
    {
        treeView.setRootItem (nullptr);
    }

    void resized() override
    {
        Component* comps[] = { &treeView, &layoutResizer, &propertyEditor };
        layout.layOutComponents (comps, 3, 0, 0, getWidth(), getHeight(), true, true);
    }

    void setTree (juce::ValueTree newTree)
    {
        if (!newTree.isValid())
        {
            treeView.setRootItem (nullptr);
        }
        else if (tree != newTree)
        {
            tree = newTree;
            rootItem = std::make_unique<Item>(&propertyEditor, tree);
            treeView.setRootItem (rootItem.get());
        }
    }

public:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueTreeDebuggerMain);

    std::unique_ptr<Item> rootItem;
    juce::ValueTree tree;
    juce::TreeView treeView;
    PropertyEditor propertyEditor;
    juce::StretchableLayoutManager layout;
    juce::StretchableLayoutResizerBar layoutResizer;
};


ValueTreeDebugger::ValueTreeDebugger()
:
    DocumentWindow ("Value Tree Debugger",
                    juce::Colours::lightgrey,
                    juce::DocumentWindow::allButtons)
{
    construct();
}

ValueTreeDebugger::ValueTreeDebugger(juce::ValueTree & tree)
:
    DocumentWindow ("Value Tree Debugger",
                    juce::Colours::lightgrey,
                    DocumentWindow::allButtons)
{
    construct();
    setSource(tree);
}

void ValueTreeDebugger::construct()
{
    main = std::make_unique<ValueTreeDebuggerMain>();
    setContentNonOwned (main.get(), true);
    setResizable (true, false);
    setUsingNativeTitleBar (true);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}

ValueTreeDebugger::~ValueTreeDebugger()
{
    main->setTree (juce::ValueTree());
}

void ValueTreeDebugger::closeButtonPressed()
{
    setVisible (false);
}

void ValueTreeDebugger::setSource (juce::ValueTree& v)
{
    main->setTree (v);
}
