#pragma once

enum ge_texture_filtering {
	// DON'T change offsets in this enum, they match sampler offsets in the HLSL sampler heap
	// we also serialize these
	GE_TF_POINT = 0,
	GE_TF_LINEAR = 1,
	GE_TF_ANISOTROPIC = 2,
	GE_TF_FROM_RENDER_SETTINGS, //< Determined according to the texture filtering quality from the render settings.
};