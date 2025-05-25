#pragma once

#include <cassert>
#include <compare>
#include <optional>
#include <vector>

#include "common.h"

namespace ls {  // laminate sketch

struct NodePosition {
    unsigned short layerPos = 0;
    unsigned short plyPos = 0;
    unsigned short nodePos = 0;

    auto operator<=>(const NodePosition&) const = default;
};

struct Node {
    domain::Point point;
    NodePosition position;
    std::optional<NodePosition> upperLink;
    std::optional<NodePosition> lowerLink;
};

template <typename T>
class VectorWrapper {
public:
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    size_t size() const noexcept { return data_.size(); }

    auto begin() noexcept { return data_.begin(); }
    auto end() noexcept { return data_.end(); }

    auto begin() const noexcept { return data_.begin(); }
    auto end() const noexcept { return data_.end(); }
    auto cbegin() const noexcept { return data_.cbegin(); }
    auto cend() const noexcept { return data_.cend(); }

    T& operator[](size_t index) {
        assert(index < data_.size() && "Index out of range");
        return data_[index];
    }

    const T& operator[](size_t index) const {
        assert(index < data_.size() && "Index out of range");
        return data_[index];
    }

    void reserve(size_t size) { data_.reserve(size); }

protected:
    std::vector<T> data_;
};

struct Ply : VectorWrapper<Node> {
    domain::Orientation orientation = domain::Orientation::Zero;

    template <typename NodeT>
    Node& addNode(NodeT&& node) {
        return data_.emplace_back(std::forward<NodeT>(node));
    }

    Node& insertNode(size_t index, Node&& node) {
        return *data_.emplace(data_.begin() + index, std::move(node));
    }

    Node& getNode(size_t index) {
        return data_.at(index);
    }
    const Node& getNode(size_t index) const {
        return data_.at(index);
    }
    Node& firstNode() {
        return data_.front();
    }
    const Node& firstNode() const {
        return data_.front();
    }
    Node& lastNode() {
        return data_.back();
    }
    const Node& lastNode() const {
        return data_.back();
    }

    size_t pointsCount() const noexcept { return data_.size(); }
};

struct Layer : VectorWrapper<Ply> {
    Ply& addPly() {
        return data_.emplace_back(Ply{});
    }

    Ply& getPly(size_t index) {
        return data_.at(index);
    }

    const Ply& getPly(size_t index) const {
        return data_.at(index);
    }

    size_t pliesCount() const noexcept { return data_.size(); }
};

class LaminateData {
    void updatePositionsAfterInsertion(NodePosition pos) {
        auto& ply = getLayer(pos.layerPos).getPly(pos.plyPos);

        for (size_t i = pos.nodePos + 1; i < ply.pointsCount(); ++i) {
            Node& current = ply.getNode(i);
            ++current.position.nodePos;
            if (current.lowerLink.has_value()) {
                ++getNode(current.lowerLink.value()).upperLink.value().nodePos;
            }
            if (current.upperLink.has_value()) {
                ++getNode(current.upperLink.value()).lowerLink.value().nodePos;
            }
        }
    }
public:

    using Iterator = std::vector<Layer>::iterator;
    using ConstIterator = std::vector<Layer>::const_iterator;

    Iterator begin() noexcept { return layers_.begin(); }
    Iterator end() noexcept { return layers_.end(); }
    ConstIterator begin() const noexcept { return layers_.begin(); }
    ConstIterator end() const noexcept { return layers_.end(); }

    Node& getNode(const NodePosition pos) {
        return layers_[pos.layerPos][pos.plyPos][pos.nodePos];
    }

    const Node& getNode(const NodePosition pos) const {
        return layers_[pos.layerPos][pos.plyPos][pos.nodePos];
    }

    NodePosition lastNodePos() {
        unsigned short layer_pos = static_cast<unsigned short>(layers_.size() - 1);
        unsigned short ply_pos = static_cast<unsigned short>(layers_[layer_pos].size() - 1);
        unsigned short node_pos = static_cast<unsigned short>(layers_[layer_pos][ply_pos].size() - 1);

        return NodePosition{ .layerPos = layer_pos,
            .plyPos = ply_pos,
            .nodePos = node_pos
        };
    }

    std::vector<Layer>& getData() { return layers_; }

    void reserveLayers(size_t size) { layers_.reserve(size); }
    size_t layersCount() const noexcept { return layers_.size(); }
    bool isEmpty() const { return layers_.empty(); }

    Layer& addLayer() {
        return layers_.emplace_back(Layer{});
    }

    Layer& getLayer(size_t index) {
        return layers_.at(index);
    }
    const Layer& getLayer(size_t index) const {
        return layers_.at(index);
    }

    Node& insertNode(const NodePosition pos, Node&& node) {
        Ply& ply = getLayer(pos.layerPos).getPly(pos.plyPos);

        Node& newNode = ply.insertNode(pos.nodePos, std::move(node));
        newNode.position = pos;

        updatePositionsAfterInsertion(pos);

        return newNode;
    }

    NodePosition findRootNode() const {
        assert(!layers_.empty() && "Data is empty");
        Node result = getNode(NodePosition{ .layerPos = 0, .plyPos = 0, .nodePos = 0 });
        Node node = result;
        while (node.upperLink.has_value()) {
            node = getNode(node.upperLink.value());
            if (node.position.nodePos != 0) {
                node = getLayer(node.position.layerPos).getPly(node.position.plyPos).firstNode();
                result = node;
            }
        }
        return result.position;
    }

    NodePosition traceToBottom(const NodePosition start) const {
        NodePosition current = start;

        while (const auto& link = getNode(current).lowerLink) {
            current = *link;
        }
        return current;
    }

    NodePosition traceToTop(const NodePosition start) const {
        NodePosition current = start;

        while (const auto& link = getNode(current).upperLink) {
            current = *link;
        }
        return current;
    }

    bool isLastPlyNode(const NodePosition pos) const {
        size_t size = getLayer(pos.layerPos).getPly(pos.plyPos).pointsCount() - 1;
        return static_cast<unsigned short>(size)
               == pos.nodePos;
    }

    bool isFirstPlyNode(const NodePosition pos) const {
        return static_cast <unsigned short>(0) == pos.nodePos;
    }

    void clear(){
        layers_.clear();
    }

private:
    std::vector<Layer> layers_;
};

} // namespace ls
