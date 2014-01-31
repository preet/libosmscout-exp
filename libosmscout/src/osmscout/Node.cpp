/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/Node.h>

namespace osmscout {

  bool NodeAttributes::SetTags(Progress& /*progress*/,
                               const TypeConfig& typeConfig,
                               std::vector<Tag>& tags)
  {
    uint32_t namePriority=0;
    uint32_t nameAltPriority=0;

    name.clear();
    nameAlt.clear();
    address.clear();

    flags=0;

    this->tags.clear();

    std::vector<Tag>::iterator tag=tags.begin();
    while (tag!=tags.end()) {
      uint32_t ntPrio;
      bool     isNameTag=typeConfig.IsNameTag(tag->key,ntPrio);
      uint32_t natPrio;
      bool     isNameAltTag=typeConfig.IsNameAltTag(tag->key,natPrio);

      if (isNameTag &&
          (name.empty() || ntPrio>namePriority)) {
        name=tag->value;
        namePriority=ntPrio;
      }

      if (isNameAltTag &&
          (nameAlt.empty() || natPrio>nameAltPriority)) {
        nameAlt=tag->value;
        nameAltPriority=natPrio;
      }

      if (isNameTag || isNameAltTag) {
        tag=tags.erase(tag);
      }
      else if (tag->key==typeConfig.tagHouseNr) {
        address=tag->value;
        tag=tags.erase(tag);
      }
      else {
        ++tag;
      }
    }

    this->tags=tags;

    if(!name.empty()) {
        flags |= hasName;
    }
    if(!nameAlt.empty()) {
        flags |= hasNameAlt;
    }
    if(!address.empty()) {
        flags |= hasAddress;
    }
    if(!this->tags.empty()) {
        flags |= hasTags;
    }

    return true;
  }

  void NodeAttributes::SetEmptyCity()
  {
      flags |= hasCity;
      cityIsEmpty = true;
  }

  void NodeAttributes::SetEmptyStreet()
  {
      flags |= hasStreet;
      streetIsEmpty = true;
  }

  void NodeAttributes::SetCity(FileOffset cityOffset,
                               bool cityIsNode)
  {
      flags |= hasCity;
      city = cityOffset;
      cityIsEmpty=false;

      if(cityIsNode) {
          flags |= hasCityAsNode;
      }
      this->cityIsNode=cityIsNode;
  }

  void NodeAttributes::SetStreet(FileOffset streetOffset)
  {
      flags |= hasStreet;
      street = streetOffset;
      streetIsEmpty=false;
  }

  void NodeAttributes::GetFlags(uint8_t& flags) const
  {
      flags = this->flags;
  }

  bool NodeAttributes::Read(FileScanner& scanner)
  {
    scanner.Read(flags);

    if (scanner.HasError()) {
      return false;
    }

    if (flags & hasName) {
      scanner.Read(name);
    }

    if (flags & hasNameAlt) {
      scanner.Read(nameAlt);
    }

    if (flags & hasAddress) {
      scanner.Read(address);
    }

    if (flags & hasCity) {
        scanner.Read(cityIsNode);
        scanner.ReadFileOffset(city);
    }

    if (flags & hasStreet) {
        scanner.ReadFileOffset(street);
    }

    if (flags & hasTags) {
      uint32_t tagCount;

      scanner.ReadNumber(tagCount);
      if (scanner.HasError()) {
        return false;
      }

      tags.resize(tagCount);
      for (size_t i=0; i<tagCount; i++) {
        scanner.ReadNumber(tags[i].key);
        scanner.Read(tags[i].value);
      }
    }

    return !scanner.HasError();
  }

  bool NodeAttributes::Write(FileWriter& writer) const
  {
    uint8_t flags;

    GetFlags(flags);

    writer.Write(flags);

    if (flags & hasName) {
      writer.Write(name);
    }

    if (flags & hasNameAlt) {
      writer.Write(nameAlt);
    }

    if (flags & hasAddress) {
      writer.Write(address);
    }

    if(flags & hasCity) {
        writer.Write(cityIsNode);
        writer.WriteFileOffset(city);
    }

    if(flags & hasStreet) {
        writer.WriteFileOffset(street);
    }

    if (flags & hasTags) {
      writer.WriteNumber((uint32_t)tags.size());

      for (size_t i=0; i<tags.size(); i++) {
        writer.WriteNumber(tags[i].key);
        writer.Write(tags[i].value);
      }
    }

    return !writer.HasError();
  }

  bool NodeAttributes::operator==(const NodeAttributes& other) const
  {
    if (name!=other.name ||
        nameAlt!=other.nameAlt ||
        address!=other.address) {
      return false;
    }

    if(this->flags != other.GetFlags()) {
        return false;
    }

    if(this->HasCity()!=other.HasCity() ||
       this->GetCity()!=other.GetCity() ||
       this->GetCityTypeIsNode()!=other.GetCityTypeIsNode()) {
        return false;
    }

    if(this->HasStreet()!=other.HasStreet() ||
       this->GetStreet()!=other.GetStreet()) {
        return false;
    }

    if (tags.empty() && other.tags.empty()) {
      return true;
    }

    if (tags.size()!=other.tags.size()) {
      return false;
    }

    for (size_t t=0; t<tags.size(); t++) {
      if (tags[t].key!=other.tags[t].key) {
        return false;
      }

      if (tags[t].value!=other.tags[t].value) {
        return false;
      }
    }

    return true;
  }

  bool NodeAttributes::operator!=(const NodeAttributes& other) const
  {
    return !this->operator==(other);
  }


  void Node::SetType(TypeId type)
  {
    this->type=type;
  }

  void Node::SetCoords(const GeoCoord& coords)
  {
    this->coords=coords;
  }

  bool Node::SetTags(Progress& progress,
                     const TypeConfig& typeConfig,
                     std::vector<Tag>& tags)
  {
    return attributes.SetTags(progress,
                              typeConfig,
                              tags);
  }

  void Node::SetEmptyCity()
  {
      attributes.SetEmptyCity();
  }

  void Node::SetEmptyStreet()
  {
      attributes.SetEmptyStreet();
  }

  void Node::SetCity(FileOffset cityOffset,
                     bool cityIsNode)
  {
      attributes.SetCity(cityOffset,
                         cityIsNode);
  }

  void Node::SetStreet(FileOffset streetOffset)
  {
      attributes.SetStreet(streetOffset);
  }

  bool Node::Read(FileScanner& scanner)
  {
    uint32_t tmpType;

    if (!scanner.GetPos(fileOffset)) {
      return false;
    }

    scanner.ReadNumber(tmpType);
    type=(TypeId)tmpType;

    scanner.ReadCoord(coords);

    if (!attributes.Read(scanner)) {
      return false;
    }

    return !scanner.HasError();
  }

  bool Node::Write(FileWriter& writer) const
  {
    writer.WriteNumber(type);
    writer.WriteCoord(coords);

    if (!attributes.Write(writer)) {
      return false;
    }

    return !writer.HasError();
  }
}

