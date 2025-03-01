#pragma once
#include <core/defines.h>
#include <core/events/event.h>
#include <core/node_system/node.h>

namespace destan::core
{

class Layer
{
    Layer(const String& name = "Layer") : m_name(name) {}
    virtual ~Layer() = default;

    virtual void On_Attach() {}
    virtual void On_Detach() {}
    virtual void On_Update(float delta_time) {}
    virtual void On_Event(Event& event) {}

    // Node management
    void Add_Node(Ref<Node> node)
    {
        m_nodes.push_back(node);
    }

    void Remove_Node(Ref<Node> node)
    {
        auto it = std::find(m_nodes.begin(), m_nodes.end(), node);
        if (it != m_nodes.end())
        {
            m_nodes.erase(it);
        }
    }

    Ref<Node> Find_Node(const String& name)
    {
        auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&name](const Ref<Node>& node) 
        {
            return node->Get_Name() == name; 
        });

        return it != m_nodes.end() ? *it : nullptr;
    }

    inline const String& Get_Name() const
    {
        return m_name;
    }

    inline const Vector<Ref<Node>>& Get_Nodes() const
    {
        return m_nodes;
    }

protected:

    String m_name;
    Vector<Ref<Node>> m_nodes;
    bool m_enabled = true;
};

} // namespace destan::core