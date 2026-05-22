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
    for (auto* child : _children) {
        if (child) {
            child->onEnter();
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
    stopAllActions();
    unscheduleAllCallbacks();
    _running = false;
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

void Node::stopAllActions() {
    Director::getInstance().getActionManager().removeAllActionsFromTarget(this);
}

void Node::sortAllChildren() {
    if (!_reorderChildDirty) {
        return;
    }

    mstd::stable_sort(_children.begin(), _children.end(), [](const Node* a, const Node* b) {
        return a->_orderOfArrival < b->_orderOfArrival;
    });

    _reorderChildDirty = false;
}

void Node::addChild(Node* child) {
    // Keep child alive while it is detached from the old parent.
    child->retain();
    if (child->_parent) {
        child->_parent->removeChild(child);
    }

    child->_parent = this;
    child->_orderOfArrival = sGlobalOrderOfArrival++;
    child->retain();
    _children.push_back(child);
    child->release();
    _reorderChildDirty = true;

    if (_running) {
        child->onEnter();
    }
}

void Node::removeChild(Node* child) {
    for (auto it = _children.begin(); it != _children.end(); ++it) {
        if (*it == child) {
            if (_running && (*it)->isRunning()) {
                (*it)->onExit();
            }
            (*it)->_parent = nullptr;
            (*it)->release();
            _children.erase(it);
            return;
        }
    }
}

void Node::removeAllChildren() {
    for (auto* child : _children) {
        if (_running && child->isRunning()) {
            child->onExit();
        }
        child->_parent = nullptr;
        child->release();
    }
    _children.clear();
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
    if (!_visible)
        return;

    sortAllChildren();

    const Mat4 world = parentWorld * localMatrix();
    draw(renderer, world);
    for (auto* ch : _children) {
        if (ch)
            ch->visit(renderer, world);
    }
}

} // namespace zocos
