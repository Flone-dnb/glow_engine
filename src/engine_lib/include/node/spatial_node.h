#pragma once

#include <node/node.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>

// Node that implement 3D space transform (location, rotation and scale).
class ge_spatial_node : public ge_node {
  public:
    ge_spatial_node();
    ge_spatial_node(const char* name);

    virtual ~ge_spatial_node() override = default;

    ge_spatial_node(const ge_spatial_node&) = delete;
    ge_spatial_node& operator=(const ge_spatial_node&) = delete;

    // Relative to the parent node.
    void set_relative_location(const glm::vec3& location);
    void set_relative_rotation(const glm::vec3& rotation); // in degrees
    void set_relative_scale(const glm::vec3& scale);

    // Recalculates relative transform to be in the specified world transform.
    void set_world_location(const glm::vec3& location);
    void set_world_rotation(glm::vec3 rotation); // in degrees
    void set_world_scale(const glm::vec3& scale);

    // Relative to the parent node.
    const glm::vec3& get_relative_location();
    const glm::vec3& get_relative_rotation(); // in degrees
    const glm::vec3& get_relative_scale();

    const glm::vec3& get_world_forward() const;
    const glm::vec3& get_world_right() const;
    const glm::vec3& get_world_up() const;
    const glm::vec3& get_world_location() const;
    const glm::vec3& get_world_rotation() const; // in degrees
    const glm::quat& get_world_rotation_quat() const;
    const glm::vec3& get_world_scale() const;
    const glm::mat4& get_world_mat() const;

  protected:
    virtual void on_after_spawned() override;
    virtual void on_after_parent_changed(bool this_nodes_parent) override;
    virtual void on_before_despawned() override;

    // Called after world location, rotation and/or scale changed.
    virtual void
    on_after_world_transform_changed() {}

  private:
    void recalc_world_transform(bool notify_children);
    void recalc_world_transform_for_node_and_children(ge_node* node);

    // Relative to the parent node.
    glm::vec3 relative_location;
    glm::vec3 relative_rotation; // in degrees
    glm::vec3 relative_scale;

    glm::mat4 world_mat;

    glm::quat world_rot_quat;
    glm::vec3 world_rot;
    glm::vec3 world_scale;

    glm::vec3 world_forward;
    glm::vec3 world_right;
    glm::vec3 world_up;

    // Next spatial parent in the parent node chain.
    ge_spatial_node* spatial_parent;

    bool is_notifying_world_transform_changed;
};