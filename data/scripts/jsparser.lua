-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

function letOperator(code)
    code = code:gsub("let %s*(%w+)%s*=", "local %1 =")
    code = code:gsub("var %s*(%w+)%s*=", "%1 =")

    return code
end

function constOperator(code)
    -- find all consts
    local consts = {}

    for const in code:gmatch("const %s*(%w+)") do
        table.insert(consts, const)
    end

    for k, v in pairs(consts) do
        code = code:gsub("const %s*" .. v .. "%s*= ?%d?%s*", function(match)
            local f = match:find("=")
            return "local " .. v .. " = --[[;__DO NOT CAPTURE__]] " .. match:sub(f + 1)
        end)

        -- make assert(false, "can't change const value") if const is changed, don't capture from above
        code = code:gsub(v .. "%s*= %s*(%w+)", "assert(false, \"Tried to change const value for " .. v .. "\")")
    end

    return code
end

function onlynumbers(str)
    local c = ""
    for i = 1, #str do
        if str:sub(i, i):match("%d") then
            c = c .. str:sub(i, i)
        end
    end
    return tonumber(c)
end

function quickOperators(code)
    -- replace all variable++ with variable = variable + 1
    code = code:gsub("(%w+)%+%+", function(variable)
        return variable .. " = " .. variable .. " + 1"
    end)

    -- same with --
    code = code:gsub("(%w+)%-%-", function(variable)
        return variable .. " = " .. variable .. " - 1"
    end)

    -- replace all variable += count with variable = variable + count
    code = code:gsub("(%w+) ?%+= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " + " .. count
    end)

    code = code:gsub("(%w+) ?%-%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " - " .. count
    end)

    code = code:gsub("(%w+) ?%*%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " * " .. count
    end)

    code = code:gsub("(%w+) ?%/%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " / " .. count
    end)

    code = code:gsub("(%w+) ?%%%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " % " .. count
    end)

    code = code:gsub("(%w+) ?%^%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " ^ " .. count
    end)

    return code
end

findall = function(code, find)
    local found = {}
    local lastPos = 0
    while code:find(find, lastPos) do
        local pos = code:find(find, lastPos)
        table.insert(found, pos)
        lastPos = pos + find:len()
    end
    return found
end

function arrowFunctions(code)
    -- replace if(condition) { body } with if(condition) then body end
    code = code:gsub("if(%b()) ?{ ?(.-) ?}", "if%1 then %2 end")

    -- replace argument => { body } with function(argument) body end
    code = code:gsub("(%w+) => ?{ ?(.-) ?}", "function(%1) %2 end")

    -- replace while(condition) { body } with while(condition) do body end
    code = code:gsub("while(%b()) ?{ ?(.-) ?}", "while%1 do %2 end")

    -- replace for(var = start; var < end; var++) { body } with for var = start, end, 1 do body end
    code = code:gsub("for(%b()) ? { ?(.-) ?}", function(a, b)
        local pos = a:find(";")
        if not pos then return end
        local var = a:sub(1, pos - 1)
        var = var:match("%w+")
        if not var then return "assert(false, 'wrong for loop')" end
        if not pos then return "assert(false, 'wrong for loop')" end
        local start = a:sub(1, pos - 1):gsub("%(?%s*(%w+)%s*=%s*", "")
        start = onlynumbers(start)
        if not start then return "assert(false, 'wrong for loop')" end
        local stop = a:sub(pos + 1):gsub("%s*(%w+)%s*<%s*(%w+)%s*;%s*(%w+)%s*%+%+", "%2"):gsub("%)", "")
        local pos = stop:find(";")
        if pos then
            stop = stop:sub(1, pos - 1):gsub("%s*(%w+)%s*%+%+", "")
        end
        stop = onlynumbers(stop)
        if not stop then return "assert(false, 'wrong for loop')" end
        local pos = a:find(";")
        local step = a:sub((pos or 1) + 1):gsub("%s*(%w+)%s*<%s*(%w+)%s*;%s*(%w+)%s*", "%3"):gsub("%)", "")

        -- if step is var++, then it's 1
        if tostring(step):find("%+%+") then
            step = 1
        end

        if tostring(step):find("%-%-") then
            step = -1
        end

        if tostring(step):match("%w* ?= ?%w* ?+ ?%d+") then
            step = step:gsub("%w* ?= ?%w* ?+ ?", "")
        end

        if tostring(step):match("%w* ?= ?%w* ?- ?%d+") then
            step = -step:gsub("%w* ?= ?%w* ?- ?", "")
        end

        if tostring(step):match("%w* ?+= ?%d+") then
            step = step:gsub("%w* ?+= ?", "")
        end

        if tostring(step):match("%w* ?-= ?%d+") then
            step = -step:gsub("%w* ?-= ?", "")
        end

        step = tonumber(step)
        if not step then return "assert(false, 'wrong for loop')" end
        return "for " .. var .. " = " .. start .. ", " .. stop .. ", " .. step .. " do " .. b .. " end"
    end)

    -- replace for(var in array) { body } with for var in pairs(array) do body end
    code = code:gsub("for%s*%((%w+)%s+of%s+(%w+)%) ?{ ?(.-) ?}", function(variable, array, body)
        return "for _," .. variable .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)

    -- replace all for(variable in array) with for variable in ipairs(array)
    code = code:gsub("for%s*%((%w+)%s+in%s+(%w+)%) ?{ ?(.-) ?}", function(variable, array, body)
        return "for " .. variable .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)

    -- replace all for(key, value in array) with for key, value in pairs(array)
    code = code:gsub("for%s*%((%w+)%s*,%s*(%w+)%s*of%s*(%w+)%) ?{ ?(.-) ?}", function(key, value, array, body)
        return "for " .. key .. ", " .. value .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)
    code = code:gsub("for%s*%((%w+)%s*,%s*(%w+)%s*in%s*(%w+)%) ?{ ?(.-) ?}", function(key, value, array, body)
        return "for " .. key .. ", " .. value .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)

    -- replace function() { body } with function() body end
    code = code:gsub("function(%b()) ?{ ?(.-) ?}", "function%1 %2 end")

    -- replace (arguments) => { body } with function(arguments) body end
    code = code:gsub("(%b()) => ?{ ?(.-) ?}", "function%1 %2 end")

    -- replace (arguments) => result; with function(arguments) return result end
    code = code:gsub("(%b()) => ?(.-) ?;", "function%1 return %2 end")

    -- replace all [] with {}
    code = code:gsub(" %[", " {")
    code = code:gsub(" %]", " }")
    code = code:gsub("{%]", "{}")

    -- find all ] with findall, if there is a { before, then replace with }
    local found = findall(code, "%]")
    for i = 1, #found do
        local pos = found[i]
        local before = code:sub(1, pos - 1)
        local after = code:sub(pos + 1)

        local all = findall(before, "%{")
        if #all > 1 then
            local last = all[#all]
            local all = findall(after, "%[")
            if #all > 1 then
                local last2 = all[#all]
                if last2 > last then
                    code = code:sub(1, pos - 1) .. "}" .. code:sub(pos + 1)
                end
            else
                code = code:sub(1, pos - 1) .. "}" .. code:sub(pos + 1)
            end
        end
        -- code = before .. "}" .. after
    end

    -- replace all [number or string} with [number or string]
    code = code:gsub("%[(%w+)%}", "[%1]")
    -- code = code:gsub("%[(%d+)%]", function(a)
    --     return "[" .. a+1 .. "]"
    -- end)

    -- code = code:gsub("for%(%w* ?= ? %d+; ?%w* ?< ?%d+; ?%w*++%) ?{ ?(.-) ?}", function(match)
    --     print(match)
    -- end)

    return code
end

function tryFunctions(code)
    -- replace all try { body } catch { body } with pcall(function() body end)
    code = code:gsub("try ?{ ?(.-) ?} ?catch ?{ ?(.-) ?}", "if not pcall(function() %1 end) then %2 end")

    -- replace all try { body } catch(error) { body } with pcall(function() body end)
    code = code:gsub("try ?{ ?(.-) ?} ?catch%((.-)%) ?{ ?(.-) ?}",
        "local __pres = pcall(function() %1 end); if not __pres then (function(%2) %3 end)(__pres) end")

    return code
end

function parse(code)
    code = letOperator(code)
    code = constOperator(code)
    code = arrowFunctions(code)
    code = quickOperators(code)
    code = tryFunctions(code)
    return code
end

return parse
