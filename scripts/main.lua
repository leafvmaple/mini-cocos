local director = cc.Director:getInstance()
assert(director and director:init(960, 540, "zocos (Lua demo)"), "Director:init failed")

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

sprite:runAction(cc.RepeatForever:create(cc.Sequence:create({
    cc.MoveTo:create(0.8, centerX, centerY + orbitRadius),
    cc.MoveTo:create(0.8, centerX - orbitRadius, centerY),
    cc.MoveTo:create(0.8, centerX, centerY - orbitRadius),
    cc.MoveTo:create(0.8, centerX + orbitRadius, centerY),
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

local label = cc.Label:create("FPS: 0.0")
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

director:runWithScene(scene)
