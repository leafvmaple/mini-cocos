local director = cc.Director:getInstance()
assert(director and director:init(960, 540, "zocos (Lua demo)"), "Director:init failed")

local defaultFont = "fonts/NotoSansSC-Regular.otf"

local scene = cc.Scene:create()
assert(scene, "Scene:create failed")

local sprite = cc.Sprite:create()
assert(sprite, "Sprite:create failed")

local imagePath = arg and arg[1]
local usingCheckerboard = false
if imagePath and imagePath ~= "" then
    if not sprite:initWithFile(imagePath) then
        print(("Falling back to checkerboard (could not load %s)"):format(imagePath))
        sprite:initWithCheckerboard()
        usingCheckerboard = true
    end
else
    sprite:initWithCheckerboard()
    usingCheckerboard = true
end

local centerX = director:getFramebufferWidth() * 0.5
local centerY = director:getFramebufferHeight() * 0.5
local orbitRadius = math.min(centerX, centerY) * 0.5
sprite:setPosition(centerX + orbitRadius, centerY)

-- Ease each leg so the sprite slows into and out of every waypoint.
sprite:runAction(cc.RepeatForever:create(cc.Sequence:create({
    cc.EaseSineInOut:create(cc.MoveTo:create(0.8, centerX, centerY + orbitRadius)),
    cc.EaseSineInOut:create(cc.MoveTo:create(0.8, centerX - orbitRadius, centerY)),
    cc.EaseSineInOut:create(cc.MoveTo:create(0.8, centerX, centerY - orbitRadius)),
    cc.EaseSineInOut:create(cc.MoveTo:create(0.8, centerX + orbitRadius, centerY)),
})))

sprite:runAction(cc.RepeatForever:create(cc.RotateBy:create(1.0, 360.0)))

if usingCheckerboard then
    local animation = cc.Animation:create({
        {0.0, 0.0, 32.0, 32.0},
        {16.0, 8.0, 32.0, 32.0},
        {24.0, 16.0, 32.0, 32.0},
        {8.0, 24.0, 32.0, 32.0},
    }, 0.1)
    if animation then
        sprite:runAction(cc.RepeatForever:create(cc.Animate:create(animation)))
    end
end

scene:addChild(sprite)

local label = cc.Label:createWithTTF("FPS: 0.0", defaultFont, 18.0)
if label then
    label:setAnchorPoint(0.0, 0.0)
    label:setPosition(12.0, 12.0)
    local fpsAccumTime = 0.0
    local fpsAccumFrames = 0
    label:schedule("fps_update", function(dt)
        fpsAccumTime = fpsAccumTime + dt
        fpsAccumFrames = fpsAccumFrames + 1
        if fpsAccumTime < 0.25 then
            return
        end

        local fps = fpsAccumFrames / fpsAccumTime
        label:setString(string.format("FPS: %.1f", fps))
        fpsAccumTime = 0.0
        fpsAccumFrames = 0
    end, 0.0)
    scene:addChild(label)
end

local clickInfo = cc.Label:createWithTTF("Button clicks: 0", defaultFont, 18.0)
if clickInfo then
    clickInfo:setAnchorPoint(0.0, 0.0)
    clickInfo:setPosition(12.0, 30.0)
    scene:addChild(clickInfo)
end

local button = cc.Button:create("Click Me")
if button then
    button:setTitleFontName(defaultFont)
    button:setTitleFontSize(22.0)
    button:setAnchorPoint(0.5, 0.5)
    button:setContentSize(220.0, 62.0)
    button:setPosition(centerX, 78.0)

    local clickCount = 0
    button:addEventListener(function(sender)
        clickCount = clickCount + 1
        sender:setString(string.format("Clicked %d", clickCount))
        if clickInfo then
            clickInfo:setString(string.format("Button clicks: %d", clickCount))
        end
    end)

    scene:addChild(button)
end

director:runWithScene(scene)
