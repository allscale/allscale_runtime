#ifndef ALLSCALE_DATA_ITEM_REFERENCE_HPP
#define ALLSCALE_DATA_ITEM_REFERENCE_HPP

#include <allscale/utils/serializer.h>



using id_type = std::size_t;

namespace allscale
{
    template<typename DataItemType>
    struct data_item_reference
    {

        id_type id;

        bool operator==(const data_item_reference& other) const {
            return id == other.id;
        }

        static data_item_reference load(allscale::utils::ArchiveReader& reader) {
            return { reader.read<id_type>() };
        }

        void store(allscale::utils::ArchiveWriter& writer) const {
            writer.write(id);
        }
    };
}

#endif
