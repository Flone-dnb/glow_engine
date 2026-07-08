#pragma once

#include <node/node.h>

class ge_spatial_node : public ge_node {
  public:
    ge_spatial_node();
    ge_spatial_node(const char* name);

    virtual ~ge_spatial_node() override = default;

    ge_spatial_node(const ge_spatial_node&) = delete;
    ge_spatial_node& operator=(const ge_spatial_node&) = delete;
};