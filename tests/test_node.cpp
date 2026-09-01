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
