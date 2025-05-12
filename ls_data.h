#pragma once

#include <cassert>
#include <compare>
#include <optional>
#include <vector>

#include "common.h"

namespace ls {  // laminate sketch

struct NodePos {
    unsigned short layer_pos = 0;
    unsigned short ply_pos = 0;
    unsigned short node_pos = 0;

    auto operator<=>(const NodePos&) const = default;
};

struct Node {
    domain::Point point;
    NodePos pos;
    std::optional<NodePos> top_pos;
    std::optional<NodePos> bottom_pos; 
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
    domain::ORI ori = domain::ORI::ZERO;

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

    Node& GetNode(const NodePos pos) {
        return layers_[pos.layer_pos][pos.ply_pos][pos.node_pos];
    }

    const Node& GetNode(const NodePos pos) const {
        return layers_[pos.layer_pos][pos.ply_pos][pos.node_pos];
    }

    NodePos GetLastNodePos() {
        unsigned short layer_pos = static_cast<unsigned short>(layers_.size() - 1);
        unsigned short ply_pos = static_cast<unsigned short>(layers_[layer_pos].size() - 1);
        unsigned short node_pos = static_cast<unsigned short>(layers_[layer_pos][ply_pos].size() - 1);

        return NodePos{ .layer_pos = layer_pos,
                        .ply_pos = ply_pos,
                        .node_pos = node_pos
        };
    }

    std::vector<Layer>& GetData() { return layers_; }

    void ReserveLayers(size_t size) { layers_.reserve(size); }
    size_t LayersCount() const noexcept { return layers_.size(); }
    bool IsEmpty() const { return layers_.empty(); }

    Layer& AddLayer() {
        return layers_.emplace_back(Layer{});
    }

    Layer& GetLayer(size_t index) {
        return layers_.at(index);
    }
    const Layer& GetLayer(size_t index) const {
        return layers_.at(index);
    }

    Node& InsertNode(const NodePos pos, Node&& node) {
        Ply& ply = GetLayer(pos.layer_pos).GetPly(pos.ply_pos);
        Node& new_node = ply.InsertNode(pos.node_pos, std::move(node));
        new_node.pos = pos;
        
        for (size_t i = pos.node_pos + 1; i < ply.PointsCount(); ++i) {
            Node& node = ply.GetNode(i);
            ++node.pos.node_pos;
            if (node.bottom_pos.has_value()) {
                ++GetNode(node.bottom_pos.value()).top_pos.value().node_pos;
            }
            if (node.top_pos.has_value()) {
                ++GetNode(node.top_pos.value()).bottom_pos.value().node_pos;
            }
        }
        return new_node;
    }

    NodePos GetStartNodePos() const {
        assert(!layers_.empty() && "Data is empty");
        Node result = GetNode(NodePos{ .layer_pos = 0, .ply_pos = 0, .node_pos = 0 });
        Node node = result;
        while (node.top_pos.has_value()) {
            node = GetNode(node.top_pos.value());
            if (node.pos.node_pos != 0) {
                node = GetLayer(node.pos.layer_pos).GetPly(node.pos.ply_pos).GetFirstNode();
                result = node;
            }
        }
        return result.pos;
    }

    NodePos GetLowestNodePos(const NodePos pos) const {
        Node node = GetNode(pos);

        while (node.bottom_pos.has_value()) {
            node = GetNode(node.bottom_pos.value());
        }

        return node.pos;
    }

    NodePos GetTopMostNodePos(const NodePos pos) const {
        Node node = GetNode(pos);

        while (node.top_pos.has_value()) {
            node = GetNode(node.top_pos.value());
        }

        return node.pos;
    }

    bool IsLastNodeInPly(const NodePos pos) const {
        size_t size = GetLayer(pos.layer_pos).GetPly(pos.ply_pos).PointsCount() - 1;
        return static_cast<unsigned short>(size)
            == pos.node_pos;
    }

    bool IsFirstNodeInPly(const NodePos pos) const {
        return static_cast <unsigned short>(0) == pos.node_pos;
    }

private:
    std::vector<Layer> layers_;
};

} // namespace ls
