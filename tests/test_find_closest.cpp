#include "gtest/gtest.h"
#include "Quadtree.h"
#include "quadtree_test.hpp"
#include <cstddef>

struct Box : public quadtree::Box<float> {
    Box (float l, float t) :
        quadtree::Box<float>(l, t, 10.0f, 10.0f)
    {
    }
};

struct TestItem
{
    TestItem (float left, float top, std::size_t idd) :
        box(left, top),
        id(idd)
    {
    }

    Box box;
    std::size_t id;
};

TEST_F(QuadtreeTest, Distance)
{
    using quadtree::distance;

    Box a{10.0f, 10.0f};
    ASSERT_FLOAT_EQ(distance(a, a), 0.0f);

    {
        Box b{15.0f, 15.0f};
        ASSERT_FLOAT_EQ(distance(a, b), 0.0f);
        ASSERT_FLOAT_EQ(distance(b, a), 0.0f);
    }

    {
        Box b{40.0f, 15.0f};
        ASSERT_FLOAT_EQ(distance(a, b), 20.0f);
    }

    {
        Box b{30.0f, 30.0f};
        ASSERT_FLOAT_EQ(distance(a, b), 14.14213562373f);
    }

    {
        Box b{0.0f, 0.0f};
        ASSERT_FLOAT_EQ(distance(a, b), 0.0f);
    }

    {
        Box b{8.0f, 55.0f};
        ASSERT_FLOAT_EQ(distance(a, b), 35.0f);
    }
}

TEST_F(QuadtreeTest, FindClosest)
{
    auto getBox = [](const TestItem& node) { return node.box; };
    quadtree::Quadtree<TestItem, decltype(getBox)> qtree({0.0f, 0.0f, 1000.0f, 1000.0f}, getBox);
    qtree.add({10.0f, 10.0f, 1});
    qtree.add({30.0f,  0.0f, 2});
    qtree.add({50.0f, 10.0f, 3});
    qtree.add({60.0f, 30.0f, 4});
    qtree.add({50.0f, 50.0f, 5});
    qtree.add({30.0f, 60.0f, 6});
    qtree.add({10.0f, 50.0f, 7});
    qtree.add({ 0.0f, 30.0f, 8});

    qtree.add({110.0f, 10.0f, 9});
    qtree.add({130.0f, 10.0f, 10});
    qtree.add({150.0f, 10.0f, 11});
    qtree.add({150.0f, 30.0f, 12});
    qtree.add({150.0f, 50.0f, 13});
    qtree.add({130.0f, 50.0f, 14});
    qtree.add({110.0f, 50.0f, 15});
    qtree.add({110.0f, 30.0f, 16});

    qtree.add({210.0f, 10.0f, 17});
    qtree.add({230.0f, 10.0f, 18});
    qtree.add({250.0f, 10.0f, 19});
    qtree.add({250.0f, 30.0f, 20});
    qtree.add({250.0f, 50.0f, 21});
    qtree.add({230.0f, 50.0f, 22});
    qtree.add({210.0f, 50.0f, 23});
    qtree.add({210.0f, 30.0f, 24});

    {
        Box searchBox{25.0f, 25.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 1);
    }

    {
        Box searchBox{29.0f, 11.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 2);
    }

    {
        Box searchBox{39.0f, 21.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 3);
    }

    {
        Box searchBox{35.0f, 25.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 3);
    }

    {
        Box searchBox{48.0f, 30.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 4);
    }

    {
        Box searchBox{39.0f, 39.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 5);
    }

    {
        Box searchBox{33.0f, 49.5f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 6);
    }

    {
        Box searchBox{22.0f, 38.5f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 7);
    }

    {
        Box searchBox{11.0f, 30.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 8);
    }

    {
        Box searchBox{5.0f, 5.0f};
        const TestItem* found = qtree.findClosest(searchBox);
        ASSERT_NE(found, nullptr);
        ASSERT_EQ(found->id, 1);
    }
}
