-- 上下浮动示例脚本:演示 fields 表(Inspector 可编辑/序列化)与 wen 变换 API。
fields = {
    amplitude = 1.0,
    frequency = 2.0,
}

local time = 0.0
local base_y = nil

function onStart()
    local x, y, z = wen.get_location()
    base_y = y
end

function onTick(dt)
    time = time + dt
    local x, y, z = wen.get_location()
    if base_y == nil then
        base_y = y
    end
    local amp = wen.get_field("amplitude")
    local freq = wen.get_field("frequency")
    wen.set_location(x, base_y + math.sin(time * freq) * amp, z)
end
