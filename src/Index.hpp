#ifndef NBS_INDEX_HPP
#define NBS_INDEX_HPP

#include <filesystem>
#include <iostream>
#include <map>
#include <vector>

#include "IndexItem.hpp"
#include "zstr/zstr.hpp"

namespace nbs {
    class Index {
    public:
        /// Empty default constructor
        Index(){};

        /**
         * @brief Construct a new Index object based on the provided list of paths
         *
         * @param paths         the paths to the nbs files we are getting the index for
         */
        template <typename T>
        Index(const T& paths) {
            for (size_t i = 0; i < paths.size(); i++) {
                auto& path                    = paths[i];
                std::filesystem::path idxPath = path + ".idx";

                // If our index file does not exist we need to create it
                if (!std::filesystem::exists(idxPath)) {
                    throw std::runtime_error("nbs index not found for file: " + path);
                }

                // Load the index file
                zstr::ifstream input(idxPath.string());

                while (input.good()) {
                    IndexItemFile itemFile{};
                    input.read(reinterpret_cast<char*>(&itemFile.item), sizeof(IndexItem));
                    itemFile.fileno = i;

                    if (input.good()) {
                        this->idx.push_back(itemFile);
                        this->typeMap[std::make_pair(itemFile.item.type, itemFile.item.subtype)];
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
                comparisonValue.item.type    = mapEntry.first.first;
                comparisonValue.item.subtype = mapEntry.first.second;

                mapEntry.second = std::equal_range(
                    idx.begin(),
                    idx.end(),
                    comparisonValue,
                    [](const IndexItemFile& a, const IndexItemFile& b) {
                        return a.item.type != b.item.type ? a.item.type < b.item.type : a.item.subtype < b.item.subtype;
                    });
            }
        }

        /**
         * Get the iterator range (begin, end) for the given type and subtype in the index
         */
        std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator> getIteratorForType(
            std::pair<uint64_t, uint32_t> typeSubtype) {
            return this->typeMap[typeSubtype];
        }

        /**
         * Get a list of iterator ranges (begin, end) for the given list of type and subtype in the index
         */
        std::vector<std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator>>
            getIteratorsForTypes(std::vector<std::pair<uint64_t, uint32_t>> types) {
            std::vector<std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator>> matches;

            for (auto& type : types) {
                if (this->typeMap.find(type) != this->typeMap.end()) {
                    matches.push_back(this->typeMap[type]);
                }
            }

            return matches;
        }

        std::vector<std::pair<uint64_t, uint32_t>> getTypes() {
            std::vector<std::pair<uint64_t, uint32_t>> types;

            for (auto& mapEntry : this->typeMap) {
                types.push_back(mapEntry.first);
            }

            return types;
        }

        std::pair<uint64_t, uint64_t> getTimestampRange() {
            std::pair<uint64_t, uint64_t> range{std::numeric_limits<uint64_t>::max(), 0};

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

        std::pair<uint64_t, uint64_t> getTimestampRange(std::pair<uint64_t, uint32_t> typeSubtype) {
            std::pair<uint64_t, uint64_t> range{0, 0};

            if (this->typeMap.find(typeSubtype) != this->typeMap.end()) {
                auto iteratorPair = this->typeMap[typeSubtype];

                range.first  = iteratorPair.first->item.timestamp;
                range.second = (iteratorPair.second - 1)->item.timestamp;
            }

            return range;
        }

    private:
        /// The index data: a vector holding the index items
        std::vector<IndexItemFile> idx;

        /// Maps a (type, subtype) to the iterator over the index items for that type and subtype
        std::map<std::pair<uint64_t, uint32_t>,
                 std::pair<std::vector<IndexItemFile>::iterator, std::vector<IndexItemFile>::iterator>>
            typeMap;
    };

}  // namespace nbs

#endif  // NBS_INDEX_HPP
