#ifndef NBS_INDEX_HPP
#define NBS_INDEX_HPP

#include <iostream>
#include <map>
#include <sys/stat.h>
#include <vector>

#include "IndexItem.hpp"
#include "TypeSubtype.hpp"
#include "zstr/zstr.hpp"
namespace nbs {

    class Index {
    public:
        /// Empty default constructor
        Index(){};

        /**
         * Construct a new Index object with items in the provided list of nbs file paths
         *
         * @param paths the paths to the nbs files to load the index for
         */
        template <typename T>
        Index(const T& paths) {
            for (size_t i = 0; i < paths.size(); i++) {
                auto& nbsPath       = paths[i];
                std::string idxPath = nbsPath + ".idx";

                // Currently we only handle nbs files that have an index file
                // If there's no index file, throw
                if (!this->fileExists(idxPath)) {
                    throw std::runtime_error("nbs index not found for file: " + nbsPath);
                }

                // Load the index file
                zstr::ifstream input(idxPath, zstr::ifstream::binary);

                // Read the index items from the file
                while (input.good()) {
                    IndexItemFile itemFile{};
                    input.read(reinterpret_cast<char*>(&itemFile.item), sizeof(IndexItem));
                    itemFile.fileno = i;

                    if (input.good()) {
                        // Add the item to our flat list of all index items
                        this->idx.push_back(itemFile);

                        // Add the item to our map of index items by type/subtype
                        this->typeMap[TypeSubtype{itemFile.item.type, itemFile.item.subtype}];
                    }
                }
            }

            // Sort the index by type, then subtype, then timestamp
            std::sort(this->idx.begin(), this->idx.end(), [](const IndexItemFile& a, const IndexItemFile& b) {
                return a.item.type != b.item.type         ? a.item.type < b.item.type
                       : a.item.subtype != b.item.subtype ? a.item.subtype < b.item.subtype
                                                          : a.item.timestamp < b.item.timestamp;
            });

            // Populate the type map
            for (auto& mapEntry : this->typeMap) {
                IndexItemFile comparisonValue{};
                comparisonValue.item.type    = mapEntry.first.type;
                comparisonValue.item.subtype = mapEntry.first.subtype;

                // The value of the map entry a pair of iterators: the first iterator points to
                // the first item in the vector that matches the comparison value, the second
                // iterator points to one past the last matching item
                mapEntry.second = std::equal_range(
                    idx.begin(),
                    idx.end(),
                    comparisonValue,
                    [](const IndexItemFile& a, const IndexItemFile& b) {
                        return a.item.type != b.item.type ? a.item.type < b.item.type : a.item.subtype < b.item.subtype;
                    });
            }
        }

        /// Get the iterator range (begin, end) for the given type and subtype in the index
        std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator> getIteratorForType(
            const TypeSubtype& type) {
            return this->typeMap[type];
        }

        /// Get a list of iterator ranges (begin, end) for the given list of type and subtype in the index
        std::vector<std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator>>
            getIteratorsForTypes(const std::vector<TypeSubtype>& types) {
            std::vector<std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator>> matches;

            for (auto& type : types) {
                if (this->typeMap.find(type) != this->typeMap.end()) {
                    matches.push_back(this->typeMap[type]);
                }
            }

            return matches;
        }

        /// Get a list of all types and subtypes in the index
        std::vector<TypeSubtype> getTypes() {
            std::vector<TypeSubtype> types;

            for (auto& mapEntry : this->typeMap) {
                types.push_back(mapEntry.first);
            }

            return types;
        }


        // Get timestamp where as many subtypes move forward without moving forward twice.
        std::uint64_t nextTimestamp(const uint64_t& timestamp,
                                    const std::vector<TypeSubtype>& types,
                                    const int& steps) {

            auto matchingIndexItems = this->getIteratorsForTypes(types);  // Iterator for each type(s).

            bool best_found         = false;
            uint64_t best_timestamp = std::numeric_limits<uint64_t>::max();
            uint64_t best_delta     = std::numeric_limits<uint64_t>::max();

            for (auto& range : matchingIndexItems) {
                auto& begin = range.first;
                auto& end   = range.second;

                IndexItemFile target;
                target.item.timestamp = timestamp;

                if (begin != end) {
                    // Find the first item with a timestamp greater than the requested timestamp.
                    auto position =
                        std::upper_bound(begin, end, target, [](const IndexItemFile& a, const IndexItemFile& b) {
                            return a.item.timestamp < b.item.timestamp;
                        });

                    int length = std::distance(begin, end);
                    int target = std::distance(begin, position) - 1 + steps;

                    // Prevent position going past begin or end, return respective iterator timestamp.
                    if (0 <= target && target < length) {
                        best_found     = true;
                        auto ts        = std::next(begin, target)->item.timestamp;
                        uint64_t delta = steps > 0 ? ts - timestamp : timestamp - ts;
                        if (delta < best_delta) {
                            best_delta     = delta;
                            best_timestamp = ts;
                        }
                    }
                }
            }
            // If no timestamp found, iterate through types and return min/max timestamp, respective of steps.
            if (!best_found) {
                uint64_t min_timestamp = std::numeric_limits<uint64_t>::max();
                uint64_t max_timestamp = std::numeric_limits<uint64_t>::min();
                for (auto& range : matchingIndexItems) {
                    auto& begin = range.first;
                    auto& end   = range.second;
                    if (begin != end) {
                        min_timestamp = std::min(min_timestamp, begin->item.timestamp);
                        max_timestamp = std::max(max_timestamp, std::prev(end)->item.timestamp);
                    }
                }

                if (steps == 0) {
                    return timestamp > max_timestamp ? max_timestamp : min_timestamp;
                }
                return steps > 0 ? max_timestamp : min_timestamp;
            }

            return best_timestamp;
        }


        /// Get the first and last timestamps across all items in the index
        std::pair<uint64_t, uint64_t> getTimestampRange() {
            // std::numeric_limits<uint64_t>::max is wrapped in () here to workaround an issue on Windows
            // See https://stackoverflow.com/a/27443191
            std::pair<uint64_t, uint64_t> range{(std::numeric_limits<uint64_t>::max)(), 0};

            for (auto& mapEntry : this->typeMap) {
                const IndexItem& firstItem = mapEntry.second.first->item;
                const IndexItem& lastItem  = (mapEntry.second.second - 1)->item;

                if (firstItem.timestamp < range.first) {
                    range.first = firstItem.timestamp;
                }

                if (lastItem.timestamp > range.second) {
                    range.second = lastItem.timestamp;
                }
            }

            return range;
        }

        /// Get the first and last timestamps in the index for the given type and subtype
        std::pair<uint64_t, uint64_t> getTimestampRange(const TypeSubtype& type) {
            std::pair<uint64_t, uint64_t> range{0, 0};

            if (this->typeMap.find(type) != this->typeMap.end()) {
                auto iteratorPair = this->typeMap[type];

                range.first  = iteratorPair.first->item.timestamp;
                range.second = (iteratorPair.second - 1)->item.timestamp;
            }

            return range;
        }

    private:
        /// The index data: a vector holding the index items
        std::vector<IndexItemFile> idx;

        /// Maps a (type, subtype) to the iterator over the index items for that type and subtype
        std::map<TypeSubtype, std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator>>
            typeMap;

        /// Check if a file exists at the given path
        bool fileExists(const std::string& path) {
            // Shamelessly stolen from: http://stackoverflow.com/a/12774387/1387006
            struct stat buffer {};
            return (stat(path.c_str(), &buffer) == 0);
        }
    };

}  // namespace nbs

#endif  // NBS_INDEX_HPP
