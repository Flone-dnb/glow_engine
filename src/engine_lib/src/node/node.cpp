#include <node/node.h>

#include <io/log.h>

ge_node::ge_node() : ge_node("Node") {}

ge_node::ge_node(const char* name) {
    this->name = name;
    parent_node = nullptr;
    world = nullptr;
}

ge_node::~ge_node() {
    if (world != nullptr) {
        ge_log_error_fmt("node \"%s\" is being destroyed but it's still spawned", name.c_str());
        abort();
    }

    for (ge_node* child_node : child_nodes) {
        delete child_node;
    }
}

void
ge_node::set_name(const char* name) {
    this->name = name;
}

const char*
ge_node::get_name() {
    return name.c_str();
}

void
ge_node::attach_child_node(ge_node* child) {
    if (child->parent_node == this) {
        return;
    }

    bool changed_parent = child->parent_node != nullptr;
    if (changed_parent) {
        // Remove from parent's child nodes array.
        bool found = false;
        for (auto it = child->parent_node->child_nodes.begin(); it != child->parent_node->child_nodes.end(); it++) {
            if ((*it) == child) {
                child->parent_node->child_nodes.erase(it);
                found = true;
                break;
            }
        }
        if (!found) {
            ge_log_error_fmt("unable to find child node \"%s\" in the parent node's array of child nodes", child->name.c_str());
            abort();
        }
    }

    child_nodes.push_back(child);

    if (child->world == nullptr) {
        if (world != nullptr) {
            child->spawn(world);
        }
    } else {
        if (world != nullptr) {
            if (world != child->world) {
                ge_log_error_fmt(
                    "can't attach node \"%s\" to node \"%s\" because they are spawned in different worlds",
                    child->name.c_str(), name.c_str());
                abort();
            }
        } else {
            child->despawn();
        }
    }

    if (changed_parent) {
        notify_about_parent_changed(true);
    }
}

void
ge_node::notify_about_parent_changed(bool this_nodes_parent) {
    on_after_parent_changed(this_nodes_parent);

    for (ge_node* node : child_nodes) {
        notify_about_parent_changed(false);
    }
}

void
ge_node::despawn_and_detach() {
    // Public despawn function has a different name to be more intuitive.
    despawn();
}

ge_node*
ge_node::get_parent_node() {
    return parent_node;
}

const std::vector<ge_node*>& const
ge_node::get_child_nodes_ref() const {
    return child_nodes;
}

ge_world*
ge_node::get_world_if_spawned() {
    return world;
}

bool
ge_node::is_spawned() {
    return world != nullptr;
}

void
ge_node::spawn(ge_world* world) {
    this->world = world;

    // First notify, then update all child nodes to avoid "holes" in the spawned node tree.
    on_after_spawned();

    for (ge_node* child_node : child_nodes) {
        child_node->spawn(world);
    }

    on_after_child_nodes_spawned();
}

void
ge_node::despawn() {
    if (world == nullptr) {
        return;
    }

    on_before_despawned();

    // Despawn from furthest child to avoid making "holes" in the spawned node tree.
    for (ge_node* child_node : child_nodes) {
        child_node->despawn();
    }

    world = nullptr;
}
