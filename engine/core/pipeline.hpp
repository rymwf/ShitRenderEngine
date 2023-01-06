#pragma once
#include "prerequisites.hpp"
#include "primitive.hpp"

struct GraphicPipelineSpec
{
	Shit::PrimitiveTopology topology;
	//Shit::PolygonMode pologygonMode;
};
inline bool operator==(const GraphicPipelineSpec &l,
					   const GraphicPipelineSpec &r)
{
	return l.topology == r.topology;
}
struct GraphicPipelineSpecHash
{
	size_t operator()(GraphicPipelineSpec const &a) const noexcept
	{
		auto h = std::hash<Shit::PrimitiveTopology>{}(a.topology);
		//hashCombine(h, std::hash<Shit::PolygonMode>{}(a.pologygonMode));
		return h;
	}
};
class GraphicPipelineWrapper
{
public:
	GraphicPipelineWrapper(Shit::GraphicsPipelineCreateInfo const &createInfo);
	~GraphicPipelineWrapper();

	Shit::GraphicsPipeline *getHandle(GraphicPipelineSpec const &spec);
	constexpr Shit::GraphicsPipelineCreateInfo const *getCreateInfoPtr() const { return _pCreateInfo; }

private:
	std::unordered_map<GraphicPipelineSpec, Shit::GraphicsPipeline *, GraphicPipelineSpecHash> _handles;
	Shit::GraphicsPipelineCreateInfo const *_pCreateInfo;

	Shit::GraphicsPipeline *createPipeline(GraphicPipelineSpec const &spec);
};