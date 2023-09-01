#pragma once

#include <nlohmann/json.hpp>
#include <set>
#include <iostream>

namespace nl = nlohmann;

/*
 * @brief 3-way merge of json objects
 */
bool three_way_merge(const nl::json& original, const nl::json& ours, const nl::json& theirs, nl::json& result)
{
    bool conflict = false;

    std::set< nl::json::string_t > all_keys;
    for (const auto& it : ours.items())
    {
        all_keys.insert(it.key());
    }
    for (const auto& it : theirs.items())
    {
        all_keys.insert(it.key());
    }

    for (const nl::json::string_t& k : all_keys)
    {
        const bool original_has_key(original.contains(k));
        const bool ours_has_key(ours.contains(k));
        const bool theirs_has_key(theirs.contains(k));
        const nl::json original_value(original_has_key ? original[k] : nl::json());
        const nl::json our_value(ours_has_key ? ours[k] : nl::json());
        const nl::json their_value(theirs_has_key ? theirs[k] : nl::json());

        // Check if ours and theirs disagree
        if (our_value != their_value)
        {
            // Check if theirs and original agree
            if (their_value == original_value)
            {
                // then use our value
                if (ours_has_key)
                    result[k] = our_value;
            }
            // if not, check if our and original agree
            else if (our_value == original_value)
            {
                // then use their value
                if (theirs_has_key)
                    result[k] = their_value;
            }
            // it could be that they disagree because subdictionaries have to be merged.
            else if (our_value.is_object() && their_value.is_object())
            {
                nl::json sub_result;
                conflict |= three_way_merge(original_value, our_value, their_value, sub_result);
                result[k] = sub_result;
            }
            else
            {
                // all disagree, conflict
                conflict = true;
                nl::json conflict_node;
                conflict_node["conflict"] = "FIXME";
                conflict_node["original"] = original_value;
                conflict_node["ours"] = our_value;
                conflict_node["theirs"] = their_value;
                result[k] = conflict_node;
            }
        } else {
            // both agree, so we just set it
            result[k] = our_value;
        }
    }

    return conflict;
}
