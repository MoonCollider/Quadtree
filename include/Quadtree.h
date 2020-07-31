#pragma once

#include <cassert>
#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>
#include "Box.h"

namespace quadtree
{

namespace detail
{
    template <typename T>
    struct StdMakeUnique
    {
        template <typename... Args>
        std::unique_ptr<T> operator() (Args&&... args)
        {
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
    };
}

template<
    typename T,
    typename GetBox,
    typename Equal = std::equal_to<T>,
    typename Float = float,
    template <typename> class Allocator = std::allocator,
    template <typename> class MakeUnique = detail::StdMakeUnique
>
class Quadtree
{
#if __cplusplus < 201703L
    static_assert(std::is_convertible<typename std::result_of<GetBox(const T&)>::type, Box<Float>>::value,
#else
    static_assert(std::is_convertible_v<std::invoke_result_t<GetBox, const T&>, Box<Float>>,
#endif
        "GetBox must be a callable of signature Box<Float>(const T&)");
#if __cplusplus < 201703L
    static_assert(std::is_convertible<typename std::result_of<Equal(const T&, const T&)>::type, bool>::value,
#else
    static_assert(std::is_convertible_v<std::invoke_result_t<Equal, const T&, const T&>, bool>,
#endif
        "Equal must be a callable of signature bool(const T&, const T&)");
    static_assert(std::is_arithmetic<Float>::value, "is_arithmetic<Float> is false");

public:
    template <typename U>
    using vector_type = std::vector< U, Allocator<U> >;

    Quadtree(const Box<Float>& box, const GetBox& getBox = GetBox(),
        const Equal& equal = Equal()) :
        mBox(box), mMakeUnique(), mRoot(mMakeUnique()), mGetBox(getBox), mEqual(equal)
    {

    }

    void add(const T& value)
    {
        add(mRoot.get(), 0, mBox, value);
    }

    void remove(const T& value)
    {
        remove(mRoot.get(), nullptr, mBox, value);
    }

    vector_type<T> query(const Box<Float>& box) const
    {
        auto values = vector_type<T>();
        query(mRoot.get(), mBox, box, values);
        return values;
    }

    vector_type<std::pair<T, T>> findAllIntersections() const
    {
        auto intersections = vector_type<std::pair<T, T>>();
        findAllIntersections(mRoot.get(), intersections);
        return intersections;
    }

    template <typename P>
    const T* findClosest (const Box<Float>& box, P&& predicate) const
    {
        return findClosestImpl(
            box,
            {nullptr, std::abs(mBox.width) + std::abs(mBox.height)},
            *mRoot,
            mBox,
            std::forward<P>(predicate)
        ).first;
    }

    const T* findClosest (const Box<Float>& box) const
    {
        return findClosest(box, [](const T&, const Box<Float>&) {return true;});
    }

private:
    static constexpr auto Threshold = std::size_t(16);
    static constexpr auto MaxDepth = std::size_t(8);

    struct Node;
#if __cplusplus < 201703L
    typedef typename std::result_of<MakeUnique<Node>()>::type UniqueNodePtr;
#else
    typedef std::invoke_result_t<MakeUnique<Node>> UniqueNodePtr;
#endif

    struct Node
    {
        std::array<UniqueNodePtr, 4> children;
        vector_type<T> values;
    };

    Box<Float> mBox;
    MakeUnique<Node> mMakeUnique;
    UniqueNodePtr mRoot;
    GetBox mGetBox;
    Equal mEqual;

    bool isLeaf(const Node* node) const
    {
        return !static_cast<bool>(node->children[0]);
    }

    Box<Float> computeBox(const Box<Float>& box, int i) const
    {
        auto origin = box.getTopLeft();
        auto childSize = box.getSize() / static_cast<Float>(2);
        switch (i)
        {
            // North West
            case 0:
                return Box<Float>(origin, childSize);
            // Norst East
            case 1:
                return Box<Float>(Vector2<Float>(origin.x + childSize.x, origin.y), childSize);
            // South West
            case 2:
                return Box<Float>(Vector2<Float>(origin.x, origin.y + childSize.y), childSize);
            // South East
            case 3:
                return Box<Float>(origin + childSize, childSize);
            default:
                assert(false && "Invalid child index");
                return Box<Float>();
        }
    }

    int getQuadrant(const Box<Float>& nodeBox, const Box<Float>& valueBox) const
    {
        auto center = nodeBox.getCenter();
        // West
        if (valueBox.getRight() < center.x)
        {
            // North West
            if (valueBox.getBottom() < center.y)
                return 0;
            // South West
            else if (valueBox.top >= center.y)
                return 2;
            // Not contained in any quadrant
            else
                return -1;
        }
        // East
        else if (valueBox.left >= center.x)
        {
            // North East
            if (valueBox.getBottom() < center.y)
                return 1;
            // South East
            else if (valueBox.top >= center.y)
                return 3;
            // Not contained in any quadrant
            else
                return -1;
        }
        // Not contained in any quadrant
        else
            return -1;
    }

    void add(Node* node, std::size_t depth, const Box<Float>& box, const T& value)
    {
        assert(node != nullptr);
        assert(box.contains(mGetBox(value)));
        if (isLeaf(node))
        {
            // Insert the value in this node if possible
            if (depth >= MaxDepth || node->values.size() < Threshold)
                node->values.push_back(value);
            // Otherwise, we split and we try again
            else
            {
                split(node, box);
                add(node, depth, box, value);
            }
        }
        else
        {
            auto i = getQuadrant(box, mGetBox(value));
            // Add the value in a child if the value is entirely contained in it
            if (i != -1)
                add(node->children[static_cast<std::size_t>(i)].get(), depth + 1, computeBox(box, i), value);
            // Otherwise, we add the value in the current node
            else
                node->values.push_back(value);
        }
    }

    void split(Node* node, const Box<Float>& box)
    {
        assert(node != nullptr);
        assert(isLeaf(node) && "Only leaves can be split");
        // Create children
        for (auto& child : node->children)
            child = mMakeUnique();
        // Assign values to children
        auto newValues = vector_type<T>(); // New values for this node
        for (const auto& value : node->values)
        {
            auto i = getQuadrant(box, mGetBox(value));
            if (i != -1)
                node->children[static_cast<std::size_t>(i)]->values.push_back(value);
            else
                newValues.push_back(value);
        }
        node->values = std::move(newValues);
    }

    void remove(Node* node, Node* parent, const Box<Float>& box, const T& value)
    {
        assert(node != nullptr);
        assert(box.contains(mGetBox(value)));
        if (isLeaf(node))
        {
            // Remove the value from node
            removeValue(node, value);
            // Try to merge the parent
            if (parent != nullptr)
                tryMerge(parent);
        }
        else
        {
            // Remove the value in a child if the value is entirely contained in it
            auto i = getQuadrant(box, mGetBox(value));
            if (i != -1)
                remove(node->children[static_cast<std::size_t>(i)].get(), node, computeBox(box, i), value);
            // Otherwise, we remove the value from the current node
            else
                removeValue(node, value);
        }
    }

    void removeValue(Node* node, const T& value)
    {
        // Find the value in node->values
        auto it = std::find_if(std::begin(node->values), std::end(node->values),
            [this, &value](const auto& rhs){ return mEqual(value, rhs); });
        assert(it != std::end(node->values) && "Trying to remove a value that is not present in the node");
        // Swap with the last element and pop back
        *it = std::move(node->values.back());
        node->values.pop_back();
    }

    void tryMerge(Node* node)
    {
        assert(node != nullptr);
        assert(!isLeaf(node) && "Only interior nodes can be merged");
        auto nbValues = node->values.size();
        for (const auto& child : node->children)
        {
            if (!isLeaf(child.get()))
                return;
            nbValues += child->values.size();
        }
        if (nbValues <= Threshold)
        {
            node->values.reserve(nbValues);
            // Merge the values of all the children
            for (const auto& child : node->children)
            {
                for (const auto& value : child->values)
                    node->values.push_back(value);
            }
            // Remove the children
            for (auto& child : node->children)
                child.reset();
        }
    }

    void query(Node* node, const Box<Float>& box, const Box<Float>& queryBox, vector_type<T>& values) const
    {
        assert(node != nullptr);
        assert(queryBox.intersects(box));
        for (const auto& value : node->values)
        {
            if (queryBox.intersects(mGetBox(value)))
                values.push_back(value);
        }
        if (!isLeaf(node))
        {
            for (auto i = std::size_t(0); i < node->children.size(); ++i)
            {
                auto childBox = computeBox(box, static_cast<int>(i));
                if (queryBox.intersects(childBox))
                    query(node->children[i].get(), childBox, queryBox, values);
            }
        }
    }

    void findAllIntersections(Node* node, vector_type<std::pair<T, T>>& intersections) const
    {
        // Find intersections between values stored in this node
        // Make sure to not report the same intersection twice
        for (auto i = std::size_t(0); i < node->values.size(); ++i)
        {
            for (auto j = std::size_t(0); j < i; ++j)
            {
                if (mGetBox(node->values[i]).intersects(mGetBox(node->values[j])))
                    intersections.emplace_back(node->values[i], node->values[j]);
            }
        }
        if (!isLeaf(node))
        {
            // Values in this node can intersect values in descendants
            for (const auto& child : node->children)
            {
                for (const auto& value : node->values)
                    findIntersectionsInDescendants(child.get(), value, intersections);
            }
            // Find intersections in children
            for (const auto& child : node->children)
                findAllIntersections(child.get(), intersections);
        }
    }

    void findIntersectionsInDescendants(Node* node, const T& value, vector_type<std::pair<T, T>>& intersections) const
    {
        // Test against the values stored in this node
        for (const auto& other : node->values)
        {
            if (mGetBox(value).intersects(mGetBox(other)))
                intersections.emplace_back(value, other);
        }
        // Test against values stored into descendants of this node
        if (!isLeaf(node))
        {
            for (const auto& child : node->children)
                findIntersectionsInDescendants(child.get(), value, intersections);
        }
    }

    template <typename P>
    std::pair<const T*, Float> findClosestImpl (
        const Box<Float>& searchBox,
        std::pair<const T*, Float> best,
        const Node& node,
        const Box<Float>& nodeBox,
        P&& predicate
    ) const
    {
        const Float& bestDist = best.second;
        if (distance(searchBox, nodeBox) > bestDist)
            return best;

        for (const auto& itm : node.values)
        {
            auto currBox = mGetBox(itm);
            const Float currDist = distance(currBox, searchBox);
            if (currDist < bestDist && predicate(itm, currBox))
                best = std::make_pair(&itm, currDist);
        }

        const std::size_t rl = (searchBox.left * 2 + searchBox.width > nodeBox.left * 2 + nodeBox.width ? 1 : 0);
        const std::size_t bt = (searchBox.top * 2 + searchBox.height > nodeBox.top * 2 + nodeBox.height ? 1 : 0);
        std::array<std::size_t, 4> indices {
            bt * 2 + rl,
            bt * 2 + (1 - rl),
            (1 - bt) * 2 + rl,
            (1 - bt) * 2 + (1 - rl)
        };

        for (std::size_t i : indices)
        {
            if (node.children[i])
            {
                best = findClosestImpl(searchBox, best, *node.children[i], computeBox(nodeBox, static_cast<int>(i)), predicate);
            }
        }

        return best;
    }
};

}
