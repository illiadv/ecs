#ifndef ECS_HPP
#define ECS_HPP

#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <typeindex>

typedef uint32_t Entity;

class IComponentStorage {

    public:
    virtual int GetSize() = 0;
    virtual void DeleteComponent(Entity) = 0;

    virtual ~IComponentStorage(){}

};

template <typename T>
class ComponentStorage : public IComponentStorage {

    public:
    int GetSize() override
    {
	return m_dense.size();
    }

    typename std::vector<T>::const_iterator begin()
    {
	return m_dense.begin();
    }

    typename std::vector<T>::const_iterator end()
    {
	return m_dense.end();
    }

    void AddComponent(Entity entity, T component)
    {
	if (HasComponent(entity)) {
	    m_dense[m_sparse[entity]] = component;
	    return;
	}

	if (entity >= m_sparse.size()) {
	    m_sparse.resize(entity+1, INVALID_INDEX);
	}

	m_sparse[entity] = m_dense.size();
	m_dense.push_back(component);
	m_entityList.push_back(entity);
    }

    T* GetComponent(Entity entity)
    {
	if (!HasComponent(entity))
	    return nullptr;
	return &m_dense[m_sparse[entity]];
    }

    bool HasComponent(Entity entity)
    {
	return entity < m_sparse.size() && m_sparse[entity] != INVALID_INDEX;
    }

    void DeleteComponent(Entity entity) override
    {
	if (!HasComponent(entity))
	    return;

	size_t diThis = m_sparse[entity];
	size_t diLast = m_dense.size() - 1;
	if (m_sparse[entity] != diLast)
	{
	    m_dense[diThis] = std::move(m_dense[diLast]);

	    Entity entityLast = m_entityList[diLast];

	    m_entityList[diThis] = entityLast;
	    m_sparse[entityLast] = diThis;
	}
	m_dense.pop_back();
	m_entityList.pop_back();
	m_sparse[entity] = INVALID_INDEX;
    }

    std::vector<T> m_dense;
    std::vector<Entity> m_entityList;

    private:
    std::vector<size_t> m_sparse;


    static constexpr size_t INVALID_INDEX = -1;
};

class EntityManager
{
    public:
    Entity AddEntity()
    {
	return topFree++;
    }

    template <typename T>
    void RegisterComponentType()
    {
	auto index = std::type_index(typeid(T));
	ComponentStorage<T> *storage = new ComponentStorage<T>;
	componentRegistry.try_emplace(index, storage);
    }

    template <typename T>
    void AddComponent(Entity entity, T component)
    {
	auto storage = GetComponentStorage<T>();
	storage->AddComponent(entity, component);
    }

    template <typename T>
    T*
    GetComponent(Entity entity)
    {
	auto storage = GetComponentStorage<T>();
	return storage->GetComponent(entity);
    }

    template<typename T, typename... Ts, typename Func>
    // void ForEach(std::function<void(T&, Ts&...)> callback)
    void ForEach(Func&& callback)
    {
	auto storage = GetComponentStorage<T>();

	for (int i = 0; i < storage->GetSize(); i++)
	{
	    Entity entity = storage->m_entityList[i];

	    T* c1 = &storage->m_dense[i];

	    std::tuple<Ts*...> comps {
		GetComponent<Ts>(entity) ...
	    };

	    if ((std::get<Ts*>(comps) && ...)) {
		
		callback(entity, *c1, *std::get<Ts*>(comps) ...);
	    }
	}
    }

    void KillEntity(Entity entity)
    {
	DeadEntities.push_back(entity);
    }

    void CleanupDead()
    {
	for(auto entity : DeadEntities)
	{
	    for (auto& [type, storage] : componentRegistry)
	    {
		storage->DeleteComponent(entity);
	    }
	    DeadEntities.clear();
	}
    }

    void DrawDebugInfo(void (*Callback)(int i, int amount, const char* name))
    {
	int i = 0;
	for (auto& [type, storage] : componentRegistry)
	{
	    Callback(i, storage->GetSize(), type.name());
	    i++;
	}
    }

    ~EntityManager()
    {
	for (auto kv : componentRegistry)
	    delete kv.second;
    }

    private:

    template<typename T>
    ComponentStorage<T> *GetComponentStorage()
    {
	auto index = std::type_index(typeid(T));
	IComponentStorage *storageInterface = componentRegistry.at(index);
	ComponentStorage<T> *storage = static_cast<ComponentStorage<T>*>(storageInterface);
	return storage;
    }

    Entity topFree = 0;

    std::unordered_map<std::type_index, IComponentStorage*> componentRegistry;
    std::vector<Entity> DeadEntities;
};

#endif
