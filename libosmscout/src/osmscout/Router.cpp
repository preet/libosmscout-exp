/*
  This source is part of the libosmscout library
  Copyright (C) 2012  Tim Teulings

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

#include <osmscout/Router.h>

#include <algorithm>
#include <iostream>

#include <osmscout/RoutingProfile.h>
#include <osmscout/TypeConfigLoader.h>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/StopClock.h>

//#define DEBUG_ROUTING

namespace osmscout {

  RouterParameter::RouterParameter()
  : wayIndexCacheSize(10000),
    wayCacheSize(0),
    debugPerformance(false)
  {
    // no code
  }

  void RouterParameter::SetWayIndexCacheSize(unsigned long wayIndexCacheSize)
  {
    this->wayIndexCacheSize=wayIndexCacheSize;
  }

  void RouterParameter::SetWayCacheSize(unsigned long wayCacheSize)
  {
    this->wayCacheSize=wayCacheSize;
  }

  void RouterParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  unsigned long RouterParameter::GetWayIndexCacheSize() const
  {
    return wayIndexCacheSize;
  }

  unsigned long RouterParameter::GetWayCacheSize() const
  {
    return wayCacheSize;
  }

  bool RouterParameter::IsDebugPerformance() const
  {
    return debugPerformance;
  }

  const char* const Router::FILENAME_INTERSECTIONS_DAT = "intersections.dat";
  const char* const Router::FILENAME_INTERSECTIONS_IDX = "intersections.idx";

  const char* const Router::FILENAME_FOOT_DAT          = "routefoot.dat";
  const char* const Router::FILENAME_FOOT_IDX          = "routefoot.idx";

  const char* const Router::FILENAME_BICYCLE_DAT       = "routebicycle.dat";
  const char* const Router::FILENAME_BICYCLE_IDX       = "routebicycle.idx";

  const char* const Router::FILENAME_CAR_DAT           = "routecar.dat";
  const char* const Router::FILENAME_CAR_IDX           = "routecar.idx";

  Router::Router(const RouterParameter& parameter,
                 Vehicle vehicle)
   : vehicle(vehicle),
     isOpen(false),
     debugPerformance(parameter.IsDebugPerformance()),
     areaDataFile("areas.dat",
                  parameter.GetWayCacheSize()),
     wayDataFile("ways.dat",
                  parameter.GetWayCacheSize()),
     routeNodeDataFile(GetDataFilename(vehicle),
                       GetIndexFilename(vehicle),
                       0,
                       6000),
     junctionDataFile(Router::FILENAME_INTERSECTIONS_DAT,
                      Router::FILENAME_INTERSECTIONS_IDX,
                      0,
                      6000),
     typeConfig(NULL)
  {
    // no code
  }

  Router::~Router()
  {
    if (isOpen) {
      Close();
    }

    delete typeConfig;
  }

  std::string Router::GetDataFilename(Vehicle vehicle) const
  {
    switch (vehicle) {
    case vehicleFoot:
      return FILENAME_FOOT_DAT;
    case vehicleBicycle:
      return FILENAME_BICYCLE_DAT;
    case vehicleCar:
      return FILENAME_CAR_DAT;
    default:
      assert(false);
    }

    return ""; // make the compiler happy
  }

  std::string Router::GetIndexFilename(Vehicle vehicle) const
  {
    switch (vehicle) {
    case vehicleFoot:
      return FILENAME_FOOT_IDX;
    case vehicleBicycle:
      return FILENAME_BICYCLE_IDX;
    case vehicleCar:
      return FILENAME_CAR_IDX;
    default:
      assert(false);
    }

    return ""; // make the compiler happy
  }

  Vehicle Router::GetVehicle() const
  {
    return vehicle;
  }

  bool Router::Open(const std::string& path)
  {
    assert(!path.empty());

    this->path=path;

    typeConfig=new TypeConfig();

    if (!LoadTypeData(path,*typeConfig)) {
      std::cerr << "Cannot load 'types.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!areaDataFile.Open(path,
                           FileScanner::LowMemRandom,false)) {
      std::cerr << "Cannot open 'areas.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!wayDataFile.Open(path,
                          FileScanner::LowMemRandom,false)) {
      std::cerr << "Cannot open 'ways.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    if (!routeNodeDataFile.Open(path,
                                FileScanner::FastRandom,true,
                                FileScanner::FastRandom,true)) {
      std::cerr << "Cannot open 'route.dat'!" << std::endl;
      delete typeConfig;
      typeConfig=NULL;
      return false;
    }

    isOpen=true;

    return true;
  }

  bool Router::IsOpen() const
  {
    return isOpen;
  }


  void Router::Close()
  {
    routeNodeDataFile.Close();
    wayDataFile.Close();
    areaDataFile.Close();

    isOpen=false;
  }

  void Router::FlushCache()
  {
    areaDataFile.FlushCache();
    wayDataFile.FlushCache();
  }

  TypeConfig* Router::GetTypeConfig() const
  {
    return typeConfig;
  }

  void Router::GetClosestForwardRouteNode(const WayRef& way,
                                          size_t nodeIndex,
                                          RouteNodeRef& routeNode,
                                          size_t& routeNodeIndex)
  {
    routeNode=NULL;

    for (size_t i=nodeIndex; i<way->nodes.size(); i++) {
      routeNodeDataFile.Get(way->ids[i],
                            routeNode);

      if (routeNode.Valid()) {
        routeNodeIndex=i;
        return;
      }
    }
  }

  void Router::GetClosestBackwardRouteNode(const WayRef& way,
                                           size_t nodeIndex,
                                           RouteNodeRef& routeNode,
                                           size_t& routeNodeIndex)
  {
    routeNode=NULL;

    if (nodeIndex>=way->nodes.size()) {
      return;
    }

    if (!way->GetAttributes().GetAccess().CanRouteBackward()) {
      return;
    }

    for (long i=nodeIndex-1; i>=0; i--) {
      routeNodeDataFile.Get(way->ids[i],routeNode);

      if (routeNode.Valid()) {
        routeNodeIndex=i;
        return;
      }
    }
  }

  void Router::ResolveRNodeChainToList(const RNodeRef& end,
                                       const CloseMap& closeMap,
                                       std::list<RNodeRef>& nodes)
  {
    CloseMap::const_iterator current=closeMap.find(end->nodeOffset);

    while (current->second->prev!=0) {
      CloseMap::const_iterator prev=closeMap.find(current->second->prev);

      nodes.push_back(current->second);

      current=prev;
    }

    nodes.push_back(current->second);

    std::reverse(nodes.begin(),nodes.end());
  }

  void Router::AddNodes(RouteData& route,
                        Id startNodeId,
                        size_t startNodeIndex,
                        const ObjectFileRef& object,
                        size_t idCount,
                        bool oneway,
                        size_t targetNodeIndex)
  {
    assert(startNodeIndex<idCount);
    assert(targetNodeIndex<idCount);

    if (std::max(startNodeIndex,targetNodeIndex)-std::min(startNodeIndex,targetNodeIndex)==1) {
      // From one node to the neighbour node (+1 or -1)
      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     targetNodeIndex);
    }
    else if (startNodeIndex<targetNodeIndex) {
      // Following the way
      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     startNodeIndex+1);

      for (size_t i=startNodeIndex+1; i<targetNodeIndex-1; i++) {
        route.AddEntry(0,
                       i,
                       object,
                       i+1);
      }

      route.AddEntry(0,
                     targetNodeIndex-1,
                     object,
                     targetNodeIndex);
    }
    else if (oneway) {
      // startNodeIndex>tragetNodeIndex, but this is a oneway we assume
      // that the way is either an area or is a roundabound
      // TODO: proof this using the right assertions or checks
      size_t pos=startNodeIndex+1;
      size_t next;

      if (pos>=idCount) {
        pos=0;
      }

      next=pos+1;
      if (next>=idCount) {
        next=0;
      }

      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     pos);

      while (next!=targetNodeIndex) {
        route.AddEntry(0,
                       pos,
                       object,
                       next);

        pos++;
        if (pos>=idCount) {
          pos=0;
        }

        next=pos+1;
        if (next>=idCount) {
          next=0;
        }
      }

      route.AddEntry(0,
                     pos,
                     object,
                     targetNodeIndex);
    }
    else {
      // We follow the way in the opposite direction
      route.AddEntry(startNodeId,
                     startNodeIndex,
                     object,
                     startNodeIndex-1);

      for (long i=startNodeIndex-1; i>(long)targetNodeIndex+1; i--) {
        route.AddEntry(0,
                       i,
                       object,
                       i-1);
      }

      route.AddEntry(0,
                     targetNodeIndex+1,
                     object,
                     targetNodeIndex);
    }
  }

  bool Router::ResolveRNodesToRouteData(const RoutingProfile& profile,
                                        const std::list<RNodeRef>& nodes,
                                        const ObjectFileRef& startObject,
                                        size_t startNodeIndex,
                                        const ObjectFileRef& targetObject,
                                        size_t targetNodeIndex,
                                        RouteData& route)
  {
    std::set<FileOffset>                      routeNodeOffsets;
    std::set<FileOffset>                      wayOffsets;
    std::set<FileOffset>                      areaOffsets;

    OSMSCOUT_HASHMAP<FileOffset,RouteNodeRef> routeNodeMap;
    OSMSCOUT_HASHMAP<FileOffset,AreaRef>      areaMap;
    OSMSCOUT_HASHMAP<FileOffset,WayRef>       wayMap;

    std::vector<Id>                           *ids=NULL;
    bool                                      oneway=false;


    // Collect all route node file offsets on the path and also
    // all area/way file offsets on the path
    for (std::list<RNodeRef>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      RNodeRef node(*n);

      routeNodeOffsets.insert(node->nodeOffset);

      if (node->object.Valid()) {
        switch (node->object.GetType()) {
        case refArea:
          areaOffsets.insert(node->object.GetFileOffset());
          break;
        case refWay:
          wayOffsets.insert(node->object.GetFileOffset());
          break;
        default:
          assert(false);
          break;
        }
      }
    }

    // Collect area/way file offset for the
    // target object (not traversed by above loop)
    switch (targetObject.GetType()) {
    case refArea:
      areaOffsets.insert(targetObject.GetFileOffset());
      break;
    case refWay:
      wayOffsets.insert(targetObject.GetFileOffset());
      break;
    default:
      assert(false);
      break;
    }

    //
    // Load data
    //

    if (!routeNodeDataFile.GetByOffset(routeNodeOffsets,
                                       routeNodeMap)) {
      std::cerr << "Cannot load route nodes" << std::endl;
      return false;
    }

    if (!areaDataFile.GetByOffset(areaOffsets,
                                  areaMap)) {
      std::cerr << "Cannot load areas" << std::endl;
      return false;
    }

    if (!wayDataFile.GetByOffset(wayOffsets,
                                 wayMap)) {
      std::cerr << "Cannot load ways" << std::endl;
      return false;
    }

    if (startObject.GetType()==refArea) {
      OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(startObject.GetFileOffset());

      assert(entry!=areaMap.end());

      ids=&entry->second->rings.front().ids;
      oneway=false;
    }
    else if (startObject.GetType()==refWay) {
      OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(startObject.GetFileOffset());

      assert(entry!=wayMap.end());

      ids=&entry->second->ids;
      oneway=!profile.CanUseBackward(entry->second);
    }
    else {
      assert(false);
    }

    if (nodes.empty()) {
      // We assume that startNode and targetNode are on the same area/way (with no routing node in between)
      assert(startObject==targetObject);

      AddNodes(route,
               (*ids)[startNodeIndex],
               startNodeIndex,
               startObject,
               ids->size(),
               oneway,
               targetNodeIndex);

      route.AddEntry(0,
                     targetNodeIndex,
                     ObjectFileRef(),
                     0);
      return true;
    }

    RouteNodeRef initialNode=routeNodeMap.find(nodes.front()->nodeOffset)->second;

    //
    // Add The path from the start node to the first routing node
    //
    if ((*ids)[startNodeIndex]!=initialNode->GetId()) {
      size_t routeNodeIndex=0;

      while ((*ids)[routeNodeIndex]!=initialNode->GetId() &&
          routeNodeIndex<ids->size()) {
        routeNodeIndex++;
      }
      assert(routeNodeIndex<ids->size());

      // Start node to initial route node
      AddNodes(route,
               (*ids)[startNodeIndex],
               startNodeIndex,
               startObject,
               ids->size(),
               oneway,
               routeNodeIndex);
    }

    //
    // Walk the routing path from route node to the next route node
    // and build entries.
    //
    for (std::list<RNodeRef>::const_iterator n=nodes.begin();
        n!=nodes.end();
        n++) {
      std::list<RNodeRef>::const_iterator nn=n;

      nn++;

      RouteNodeRef node=routeNodeMap.find((*n)->nodeOffset)->second;

      //
      // The path from the last routing node to the target node and the
      // target node itself
      //
      if (nn==nodes.end()) {
        if (targetObject.GetType()==refArea) {
          OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find(targetObject.GetFileOffset());

          assert(entry!=areaMap.end());

          ids=&entry->second->rings.front().ids;
          oneway=false;
        }
        else if (targetObject.GetType()==refWay) {
          OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find(targetObject.GetFileOffset());

          assert(entry!=wayMap.end());

          ids=&entry->second->ids;
          oneway=!profile.CanUseBackward(entry->second);
        }
        else {
          assert(false);
        }

        size_t currentNodeIndex=0;

        while ((*ids)[currentNodeIndex]!=node->GetId() &&
            currentNodeIndex<ids->size()) {
          currentNodeIndex++;
        }
        assert(currentNodeIndex<ids->size());

        if (currentNodeIndex!=targetNodeIndex) {
          AddNodes(route,
                   (*ids)[currentNodeIndex],
                   currentNodeIndex,
                   targetObject,
                   ids->size(),
                   oneway,
                   targetNodeIndex);
        }

        route.AddEntry(0,
                       targetNodeIndex,
                       ObjectFileRef(),
                       0);

        break;
      }

      RouteNodeRef nextNode=routeNodeMap.find((*nn)->nodeOffset)->second;

      if ((*nn)->object.GetType()==refArea) {
        OSMSCOUT_HASHMAP<FileOffset,AreaRef>::const_iterator entry=areaMap.find((*nn)->object.GetFileOffset());

        assert(entry!=areaMap.end());

        ids=&entry->second->rings.front().ids;
        oneway=false;
      }
      else if ((*nn)->object.GetType()==refWay) {
        OSMSCOUT_HASHMAP<FileOffset,WayRef>::const_iterator entry=wayMap.find((*nn)->object.GetFileOffset());

        assert(entry!=wayMap.end());

        ids=&entry->second->ids;
        oneway=!profile.CanUseBackward(entry->second);
      }
      else {
        assert(false);
      }

      size_t currentNodeIndex=0;

      while ((*ids)[currentNodeIndex]!=node->id &&
          currentNodeIndex<ids->size()) {
        currentNodeIndex++;
      }
      assert(currentNodeIndex<ids->size());

      size_t nextNodeIndex=0;

      while ((*ids)[nextNodeIndex]!=nextNode->id &&
          nextNodeIndex<ids->size()) {
        nextNodeIndex++;
      }
      assert(nextNodeIndex<ids->size());

      AddNodes(route,
               (*ids)[currentNodeIndex],
               currentNodeIndex,
               (*nn)->object,
               ids->size(),
               oneway,
               nextNodeIndex);
    }

    return true;
  }

  bool Router::ResolveRouteDataJunctions(RouteData& route)
  {
    std::set<Id> nodeIds;

    for (std::list<RouteData::RouteEntry>::const_iterator routeEntry=route.Entries().begin();
         routeEntry!=route.Entries().end();
         ++routeEntry) {
      if (routeEntry->GetCurrentNodeId()!=0) {
        nodeIds.insert(routeEntry->GetCurrentNodeId());
      }
    }

    if (!junctionDataFile.Open(path,
                               FileScanner::FastRandom,
                               false,
                               FileScanner::FastRandom,
                               false)) {
      return false;
    }

    std::vector<JunctionRef> junctions;

    if (!junctionDataFile.Get(nodeIds,
                              junctions)) {
      std::cerr << "Error while resolving junction ids to junctions" << std::endl;
    }

    nodeIds.clear();

    OSMSCOUT_HASHMAP<Id,JunctionRef> junctionMap;

    for (std::vector<JunctionRef>::const_iterator j=junctions.begin();
        j!=junctions.end();
        ++j) {
      JunctionRef junction(*j);

      junctionMap.insert(std::make_pair(junction->GetId(),junction));
    }

    junctions.clear();

    for (std::list<RouteData::RouteEntry>::iterator routeEntry=route.Entries().begin();
         routeEntry!=route.Entries().end();
         ++routeEntry) {
      if (routeEntry->GetCurrentNodeId()!=0) {
        OSMSCOUT_HASHMAP<Id,JunctionRef>::const_iterator junction=junctionMap.find(routeEntry->GetCurrentNodeId());
        if (junction!=junctionMap.end()) {
          routeEntry->SetObjects(junction->second->GetObjects());
        }
      }
    }

    return junctionDataFile.Close();
  }

  bool Router::GetStartNodes(const RoutingProfile& profile,
                             const ObjectFileRef& object,
                             size_t nodeIndex,
                             double& targetLon,
                             double& targetLat,
                             RouteNodeRef& forwardRouteNode,
                             RouteNodeRef& backwardRouteNode,
                             RNodeRef& forwardRNode,
                             RNodeRef& backwardRNode)
  {
    if (object.GetType()==refArea) {
      // TODO:
      return false;
    }
    else if (object.GetType()==refWay) {
      WayRef        way;
      double        startLon=0.0L;
      double        startLat=0.0L;
      size_t        forwardNodePos;
      FileOffset    forwardOffset;
      size_t        backwardNodePos;
      FileOffset    backwardOffset;

      if (!wayDataFile.GetByOffset(object.GetFileOffset(),
                                   way)) {
        std::cerr << "Cannot get start way!" << std::endl;
        return false;
      }

      if (nodeIndex>=way->nodes.size()) {
        std::cerr << "Given start node index " << nodeIndex << " is not within valid range [0," << way->nodes.size()-1 << std::endl;
        return false;
      }

      startLon=way->nodes[nodeIndex].GetLon();
      startLat=way->nodes[nodeIndex].GetLat();

      GetClosestForwardRouteNode(way,
                                 nodeIndex,
                                 forwardRouteNode,
                                 forwardNodePos);

      GetClosestBackwardRouteNode(way,
                                  nodeIndex,
                                  backwardRouteNode,
                                  backwardNodePos);

      if (forwardRouteNode.Invalid() &&
          backwardRouteNode.Invalid()) {
        std::cerr << "No route node found for start way" << std::endl;
        return false;
      }

      if (forwardRouteNode.Valid()) {
        if (!routeNodeDataFile.GetOffset(forwardRouteNode->id,
                                         forwardOffset)) {
          std::cerr << "Cannot get offset of startForwardRouteNode" << std::endl;
        }

        RNodeRef node=new RNode(forwardOffset,
                                object);

        node->currentCost=profile.GetCosts(way,
                                           GetSphericalDistance(startLon,
                                                                startLat,
                                                                way->nodes[forwardNodePos].GetLon(),
                                                                way->nodes[forwardNodePos].GetLat()));
        node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                                 startLat,
                                                                 targetLon,
                                                                 targetLat));

        node->overallCost=node->currentCost+node->estimateCost;

        forwardRNode=node;
      }

      if (backwardRouteNode.Valid()) {
        if (!routeNodeDataFile.GetOffset(backwardRouteNode->id,
                                         backwardOffset)) {
          std::cerr << "Cannot get offset of startBackwardRouteNode" << std::endl;
        }

        RNodeRef node=new RNode(backwardOffset,
                                object);

        node->currentCost=profile.GetCosts(way,
                                           GetSphericalDistance(startLon,
                                                                startLat,
                                                                way->nodes[backwardNodePos].GetLon(),
                                                                way->nodes[backwardNodePos].GetLat()));
        node->estimateCost=profile.GetCosts(GetSphericalDistance(startLon,
                                                                 startLat,
                                                                 targetLon,
                                                                 targetLat));

        node->overallCost=node->currentCost+node->estimateCost;

        backwardRNode=node;
      }

      return true;
    }
    else {
      return false;
    }
  }

  bool Router::GetTargetNodes(const ObjectFileRef& object,
                              size_t nodeIndex,
                              double& targetLon,
                              double& targetLat,
                              RouteNodeRef& forwardNode,
                              RouteNodeRef& backwardNode)
  {
    if (object.GetType()==refArea) {
      // TODO:
      return false;
    }
    else if (object.GetType()==refWay) {
      WayRef way;
      size_t forwardNodePos;
      size_t backwardNodePos;

      if (!wayDataFile.GetByOffset(object.GetFileOffset(),
                                   way)) {
        std::cerr << "Cannot get end way!" << std::endl;
        return false;
      }

      if (nodeIndex>=way->nodes.size()) {
        std::cerr << "Given target node index " << nodeIndex << " is not within valid range [0," << way->nodes.size()-1 << std::endl;
        return false;
      }

      targetLon=way->nodes[nodeIndex].GetLon();
      targetLat=way->nodes[nodeIndex].GetLat();

      GetClosestForwardRouteNode(way,
                                 nodeIndex,
                                 forwardNode,
                                 forwardNodePos);
      GetClosestBackwardRouteNode(way,
                                  nodeIndex,
                                  backwardNode,
                                  backwardNodePos);

      if (forwardNode.Invalid() &&
          backwardNode.Invalid()) {
        std::cerr << "No route node found for target way" << std::endl;
        return false;
      }

      if (forwardNode.Valid()) {
        FileOffset forwardRouteNodeOffset;

        if (!routeNodeDataFile.GetOffset(forwardNode->id,
                                         forwardRouteNodeOffset)) {
          std::cerr << "Cannot get offset of targetForwardRouteNode" << std::endl;
        }
      }

      if (backwardNode.Valid()) {
        FileOffset backwardRouteNodeOffset;

        if (!routeNodeDataFile.GetOffset(backwardNode->id,
                                         backwardRouteNodeOffset)) {
          std::cerr << "Cannot get offset of targetBackwardRouteNode" << std::endl;
        }
      }

      return true;
    }
    else {
      return false;
    }
  }


  bool Router::CalculateRoute(const RoutingProfile& profile,
                              const ObjectFileRef& startObject,
                              size_t startNodeIndex,
                              const ObjectFileRef& targetObject,
                              size_t targetNodeIndex,
                              RouteData& route)
  {
    RouteNodeRef             startForwardRouteNode;
    RouteNodeRef             startBackwardRouteNode;
    RNodeRef                 startForwardNode;
    RNodeRef                 startBackwardNode;

    double                   targetLon=0.0L,targetLat=0.0L;

    RouteNodeRef             targetForwardRouteNode;
    RouteNodeRef             targetBackwardRouteNode;

    // Sorted list (smallest cost first) of ways to check (we are using a std::set)
    OpenList                 openList;
    // Map routing nodes by id
    OpenMap                  openMap;
    CloseMap                 closeMap;

    size_t                   nodesLoadedCount=0;
    size_t                   nodesIgnoredCount=0;
    size_t                   maxOpenList=0;
    size_t                   maxCloseMap=0;

    route.Clear();

#if defined(OSMSCOUT_HASHMAP_HAS_RESERVE)
    openMap.reserve(10000);
    closeMap.reserve(300000);
#endif

    if (!GetTargetNodes(targetObject,
                        targetNodeIndex,
                        targetLon,
                        targetLat,
                        targetForwardRouteNode,
                        targetBackwardRouteNode)) {
      return false;
    }

    if (!GetStartNodes(profile,
                       startObject,
                       startNodeIndex,
                       targetLon,
                       targetLat,
                       startForwardRouteNode,
                       startBackwardRouteNode,
                       startForwardNode,
                       startBackwardNode)) {
      return false;
    }

    if (startForwardNode.Valid()) {
      std::pair<OpenListRef,bool> result=openList.insert(startForwardNode);
      openMap[startForwardNode->nodeOffset]=result.first;
    }

    if (startBackwardNode.Valid()) {
      std::pair<OpenListRef,bool> result=openList.insert(startBackwardNode);
      openMap[startBackwardNode->nodeOffset]=result.first;
    }

    StopClock    clock;
    RNodeRef     current;
    RouteNodeRef currentRouteNode;

    do {
      //
      // Take entry from open list with lowest cost
      //

      current=*openList.begin();

      openMap.erase(current->nodeOffset);
      openList.erase(openList.begin());

      if (!routeNodeDataFile.GetByOffset(current->nodeOffset,
                                         currentRouteNode)) {
        std::cerr << "Cannot load route node with id " << current->nodeOffset << std::endl;
        return false;
      }

      nodesLoadedCount++;

      // Get potential follower in the current way

#if defined(DEBUG_ROUTING)
      std::cout << "Analysing follower of node " << currentRouteNode->GetFileOffset();
      std::cout << " (" << current->object.GetTypeName() << " " << current->object.GetFileOffset() << "["  << currentRouteNode->GetId() << "]" << ")";
      std::cout << " " << current->currentCost << " " << current->estimateCost << " " << current->overallCost << std::endl;
#endif
      size_t i=0;
      for (std::vector<osmscout::RouteNode::Path>::const_iterator path=currentRouteNode->paths.begin();
           path!=currentRouteNode->paths.end();
           ++path,
           ++i) {
        if (path->offset==current->prev) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path->offset;
          std::cout << " (" << currentRouteNode->objects[path->objectIndex].GetTypeName() << " " << currentRouteNode->objects[path->objectIndex].GetFileOffset() << ")";
          std::cout << " => back to the last node visited" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!current->access &&
            path->HasAccess()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path->offset;
          std::cout << " (" << currentRouteNode->objects[path->objectIndex].GetTypeName() << " " << currentRouteNode->objects[path->objectIndex].GetFileOffset() << ")";
          std::cout << " => moving from non-accessible way back to accessible way" << std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (!profile.CanUse(*currentRouteNode,i)) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path->offset;
          std::cout << " (" << currentRouteNode->objects[path->objectIndex].GetTypeName() << " " << currentRouteNode->objects[path->objectIndex].GetFileOffset() << ")";
          std::cout << " => Cannot be used"<< std::endl;
#endif
          nodesIgnoredCount++;
          continue;
        }

        if (closeMap.find(path->offset)!=closeMap.end()) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path->offset;
          std::cout << " (" << currentRouteNode->objects[path->objectIndex].GetTypeName() << " " << currentRouteNode->objects[path->objectIndex].GetFileOffset() << ")";
          std::cout << " => already calculated" << std::endl;
#endif
          continue;
        }

        if (!currentRouteNode->excludes.empty()) {
          bool canTurnedInto=true;
          for (size_t e=0; e<currentRouteNode->excludes.size(); e++) {
            if (currentRouteNode->excludes[e].source==current->object &&
                currentRouteNode->excludes[e].targetIndex==i) {
#if defined(DEBUG_ROUTING)
              std::cout << "  Skipping route";
              std::cout << " to " << path->offset;
              std::cout << " (" << currentRouteNode->objects[path->objectIndex].GetTypeName() << " " << currentRouteNode->objects[path->objectIndex].GetFileOffset() << ")";
              std::cout << " => turn not allowed" << std::endl;
#endif
              canTurnedInto=false;
              break;
            }
          }

          if (!canTurnedInto) {
            nodesIgnoredCount++;
            continue;
          }
        }

        double currentCost=current->currentCost+
                           profile.GetCosts(*currentRouteNode,i);

        OpenMap::iterator openEntry=openMap.find(path->offset);

        // Check, if we already have a cheaper path to the new node. If yes, do not put the new path
        // into the open list
        if (openEntry!=openMap.end() &&
            (*openEntry->second)->currentCost<=currentCost) {
#if defined(DEBUG_ROUTING)
          std::cout << "  Skipping route";
          std::cout << " to " << path->offset;
          std::cout << " (" << currentRouteNode->objects[path->objectIndex].GetTypeName() << " " << currentRouteNode->objects[path->objectIndex].GetFileOffset() << ")";
          std::cout << "  => cheaper route exists " << currentCost << "<=>" << (*openEntry->second)->currentCost << std::endl;
#endif
          continue;
        }

        double distanceToTarget=GetSphericalDistance(path->lon,
                                                     path->lat,
                                                     targetLon,
                                                     targetLat);
        // Estimate costs for the rest of the distance to the target
        double estimateCost=profile.GetCosts(distanceToTarget);
        double overallCost=currentCost+estimateCost;

        // If we already have the node in the open list, but the new path is cheaper,
        // update the existing entry
        if (openEntry!=openMap.end()) {
          RNodeRef node=*openEntry->second;

          node->prev=current->nodeOffset;
          node->object=currentRouteNode->objects[path->objectIndex];

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=currentRouteNode->paths[i].HasAccess();

#if defined(DEBUG_ROUTING)
          std::cout << "  Updating route " << current->nodeOffset << " via " << node->object.GetTypeName() << " " << node->object.GetFileOffset() << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->id << std::endl;
#endif

          openList.erase(openEntry->second);

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openEntry->second=result.first;
        }
        else {
          RNodeRef node=new RNode(path->offset,
                                  currentRouteNode->objects[path->objectIndex],
                                  current->nodeOffset);

          node->currentCost=currentCost;
          node->estimateCost=estimateCost;
          node->overallCost=overallCost;
          node->access=path->HasAccess();

#if defined(DEBUG_ROUTING)
          std::cout << "  Inserting route to " << path->offset;
          std::cout <<  " (" << node->object.GetTypeName() << " " << node->object.GetFileOffset() << ")";
          std::cout << " " << currentCost << " " << estimateCost << " " << overallCost << " " << currentRouteNode->id << std::endl;
#endif

          std::pair<OpenListRef,bool> result=openList.insert(node);
          openMap[node->nodeOffset]=result.first;
        }
      }

      //
      // Added current node to close map
      //

      closeMap[current->nodeOffset]=current;

      maxOpenList=std::max(maxOpenList,openMap.size());
      maxCloseMap=std::max(maxCloseMap,closeMap.size());

#if defined(DEBUG_ROUTING)
      if (openList.empty()) {
        std::cout << "No more alternatives, stopping" << std::endl;
      }

      if ((targetForwardRouteNode.Valid() && current->nodeOffset==targetForwardRouteNode->fileOffset)) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetForwardRouteNode->fileOffset << " (forward)" << std::endl;
      }

      if (targetBackwardRouteNode.Valid() && current->nodeOffset==targetBackwardRouteNode->fileOffset) {
        std::cout << "Reached target: " << current->nodeOffset << " == " << targetBackwardRouteNode->fileOffset << " (backward)" << std::endl;
      }
#endif
    } while (!openList.empty() &&
             (targetForwardRouteNode.Invalid() || current->nodeOffset!=targetForwardRouteNode->fileOffset) &&
             (targetBackwardRouteNode.Invalid() || current->nodeOffset!=targetBackwardRouteNode->fileOffset));

    clock.Stop();

    if (debugPerformance) {
      std::cout << "From:                " << startObject.GetTypeName() << " " << startObject.GetFileOffset();
      std::cout << "[";
      if (startForwardRouteNode.Valid()) {
        std::cout << startForwardRouteNode->GetId() << " - ";
      }
      std::cout << startNodeIndex;
      if (startBackwardRouteNode.Valid()) {
        std::cout << " - " << startBackwardRouteNode->GetId();
      }
      std::cout << "]" << std::endl;

      std::cout << "To:                  " << targetObject.GetTypeName() <<  " " << targetObject.GetFileOffset();
      std::cout << "[";
      if (targetForwardRouteNode.Valid()) {
        std::cout << targetForwardRouteNode->GetId() << " - ";
      }
      std::cout << targetNodeIndex;
      if (targetBackwardRouteNode.Valid()) {
        std::cout << " - " << targetBackwardRouteNode->GetId();
      }
      std::cout << "]" << std::endl;

      std::cout << "Time:                " << clock << std::endl;

      std::cout << "Route nodes loaded:  " << nodesLoadedCount << std::endl;
      std::cout << "Route nodes ignored: " << nodesIgnoredCount << std::endl;
      std::cout << "Max. OpenList size:  " << maxOpenList << std::endl;
      std::cout << "Max. CloseMap size:  " << maxCloseMap << std::endl;
    }

    if (!((targetForwardRouteNode.Valid() && currentRouteNode->GetId()==targetForwardRouteNode->id) ||
          (targetBackwardRouteNode.Valid() && currentRouteNode->GetId()==targetBackwardRouteNode->id))) {
      std::cout << "No route found!" << std::endl;
      route.Clear();

      return true;
    }

    std::list<RNodeRef> nodes;

    ResolveRNodeChainToList(current,
                            closeMap,
                            nodes);

    if (!ResolveRNodesToRouteData(profile,
                                  nodes,
                                  startObject,
                                  startNodeIndex,
                                  targetObject,
                                  targetNodeIndex,
                                  route)) {
      //std::cerr << "Cannot convert routing result to route data" << std::endl;
      return false;
    }

    ResolveRouteDataJunctions(route);

    return true;
  }

  bool Router::TransformRouteDataToWay(const RouteData& data,
                                       Way& way)
  {
    TypeId routeType;
    Way    tmp;

    routeType=typeConfig->GetWayTypeId("_route");

    assert(routeType!=typeIgnore);

    way=tmp;

    way.SetType(routeType);
    way.SetLayerToMax();
    way.nodes.reserve(data.Entries().size());

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathObject().Valid()) {
        if (iter->GetPathObject().GetType()==refArea) {
          AreaRef a;

          if (!areaDataFile.GetByOffset(iter->GetPathObject().GetFileOffset(),a)) {
            return false;
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            way.ids.push_back(a->rings.front().ids[index]);
            way.nodes.push_back(a->rings.front().nodes[index]);
          }

          size_t index=iter->GetTargetNodeIndex();

          way.ids.push_back(a->rings.front().ids[index]);
          way.nodes.push_back(a->rings.front().nodes[index]);
        }
        else if (iter->GetPathObject().GetType()==refWay) {
          WayRef w;

          if (!wayDataFile.GetByOffset(iter->GetPathObject().GetFileOffset(),w)) {
            return false;
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            way.ids.push_back(w->ids[index]);
            way.nodes.push_back(w->nodes[index]);
          }

          size_t index=iter->GetTargetNodeIndex();

          way.ids.push_back(w->ids[index]);
          way.nodes.push_back(w->nodes[index]);
        }
      }
    }

    return true;
  }

  bool Router::TransformRouteDataToPoints(const RouteData& data,
                                          std::list<Point>& points)
  {
    AreaRef a;
    WayRef  w;

    points.clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      if (iter->GetPathObject().Valid()) {
        if (iter->GetPathObject().GetType()==refArea) {
          if (a.Invalid() ||
              a->GetFileOffset()!=iter->GetPathObject().GetFileOffset()) {
            if (!areaDataFile.GetByOffset(iter->GetPathObject().GetFileOffset(),a)) {
              std::cerr << "Cannot load area with id " << iter->GetPathObject().GetFileOffset() << std::endl;
              return false;
            }
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            points.push_back(Point(a->rings.front().ids[index],
                                   a->rings.front().nodes[index].GetLat(),
                                   a->rings.front().nodes[index].GetLon()));
          }

          // target node of current path
          size_t index=iter->GetTargetNodeIndex();

          points.push_back(Point(a->rings.front().ids[index],
                                 a->rings.front().nodes[index].GetLat(),
                                 a->rings.front().nodes[index].GetLon()));
        }
        else if (iter->GetPathObject().GetType()==refWay) {
          if (w.Invalid() ||
              w->GetFileOffset()!=iter->GetPathObject().GetFileOffset()) {
            if (!wayDataFile.GetByOffset(iter->GetPathObject().GetFileOffset(),w)) {
              std::cerr << "Cannot load way with id " << iter->GetPathObject().GetFileOffset() << std::endl;
              return false;
            }
          }

          // Initial starting point
          if (iter==data.Entries().begin()) {
            size_t index=iter->GetCurrentNodeIndex();

            points.push_back(Point(w->ids[index],
                                   w->nodes[index].GetLat(),
                                   w->nodes[index].GetLon()));
          }

          // target node of current path
          size_t index=iter->GetTargetNodeIndex();

          points.push_back(Point(w->ids[index],
                                 w->nodes[index].GetLat(),
                                 w->nodes[index].GetLon()));
        }
      }
    }

    return true;
  }

  bool Router::TransformRouteDataToRouteDescription(const RouteData& data,
                                                    RouteDescription& description)
  {
    description.Clear();

    if (data.Entries().empty()) {
      return true;
    }

    for (std::list<RouteData::RouteEntry>::const_iterator iter=data.Entries().begin();
         iter!=data.Entries().end();
         ++iter) {
      description.AddNode(iter->GetCurrentNodeIndex(),
                          iter->GetObjects(),
                          iter->GetPathObject(),
                          iter->GetTargetNodeIndex());
    }

    return true;
  }

  void Router::DumpStatistics()
  {
    wayDataFile.DumpStatistics();
    routeNodeDataFile.DumpStatistics();
  }
}

