#include "2d/ZCNode.h"

#include "base/ZCActionManager.h"
#include "base/ZCDirector.h"
#include "base/ZCRenderer.h"

#include "base/ZCStd.h"
#include <cassert>

namespace zocos {

namespace {
mstd::size_t sGlobalOrderOfArrival = 1;
}

Node* Node::create() {
    auto* node = new (mstd::nothrow) Node();
    if (node && node->init()) {
        node->autorelease();
        return node;
    }
    delete node;
    return nullptr;
}

Node::~Node() {
    Director::getInstance().getEventDispatcher().removeEventListenersForTarget(this);
    stopAllActions();
    unscheduleAllCallbacks();
    removeAllChildren();
}

void Node::onEnter() {
    if (_running) {
        return;
    }
    _running = true;
    _transitionFinished = false;
    for (auto* child : _children) {
        if (child) {
            child->onEnter();
        }
    }
}

void Node::onEnterTransitionDidFinish() {
    if (!_running) {
        return;
    }
    _transitionFinished = true;
    for (auto* child : _children) {
        if (child) {
            child->onEnterTransitionDidFinish();
        }
    }
}

void Node::onExitTransitionDidStart() {
    if (!_running) {
        return;
    }
    _transitionFinished = false;
    for (auto* child : _children) {
        if (child) {
            child->onExitTransitionDidStart();
        }
    }
}

void Node::onExit() {
    if (!_running) {
        return;
    }
    for (auto* child : _children) {
        if (child) {
            child->onExit();
        }
    }
    _running = false;
    _transitionFinished = false;
}

void Node::cleanup() {
    stopAllActions();
    unscheduleAllCallbacks();
    for (auto* child : _children) {
        if (child) {
            child->cleanup();
        }
    }
}

void Node::schedule(const mstd::string& key, ScheduleCallback callback, float interval, int repeat,
                    float delay, int priority) {
    Director::getInstance().getScheduler().schedule(this, key, mstd::move(callback), interval,
                                                    repeat, delay, priority);
}

void Node::scheduleOnce(const mstd::string& key, ScheduleCallback callback, float delay,
                        int priority) {
    Director::getInstance().getScheduler().scheduleOnce(this, key, mstd::move(callback), delay,
                                                        priority);
}

void Node::unschedule(const mstd::string& key) {
    Director::getInstance().getScheduler().unschedule(this, key);
}

void Node::unscheduleAllCallbacks() {
    Director::getInstance().getScheduler().unscheduleAllForTarget(this);
}

Action* Node::runAction(Action* action) {
    Director::getInstance().getActionManager().addAction(action, this);
    return action;
}

void Node::stopAction(Action* action) {
    Director::getInstance().getActionManager().removeAction(action);
}

void Node::stopActionByTag(int tag) {
    Director::getInstance().getActionManager().removeActionByTag(tag, this);
}

void Node::stopAllActionsByTag(int tag) {
    Director::getInstance().getActionManager().removeAllActionsByTag(tag, this);
}

void Node::stopAllActions() {
    Director::getInstance().getActionManager().removeAllActionsFromTarget(this);
}

Action* Node::getActionByTag(int tag) const {
    return Director::getInstance().getActionManager().getActionByTag(tag, this);
}

mstd::size_t Node::getNumberOfRunningActions() const {
    return Director::getInstance().getActionManager().getNumberOfRunningActionsInTarget(this);
}

mstd::size_t Node::getNumberOfRunningActionsByTag(int tag) const {
    return Director::getInstance().getActionManager().getNumberOfRunningActionsInTargetByTag(this,
                                                                                             tag);
}

void Node::setLocalZOrder(int localZOrder) {
    if (_localZOrder == localZOrder) {
        return;
    }

    if (_parent) {
        _parent->reorderChild(this, localZOrder);
    } else {
        _localZOrder = localZOrder;
    }
}

void Node::sortAllChildren() {
    if (!_reorderChildDirty) {
        return;
    }

    mstd::stable_sort(_children.begin(), _children.end(), [](const Node* a, const Node* b) {
        if (a->_localZOrder != b->_localZOrder) {
            return a->_localZOrder < b->_localZOrder;
        }
        return a->_orderOfArrival < b->_orderOfArrival;
    });

    _reorderChildDirty = false;
}

void Node::addChild(Node* child) {
    if (!child) {
        return;
    }
    addChild(child, child->_localZOrder);
}

void Node::addChild(Node* child, int localZOrder) {
    if (!child || child == this) {
        return;
    }

    // Keep child alive while it is detached from the old parent.
    child->retain();
    if (child->_parent) {
        child->_parent->removeChild(child, false);
    }

    child->_parent = this;
    child->_localZOrder = localZOrder;
    child->_orderOfArrival = sGlobalOrderOfArrival++;
    child->retain();
    _children.push_back(child);
    child->release();
    _reorderChildDirty = true;

    if (_running) {
        child->onEnter();
        if (_transitionFinished) {
            child->onEnterTransitionDidFinish();
        }
    }
}

void Node::addChild(Node* child, int localZOrder, int tag) {
    if (!child || child == this) {
        return;
    }
    child->_tag = tag;
    addChild(child, localZOrder);
}

void Node::reorderChild(Node* child, int localZOrder) {
    assert(child && "Child must not be null.");
    assert(child && child->_parent == this && "Child must belong to this node.");
    if (!child || child->_parent != this) {
        return;
    }

    child->_localZOrder = localZOrder;
    child->_orderOfArrival = sGlobalOrderOfArrival++;
    _reorderChildDirty = true;
}

Node* Node::getChildByTag(int tag) const {
    for (auto* child : _children) {
        if (child->_tag == tag) {
            return child;
        }
    }
    return nullptr;
}

void Node::removeChild(Node* child, bool cleanup) {
    for (auto it = _children.begin(); it != _children.end(); ++it) {
        if (*it == child) {
            if (_running && (*it)->isRunning()) {
                (*it)->onExitTransitionDidStart();
                (*it)->onExit();
            }
            if (cleanup) {
                (*it)->cleanup();
            }
            (*it)->_parent = nullptr;
            (*it)->release();
            _children.erase(it);
            return;
        }
    }
}

void Node::removeChildByTag(int tag, bool cleanup) {
    if (Node* child = getChildByTag(tag)) {
        removeChild(child, cleanup);
    }
}

void Node::removeAllChildren() { removeAllChildrenWithCleanup(true); }

void Node::removeAllChildrenWithCleanup(bool cleanup) {
    for (auto* child : _children) {
        if (_running && child->isRunning()) {
            child->onExitTransitionDidStart();
            child->onExit();
        }
        if (cleanup) {
            child->cleanup();
        }
        child->_parent = nullptr;
        child->release();
    }
    _children.clear();
}

void Node::removeFromParent() { removeFromParentAndCleanup(true); }

void Node::removeFromParentAndCleanup(bool cleanup) {
    if (_parent) {
        _parent->removeChild(this, cleanup);
    }
}

Mat4 Node::getNodeToWorldTransform() const {
    Mat4 transform = getNodeToParentTransform();
    for (const Node* parent = _parent; parent; parent = parent->_parent) {
        transform = parent->getNodeToParentTransform() * transform;
    }
    return transform;
}

Vec2 Node::convertToWorldSpace(const Vec2& nodePoint) const {
    return getNodeToWorldTransform().transformPoint(nodePoint);
}

bool Node::convertToNodeSpace(const Vec2& worldPoint, Vec2& nodePoint) const {
    Mat4 worldToNode;
    if (!getNodeToWorldTransform().getAffineInverse2D(worldToNode)) {
        return false;
    }
    nodePoint = worldToNode.transformPoint(worldPoint);
    return true;
}

void Node::updateTree(float dt) {
    if (!_paused) {
        update(dt);
    }

    for (auto* ch : _children) {
        if (ch)
            ch->updateTree(dt);
    }
}

void Node::visit(Renderer& renderer, const Mat4& parentWorld) {
    if (!_visible) {
        return;
    }

    sortAllChildren();

    const Mat4 world = parentWorld * getNodeToParentTransform();
    mstd::size_t childIndex = 0;
    for (; childIndex < _children.size(); ++childIndex) {
        Node* child = _children[childIndex];
        if (child->_localZOrder >= 0) {
            break;
        }
        child->visit(renderer, world);
    }

    draw(renderer, world);

    for (; childIndex < _children.size(); ++childIndex) {
        _children[childIndex]->visit(renderer, world);
    }
}

} // namespace zocos
