#pragma once

#include "Data/Defs.h"

#include <args.hxx>
#include <set>
#include <map>

namespace Haboob
{
  // Collection of *unifying* environment objects
  class Environment;
  class EnvironmentVariable;
  class EnvironmentGroup;

  class EnvironmentBase
  {
    public:
    EnvironmentBase(bool showGUI = true);

    inline bool shouldUseGUI() const { return hasGUI; }
    inline std::string getName() { return name; }

    protected:
    bool hasGUI;
    std::string name;
  };

  class EnvironmentVariable : public EnvironmentBase
  {
    friend EnvironmentGroup;

    public:
    enum class Type
    {
      Bool,
      Float,
      Float2,
      Float3,
      Float4,
      Flags,
      Int,
      Int2,
      Int3,
      Int4,
      UInt4
    };

    EnvironmentVariable(Type type, args::FlagBase* hook = nullptr, void* reflectionStorage = nullptr, bool showGUI = true);
    ~EnvironmentVariable();

    args::FlagBase* getArg() const { return variableHook; }

    inline const void* getGUISetting1() const { return guiSetting1; }
    inline const void* getGUISetting2() const { return guiSetting2; }
    inline const void* getGUISetting3() const { return guiSetting3; }

    inline EnvironmentVariable* setName(const std::string& newName) { name = newName; return this; }

    void reflect();

    // Hacks
    EnvironmentVariable* setGUISettings(float speed, int min, int max);
    EnvironmentVariable* setGUISettings(float speed, float min, float max);
    EnvironmentVariable* setGUISettings(float speed, UInt min, UInt max);
    EnvironmentVariable* setGUISettings(
      #include "Data/Defs.h"
      
      UInt bitMask);
    void imguiGUIShow();

    protected:
    void makeGUISettings();
    void killGUISettings();

    private:
    Type baseType;
    args::FlagBase* variableHook; // The program argument hook (voidable)
    void* destination; // The destination storage to copy to (voidable)

    void* guiSetting1;
    void* guiSetting2;
    void* guiSetting3;
  };

  class EnvironmentGroup : public EnvironmentBase
  {
    friend Environment;

    public:
    EnvironmentGroup(args::Group* hook = nullptr, bool showGUI = true);
    ~EnvironmentGroup();

    args::Group* getArgGroup() const { return groupHook; }
    void setArgGroup(args::Group* newGroup = nullptr) { groupHook = newGroup; } // Warning, does not kill storage
    
    void reflectVariables();
    void imguiGUIShow();

    inline EnvironmentGroup* setName(const std::string& newName) { name = newName; return this; }

    inline void addVariable(EnvironmentVariable* variable) { if (variable) { variables.insert(variable); } }
    inline void addChildGroup(EnvironmentGroup* group) { if (group) { groups.insert(group); } }

    inline std::set<EnvironmentGroup*>& getChildGroups() { return groups; }
    inline std::set<EnvironmentVariable*>& getVariables() { return variables; }

    protected:
    args::Group* groupHook; // The group program argument hook (voidable)
    std::set<EnvironmentVariable*> variables;
    std::set<EnvironmentGroup*> groups;
  };

  class Environment
  {
    public:
    Environment(args::Group* hook);
    ~Environment();

    inline args::Group* getArgRoot() { return rootHook; }
    inline EnvironmentGroup& getRoot() { return groups; }

    // Does some initial cleaning
    void setupEnvironment();

    protected:
    void appendVariables(EnvironmentGroup* group, const std::string& baseName);

    private:
    args::Group* rootHook;
    EnvironmentGroup groups;
    std::map<std::string, EnvironmentVariable*> variableCollection;
  };

}