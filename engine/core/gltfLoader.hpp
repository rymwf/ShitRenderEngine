#pragma once
#include "prerequisites.hpp"
#include "resource.hpp"
#include "singleton.hpp"

class GLTFModelLoader : public ManualResourceLoader, public Singleton<GLTFModelLoader>
{
public:
	~GLTFModelLoader() {}
	void loadResource(Resource *resource) override;
};
