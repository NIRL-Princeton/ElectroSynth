//
// Created by Davis Polito on 6/5/25.
//

#ifndef MODULELIST_H
#define MODULELIST_H

#include <tracktion_ValueTreeUtilities.h>

#include "Factory.h"
#include "Identifiers.h"
#include "ProcessorBase.h"
class SynthBase;
class ProcessorBase;
class ModulatorBase;





template <typename T>
class ModuleList : public tracktion::engine::ValueTreeObjectList<T>{
public:
    class Listener {
        public:
        virtual ~Listener() {}
        virtual void moduleListChanged() = 0;
        virtual void moduleAdded(T* newModule) = 0;
        virtual void removeModule(T* moduleToRemove) = 0;
    };


    ModuleList(SynthBase* synth,const ValueTree& );


    void appendChild (const ValueTree& child, UndoManager* undoManager)
    {
    //In templates, names from dependent base classes (i.e. that depend on template parameters like T)
    // are not automatically found unless you use this-> or a fully qualified name.
    //Without this->, the compiler assumes parent might be a local variable or member of ModuleList<T>,
    //and doesn’t look in the base class template.
        this->parent.appendChild(child,undoManager);
    }
    void setValueTree(const ValueTree& v) {
       tracktion::engine::ValueTreeObjectList<T>::parent = v;
    }



    ~ModuleList();
    T* createNewObject(const juce::ValueTree &) override;
    void newObjectAdded (T*) override;
    void objectRemoved (T*) override;
    void objectOrderChanged() override              { }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override{};

    bool isSuitableType (const juce::ValueTree& v) const override
    {
        if constexpr (std::is_same<T, ProcessorBase>::value)
            return v.hasType(IDs::SOUNDMODULE);
        else if constexpr (std::is_same<T, ModulatorBase>::value)
            return v.hasType(IDs::MODULATOR);

    }
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override;



    void deleteObject(T*) override;
    void addListener (Listener* l) { listeners_.push_back (l); }

    void removeListener (Listener* l) {listeners_.erase(
                std::remove(listeners_.begin(), listeners_.end(), l),
                listeners_.end());
    }
    void valueTreeRedirected (juce::ValueTree&) override{
       tracktion::ValueTreeObjectList<T>:: deleteAllObjects();
       tracktion::ValueTreeObjectList<T>:: rebuildObjects();
        for (auto object : tracktion::ValueTreeObjectList<T>::objects)
        {
            newObjectAdded (object);
        }
    };
    ValueTree state;
private:
    Factory<T> factory;
    std::vector<Listener*> listeners_;
    SynthBase* synth_;

    // std::vector<std::shared_ptr<T>> modules_;
};
template <typename T>
class ChainList : public tracktion::ValueTreeObjectList<ModuleList<T>> {

    public:
    class Listener {
    public:
        virtual ~Listener() {}
        virtual void chainChanged() = 0;
        virtual void chainAdded(ModuleList<T>* newModule) = 0;
        virtual void removeChain(ModuleList<T>* moduleToRemove) = 0;
    };
    ChainList(SynthBase* synth,const ValueTree& );


    void appendChild (const ValueTree& child, UndoManager* undoManager)
    {
        //In templates, names from dependent base classes (i.e. that depend on template parameters like T)
        // are not automatically found unless you use this-> or a fully qualified name.
        //Without this->, the compiler assumes parent might be a local variable or member of ModuleList<T>,
        //and doesn’t look in the base class template.
        this->parent.appendChild(child,undoManager);
    }
    void setValueTree(const ValueTree& v) {
        tracktion::engine::ValueTreeObjectList<ModuleList<T>>::parent = v;
    }



    ~ChainList();
    ModuleList<T>* createNewObject(const juce::ValueTree &) override;
    void newObjectAdded ( ModuleList<T>*) override;
    void objectRemoved ( ModuleList<T>*) override;
    void objectOrderChanged() override              { }//resized(); }
    void valueTreeParentChanged (juce::ValueTree&) override{};

    bool isSuitableType (const juce::ValueTree& v) const override
    {
        return v.hasType(IDs::CHAIN);
    }
    void valueTreePropertyChanged (juce::ValueTree& v, const juce::Identifier& i) override{}



    void deleteObject(ModuleList<T>*) override;
    void addListener (Listener* l) { listeners_.push_back (l); }

    void removeListener (Listener* l) {listeners_.erase(
                std::remove(listeners_.begin(), listeners_.end(), l),
                listeners_.end());
    }
    void valueTreeRedirected (juce::ValueTree&) override{
        tracktion::ValueTreeObjectList<ModuleList<T>>:: deleteAllObjects();
        tracktion::ValueTreeObjectList<ModuleList<T>>:: rebuildObjects();
        for (auto object : tracktion::ValueTreeObjectList<ModuleList<T>>::objects)
        {
            newObjectAdded (object);
        }
    };

    // void moduleListChanged() {
    //     for (auto listener: listeners_) {
    //         listeners_->moduleListChanged();
    //     };
    // }
    // void moduleAdded(T* newModule) {
    //     for (auto listener: listeners_) {
    //
    //     }
    // }
    // void removeModule(T* moduleToRemove){}
    std::vector<Listener*> listeners_;

    SynthBase* synth_;
};


#endif //MODULELIST_H

