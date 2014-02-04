#ifndef OSMSCOUT_IMPORT_GENTEXTDAT_H
#define OSMSCOUT_IMPORT_GENTEXTDAT_H

/*
 This source is part of the libosmscout library
 Copyright (C) 2013 Preet Desai

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#include <osmscout/import/Import.h>

#include <marisa.h>

namespace osmscout
{
    class TextDataGenerator : public ImportModule
    {
    public:
        std::string GetDescription() const;

        bool Import(ImportParameter const &parameter,
                    Progress &progress,
                    TypeConfig const &typeConfig);

        bool addNodeTextToKeysets(ImportParameter const &parameter,
                                  Progress &progress,
                                  TypeConfig const &typeConfig,
                                  uint32_t const max_keycount,
                                  marisa::Keyset &keyset_poi,
                                  marisa::Keyset &keyset_loc,
                                  marisa::Keyset &keyset_region,
                                  marisa::Keyset &keyset_other);

        void cutNodeNameDataFromTags(TypeConfig const &typeConfig,
                                     std::vector<Tag> &tags,
                                     std::string &name,
                                     std::string &name_alt);
    };
}


#endif // OSMSCOUT_IMPORT_GENTEXTDAT_H
