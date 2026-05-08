#include "2d/ZCLabel.h"

#include "base/ZCDirector.h"
#include "base/ZCRenderDevice.h"
#include "base/ZCRenderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <new>
#include <vector>

namespace zocos {

namespace {

constexpr int kGlyphWidth = 5;
constexpr int kGlyphHeight = 7;
constexpr int kGlyphAdvance = 6;

using Glyph = std::array<unsigned char, kGlyphHeight>;

const Glyph kGlyphSpace = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const Glyph kGlyphQuestion = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};

const Glyph kGlyph0 = {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
const Glyph kGlyph1 = {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
const Glyph kGlyph2 = {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
const Glyph kGlyph3 = {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
const Glyph kGlyph4 = {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
const Glyph kGlyph5 = {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E};
const Glyph kGlyph6 = {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E};
const Glyph kGlyph7 = {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
const Glyph kGlyph8 = {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
const Glyph kGlyph9 = {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x1C};

const Glyph kGlyphF = {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
const Glyph kGlyphP = {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
const Glyph kGlyphS = {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
const Glyph kGlyphColon = {0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00};
const Glyph kGlyphDot = {0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06};
const Glyph kGlyphMinus = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};

const Glyph& pickGlyph(char c) {
    if (c >= '0' && c <= '9') {
        static const Glyph digits[] = {kGlyph0, kGlyph1, kGlyph2, kGlyph3, kGlyph4,
            kGlyph5, kGlyph6, kGlyph7, kGlyph8, kGlyph9};
        return digits[c - '0'];
    }

    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) {
    case 'F':
        return kGlyphF;
    case 'P':
        return kGlyphP;
    case 'S':
        return kGlyphS;
    case ':':
        return kGlyphColon;
    case '.':
        return kGlyphDot;
    case '-':
        return kGlyphMinus;
    case ' ':
        return kGlyphSpace;
    default:
        return kGlyphQuestion;
    }
}

void destroyTexture(Director& director, TextureHandle& inOutTexture) {
    auto* device = director.getRenderDevice();
    if (!device || !inOutTexture.isValid()) {
        return;
    }
    device->destroyTexture(inOutTexture);
    inOutTexture = {};
}

} // namespace

Label::Label(Director& director) : _director(director) {
}

Label* Label::create(Director& director, const std::string& text) {
    auto* label = new (std::nothrow) Label(director);
    if (!label) {
        return nullptr;
    }
    if (!label->init()) {
        delete label;
        return nullptr;
    }
    label->setString(text);
    return static_cast<Label*>(label->autorelease());
}

Label::~Label() {
    destroyTexture(_director, _texture);
}

bool Label::init() {
    if (!Node::init()) {
        return false;
    }
    setContentSize({1.f, static_cast<float>(kGlyphHeight)});
    return true;
}

void Label::setString(const std::string& text) {
    if (_text == text) {
        return;
    }
    _text = text;
    _dirty = true;
}

bool Label::rebuildTexture() {
    auto* device = _director.getRenderDevice();
    if (!device) {
        _ready = false;
        return false;
    }

    const std::string view = _text.empty() ? std::string(" ") : _text;
    const int width = std::max(1, static_cast<int>(view.size()) * kGlyphAdvance - 1);
    const int height = kGlyphHeight;
    std::vector<unsigned char> pixels(static_cast<size_t>(width * height * 4), 0);

    for (int i = 0; i < static_cast<int>(view.size()); ++i) {
        const Glyph& glyph = pickGlyph(view[static_cast<size_t>(i)]);
        const int xOffset = i * kGlyphAdvance;
        for (int y = 0; y < kGlyphHeight; ++y) {
            const unsigned char bits = glyph[static_cast<size_t>(y)];
            for (int x = 0; x < kGlyphWidth; ++x) {
                const bool on = ((bits >> (kGlyphWidth - 1 - x)) & 0x1U) != 0;
                if (!on) {
                    continue;
                }
                const int px = xOffset + x;
                if (px < 0 || px >= width) {
                    continue;
                }
                const size_t idx = static_cast<size_t>((y * width + px) * 4);
                pixels[idx + 0] = 255;
                pixels[idx + 1] = 255;
                pixels[idx + 2] = 255;
                pixels[idx + 3] = 255;
            }
        }
    }

    destroyTexture(_director, _texture);
    TextureCreateInfo createInfo;
    createInfo.width = width;
    createInfo.height = height;
    createInfo.format = TextureFormat::RGBA8Unorm;
    createInfo.initialData.pixels = pixels.data();
    createInfo.initialData.rowPitchBytes = width * 4;
    createInfo.initialData.origin = TextureDataOrigin::TopLeft;
    _texture = device->createTexture(createInfo);
    _ready = _texture.isValid();
    _dirty = false;
    setContentSize({static_cast<float>(width), static_cast<float>(height)});
    return _ready;
}

void Label::draw(Renderer& renderer, const Mat4& world) {
    if (_dirty && !rebuildTexture()) {
        return;
    }
    if (!_ready || !_texture.isValid()) {
        return;
    }

    const RenderSortKey sortKey = makeRenderSortKey(0, 0, _texture.value);
    renderer.addDrawSprite(world, _contentSize, _texture, sortKey);
}

} // namespace zocos