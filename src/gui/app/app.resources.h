// --------------------------------------------------------------------------------------------------------------
//
//
//
//
// --------------------------------------------------------------------------------------------------------------

#pragma once

#include <string>
#include <vector>

namespace RetrodevGui {

	//
	// Resource class. Represents a resource in the system
	//
	class Resource {
	public:
		// Create a new resource
		// resName: Resource's name.
		// inPtr: Pointer to the resource's data.
		// size: Size of the resource's body.
		// returns a pointer to the resource
		static Resource* Create(const std::string& resName, const void* inPtr, const size_t size);

		Resource() = default;
		Resource(const std::string& resName, const void* inPtr, const size_t size);

		// The pointer to the resource's data
		const void* _ptr = nullptr;
		// The size of the resource's data
		const size_t _size = 0;
		// The name of the resource
		std::string _name = nullptr;
	};

	//
	//
	//
	class Resources {
		friend class Resource;

	public:
		// Get a resource by name
		// resName The name of the resource
		// returns a reference to the resource that maybe empty if the resource doesn't exists.
		static const Resource& GetResource(const std::string& resName);

		// Get a list of resource names
		// If startsWith is specified, only resources starting with that string will be returned
		// If no resources are found, an empty list is returned
		static std::vector<std::string> GetResourceNames(const std::string& startsWith = "");

	private:
		// Add a resource to the list
		static void AddResource(Resource* resource);
	};

} // namespace RetrodevGui
