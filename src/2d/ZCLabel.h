#pragma once

#include "base/ZCRenderCommand.h"
#include "2d/ZCNode.h"

#include <string>

namespace zocos {

class Director;
class Renderer;

class Label : public Node {
public:
    static Label* create(Director& director, const std::string& text = "");

    ~Label() override;

    bool init() override;

    void setString(const std::string& text);
    const std::string& getString() const { return _text; }

    void draw(Renderer& renderer, const Mat4& world) override;

protected:
    explicit Label(Director& director);

private:
    bool rebuildTexture();

    Director& _director;
    std::string _text;
    TextureHandle _texture{};
    bool _ready = false;
    bool _dirty = true;
};

} // namespace zocos