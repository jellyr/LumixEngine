#include "engine/property_register.h"
#include "engine/associative_array.h"
#include "engine/crc32.h"
#include "engine/iproperty_descriptor.h"


namespace Lumix
{


namespace PropertyRegister
{


struct ComponentTypeData
{
	char m_id[50];

	uint32 m_id_hash;
	ComponentType m_dependency;
};


typedef AssociativeArray<ComponentType, Array<IPropertyDescriptor*>> PropertyMap;


static PropertyMap* g_properties = nullptr;
static IAllocator* g_allocator = nullptr;


static Array<ComponentTypeData>& getComponentTypes()
{
	static DefaultAllocator allocator;
	static Array<ComponentTypeData> types(allocator);
	return types;
}


void init(IAllocator& allocator)
{
	ASSERT(!g_properties);
	g_properties = LUMIX_NEW(allocator, PropertyMap)(allocator);
	g_allocator = &allocator;
}


void shutdown()
{
	for (int j = 0; j < g_properties->size(); ++j)
	{
		Array<IPropertyDescriptor*>& props = g_properties->at(j);
		for (auto* prop : props)
		{
			LUMIX_DELETE(*g_allocator, prop);
		}
	}

	LUMIX_DELETE(*g_allocator, g_properties);
	g_properties = nullptr;
	g_allocator = nullptr;
}


void add(const char* component_type, IPropertyDescriptor* descriptor)
{
	getDescriptors(getComponentType(component_type)).push(descriptor);
}


Array<IPropertyDescriptor*>& getDescriptors(ComponentType type)
{
	int props_index = g_properties->find(type);
	if (props_index < 0)
	{
		g_properties->insert(type, Array<IPropertyDescriptor*>(*g_allocator));
		props_index = g_properties->find(type);
	}
	return g_properties->at(props_index);
}


const IPropertyDescriptor* getDescriptor(ComponentType type, uint32 name_hash)
{
	Array<IPropertyDescriptor*>& props = getDescriptors(type);
	for (int i = 0; i < props.size(); ++i)
	{
		if (props[i]->getNameHash() == name_hash)
		{
			return props[i];
		}
		auto& children = props[i]->getChildren();
		for (int j = 0; j < children.size(); ++j)
		{
			if (children[j]->getNameHash() == name_hash)
			{
				return children[j];
			}

		}
	}
	return nullptr;
}


const IPropertyDescriptor* getDescriptor(const char* component_type, const char* property_name)
{
	return getDescriptor(getComponentType(component_type), crc32(property_name));
}


void registerComponentDependency(const char* id, const char* dependency_id)
{
	uint32 id_hash = crc32(id);
	for (ComponentTypeData& cmp_type : getComponentTypes())
	{
		if (cmp_type.m_id_hash == id_hash)
		{
			cmp_type.m_dependency = getComponentType(dependency_id);
			return;
		}
	}
	ASSERT(false);
}


bool componentDepends(ComponentType dependent, ComponentType dependency)
{
	return getComponentTypes()[dependent.index].m_dependency == dependency;
}


ComponentType getComponentTypeFromHash(uint32 hash)
{
	for (int i = 0; i < getComponentTypes().size(); ++i)
	{
		if (getComponentTypes()[i].m_id_hash == hash)
		{
			return{ i };
		}
	}
	ASSERT(false);
	return {-1};
}


uint32 getComponentTypeHash(ComponentType type)
{
	return getComponentTypes()[type.index].m_id_hash;
}


ComponentType getComponentType(const char* id)
{
	uint32 id_hash = crc32(id);
	for (int i = 0; i < getComponentTypes().size(); ++i)
	{
		if (getComponentTypes()[i].m_id_hash == id_hash)
		{
			return {i};
		}
	}

	ComponentTypeData& type = getComponentTypes().emplace();
	copyString(type.m_id, id);
	type.m_id_hash = id_hash;
	type.m_dependency = {-1};
	return{ getComponentTypes().size() - 1 };
}


int getComponentTypesCount()
{
	return getComponentTypes().size();
}


const char* getComponentTypeID(int index)
{
	return getComponentTypes()[index].m_id;
}


} // namespace PropertyRegister


} // namespace Lumix
