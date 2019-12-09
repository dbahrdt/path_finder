//
// Created by sokol on 03.10.19.
//

#include <chrono>
#include <future>
#include "../include/HubLabels.h"
#include "../include/CHDijkstra.h"
#include "algorithm"


pathFinder::HubLabels::HubLabels(pathFinder::CHGraph &graph, Level level) : graph(graph), labelsUntilLevel(level) {
    cost.reserve(graph.getNodes().size());
    while(cost.size() < graph.getNodes().size())
        cost.push_back(MAX_DISTANCE);
    hubLabels.reserve(graph.getNodes().size());
    backHubLabels.reserve(graph.getNodes().size());

    graph.sortByLevel(sortedNodes);

    // calculate node ranges with same level
    std::vector<std::pair<uint32_t, uint32_t >> sameLevelRanges;
    auto maxLevel = sortedNodes.begin()->level;
    auto currentLevel = sortedNodes.begin()->level;
    for(auto j = 0; j < sortedNodes.size(); ++j) {
        std::cout << currentLevel << std::endl;
        auto i = j;
        while(sortedNodes[j].level == currentLevel && j < sortedNodes.size()){
            ++j;
        }
        sameLevelRanges.emplace_back(i, j+1);
        currentLevel--;
    }

    //initialize vector for labels for each node
    while (hubLabels.size() < graph.getNodes().size()) {
        hubLabels.emplace_back(costNodeVec_t());
        backHubLabels.emplace_back(costNodeVec_t());
    }
    auto start = std::chrono::steady_clock::now();
    Print("constructing labels...");
    constructAllLabels(sameLevelRanges, maxLevel, labelsUntilLevel);

    // constrauct forward and backward labels
    auto end = std::chrono::steady_clock::now();
    std::cout << "Constructed Labels in  "
              << std::chrono::duration_cast<std::chrono::seconds>(end - start).count()
              << " s" << std::endl;
}

void pathFinder::HubLabels::constructAllLabels(const std::vector<std::pair<uint32_t, uint32_t >>& sameLevelRanges, int maxLevel, int minLevel) {
    int currentLevel = maxLevel;
    for(const auto& sameLevelRange : sameLevelRanges) {
        std:: cout << "constructing level: " << currentLevel << std::endl;
        processRange(sameLevelRange, EdgeDirection::FORWARD);
        processRange(sameLevelRange, EdgeDirection::BACKWARD);
        if(--currentLevel < minLevel)
            break;
    }
}

pathFinder::costNodeVec_t& pathFinder::HubLabels::getLabels(pathFinder::NodeId nodeId, pathFinder::EdgeDirection direction) {
    auto node = graph.getNodes()[nodeId];

    switch (direction) {
        case EdgeDirection::FORWARD:
            return hubLabels[nodeId];
        case EdgeDirection::BACKWARD:
            return backHubLabels[nodeId];
    }
    return hubLabels[nodeId];
}

std::optional<pathFinder::Distance> pathFinder::HubLabels::getShortestDistance(pathFinder::NodeId source, pathFinder::NodeId target) {
    if(source >= graph.getNodes().size() || target >= graph.getNodes().size())
        return std::nullopt;
    auto forwardLabels = calcLabel(source, EdgeDirection::FORWARD);
    auto backwardLabels = calcLabel(target, EdgeDirection::BACKWARD);
    NodeId topNode;
    return getShortestDistance(forwardLabels, backwardLabels, topNode);
}

std::optional<pathFinder::Distance> pathFinder::HubLabels::getShortestDistancePrep(pathFinder::NodeId source, pathFinder::NodeId target) {
    if(source >= graph.getNodes().size() || target >= graph.getNodes().size())
        return std::nullopt;
    auto forwardLabels = getLabels(source, EdgeDirection::FORWARD);
    auto backwardLabels = getLabels(target, EdgeDirection::BACKWARD);
    NodeId topNode;
    return getShortestDistance(forwardLabels, backwardLabels, topNode);
}

std::optional<pathFinder::Distance>
pathFinder::HubLabels::getShortestDistance(costNodeVec_t &forwardLabels, costNodeVec_t &backwardLabels, NodeId& nodeId) {
    Distance shortestDistance = MAX_DISTANCE;
    nodeId = forwardLabels[0].id;
    for(int i = 0, j = 0; i < forwardLabels.size()&& j < backwardLabels.size();) {
        auto& forwardCostNode = forwardLabels[i];
        auto& backwardCostNode = backwardLabels[j];
        if (forwardCostNode.id == backwardCostNode.id ) {
            if(forwardCostNode.cost + backwardCostNode.cost < shortestDistance)
                    shortestDistance = forwardCostNode.cost + backwardCostNode.cost;
            ++j;
            ++i;
            nodeId = forwardCostNode.id;
        }
        else if(forwardCostNode.id < backwardCostNode.id)
            ++i;
        else
            ++j;
    }
    return shortestDistance;
}

void pathFinder::HubLabels::setLabel(pathFinder::NodeId nodeId, pathFinder::EdgeDirection direction) {
    Level level = graph.getLevel(nodeId);
    auto& label = getLabels(nodeId, direction);
    for(const auto& edge: graph.edgesFor(nodeId, direction)) {
        if(level < graph.getLevel(edge.target)) {
            const auto& targetLabel = getLabels(edge.target, direction);
            mergeLabels(label, targetLabel, edge.distance);
        }
    }
    label.emplace_back(nodeId, 0);
    sortLabel(label);
    selfPrune(nodeId, direction);
    sortLabel(label);
}

void pathFinder::HubLabels::sortLabel(costNodeVec_t &label) {
    std::sort(label.begin(), label.end(), [](const CostNode& node1, const CostNode& node2) {
        return node1.id == node2.id ? node1.cost < node2.cost : node1.id < node2.id;
    });
}

void pathFinder::HubLabels::processRange(std::pair<uint32_t, uint32_t> range, EdgeDirection direction){
    for(auto i = range.first; i < range.second; ++i) {
        setLabel(sortedNodes[i].id, direction);
    }
}

void pathFinder::HubLabels::selfPrune(pathFinder::NodeId labelId, pathFinder::EdgeDirection direction) {
    auto& labels = getLabels(labelId, direction);
    bool forward = (direction == EdgeDirection::FORWARD);
    for(int i = labels.size()-1; i > 0; --i) {
        auto& [id, cost] = labels[i];
        auto& otherLabels = getLabels(id, (EdgeDirection) !direction);
        auto d = forward ? getShortestDistancePrep(labelId, id) : getShortestDistancePrep(id, labelId);
        if(d < cost){
            labels[i] = labels[labels.size()-1];
            labels.pop_back();
        }
    }
}

std::vector<pathFinder::LatLng> pathFinder::HubLabels::getShortestPath(pathFinder::NodeId source, pathFinder::NodeId target) {
    return std::vector<LatLng>();
}

pathFinder::costNodeVec_t pathFinder::HubLabels::calcLabel(NodeId source, EdgeDirection direction){
    auto& sourceLabel = getLabels(source, direction);
    if(!sourceLabel.empty())
        return sourceLabel;
    std::vector<CostNode> settledNodes;
    std::vector<CostNode> labelsToCollect;
    for(auto nodeId: visited)
        cost[nodeId] = MAX_DISTANCE;
    visited.clear();
    std::priority_queue<CostNode> q;
    q.emplace(source, 0);
    cost[source] = 0;
    visited.emplace_back(source);
    while(!q.empty()) {
        auto costNode = q.top();
        q.pop();
        if(costNode.cost > cost[costNode.id])
            continue;
        settledNodes.emplace_back(costNode.id, costNode.cost);
        auto currentNode = graph.getNodes()[costNode.id];
        if(currentNode.level >= labelsUntilLevel){
            labelsToCollect.emplace_back(costNode.id, costNode.cost);
            continue;
        }
        for(const auto& edge: graph.edgesFor(costNode.id, direction)) {
            if(graph.getLevel(edge.source) > graph.getLevel(edge.target))
                continue;
            auto addedCost = costNode.cost + edge.distance;
            if(addedCost < cost[edge.target]) {
                visited.emplace_back(edge.target);
                cost[edge.target] = addedCost;
                q.emplace(edge.target, addedCost);
            }
        }
    }
    for(auto [id, m_cost] : labelsToCollect) {
        mergeLabels(settledNodes, getLabels(id, direction), cost[id]);
    }
    sortLabel(settledNodes);
    std::cout << source << " [";
    auto first = true;
    for(const auto& [id, cost] : settledNodes){
        if(!first)
            std::cout << ",";
        first = false;
        std::cout << "{" <<  id << "," << cost << "}";
    }
    std::cout << "]\n";

    return settledNodes;
}

void pathFinder::HubLabels::mergeLabels(std::vector<CostNode>& label1, const std::vector<CostNode>& label2, Distance distanceToLabel) {
    // TODO
    // fix merge
    for(const auto [idTarget, distanceTarget] : label2) {
        bool found = false;
        const auto addedDistance = distanceTarget + distanceToLabel;
        for(auto&& [id, distance] : label1) {
            if(id == idTarget) {
                found = true;
                if(addedDistance < distance)
                    distance = addedDistance;
                break;
            }
        }
        if(!found) {
            label1.emplace_back(idTarget, addedDistance);
        }
    }
}

void pathFinder::HubLabels::setMinLevel(pathFinder::Level level) {
    this->labelsUntilLevel = level;
}

void pathFinder::HubLabels::writeToFile(boost::filesystem::path filePath){
    boost::filesystem::ofstream ofs{filePath};
    for(int i = 0; i < hubLabels.size(); ++i) {
        ofs << i << " [";
        bool first = true;
        for(auto [id, cost] : hubLabels[i]){
            if(!first)
                ofs << ",";
            ofs << "{" << id << "," << cost << "}";
            first = false;
        }
        ofs << "]\n";
    }
}
