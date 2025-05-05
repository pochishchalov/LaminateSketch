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

    NodePos GetPrevPos() const {
        return NodePos{ .layer_pos = layer_pos,
                       .ply_pos = ply_pos,
                       .node_pos = static_cast<unsigned short>(node_pos - 1) };
    }

    NodePos GetNextPos() const {
        return NodePos{ .layer_pos = layer_pos,
                       .ply_pos = ply_pos,
                       .node_pos = static_cast<unsigned short>(node_pos + 1) };
    }
};

struct Node {
    domain::Point point_;
    std::optional<NodePos> top_pos_;
    std::optional<NodePos> bottom_pos_;
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

struct Ply : public VectorWrapper<Node> {
    domain::ORI ori = domain::ORI::ZERO;

    // Добавляем методы для работы с нодами
    Node& AddNode(Node&& node) {
        return data_.emplace_back(std::move(node));
    }
};

struct Layer : public VectorWrapper<Ply> {
    // Добавляем методы для работы со слоями
    Ply& AddPly() {
        return data_.emplace_back(Ply{});
    }
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

    // Управление памятью
    void ReserveLayers(size_t size) { layers_.reserve(size); }
    size_t LayersCount() const noexcept { return layers_.size(); }
    bool IsEmpty() const { return layers_.empty(); }

    // Создание новых слоев
    Layer& AddLayer() {
        return layers_.emplace_back(Layer{});
    }

    Layer& GetLayer(size_t idx) {
        return layers_.at(idx);
    }

    bool IsLastNodeInPly(const NodePos pos) {
        return static_cast<unsigned short>(layers_[pos.layer_pos][pos.ply_pos].size() - 1)
            == pos.node_pos;
    }

    bool IsFirstNodeInPly(const NodePos pos) {
        return static_cast <unsigned short>(0) == pos.node_pos;
    }

    void ReverseLayers() { std::reverse(layers_.begin(), layers_.end()); }

private:
    std::vector<Layer> layers_;
};

} // namespace ls
