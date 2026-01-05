#pragma once
#include <complex>
#include <memory>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class UStructEntry;
class UObject;
class UStruct;

class ClassEntry;
class ConstEntry;
class EnumEntry;
class ObjectEntry;
class UFunctionEntry;
class UScriptStructEntry;

using GroupedBase = std::unordered_map<std::string, std::vector<ObjectEntry*>>;

enum class InvalidUObjectReason {
    InvalidAddress,
    InvalidOuterAddress,
    NullObject,
    NullClass,
    DefaultObject,
};

inline auto toString(const InvalidUObjectReason reason) -> std::string {
    switch (reason) {
        case InvalidUObjectReason::InvalidAddress:      return "💥 Invalid memory address";
        case InvalidUObjectReason::InvalidOuterAddress: return "🧟 Invalid Outer pointer";
        case InvalidUObjectReason::NullObject:          return "⚠️ Null object";
        case InvalidUObjectReason::NullClass:           return "📛 Null class";
        case InvalidUObjectReason::DefaultObject:       return "🚫 Skipped Default__ object";
        default:                                        return "❓ Unknown reason";
    }
}

class ObjectStore {
  public:
    static void create() {
        if (!instance_) instance_ = new ObjectStore();
    }

    static void yeet() {
        delete instance_;
        instance_ = nullptr;
    }

    static auto instance() -> ObjectStore& {
        return *instance_;
    }

    void initialize();

    auto isProbablyValidPtr(uintptr_t ptr) -> bool;
    auto isProbablyValidUObject(UObject* rawObj) -> bool;

    void storeInvalidObject(UObject* rawObj, InvalidUObjectReason);
    void storeInvalidObject(uintptr_t addr, InvalidUObjectReason reason);

    auto getTotalGObjObjectsCount() const -> size_t;
    auto getTotalSeenCount() const -> size_t;
    auto getInvalidCount() const -> size_t;
    void iterateObjects(const std::function<bool(UObject*)>& fn);

    auto existingEntryFor(const UObject* obj) const -> ObjectEntry*;

    auto add(UObject* rawObj, const std::string& origin) -> ObjectEntry*;

    auto countStructsWithName(std::string_view sanitizedName) -> int;
    auto getAllStructEntries() -> std::vector<UScriptStructEntry*>;
    auto getAllEnumEntries() -> std::vector<EnumEntry*>;
    auto getAllConstEntries() -> std::vector<ConstEntry*>;
    auto getAllClassEntries() -> std::vector<ClassEntry*>;
    auto getAllFunctionEntries() -> std::vector<UFunctionEntry*>;
    auto getStructByName(const std::string& given) -> UScriptStructEntry*;
    auto get(UObject* obj) const -> ObjectEntry*;

    auto getClassEntryByName(std::string& name) -> ClassEntry*;

    auto getAll() -> const std::vector<std::unique_ptr<ObjectEntry>>& { return all_; };


    template <typename T>
    [[nodiscard]] auto getEntryByName(const std::string& given) const -> T* {
        for (auto& [storedName, entry] : nameToEntry_) {
            if (storedName == given) {
                return static_cast<T*>(entry);
            }
        }
        return nullptr;
    }

    template <typename T>
    auto getGroupedEntries() const -> std::unordered_map<std::string, std::vector<T*>> {
        std::unordered_map<std::string, std::vector<T*>> result;
        for (const auto& entry : all_) {
            if (auto* s = dynamic_cast<T*>(entry.get())) {
                const std::string& pkg = s->getGroupName();
                if (pkg.empty()) {
                    result["Global"].push_back(s);
                } else {
                    result[pkg].push_back(s);
                }
            }
        }
        return result;
    }

    template <typename T>
    auto getEntriesGroupedByPackage() -> std::unordered_map<std::string, std::vector<ObjectEntry*>> {
        std::unordered_map<std::string, std::vector<ObjectEntry*>> groupedBase;

        const auto& grouped = ObjectStore::instance().getGroupedEntries<T>();
        for (auto& [pkg, entries] : grouped) {
            groupedBase[pkg].insert(groupedBase[pkg].end(), entries.begin(), entries.end());
        }

        return groupedBase;
    }

    // sort deps
    // auto order = topoSort(allStructs, [](auto* s){ return s->deps(); });


//    template<typename Container, typename GetDeps>
//    auto topoSort(const Container& nodes, GetDeps getDeps)
//        -> std::vector<typename Container::value_type>
//    {
//        using Node = typename Container::value_type;
//            using Node = typename Container::value_type;
//
//        // Compute indegrees and adjacency list
//        std::unordered_map<Node, size_t> indegree;
//        std::unordered_map<Node, std::vector<Node>> adj;
//
//        // Initialize all nodes with indegree 0
//        for (const auto& node : nodes) {
//            indegree[node] = 0;
//            adj[node]; // ensure it exists
//        }
//
//        // Populate indegrees and adjacency
//        for (const auto& node : nodes) {
//            for (auto dep : getDeps(node)) {
//                if (!dep) continue;
//                adj[dep].push_back(node);
//                indegree[node]++;
//            }
//        }
//
//        // Priority queue for deterministic ordering (replace with std::queue for arbitrary order)
//        auto cmp = [](const Node& a, const Node& b) { return a > b; };
//        std::priority_queue<Node, std::vector<Node>, decltype(cmp)> ready(cmp);
//
//        // Enqueue all indegree 0 nodes
//        for (const auto& [node, deg] : indegree) {
//            if (deg == 0) ready.push(node);
//        }
//
//        std::vector<Node> result;
//        result.reserve(nodes.size());
//
//        while (!ready.empty()) {
//            Node n = ready.top();
//            ready.pop();
//            result.push_back(n);
//
//            for (auto& neighbor : adj[n]) {
//                if (--indegree[neighbor] == 0) {
//                    ready.push(neighbor);
//                }
//            }
//        }
//
//        if (result.size() != nodes.size()) {
//            throw std::runtime_error("Cycle detected in topoSort()");
//        }
//
//        return result;
//    }

  private:
    ObjectStore();
    ~ObjectStore() = default;
    ObjectStore(const ObjectStore&) = delete;
    auto operator=(const ObjectStore&) -> ObjectStore& = delete;

    static ObjectStore* instance_;

    std::unordered_map<UObject*, ObjectEntry*> objectToEntry_;

    std::unordered_map<std::string, ObjectEntry*> nameToEntry_;
    std::unordered_map<std::string, ObjectEntry*> nameToStruct_;
    std::vector<ObjectEntry*> allStructs_;
    std::unordered_map<std::string, ObjectEntry*> structsByName_;

    std::unordered_set<UObject*> seen_;
    std::vector<std::unique_ptr<ObjectEntry>> all_;


    std::unordered_set<uintptr_t> invalidPtrs_;
    std::unordered_map<uintptr_t, InvalidUObjectReason> invalidObjects_;

    std::unordered_map<ObjectEntry*, std::unordered_set<ObjectEntry*>> structDependencies_;
};