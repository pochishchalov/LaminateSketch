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

    Node& AddNode(Node&& node) {
        return data_.emplace_back(std::move(node));
    }
    Node& AddNode(const Node& node) {
        return data_.emplace_back(node);
    }

    Node& InsertNode(size_t index, Node&& node) {
        return *data_.emplace(data_.begin() + index, std::move(node));
    }

    Node& GetNode(size_t index) {
        return data_.at(index);
    }
    const Node& GetNode(size_t index) const {
        return data_.at(index);
    }
    Node& GetFirstNode() {
        return *data_.begin();
    }
    const Node& GetFirstNode() const {
        return *data_.begin();
    }
    Node& GetLastNode() {
        return data_.back();
    }
    const Node& GetLastNode() const {
        return data_.back();
    }

    size_t PointsCount() const noexcept { return data_.size(); }

    void OffsetPly(double offset) {
        domain::Polyline poly;
        poly.reserve(PointsCount());
        for (const auto& node : data_) {
            poly.emplace_back(domain::Point{ node.point });
        }
        poly = domain::OffsetPolyline(poly, offset);
        for (size_t i = 0; i < poly.size(); ++i) {
            data_[i].point = poly[i];
        }
    }

};

struct Layer : VectorWrapper<Ply> {
    Ply& AddPly() {
        return data_.emplace_back(Ply{});
    }

    Ply& GetPly(size_t index) {
        return data_.at(index);
    }

    const Ply& GetPly(size_t index) const {
        return data_.at(index);
    }

    size_t PliesCount() const noexcept { return data_.size(); }
};

class Data {
public:

    using Iterator = std::vector<Layer>::iterator;
    using ConstIterator = std::vector<Layer>::const_iterator;

    Iterator begin() noexcept { return layers_.begin(); }
    Iterator end() noexcept { return layers_.end(); }
    ConstIterator begin() const noexcept { return layers_.begin(); }
    ConstIterator end() const noexcept { return layers_.end(); }

    Node& GetNode(const NodePosition pos) {
        return layers_[pos.layerPos][pos.plyPos][pos.nodePos];
    }

    const Node& GetNode(const NodePosition pos) const {
        return layers_[pos.layerPos][pos.plyPos][pos.nodePos];
    }

    NodePosition GetLastNodePos() {
        unsigned short layer_pos = static_cast<unsigned short>(layers_.size() - 1);
        unsigned short ply_pos = static_cast<unsigned short>(layers_[layer_pos].size() - 1);
        unsigned short node_pos = static_cast<unsigned short>(layers_[layer_pos][ply_pos].size() - 1);

        return NodePosition{ .layerPos = layer_pos,
            .plyPos = ply_pos,
            .nodePos = node_pos
        };
    }

    std::vector<Layer>& getData() { return layers_; }

    void ReserveLayers(size_t size) { layers_.reserve(size); }
    size_t LayersCount() const noexcept { return layers_.size(); }
    bool isEmpty() const { return layers_.empty(); }

    Layer& AddLayer() {
        return layers_.emplace_back(Layer{});
    }

    Layer& GetLayer(size_t index) {
        return layers_.at(index);
    }
    const Layer& GetLayer(size_t index) const {
        return layers_.at(index);
    }

    Node& InsertNode(const NodePosition pos, Node&& node) {
        Ply& ply = GetLayer(pos.layerPos).GetPly(pos.plyPos);
        Node& new_node = ply.InsertNode(pos.nodePos, std::move(node));
        new_node.position = pos;

        for (size_t i = pos.nodePos + 1; i < ply.PointsCount(); ++i) {
            Node& node = ply.GetNode(i);
            ++node.position.nodePos;
            if (node.lowerLink.has_value()) {
                ++GetNode(node.lowerLink.value()).upperLink.value().nodePos;
            }
            if (node.upperLink.has_value()) {
                ++GetNode(node.upperLink.value()).lowerLink.value().nodePos;
            }
        }
        return new_node;
    }

    NodePosition GetStartNodePos() const {
        assert(!layers_.empty() && "Data is empty");
        Node result = GetNode(NodePosition{ .layerPos = 0, .plyPos = 0, .nodePos = 0 });
        Node node = result;
        while (node.upperLink.has_value()) {
            node = GetNode(node.upperLink.value());
            if (node.position.nodePos != 0) {
                node = GetLayer(node.position.layerPos).GetPly(node.position.plyPos).GetFirstNode();
                result = node;
            }
        }
        return result.position;
    }

    NodePosition GetLowestNodePos(const NodePosition pos) const {
        Node node = GetNode(pos);

        while (node.lowerLink.has_value()) {
            node = GetNode(node.lowerLink.value());
        }

        return node.position;
    }

    NodePosition GetTopMostNodePos(const NodePosition pos) const {
        Node node = GetNode(pos);

        while (node.upperLink.has_value()) {
            node = GetNode(node.upperLink.value());
        }

        return node.position;
    }

    bool IsLastNodeInPly(const NodePosition pos) const {
        size_t size = GetLayer(pos.layerPos).GetPly(pos.plyPos).PointsCount() - 1;
        return static_cast<unsigned short>(size)
               == pos.nodePos;
    }

    bool IsFirstNodeInPly(const NodePosition pos) const {
        return static_cast <unsigned short>(0) == pos.nodePos;
    }

private:
    std::vector<Layer> layers_;
};

} // namespace ls
