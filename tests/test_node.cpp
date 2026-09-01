#include "ZCTestFramework.h"

#include "2d/ZCNode.h"
#include "base/ZCRenderer.h"
#include "base/ZCStd.h"

using namespace zocos;

namespace {
class RecordingNode final : public Node {
public:
    RecordingNode(int id, mstd::vector<int>& drawOrder) : _id(id), _drawOrder(drawOrder) {}

protected:
    void draw(Renderer&, const Mat4&) override { _drawOrder.push_back(_id); }

private:
    int _id = 0;
    mstd::vector<int>& _drawOrder;
};
} // namespace

ZC_TEST(node_local_z_order_places_negative_children_behind_parent) {
    mstd::vector<int> drawOrder;
    Renderer renderer;
    RecordingNode parent(0, drawOrder);
    RecordingNode frontFirst(3, drawOrder);
    RecordingNode backNear(2, drawOrder);
    RecordingNode frontLast(4, drawOrder);
    RecordingNode backFar(1, drawOrder);
    RecordingNode frontSameZ(5, drawOrder);

    parent.addChild(&frontFirst, 0);
    parent.addChild(&backNear, -1);
    parent.addChild(&frontLast, 1);
    backFar.setLocalZOrder(-2);
    parent.addChild(&backFar);
    parent.addChild(&frontSameZ, 0);

    parent.visit(renderer, Mat4::identity());

    ZC_CHECK_EQ(drawOrder.size(), static_cast<mstd::size_t>(6));
    const int expected[] = {1, 2, 0, 3, 5, 4};
    for (mstd::size_t i = 0; i < drawOrder.size(); ++i) {
        ZC_CHECK_EQ(drawOrder[i], expected[i]);
    }

    parent.removeAllChildren();
}

ZC_TEST(node_reorder_updates_z_order_and_same_z_arrival_order) {
    mstd::vector<int> drawOrder;
    Renderer renderer;
    RecordingNode parent(0, drawOrder);
    RecordingNode first(1, drawOrder);
    RecordingNode second(2, drawOrder);

    parent.addChild(&first, 0);
    parent.addChild(&second, 0);
    parent.reorderChild(&first, 0);
    parent.visit(renderer, Mat4::identity());

    ZC_CHECK_EQ(drawOrder.size(), static_cast<mstd::size_t>(3));
    ZC_CHECK_EQ(drawOrder[0], 0);
    ZC_CHECK_EQ(drawOrder[1], 2);
    ZC_CHECK_EQ(drawOrder[2], 1);

    drawOrder.clear();
    first.setLocalZOrder(-1);
    ZC_CHECK_EQ(first.getLocalZOrder(), -1);
    parent.visit(renderer, Mat4::identity());

    ZC_CHECK_EQ(drawOrder.size(), static_cast<mstd::size_t>(3));
    ZC_CHECK_EQ(drawOrder[0], 1);
    ZC_CHECK_EQ(drawOrder[1], 0);
    ZC_CHECK_EQ(drawOrder[2], 2);

    parent.removeAllChildren();
}

ZC_TEST(node_converts_points_across_nested_coordinate_spaces) {
    mstd::vector<int> drawOrder;
    RecordingNode parent(0, drawOrder);
    RecordingNode child(1, drawOrder);

    parent.setAnchorPoint({0.f, 0.f});
    parent.setPosition(10.f, 20.f);
    parent.setRotation(90.f);
    child.setAnchorPoint({0.f, 0.f});
    child.setPosition(5.f, 0.f);
    child.setScale({2.f, 3.f});
    parent.addChild(&child);

    const Vec2 parentPoint = child.getNodeToParentTransform().transformPoint({1.f, 2.f});
    ZC_CHECK_NEAR(parentPoint.x, 7.f, 1e-4);
    ZC_CHECK_NEAR(parentPoint.y, 6.f, 1e-4);

    const Vec2 worldPoint = child.convertToWorldSpace({1.f, 2.f});
    ZC_CHECK_NEAR(worldPoint.x, 4.f, 1e-4);
    ZC_CHECK_NEAR(worldPoint.y, 27.f, 1e-4);

    Vec2 restored;
    ZC_CHECK(child.convertToNodeSpace(worldPoint, restored));
    ZC_CHECK_NEAR(restored.x, 1.f, 1e-4);
    ZC_CHECK_NEAR(restored.y, 2.f, 1e-4);

    child.setScale({0.f, 1.f});
    ZC_CHECK(!child.convertToNodeSpace(worldPoint, restored));

    parent.removeAllChildren();
}

ZC_TEST(node_finds_and_removes_children_by_tag) {
    mstd::vector<int> drawOrder;
    RecordingNode parent(0, drawOrder);
    RecordingNode first(1, drawOrder);
    RecordingNode second(2, drawOrder);

    parent.addChild(&first, -1, 42);
    second.setTag(7);
    parent.addChild(&second);

    ZC_CHECK_EQ(first.getTag(), 42);
    ZC_CHECK_EQ(parent.getChildByTag(42), &first);
    ZC_CHECK_EQ(parent.getChildByTag(7), &second);
    ZC_CHECK_EQ(parent.getChildByTag(99), nullptr);

    parent.removeChildByTag(42);
    ZC_CHECK_EQ(first.getParent(), nullptr);
    ZC_CHECK_EQ(parent.getChildByTag(42), nullptr);
    ZC_CHECK_EQ(parent.getChildren().size(), static_cast<mstd::size_t>(1));

    second.removeFromParent();
    ZC_CHECK_EQ(second.getParent(), nullptr);
    ZC_CHECK(parent.getChildren().empty());
}
