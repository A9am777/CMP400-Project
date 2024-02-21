#include "Data/EnvironmentArgs.h"
#include <imgui.h>

namespace Haboob
{

  EnvironmentBase::EnvironmentBase(bool showGUI)
  {
    hasGUI = showGUI;
  }

  EnvironmentGroup::EnvironmentGroup(args::Group* hook, bool showGUI) : groupHook{ hook }, EnvironmentBase(showGUI)
  {
  }

  EnvironmentGroup::~EnvironmentGroup()
  {
    if (groupHook)
    {
      delete groupHook; // Uses virtual
    }
  }

  void EnvironmentGroup::reflectVariables()
  {
    for (auto group : groups)
    {
      group->reflectVariables();
    }

    for (auto variable : variables)
    {
      variable->reflect();
    }
  }

  void EnvironmentGroup::imguiGUIShow()
  {
    bool show = hasGUI;
    if (name.length())
    {
      show = show && ImGui::CollapsingHeader(name.c_str());
    }

    if (show)
    {
      for (auto variable : variables)
      {
        variable->imguiGUIShow();
      }

      for (auto group : groups)
      {
        group->imguiGUIShow();
      }
    }
  }

  EnvironmentVariable::EnvironmentVariable(Type type, args::FlagBase* hook, void* reflectionStorage, bool showGUI) : EnvironmentBase(showGUI)
  {
    baseType = type;
    variableHook = hook;
    destination = reflectionStorage;

    makeGUISettings();

    if (variableHook)
    {
      name = hook->Name();
    }
  }

  EnvironmentVariable::~EnvironmentVariable()
  {
    if (variableHook)
    {
      delete variableHook; // Uses virtual
    }

    killGUISettings();
  }

  void EnvironmentVariable::reflect()
  {
    if (!destination || !variableHook) { return; }
    if (!variableHook->HasFlag() || !variableHook->Matched()) { return; }
    
    switch (baseType)
    {
      default:
        throw std::exception("Not Implemented");
      case Type::Bool:
        *(bool*)destination = ((args::ValueFlag<bool>*)variableHook)->Get();
        break;
      case Type::Float:
        *(float*)destination = ((args::ValueFlag<float>*)variableHook)->Get();
        break;
      //case Type::Float2:

        break;
      //case Type::Float3:

        break;
      //case Type::Float4:

        break;
      case Type::Flags:
        *(UInt*)destination = ((args::ValueFlag<UInt>*)variableHook)->Get() ? BitSet(*(UInt*)destination, *(UInt*)guiSetting1) : BitClear(*(UInt*)destination, *(UInt*)guiSetting1);
        break;
      case Type::Int:
        *(int*)destination = ((args::ValueFlag<int>*)variableHook)->Get();
        break;
      //case Type::Int2:

        break;
      //case Type::Int3:

        break;
      //case Type::Int4:

        break;
      //case Type::UInt4:

        break;
    }
  }

  EnvironmentVariable* EnvironmentVariable::setGUISettings(float speed, int min, int max)
  {
    *(float*)guiSetting1 = speed;
    *(int*)guiSetting2 = min;
    *(int*)guiSetting3 = max;

    return this;
  }

  EnvironmentVariable* EnvironmentVariable::setGUISettings(float speed, float min, float max)
  {
    *(float*)guiSetting1 = speed;
    *(float*)guiSetting2 = min;
    *(float*)guiSetting3 = max;

    return this;
  }

  EnvironmentVariable* EnvironmentVariable::setGUISettings(float speed, UInt min, UInt max)
  {
    *(float*)guiSetting1 = speed;
    *(UInt*)guiSetting2 = min;
    *(UInt*)guiSetting3 = max;

    return this;
  }

  EnvironmentVariable* EnvironmentVariable::setGUISettings(UInt bitMask)
  {
    *(UInt*)guiSetting1 = bitMask;

    return this;
  }

  void EnvironmentVariable::imguiGUIShow()
  {
    if (!hasGUI || !destination) { return; }

    switch (baseType)
    {
      default:
        throw std::exception("Not Implemented");
      case Type::Symbolic:
        // Nothing, just symbolic
        break;
      case Type::Bool:
        break;
      case Type::Float:
        ImGui::DragFloat(name.c_str(), (float*)destination, *(float*)guiSetting1, *(float*)guiSetting2, *(float*)guiSetting3);
        break;
      case Type::Float2:
        ImGui::DragFloat2(name.c_str(), (float*)destination, *(float*)guiSetting1, *(float*)guiSetting2, *(float*)guiSetting3);
        break;
      case Type::Float3:
        ImGui::DragFloat3(name.c_str(), (float*)destination, *(float*)guiSetting1, *(float*)guiSetting2, *(float*)guiSetting3);
        break;
      case Type::Float4:
        ImGui::DragFloat4(name.c_str(), (float*)destination, *(float*)guiSetting1, *(float*)guiSetting2, *(float*)guiSetting3);
        break;
      case Type::Flags:
        ImGui::CheckboxFlags(name.c_str(), (UInt*)destination, *(UInt*)guiSetting1);
        break;
      case Type::Int:
        ImGui::DragInt(name.c_str(), (int*)destination, *(float*)guiSetting1, *(int*)guiSetting2, *(int*)guiSetting3);
        break;
      case Type::Int2:
        ImGui::DragInt2(name.c_str(), (int*)destination, *(float*)guiSetting1, *(int*)guiSetting2, *(int*)guiSetting3);
        break;
      case Type::Int3:
        ImGui::DragInt3(name.c_str(), (int*)destination, *(float*)guiSetting1, *(int*)guiSetting2, *(int*)guiSetting3);
        break;
      case Type::Int4:
        ImGui::DragInt4(name.c_str(), (int*)destination, *(float*)guiSetting1, *(int*)guiSetting2, *(int*)guiSetting3);
        break;
      case Type::UInt4:
        ImGui::DragScalarN(name.c_str(), ImGuiDataType_U32, (UInt*)destination, 4);
        break;
    }
  }

  void EnvironmentVariable::makeGUISettings()
  {
    switch (baseType)
    {
      default:
        throw std::exception("Not Implemented");
      case Type::Symbolic:
        // Nothing, just symbolic
        break;
      case Type::Bool:
        break;
      case Type::Float:
      case Type::Float2:
      case Type::Float3:
      case Type::Float4:
        guiSetting1 = new float;
        guiSetting2 = new float;
        guiSetting3 = new float;
        break;
      case Type::Flags:
        guiSetting1 = new UInt;
        break;
      case Type::Int:
      case Type::Int2:
      case Type::Int3:
      case Type::Int4:
        guiSetting1 = new float;
        guiSetting2 = new int;
        guiSetting3 = new int;
        break;
      case Type::UInt4:
        guiSetting1 = new float;
        guiSetting2 = new UInt;
        guiSetting3 = new UInt;
        break;
    }
  }

  void EnvironmentVariable::killGUISettings()
  {
    switch (baseType)
    {
      default:
        throw std::exception("Not Implemented");
      case Type::Symbolic:
        // Nothing, just symbolic
        break;
      case Type::Bool:
        break;
      case Type::Float:
      case Type::Float2:
      case Type::Float3:
      case Type::Float4:
        delete (float*)guiSetting1;
        delete (float*)guiSetting2;
        delete (float*)guiSetting3;
        break;
      case Type::Flags:
        delete (UInt*)guiSetting1;
        break;
      case Type::Int:
      case Type::Int2:
      case Type::Int3:
      case Type::Int4:
        delete (float*)guiSetting1;
        delete (int*)guiSetting2;
        delete (int*)guiSetting3;
        break;
      case Type::UInt4:
        delete (float*)guiSetting1;
        delete (UInt*)guiSetting2;
        delete (UInt*)guiSetting3;
        break;
    }

    guiSetting1 = nullptr;
    guiSetting2 = nullptr;
    guiSetting3 = nullptr;
  }

  Environment::Environment(args::Group* hook)
  {
    rootHook = hook;

    groups.setArgGroup(rootHook);
  }

  Environment::~Environment()
  {
    groups.setArgGroup(nullptr); // Forget storage -> no ownership

    setupEnvironment(); // Re-register all variables

    for (auto& variable : variableCollection)
    {
      delete variable.second;
    }
  }

  void Environment::setupEnvironment()
  {
    variableCollection.clear();

    appendVariables(&groups, "");
  }

  void Environment::appendVariables(EnvironmentGroup* group, const std::string& baseName)
  {
    if (!group) { return; }

    static constexpr Literal delim = "::";

    for (auto childGroup : group->getChildGroups())
    {
      appendVariables(childGroup, baseName + childGroup->getName() + delim);
    }

    for (auto variable : group->getVariables())
    {
      variableCollection.insert({ baseName + variable->getName(), variable });
    }
  }

}