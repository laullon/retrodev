
// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#include "app.resources.h"
#include <unordered_map>
// #include <ranges>

namespace RetrodevGui {

	// The list of resources
	static std::unordered_map<std::string, Resource*> _resourceList;

	//---
	static const Resource _emptyResource("", nullptr, 0);

	Resource* Resource::Create(const std::string& resName, const void* inPtr, const size_t size) {
		Resource* resource = new Resource(resName, inPtr, size);
		Resources::AddResource(resource);
		return resource;
	}

	Resource::Resource(const std::string& resName, const void* inPtr, const size_t size) : _ptr(inPtr), _size(size), _name(resName) {}

	const Resource& Resources::GetResource(const std::string& resName) {
		if (_resourceList.contains(resName) == false) {
			return _emptyResource;
		}
		const Resource& resource = *_resourceList[resName];
		return resource;
	}

	std::vector<std::string> Resources::GetResourceNames(const std::string& startsWith) {
		std::vector<std::string> list;
		for (const auto& [name, resource] : _resourceList) {
			if (resource->_name.starts_with(startsWith)) {
				list.push_back(resource->_name);
			}
		}
		return list;
	}

	void Resources::AddResource(Resource* resource) {
		_resourceList.emplace(resource->_name, resource);
	}

} // namespace RetrodevGui
