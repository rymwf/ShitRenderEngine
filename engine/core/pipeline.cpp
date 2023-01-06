#include "pipeline.hpp"
#include "root.hpp"
#include "renderable.hpp"

GraphicPipelineWrapper::GraphicPipelineWrapper(Shit::GraphicsPipelineCreateInfo const &createInfo)
{
	auto desc = GraphicPipelineSpec{
		createInfo.inputAssemblyState.topology,
	};
	auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(Root::getSingleton().getDevice()->Create(createInfo));
	_handles.emplace(desc, pipeline);
	_pCreateInfo = pipeline->GetCreateInfoPtr();
}
GraphicPipelineWrapper::~GraphicPipelineWrapper()
{
	auto device = Root::getSingleton().getDevice();
	for (auto &&e : _handles)
	{
		device->Destroy(e.second->GetPipelineLayout());
		device->Destroy(e.second);
	}
}
Shit::GraphicsPipeline *GraphicPipelineWrapper::createPipeline(GraphicPipelineSpec const &spec)
{
	Shit::GraphicsPipelineCreateInfo createInfo = *_pCreateInfo;
	createInfo.inputAssemblyState.topology = spec.topology;
	return _handles.emplace(
					   spec, dynamic_cast<Shit::GraphicsPipeline *>(Root::getSingleton().getDevice()->Create(createInfo)))
		.first->second;
}
Shit::GraphicsPipeline *GraphicPipelineWrapper::getHandle(GraphicPipelineSpec const &spec)
{
	if (_handles.contains(spec))
		return _handles[spec];
	return createPipeline(spec);
}