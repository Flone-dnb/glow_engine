#include <node/spatial_node.h>

#include <math_funcs.h>
#include <glm/gtx/transform.hpp>
#include <glm/trigonometric.hpp>
#include <misc/globals.h>

ge_spatial_node::ge_spatial_node() : ge_spatial_node("Spatial Node") {}

ge_spatial_node::ge_spatial_node(const char* name) : ge_node(name) {
    relative_location = {0.0f, 0.0f, 0.0f};
    relative_rotation = {0.0f, 0.0f, 0.0f};
    relative_scale = {1.0f, 1.0f, 1.0f};

    spatial_parent = nullptr;
    is_notifying_world_transform_changed = false;

    recalc_world_transform(false);
}

void
ge_spatial_node::set_relative_location(const glm::vec3& location) {
    relative_location = location;
    recalc_world_transform(true);
}

void
ge_spatial_node::set_relative_rotation(const glm::vec3& rotation) {
    relative_rotation = rotation;
    recalc_world_transform(true);
}

void
ge_spatial_node::set_relative_scale(const glm::vec3& scale) {
#if defined(DEBUG)
    // Make sure we don't have negative scale specified.
    if (scale.x < 0.0f || scale.y < 0.0f || scale.z < 0.0f) {
        ge_log_warn("avoid using negative scale as it may cause issues");
    }
#endif

    relative_scale = scale;
    recalc_world_transform(true);
}

void
ge_spatial_node::set_world_location(const glm::vec3& location) {
    if (spatial_parent != nullptr) {
        glm::mat3 rot_mat(world_mat);
        rot_mat[0] = glm::normalize(rot_mat[0]);
        rot_mat[1] = glm::normalize(rot_mat[1]);
        rot_mat[2] = glm::normalize(rot_mat[2]);

        glm::vec3 inv = glm::inverse(rot_mat) * (location - glm::vec3(spatial_parent->get_world_mat()[3]));
        relative_location = inv * ge_math_calc_reciprocal(spatial_parent->get_world_scale());
    } else {
        relative_location = location;
    }

    recalc_world_transform(true);
}

void
ge_spatial_node::set_world_rotation(glm::vec3 rotation) {
    rotation.x = ge_math_wrap_to_range(rotation.x, -360.0f, 360.0f);
    rotation.y = ge_math_wrap_to_range(rotation.y, -360.0f, 360.0f);
    rotation.z = ge_math_wrap_to_range(rotation.z, -360.0f, 360.0f);

    if (spatial_parent != nullptr) {
        // Don't care for negative scale (mirror rotations) because it's rarely used and we warn about it.
        relative_rotation = glm::degrees(glm::eulerAngles(
            glm::inverse(spatial_parent->get_world_rotation_quat())
            * glm::toQuat(ge_math_make_rotation_mat(rotation))));
    } else {
        relative_rotation = rotation;
    }

    recalc_world_transform(true);
}

void
ge_spatial_node::set_world_scale(const glm::vec3& scale) {
#if defined(DEBUG)
    // Make sure we don't have negative scale specified.
    if (scale.x < 0.0f || scale.y < 0.0f || scale.z < 0.0f) {
        ge_log_warn("avoid using negative scale as it may cause issues");
    }
#endif

    if (spatial_parent != nullptr) {
        relative_scale = scale * ge_math_calc_reciprocal(spatial_parent->get_world_scale());
    } else {
        relative_scale = scale;
    }

    recalc_world_transform(true);
}

const glm::vec3&
ge_spatial_node::get_relative_location() {
    return relative_location;
}

const glm::vec3&
ge_spatial_node::get_relative_rotation() {
    return relative_rotation;
}

const glm::vec3&
ge_spatial_node::get_relative_scale() {
    return relative_scale;
}

const glm::vec3&
ge_spatial_node::get_world_forward() const {
    return world_forward;
}

const glm::vec3&
ge_spatial_node::get_world_right() const {
    return world_right;
}

const glm::vec3&
ge_spatial_node::get_world_up() const {
    return world_up;
}

#pragma warning(disable : 4172) // returning address of local variable or temporary
const glm::vec3&
ge_spatial_node::get_world_location() const {
    return world_mat[3];
}
#pragma warning(pop)

const glm::vec3&
ge_spatial_node::get_world_rotation() const {
    return world_rot;
}

const glm::quat&
ge_spatial_node::get_world_rotation_quat() const {
    return world_rot_quat;
}

const glm::vec3&
ge_spatial_node::get_world_scale() const {
    return world_scale;
}

const glm::mat4&
ge_spatial_node::get_world_mat() const {
    return world_mat;
}

static ge_spatial_node*
find_spatial_parent(ge_node* parent) {
    if (parent == nullptr) {
        return nullptr;
    }

    ge_spatial_node* found = dynamic_cast<ge_spatial_node*>(parent);
    if (found != nullptr) {
        return found;
    }

    for (ge_node* node : parent->get_child_nodes_ref()) {
        found = find_spatial_parent(node);
        if (found != nullptr) {
            return found;
        }
    }

    return nullptr;
}

void
ge_spatial_node::on_after_spawned() {
    // No need to notify child nodes since this function is called before any of the child nodes are spawned.
    recalc_world_transform(false);
}

void
ge_spatial_node::on_after_parent_changed(bool this_nodes_parent) {
    spatial_parent = find_spatial_parent(get_parent_node());
}

void
ge_spatial_node::on_before_despawned() {
    spatial_parent = nullptr;
    recalc_world_transform(false);
}

void
ge_spatial_node::recalc_world_transform(bool notify_children) {
    if (!is_spawned()) {
        return;
    }

    glm::mat4 local_rotation_mat = ge_math_make_rotation_mat(relative_rotation);
    glm::mat4 world_mat = glm::translate(relative_location) * local_rotation_mat * glm::scale(relative_scale);

    glm::vec3 parent_world_scale = {1.0f, 1.0f, 1.0f};
    glm::quat parent_world_rot_quat = glm::identity<glm::quat>();

    if (spatial_parent != nullptr) {
        world_mat = spatial_parent->get_world_mat() * world_mat;
        parent_world_rot_quat = spatial_parent->get_world_rotation();
        parent_world_scale = spatial_parent->get_world_scale();
    }

    world_rot_quat = parent_world_rot_quat * glm::toQuat(local_rotation_mat);
    world_rot = glm::degrees(glm::eulerAngles(world_rot_quat));
    world_scale = parent_world_scale * relative_scale;

    // recalculate world directions

    glm::vec4 dir;
    dir.w = 0.0f;

    ge_get_world_forward(&dir.x);
    world_forward = world_mat * dir;

    ge_get_world_right(&dir.x);
    world_right = world_mat * dir;

    ge_get_world_up(&dir.x);
    world_up = world_mat * dir;

    // notify

    if (is_notifying_world_transform_changed) {
        // stop recursion
        return;
    }
    is_notifying_world_transform_changed = true;
    on_after_world_transform_changed();
    is_notifying_world_transform_changed = false;

    if (notify_children) {
        for (ge_node* child_node : get_child_nodes_ref()) {
            recalc_world_transform_for_node_and_children(child_node);
        }
    }
}

void
ge_spatial_node::recalc_world_transform_for_node_and_children(ge_node* node) {
    if (ge_spatial_node* spatial_node = dynamic_cast<ge_spatial_node*>(node)) {
        spatial_node->recalc_world_transform(true);
        return;
    }
    // This is not a spatial node, notify children maybe there's a spatial node somewhere.
    for (ge_node* child_node : node->get_child_nodes_ref()) {
        recalc_world_transform_for_node_and_children(child_node);
    }
}
