/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscout/import/GenTypeDat.h>

#include <osmscout/util/File.h>
#include <osmscout/util/FileWriter.h>
#include <vector>

namespace osmscout {

  std::string TypeDataGenerator::GetDescription() const
  {
    return "Generate 'types.dat'";
  }

  bool TypeDataGenerator::Import(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig)
  {
    //
    // Analysing distribution of nodes in the given interval size
    //

    progress.SetAction("Generate types.dat");

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "types.dat"))) {
      progress.Error("Cannot create 'types.dat'");
      return false;
    }

    writer.WriteNumber((uint32_t)typeConfig.GetTags().size());
    for (std::vector<TagInfo>::const_iterator tag=typeConfig.GetTags().begin();
         tag!=typeConfig.GetTags().end();
         ++tag) {
      writer.WriteNumber(tag->GetId());
      writer.Write(tag->GetName());
      writer.Write(tag->IsInternalOnly());
    }

    uint32_t nameTagCount=0;
    uint32_t nameAltTagCount=0;

    for (std::vector<TagInfo>::const_iterator tag=typeConfig.GetTags().begin();
         tag!=typeConfig.GetTags().end();
         ++tag) {
      uint32_t priority;

      if (typeConfig.IsNameTag(tag->GetId(),priority)) {
        nameTagCount++;
      }

      if (typeConfig.IsNameAltTag(tag->GetId(),priority)) {
        nameAltTagCount++;
      }
    }

    writer.WriteNumber(nameTagCount);
    for (std::vector<TagInfo>::const_iterator tag=typeConfig.GetTags().begin();
     tag!=typeConfig.GetTags().end();
     ++tag) {
      uint32_t priority;

      if (typeConfig.IsNameTag(tag->GetId(),priority)) {
        writer.WriteNumber(tag->GetId());
        writer.WriteNumber((uint32_t)priority);
      }
    }

    writer.WriteNumber(nameAltTagCount);
    for (std::vector<TagInfo>::const_iterator tag=typeConfig.GetTags().begin();
     tag!=typeConfig.GetTags().end();
     ++tag) {
      uint32_t priority;

      if (typeConfig.IsNameAltTag(tag->GetId(),priority)) {
        writer.WriteNumber(tag->GetId());
        writer.WriteNumber((uint32_t)priority);
      }
    }


    writer.WriteNumber((uint32_t)typeConfig.GetTypes().size());

    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
         type!=typeConfig.GetTypes().end();
         ++type) {
      writer.WriteNumber(type->GetId());
      writer.Write(type->GetName());
      writer.Write(type->CanBeNode());
      writer.Write(type->CanBeWay());
      writer.Write(type->CanBeArea());
      writer.Write(type->CanBeRelation());
      writer.Write(type->CanRouteFoot());
      writer.Write(type->CanRouteBicycle());
      writer.Write(type->CanRouteCar());
      writer.Write(type->GetIndexAsLocation());
      writer.Write(type->GetIndexAsRegion());
      writer.Write(type->GetIndexAsPOI());
      writer.Write(type->GetConsumeChildren());
      writer.Write(type->GetOptimizeLowZoom());
      writer.Write(type->GetMultipolygon());
      writer.Write(type->GetPinWay());
      writer.Write(type->GetIgnoreSeaLand());
      writer.Write(type->GetIgnore());
    }

    return !writer.HasError() && writer.Close();
  }
}

