#include "JsonDatabaseBackend.hpp"
#include "FilesystemBasedLock.hpp"
#include <fstream>
#include <deque>
#include <set>
#include <xtypes_generator/utils.hpp>

namespace xdbi
{

    JsonDatabaseBackend::JsonDatabaseBackend(const fs::path &db_path, const std::string graph)
        : FilesystemBasedBackend(db_path), m_graph(graph)
    {
    }

    bool JsonDatabaseBackend::isReady()
    {
        return m_graph != "";
    }

    nl::json JsonDatabaseBackend::getXtypesByURI(const std::string &graph, const std::string &uri, const std::string &classname)
    {
        nl::json xtypes;
        const std::string filename = getFileName(uri);
        const std::map<std::string, fs::path> files = this->getFiles(graph, classname);
        if (files.find(filename) == files.end())
            return xtypes;

        const nl::json info = this->loadAndCheck(filename, files.at(filename), classname);
        if (!info.empty())
        {
            xtypes[uri] = info;
        }
        return xtypes;
    }

    nl::json JsonDatabaseBackend::getXtypes(const std::string &graph, const std::string &classname)
    {
        nl::json xtypes;
        const std::map<std::string, fs::path> files = this->getFiles(graph, classname);
        for (const auto &[fname, fpath] : files)
        {
            const nl::json info = this->loadAndCheck(fname, fpath, classname);
            if (info.empty())
                continue;
            const std::string uri = info["uri"].get<std::string>();
            xtypes[uri] = info;
        }
        return xtypes;
    }

    nl::json JsonDatabaseBackend::loadAndCheck(const std::string &fname, const fs::path &fpath, const std::string &classname)
    {
        LOGI("Loading from file " << fpath << "...");
        std::ifstream ifs{fpath.string()};
        nl::json info;
        try
        {
            info = nl::json::parse(ifs);
        }
        catch (const std::runtime_error &e)
        {
            LOGE("Couldn't load: " << std::endl
                                   << fpath);
            throw;
        }
        ifs.close();

        if (!info.contains("uri"))
        {
            LOGE("info has no uri");
            return nl::json();
        }
        if (!info.contains("uuid"))
        {
            LOGE("info has no uuid");
            return nl::json();
        }
        if (info["uuid"].get<std::string>() != fname)
        {
            LOGE("uuid " << info["uuid"].get<std::string>() << " != " << fname);
            return nl::json();
        }

        if (!info.contains("classname"))
        {
            LOGE("info has no classname");
            return nl::json();
        }
        if (!classname.empty() && info["classname"].get<std::string>() != classname)
        {
            LOGE("classname " << info["classname"].get<std::string>() << " != " << classname);
            return nl::json();
        }
        LOGI("Loaded file " << fpath);
        return info;
    }

    bool JsonDatabaseBackend::add(const nl::json &models)
    {
        GUARD_DATABASE(m_graph);
        return this->_add(models);
    }
    bool JsonDatabaseBackend::_add(const nl::json &models)
    {
        LOGI("Adding " << models.size() << " models to m_graph " << m_graph << " ...");
        bool success = true;
        for (auto model : models)
        {
            if (!model.contains("uri") || !model.contains("classname"))
            {
                // means we received a model without uri and class name, (properties)
                LOGI("No uri or classname found, skipping model " << model);
                continue;
            }
            const std::string uri = model["uri"].get<std::string>();
            const std::string classname = model["classname"].get<std::string>();
            nl::json output_model = this->_load(uri, classname);

            if (!output_model.empty())
            {
                // Add new properties (do not overwrite existing property values)
                const bool has_properties_key(output_model.contains("properties"));
                for (const auto &[k,v] : model["properties"].items())
                {
                    if (has_properties_key)
                    {
                        // Do not overwrite existing properties
                        if (output_model["properties"].contains(k))
                            continue;
                        output_model["properties"][k] = v;
                    } else {
                        // Do not overwrite existing properties
                        if (output_model.contains(k))
                            continue;
                        output_model[k] = v;
                    }
                }

                // Check if we already have the "relations" key in the old model
                const bool has_relations_key(output_model.contains("relations"));
                if (model.contains("relations"))
                {
                    for (const auto &[k, v] : model["relations"].items())
                    {
                        // First make sure, that we create the relation entry (so we do not miss EMPTY relations)
                        if (has_relations_key)
                        {
                            if (!output_model["relations"].contains(k))
                            {
                                output_model["relations"][k] = nl::json::array();
                            }
                        } else {
                            if (!output_model.contains(k))
                            {
                                output_model[k] = nl::json::array();
                            }
                        }
                        // Now we add any edge which is not present in the old model
                        for (auto potential_edge : v)
                        {
                            // NOTE: When we have 'old' data we have to stick to that, otherwise we use the better 'relations' subkey
                            if (has_relations_key)
                            {
                                bool found = std::any_of(output_model["relations"][k].begin(), output_model["relations"][k].end(), [&](const nl::json &existing_edge)
                                                         { return existing_edge["target"] == potential_edge["target"]; });
                                if (!found)
                                {
                                    output_model["relations"][k].push_back(potential_edge);
                                }
                            } else {
                                bool found = std::any_of(output_model[k].begin(), output_model[k].end(), [&](const nl::json &existing_edge)
                                                         { return existing_edge["target"] == potential_edge["target"]; });
                                if (!found)
                                {
                                    output_model[k].push_back(potential_edge);
                                }
                            }
                        }
                    }
                }
            } else {
                output_model = model;
            }
            success &= this->_store(output_model);
        }
        return success;
    }

    bool JsonDatabaseBackend::update(const nl::json &models)
    {
        GUARD_DATABASE(m_graph);
        return this->_update(models);
    }
    bool JsonDatabaseBackend::_update(const nl::json &models)
    {
        std::set<std::string> to_be_removed;
        bool success = true;
        LOGI("Updating " << models.size() << " models to m_graph " << m_graph << " ...");
        for (auto model : models)
        {
            if (!model.contains("uri") || !model.contains("classname"))
            {
                LOGE("Got model without uri or classname");
                continue;
            }
            const std::string uri = model["uri"].get<std::string>();
            const std::string classname = model["classname"].get<std::string>();
            nl::json outputModel = this->_load(uri, classname);
            nl::json previous_edges;
            if (!outputModel.empty())
            {
                // First check if the old model already has the new 'properties' and 'relations' keys
                const bool has_properties_key(outputModel.contains("properties"));
                const bool has_relations_key(outputModel.contains("relations"));

                // Find the current edges
                // NOTE: Siganture is [uri][relation_name][edge_index]
                previous_edges = this->_findEdgesFrom({uri});

                // Update properties
                if (has_properties_key)
                {
                    outputModel["properties"].update(model["properties"]);
                } else {
                    outputModel.update(model["properties"]);
                }
                // Find old property keys ...
                std::set<std::string> old_keys;
                auto items = has_properties_key ? outputModel["properties"].items() : outputModel.items();
                for (const auto& [k,v] : items)
                {
                    // check if key is still valid
                    if (model["properties"].contains(k))
                        continue;
                    // found old key to be removed
                    old_keys.insert(k);
                }
                // ... and remove the old keys
                for (const auto& k : old_keys)
                {
                    if (has_properties_key)
                    {
                        outputModel["properties"].erase(k);
                    } else {
                        // Here, we have to exclude uri, classname and uuid
                        if ((k == "uri") || (k == "uuid") || k == ("classname"))
                            continue;
                        outputModel.erase(k);
                    }
                }

                // Update relations
                if (has_relations_key)
                {
                    outputModel["relations"].update(model["relations"]);
                } else {
                    outputModel.update(model["relations"]);
                }
            }
            else
            {
                outputModel = model;
            }

            success &= this->_store(outputModel);
            // TODO: We already know the new edges, it would be faster to not use _findEdgesFrom() here
            const nl::json new_edges = this->_findEdgesFrom({uri});

            // For every edge, which is removed we have to check the delete_policy to decide if either source, target or both have to be removed
            for (const auto &[src_uri, entry] : previous_edges.items())
            {
                // If src_uri is not in new_edges, then no relations have been specified at all and we cannot proceed
                if (!new_edges.contains(src_uri))
                {
                    continue;
                }
                for (const auto &[relname, edges] : entry.items())
                {
                    for (const auto &edge : edges)
                    {
                        // If relname is not specified then this relation is considered unknown and we cannot proceed
                        if (!new_edges[src_uri].contains(relname))
                        {
                            continue;
                        }
                        bool is_deleted = std::none_of(new_edges[src_uri][relname].cbegin(),new_edges[src_uri][relname].cend(), [&edge](const nl::json &new_edge) -> bool 
                        { return edge["target"] == new_edge["target"]; });
                        
                        if (!is_deleted)
                        {
                            continue;
                        }
                        // Found a deleted edge, now decide which uri has to be removed
                        if (!edge.contains("delete_policy"))
                        {
                            LOGI("update(): Ignoring forward edge " << edge << " because of missing delete_policy");
                            continue;
                        }
                        if (!edge.contains("relation_dir_forward"))
                        {
                            LOGI("update(): Ignoring forward edge " << edge << " because of missing relation_dir_forward");
                            continue;
                        }
                        const std::string delete_policy = edge["delete_policy"].get<std::string>();
                        const bool relation_dir_forward = edge["relation_dir_forward"].get<bool>();
                        const std::string target_uri = edge["target"].get<std::string>();
                        const std::string source_uri = edge["source"].get<std::string>();
                        // Check if either one or both of source and target have to be removed
                        if (relation_dir_forward && delete_policy == "DELETETARGET")
                        {
                            to_be_removed.insert(target_uri);
                        }
                        else if (relation_dir_forward && delete_policy == "DELETESOURCE")
                        {
                            to_be_removed.insert(source_uri);
                        }
                        else if (!relation_dir_forward && delete_policy == "DELETETARGET")
                        {
                            to_be_removed.insert(source_uri);
                        }
                        else if (!relation_dir_forward && delete_policy == "DELETESOURCE")
                        {
                            to_be_removed.insert(target_uri);
                        }
                        else if (delete_policy == "DELETEBOTH")
                        {
                            to_be_removed.insert(target_uri);
                            to_be_removed.insert(source_uri);
                        }
                    }
                }
            }
        }
        // Remove all those models which have been marked before
        for (const auto &uri : to_be_removed)
        {
            success &= this->_remove(uri);
        }
        return success;
    }

    nl::json JsonDatabaseBackend::find(const std::string &classname, const nl::json &properties)
    {
        GUARD_DATABASE(m_graph);
        nl::json results = this->_find(classname, properties);
        return results;
    }
    nl::json JsonDatabaseBackend::_find(const std::string &classname, const nl::json &properties)
    {
        LOGI("Finding " << classname << " with properties " << properties << " ...");
        nl::json results(nl::json::value_t::array);
        if (properties.contains("uri"))
        {
            const std::string uri = properties["uri"].get<std::string>();
            const nl::json model = this->_load(uri, classname);
            if(not model.is_null())
                results.push_back(model);
            return results;
        }
        const nl::json all_xtypes = this->getXtypes(m_graph, classname);
        for (const auto &[uri, model] : all_xtypes.items())
        {
            bool matches = true;
            const nl::json& model_properties = model.contains("properties") ? model["properties"] : model;
            for (const auto &[k2, v2] : properties.items())
            {
                if (!model_properties.contains(k2))
                {
                    matches = false;
                    break;
                }
                if (v2 != model_properties[k2])
                {
                    matches = false;
                    break;
                }
            }
            if (matches)
                results.push_back(model);
        }

        return results;
    }

    bool JsonDatabaseBackend::remove(const std::string &uri)
    {
        GUARD_DATABASE(m_graph);
        return this->_remove(uri);
    }
    bool JsonDatabaseBackend::_remove(const std::string &uri)
    {
        LOGI("Removing " << uri << " from m_graph " << m_graph);
        std::set<std::string> to_be_removed;
        std::deque<std::string> to_be_visited = {uri};
        while (to_be_visited.size() > 0)
        {
            std::string other_uri = to_be_visited.front();
            to_be_visited.pop_front();
            // Check if we already visited and marked the target for removal
            if (to_be_removed.count(other_uri) > 0)
                continue;
            to_be_removed.insert(other_uri);
            // Follow forward edges and remove any targets which are referenced by a matching delete_policy
            const nl::json forward_edges = this->_findEdgesFrom({other_uri});
            if (forward_edges.contains(other_uri))
            {
                for (const auto &[attr, edges] : forward_edges[other_uri].items())
                {
                    for (const auto &edge : edges)
                    {
                        if (!edge.contains("delete_policy"))
                        {
                            LOGI("remove(): Ignoring forward edge " << edge << " because of missing delete_policy");
                            continue;
                        }
                        if (!edge.contains("relation_dir_forward"))
                        {
                            LOGI("remove(): Ignoring forward edge " << edge << " because of missing relation_dir_forward");
                            continue;
                        }
                        const std::string delete_policy = edge["delete_policy"].get<std::string>();
                        const bool relation_dir_forward = edge["relation_dir_forward"].get<bool>();
                        const std::string target_uri = edge["target"].get<std::string>();
                        // NOTE: We do not need to handle the source_uri cases here, because it is already marked as being removed :)
                        // Any edge in forward direction which has DELETETARGET policy will trigger the removal of the target
                        if (relation_dir_forward && delete_policy == "DELETETARGET")
                        {
                            to_be_visited.push_back(target_uri);
                        }
                        // Any inverted edge which has DELETESOURCE policy will trigger the removal of the target
                        else if (!relation_dir_forward && delete_policy == "DELETESOURCE")
                        {
                            to_be_visited.push_back(target_uri);
                        }
                        // Any edge with DELETEBOTH will trigger the removal of the target
                        else if (delete_policy == "DELETEBOTH")
                        {
                            to_be_visited.push_back(target_uri);
                        }
                    }
                }
            }
            // Follow backward edges and remove any targets which are referenced by a matching delete_policy
            const nl::json backward_edges = this->_findEdgesTo({other_uri});
            for (const auto &[backward_uri, _] : backward_edges.items())
            {
                for (const auto &[attr, edges] : backward_edges[backward_uri].items())
                {
                    for (const auto &edge : edges)
                    {
                        if (!edge.contains("delete_policy"))
                        {
                            LOGI("remove(): Ignoring backward edge " << edge << " because of missing delete_policy");
                            continue;
                        }
                        if (!edge.contains("relation_dir_forward"))
                        {
                            LOGI("remove(): Ignoring backward edge " << edge << " because of missing relation_dir_forward");
                            continue;
                        }
                        const std::string delete_policy = edge["delete_policy"].get<std::string>();
                        const bool relation_dir_forward = edge["relation_dir_forward"].get<bool>();
                        const std::string source_uri = edge["source"].get<std::string>();
                        if (source_uri != backward_uri)
                            throw std::runtime_error("_remove(): " + source_uri + " not equal to " + backward_uri);
                        // NOTE: We do not need to handle the source_uri cases here, because it is already marked as being removed :)
                        // Any edge in forward direction which has DELETESOURCE policy will trigger the removal of the target
                        if (relation_dir_forward && delete_policy == "DELETESOURCE")
                        {
                            to_be_visited.push_back(source_uri);
                        }
                        // Any inverted edge which has DELETETARGET policy will trigger the removal of the target
                        else if (!relation_dir_forward && delete_policy == "DELETETARGET")
                        {
                            to_be_visited.push_back(source_uri);
                        }
                        // Any edge with DELETEBOTH will trigger the removal of the target
                        else if (delete_policy == "DELETEBOTH")
                        {
                            to_be_visited.push_back(source_uri);
                        }
                    }
                }
            }
        }

        // Remove the given uris
        bool result = removeFiles(m_graph, to_be_removed);
        for (const auto &rm_uri : to_be_removed)
        {
            // Fix any remaining dangling references
            this->_removeEdgesTo({rm_uri});
        }
        return result;
    }

    bool JsonDatabaseBackend::clear()
    {
        GUARD_DATABASE(m_graph);
        return this->_clear();
    }
    bool JsonDatabaseBackend::_clear()
    {
        LOGI("Clearing Database ...");
        return removeAllFiles(m_graph);
    }

    nl::json JsonDatabaseBackend::load(const std::string &uri, const std::string &classname)
    {
        GUARD_DATABASE(m_graph);
        nl::json result = this->_load(uri, classname);
        return result;
    }
    nl::json JsonDatabaseBackend::_load(const std::string &uri, const std::string &classname)
    {
        nl::json result;
        LOGI("Loading by URI " << uri << " or classname " << classname);
        if (!uri.empty())
        {
            nl::json all_xtypes = getXtypesByURI(m_graph, uri, classname);
            if (!all_xtypes.empty() && all_xtypes.contains(uri))
            {
                return all_xtypes[uri];
            }
            return nl::json();
        }
        return getXtypes(m_graph, classname);
    }

    bool JsonDatabaseBackend::_store(const nl::json &xtype)
    {
        LOGI("Storing xtype " << xtype["uri"] << " into graph " << m_graph);
        const std::string classname = xtype["classname"].get<std::string>();
        const std::string uri = xtype["uri"].get<std::string>();
        const fs::path path = createFilePath(m_graph, classname, uri);

        // - Open file at 'path' and write to it
        if (std::ofstream ofs{path.string()})
        {
            ofs << std::setw(4) << xtype;
            ofs.close();
            return true;
        }
        else
            LOGE("Could not open file " << path.string());
        return false;
    }

    nl::json JsonDatabaseBackend::findEdgesFrom(const std::vector<std::string> &uris)
    {
        GUARD_DATABASE(m_graph);
        nl::json edges = this->_findEdgesFrom(uris);
        return edges;
    }
    nl::json JsonDatabaseBackend::_findEdgesFrom(const std::vector<std::string> &uris)
    {
        nl::json edges;
        for (const auto &uri : uris)
        {
            const nl::json db_model = this->_load(uri);
            if (db_model.empty())
                continue;
            const nl::json& relations = db_model.contains("relations") ? db_model["relations"] : db_model;
            for (const auto &[k, v] : relations.items())
            {
                if (!v.is_array())
                    continue;
                for (auto potential_edge : v)
                {
                    if (!potential_edge.is_structured())
                        continue;
                    if (!potential_edge.contains("edge_properties"))
                        continue;
                    if (!potential_edge.contains("target"))
                        continue;
                    potential_edge["source"] = uri;
                    edges[uri][k].push_back(potential_edge);
                }
            }
        }
        return edges;
    }

    nl::json JsonDatabaseBackend::findEdgesTo(const std::vector<std::string> &uris)
    {
        GUARD_DATABASE(m_graph);
        nl::json edges = this->_findEdgesTo(uris);
        return edges;
    }
    nl::json JsonDatabaseBackend::_findEdgesTo(const std::vector<std::string> &uris)
    {
        nl::json edges;
        const nl::json all_xtypes = this->getXtypes(m_graph);
        for (const auto &[db_uri, db_model] : all_xtypes.items())
        {
            const nl::json& relations = db_model.contains("relations") ? db_model["relations"] : db_model;
            for (const auto &[k, v] : relations.items())
            {
                if (!v.is_array())
                    continue;
                for (auto potential_edge : v)
                {
                    if (!potential_edge.is_structured())
                        continue;
                    if (!potential_edge.contains("edge_properties"))
                        continue;
                    if (!potential_edge.contains("target"))
                        continue;
                    if (std::find(uris.begin(), uris.end(), potential_edge["target"].get<std::string>()) == uris.end())
                        continue;
                    potential_edge["source"] = db_uri;
                    edges[db_uri][k].push_back(potential_edge);
                }
            }
        }
        return edges;
    }

    void JsonDatabaseBackend::removeEdgesTo(const std::vector<std::string> &uris)
    {
        GUARD_DATABASE(m_graph);
        this->_removeEdgesTo(uris);
    }

    void JsonDatabaseBackend::_removeEdgesTo(const std::vector<std::string> &uris)
    {
        nl::json all_xtypes = this->getXtypes(m_graph);
        for (auto &[db_uri, db_model] : all_xtypes.items())
        {
            // Check if we already have the "relations" key
            auto relations_items = db_model.contains("relations") ? db_model["relations"].items() : db_model.items();
            // Cycle through all references and remove given uri(s) from it
            for (auto &[k, v] : relations_items)
            {
                if (!v.is_array()) // if v is not list
                    continue;
                // Found a potential edge list to be processed
                std::size_t index = 0;
                bool found_one = false;
                do {
                    found_one = false;
                    for (; index < v.size(); ++index)
                    {
                        const auto &potential_edge = v[index];
                        if (!potential_edge.is_structured())
                            continue;
                        if (!potential_edge.contains("edge_properties"))
                            continue;
                        if (!potential_edge.contains("target"))
                            continue;
                        // if target uri is not in uris
                        if (std::find(uris.begin(), uris.end(), potential_edge["target"].get<std::string>()) == uris.end())
                            continue;
                        found_one = true;
                        break;
                    }
                    if (found_one)
                    {
                        // NOTE: When we delete an index the entries AFTER the current index will move forward, so we start again
                        v.erase(index);
                    }
                } while (found_one);
            }
            this->_store(db_model);
        }
    }

    void JsonDatabaseBackend::setWorkingGraph(const std::string &graph)
    {
        m_graph = graph;
    }

    std::string JsonDatabaseBackend::getWorkingGraph()
    {
        return m_graph;
    }

    std::string JsonDatabaseBackend::dumps(const nl::json &dict)
    {
        return dict.dump();
    }
}
