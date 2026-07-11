#pragma once

#include <string>
#include <vector>

class ge_world;

// Base class for game entities.
// Implements hierarchy functionality: allows attaching node to some (other) parent node.
class ge_node {
    // World spawns root node.
    friend class ge_world;

  public:
    ge_node();
    ge_node(const char* name);

    virtual ~ge_node();

    ge_node(const ge_node&) = delete;
    ge_node& operator=(const ge_node&) = delete;

    void set_name(const char* name);

    // This is the only function to create node hierarchy, with this you can not only attach child
    // nodes but also change parent nodes: if node A already has a parent you can attach it to another parent
    // by using this function.
    // This function can also spawn/despawn the child node depending on the state of the parent.
    void attach_child_node(ge_node* child);

    // If spawned: despawns the node (and child nodes) and detaches it from the spawned parent
    // thus removing the node (and child nodes) from the world.
    // After this function the caller is responsible for destroying the node.
    void despawn_and_detach();

    ge_node* get_parent_node();
    const std::vector<ge_node*>& get_child_nodes_ref() const;

    // Returns `nullptr` if not spawned, named this was specifically to tell that `nullptr` can be returned.
    ge_world* get_world_if_spawned();

    // Do not delete returned string, valid while the node exists and while its name is not changed.
    const char* get_name();

    bool is_spawned();

  protected:
    // Called after this node is spawned but before child nodes are spawned,
    // also see @ref on_after_child_nodes_spawned.
    virtual void
    on_after_spawned() {}

    // Called after @ref on_after_spawned and after all child nodes were spawned.
    virtual void
    on_after_child_nodes_spawned() {}

    virtual void
    on_before_despawned() {}

    // Called after parent node is changed.
    // If this node's parent changed then `this_nodes_parent` is `true`,
    // otherwise some parent node changed its parent (in the parent node chain).
    virtual void
    on_after_parent_changed(bool this_nodes_parent) {}

  private:
    // Called by a world (to spawn root node) or by a parent spawned node.
    // Causes child nodes to also spawn.
    void spawn(ge_world* world);
    void despawn();

    // Called after parent node is changed. Recursively calls on all child nodes.
    // If this node's parent changed then `this_nodes_parent` is `true`,
    // otherwise some parent node changed its parent (in the parent node chain).
    void notify_about_parent_changed(bool this_nodes_parent);

    // A node owns child nodes and manages their destruction.
    std::vector<ge_node*> child_nodes;

    std::string name;

    ge_node* parent_node;

    // `nullptr` if not spawned.
    ge_world* world;
};
