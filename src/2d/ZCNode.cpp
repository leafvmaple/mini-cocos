#include "2d/ZCNode.h"

#include "base/ZCDirector.h"

#include <algorithm>
#include <new>
#include <utility>

namespace zocos {

Node* Node::create() {
    auto* node = new (std::nothrow) Node();
    if (node && node->init()) {
        node->autorelease();
        return node;
    }
    delete node;
    return nullptr;
}

Node::~Node() {
    Director::getInstance().getEventDispatcher().removeListenersForTarget(this);
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
    unscheduleAllCallbacks();
    _running = false;
}

void Node::schedule(const std::string& key, ScheduleCallback callback, float interval, int repeat,
                    float delay, int priority) {
    Director::getInstance().getScheduler().schedule(this, key, std::move(callback), interval, repeat, delay,
                                                    priority);
}

void Node::scheduleOnce(const std::string& key, ScheduleCallback callback, float delay, int priority) {
    Director::getInstance().getScheduler().scheduleOnce(this, key, std::move(callback), delay, priority);
}

void Node::unschedule(const std::string& key) {
    Director::getInstance().getScheduler().unschedule(this, key);
}

void Node::unscheduleAllCallbacks() {
    Director::getInstance().getScheduler().unscheduleAllForTarget(this);
}

void Node::addChild(Node* child) {
    if (!child || child == this || child->_parent == this) {
        return;
    }

    // Keep child alive if it is being re-parented.
    child->retain();
    if (child->_parent) {
        child->_parent->removeChild(child);
    }

    child->_parent = this;
    child->retain();
    _children.push_back(child);
    child->release();

    if (_running) {
        child->onEnter();
    }
}

void Node::removeChild(Node* child) {
    if (!child) {
        return;
    }
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
        if (!child) {
            continue;
        }
        if (_running && child->isRunning()) {
            child->onExit();
        }
        child->_parent = nullptr;
        child->release();
    }
    _children.clear();
}

void Node::updateTree(float dt) {
    if (!_running) {
        return;
    }

    if (!_paused) {
        update(dt);
    }

    for (auto* ch : _children) {
        if (ch) ch->updateTree(dt);
    }
}

void Node::visit(const Mat4& parentWorld) {
    if (!_visible) return;
    const Mat4 world = parentWorld * localMatrix();
    draw(world);
    for (auto* ch : _children) {
        if (ch) ch->visit(world);
    }
}

} // namespace zocos
