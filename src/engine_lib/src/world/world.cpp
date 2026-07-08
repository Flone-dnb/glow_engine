#include <world/world.h>

#include <node/node.h>

ge_world::ge_world(ge_world_manager* manager) {
    this->manager = manager;

    root_node = new ge_node("Root Node");
    root_node->spawn(this);
}

ge_node*
ge_world::get_root_node() {
    return root_node;
}

ge_world::~ge_world() {
    root_node->despawn();
    delete root_node;
}
